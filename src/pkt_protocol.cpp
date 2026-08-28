// Packet protocol analyser - faithful port of vm_pkt_analyser.c from the
// original magic_Linux32x driver.  Implements the comm_desc.txt-driven command
// protocol used by the game client service (port 8101).
//
// Registration (LPC side, e.g. communicated.c's create()):
//   pkt_init_analyser(65536, 1024);
//   load_comm_desc((: process_command :), 1);
//
// For each "void cmd_name(field desc)" line, load_comm_desc calls:
//   pkt_add_interface(interface_flag, line, find_function(callback, cmd_name), encap_type)
// The driver parses the interface name -> uppercase -> macro -> cmdNo (e.g.
// MSG_L_AUTH -> 0x5351), derives the field layout from the argument list, and
// registers allPktInterfaces[cmdNo] = {cmdFilter, fields, function}.
//
// Dispatch (vm_clientSocketCallback):
//   vm_pktAnalyse(cmdNo, body, len) parses the body per the fields and applies
//   the registered LPC function with the decoded arguments; returns 1 when
//   handled.  When no interface matches (or parsing fails) the caller falls
//   back to cmdIndex/cmdFunc dispatch.

#include "pkt_protocol.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <mutex>

#include "builtins.h"
#include "original_vm.h"

namespace lpc {

namespace {

struct PktState {
    bool inited = false;
    int maxInterfaces = 65536;
    std::map<uint16_t, PktInterface> interfaces;
    std::map<std::string, std::vector<PktField>> defs;   // encap:mapping types
    std::map<std::string, uint16_t> nameToCmdNo;         // uppercase name -> cmdNo
};
PktState& pktState() {
    static PktState s;
    return s;
}
std::mutex& pktMutex() {
    static std::mutex m;
    return m;
}

std::string trimStr(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}
std::string upperStr(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}

// vm_deriveArguments: parse "int32 result, string2 msg, ..." into fields.
// Supports: int8/16/32/64, string, string2, string4, buffer, buffer2, buffer4,
// <type>[] (array), and named encapsulated types (from encap:mapping defs).
bool deriveArguments(const std::string& desc, std::vector<PktField>& out) {
    size_t i = 0;
    while (i < desc.size()) {
        while (i < desc.size() && (desc[i] == ' ' || desc[i] == ',')) ++i;
        if (i >= desc.size()) break;
        size_t sp = i;
        while (sp < desc.size() && desc[sp] != ' ' && desc[sp] != ',') ++sp;
        std::string type = desc.substr(i, sp - i);
        i = sp;
        while (i < desc.size() && desc[i] == ' ') ++i;
        size_t nm = i;
        while (nm < desc.size() && (std::isalnum(static_cast<unsigned char>(desc[nm])) || desc[nm] == '_')) ++nm;
        std::string name = desc.substr(i, nm - i);
        i = nm;
        if (name.empty()) return false;
        PktField f;
        bool isArr = false;
        // trailing [] on the name side: "string name[]"
        if (i + 1 < desc.size() && desc[i] == '[' && desc[i + 1] == ']') {
            isArr = true;
            i += 2;
        }
        // or trailing [] on the type side: "string[] name"
        if (type.size() >= 2 && type.back() == ']' && type[type.size() - 2] == '[') {
            isArr = true;
            type = type.substr(0, type.size() - 2);
        }
        f.type = type;
        f.name = name;
        f.isArray = isArr;
        if (type.rfind("string", 0) == 0) f.fixedLen = type == "string" ? 1 : (type == "string2" ? 2 : 4);
        else if (type.rfind("buffer", 0) == 0) f.fixedLen = type == "buffer" ? 1 : (type == "buffer2" ? 2 : 4);
        out.push_back(std::move(f));
    }
    return true;
}

// ---- body decoding --------------------------------------------------------

uint16_t rd16be(const uint8_t* p) { return static_cast<uint16_t>((p[0] << 8) | p[1]); }
uint32_t rd32be(const uint8_t* p) {
    return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
}
uint64_t rd64be(const uint8_t* p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v = (v << 8) | p[i];
    return v;
}

// Decode one field value from body; advances *off.  Returns false on OOB.
bool decodeField(const PktField& f, const uint8_t* body, size_t len, size_t& off, Value& out) {
    auto need = [&](size_t n) -> bool { return off + n <= len; };
    if (f.isArray) {
        // array of the element type: u16 count then elements
        auto arr = std::make_shared<ArrayValue>();
        if (!need(2)) return false;
        uint16_t cnt = rd16be(body + off);
        off += 2;
        for (uint16_t k = 0; k < cnt; ++k) {
            Value item;
            PktField elem = f;
            elem.isArray = false;
            if (!decodeField(elem, body, len, off, item)) return false;
            arr->items.push_back(item);
        }
        out = arr;
        return true;
    }
    if (f.type == "int8" || f.type == "int16" || f.type == "int32" || f.type == "int64") {
        if (f.type == "int8") { if (!need(1)) return false; out = make_int(static_cast<int8_t>(body[off])); off += 1; }
        else if (f.type == "int16") { if (!need(2)) return false; out = make_int(static_cast<int16_t>(rd16be(body + off))); off += 2; }
        else if (f.type == "int32") { if (!need(4)) return false; out = make_int(static_cast<int32_t>(rd32be(body + off))); off += 4; }
        else { if (!need(8)) return false; out = make_int(static_cast<int64_t>(rd64be(body + off))); off += 8; }
        return true;
    }
    if (f.type.rfind("string", 0) == 0 || f.type.rfind("buffer", 0) == 0) {
        int llen = f.fixedLen ? f.fixedLen : 1;
        if (!need(static_cast<size_t>(llen))) return false;
        uint32_t slen = 0;
        for (int k = 0; k < llen; ++k) slen = (slen << 8) | body[off + k];
        off += static_cast<size_t>(llen);
        if (!need(slen)) return false;
        out = make_string(std::string(reinterpret_cast<const char*>(body + off), slen));
        off += slen;
        return true;
    }
    // encapsulated named type (encap:mapping) - decode as a mapping of fields
    auto dit = pktState().defs.find(f.type);
    if (dit != pktState().defs.end()) {
        auto mp = std::make_shared<MappingValue>();
        for (const auto& sf : dit->second) {
            Value v;
            if (!decodeField(sf, body, len, off, v)) return false;
            mp->insert(make_string(sf.name), v);
        }
        out = mp;
        return true;
    }
    return false;
}

} // namespace

void pktInitAnalyser(int maxInterfaces, int) {
    std::lock_guard<std::mutex> lk(pktMutex());
    pktState().inited = true;
    pktState().maxInterfaces = maxInterfaces;
    pktState().interfaces.clear();
    pktState().defs.clear();
    pktState().nameToCmdNo.clear();
}

void pktAddDef(const std::string& name, const std::string& signature) {
    std::lock_guard<std::mutex> lk(pktMutex());
    std::vector<PktField> fields;
    deriveArguments(signature, fields);
    pktState().defs[name] = std::move(fields);
}

void pktAddInterface(int cmdNo, const std::string& signature, const Value& func, int encapType, uint32_t flag) {
    std::lock_guard<std::mutex> lk(pktMutex());
    // signature: "void msg_l_auth(int32 result, int32 auth_key, string2 msg)"
    size_t lp = signature.find('(');
    if (lp == std::string::npos) return;
    std::string head = trimStr(signature.substr(0, lp));
    if (head.rfind("void ", 0) == 0) head = head.substr(5);
    head = trimStr(head);
    size_t rp = signature.rfind(')');
    std::string args = (rp != std::string::npos && rp > lp) ? signature.substr(lp + 1, rp - lp - 1) : "";
    PktInterface pi;
    pi.cmdNo = static_cast<uint16_t>(cmdNo);
    pi.name = head;
    pi.flag = flag;
    pi.func = func;
    pi.encapType = encapType;
    deriveArguments(args, pi.fields);
    pktState().interfaces[pi.cmdNo] = std::move(pi);
    pktState().nameToCmdNo[upperStr(head)] = static_cast<uint16_t>(cmdNo);
}

const PktInterface* pktFindInterface(uint16_t cmdNo) {
    std::lock_guard<std::mutex> lk(pktMutex());
    auto it = pktState().interfaces.find(cmdNo);
    return it == pktState().interfaces.end() ? nullptr : &it->second;
}

int pktAnalyse(int clientNo, uint32_t attrib, uint16_t cmdNo,
               const uint8_t* body, size_t len) {
    std::lock_guard<std::mutex> lk(pktMutex());
    auto it = pktState().interfaces.find(cmdNo);
    if (it == pktState().interfaces.end()) {
        if (std::getenv("LPCVM_TRACE"))
            std::cerr << "[pkt] analyse cmd=0x" << std::hex << cmdNo << std::dec
                      << " no interface\n";
        return 0;   // not handled -> cmdIndex
    }
    const PktInterface& pi = it->second;
    if (std::getenv("LPCVM_TRACE"))
        std::cerr << "[pkt] analyse cmd=0x" << std::hex << cmdNo << std::dec
                  << " iface=" << pi.name << " flag=" << pi.flag
                  << " attrib=" << attrib << " fields=" << pi.fields.size() << "\n";
    // cmdFilter check: (attrib & cmdFilter & 7) != 0  (vm_clientSocketCallback)
    if ((attrib & pi.flag & 7) == 0) {
        if (std::getenv("LPCVM_TRACE"))
            std::cerr << "[pkt] analyse cmd=0x" << std::hex << cmdNo << std::dec
                      << " denied attrib=" << attrib << " flag=" << pi.flag << "\n";
        return 0;
    }
    size_t off = 0;
    // vm_matchPktArguments produces the argument collection; vm_pktAnalyse
    // then applies the interface function with (clientNo, collection):
    //   encapType==1 -> array of field values
    //   encapType==0 -> mapping of name->value (+ extra_data buffer)
    std::vector<Value> fieldVals;
    for (const auto& f : pi.fields) {
        Value v;
        if (!decodeField(f, body, len, off, v)) {
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[pkt] analyse field '" << f.name << "' decode failed at off=" << off << "\n";
            return -1;
        }
        fieldVals.push_back(v);
    }
    Value collection;
    if (pi.encapType == 0) {
        auto mp = std::make_shared<MappingValue>();
        for (size_t i = 0; i < pi.fields.size() && i < fieldVals.size(); ++i)
            mp->insert(make_string(pi.fields[i].name), fieldVals[i]);
        collection = mp;
    } else {
        auto arr = std::make_shared<ArrayValue>();
        arr->items = std::move(fieldVals);
        collection = arr;
    }
    std::vector<Value> args;
    args.push_back(make_int(clientNo));   // implicit first argument
    args.push_back(collection);
    if (std::getenv("LPCVM_TRACE"))
        std::cerr << "[pkt] analyse dispatching " << pi.name
                  << " client=" << clientNo << " argc=" << args.size()
                  << " encap=" << pi.encapType << "\n";
    if (pi.func.index() == 8) {
        auto cl = std::get<std::shared_ptr<ClosureValue>>(pi.func);
        if (std::getenv("LPCVM_TRACE"))
            std::cerr << "[pkt] calling closure " << cl->functionName
                      << " boundArgs=" << cl->boundArgs.size() << "\n";
        std::vector<Value> all = cl->boundArgs;
        all.insert(all.end(), args.begin(), args.end());
        if (cl->functionName.empty()) return 1;
        applyClosureOnOwner(pi.func, args);
        return 1;
    }
    if (pi.func.index() == 4) {
        if (std::getenv("LPCVM_TRACE"))
            std::cerr << "[pkt] calling string fn " << std::get<std::string>(pi.func) << "\n";
        callMasterProgramFunction(std::get<std::string>(pi.func), args, lpc::masterSearchRoot());
        return 1;
    }
    if (std::getenv("LPCVM_TRACE"))
        std::cerr << "[pkt] func type=" << static_cast<int>(pi.func.index()) << " not callable\n";
    return 1;
}

void pktLoadCommDesc(const std::string& content,
                     const std::function<void(int, const std::string&, const Value&, int)>&) {
    (void)content;
    // The actual registration is driven from LPC (load_comm_desc calls
    // pkt_add_interface per line); this helper is not used directly.
}

} // namespace lpc
