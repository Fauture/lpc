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
// Recursive: pktAnalyse holds the lock while applying the interface function,
// and that function may re-enter the analyser (send_message ->
// pkt_pad_interface) on the same thread.
std::recursive_mutex& pktMutex() {
    static std::recursive_mutex m;
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
    if (f.type == "int8" || f.type == "uint8" || f.type == "int16" || f.type == "uint16" ||
        f.type == "int32" || f.type == "uint32" || f.type == "int64") {
        // Original vm_pktInitAnalyser: int8/uint8=1B, int16/uint16=2B,
        // int32/uint32=4B (matchType 1/4, 2/5, 3/6 encode identically).
        if (f.type == "int8" || f.type == "uint8") { if (!need(1)) return false; out = make_int(static_cast<int8_t>(body[off])); off += 1; }
        else if (f.type == "int16" || f.type == "uint16") { if (!need(2)) return false; out = make_int(static_cast<int16_t>(rd16be(body + off))); off += 2; }
        else if (f.type == "int32" || f.type == "uint32") { if (!need(4)) return false; out = make_int(static_cast<int32_t>(rd32be(body + off))); off += 4; }
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

// ---- body encoding (vm_pktPadDefValue / vm_pktPadUserDefinedValue) --------

void wr16be(std::string& s, uint16_t v) { s.push_back(static_cast<char>((v >> 8) & 0xFF)); s.push_back(static_cast<char>(v & 0xFF)); }
void wr32be(std::string& s, uint32_t v) {
    s.push_back(static_cast<char>((v >> 24) & 0xFF));
    s.push_back(static_cast<char>((v >> 16) & 0xFF));
    s.push_back(static_cast<char>((v >> 8) & 0xFF));
    s.push_back(static_cast<char>(v & 0xFF));
}
void wr64be(std::string& s, uint64_t v) {
    for (int i = 7; i >= 0; --i) s.push_back(static_cast<char>((v >> (8 * i)) & 0xFF));
}

int64_t valInt(const Value& v) {
    if (v.index() == 2) return std::get<int64_t>(v);
    if (v.index() == 3) return static_cast<int64_t>(std::get<double>(v));
    if (v.index() == 4) {
        const std::string& t = std::get<std::string>(v);
        return t.empty() ? 0 : std::strtoll(t.c_str(), nullptr, 10);
    }
    return 0;
}

// Serialize one field value; appends to out.  Mirrors decodeField.
void encodeField(const PktField& f, const Value& val, std::string& out) {
    if (f.isArray) {
        // u16 element count then elements
        size_t cnt = 0;
        std::vector<Value> items;
        if (val.index() == 5) {
            items = std::get<std::shared_ptr<ArrayValue>>(val)->items;
            cnt = items.size();
        }
        wr16be(out, static_cast<uint16_t>(cnt));
        PktField elem = f;
        elem.isArray = false;
        for (size_t k = 0; k < cnt; ++k) encodeField(elem, items[k], out);
        return;
    }
    if (f.type == "int8" || f.type == "uint8") {
        // Original vm_pktPadDefValue matchType 1/4: only an int value is
        // encoded; any other type writes 0.
        out.push_back(static_cast<char>((val.index() == 2 ? valInt(val) : 0) & 0xFF));
    } else if (f.type == "int16" || f.type == "uint16") {
        wr16be(out, static_cast<uint16_t>((val.index() == 2 ? valInt(val) : 0) & 0xFFFF));
    } else if (f.type == "int32" || f.type == "uint32") {
        wr32be(out, static_cast<uint32_t>(val.index() == 2 ? valInt(val) : 0));
    } else if (f.type == "int64") {
        wr64be(out, static_cast<uint64_t>(val.index() == 2 ? valInt(val) : 0));
    } else if (f.type.rfind("string", 0) == 0 || f.type.rfind("buffer", 0) == 0) {
        // length-prefixed string/buffer (1/2/4 byte length)
        std::string s;
        if (val.index() == 4) s = std::get<std::string>(val);
        else if (val.index() == 10)
            s = std::get<std::shared_ptr<BufferValue>>(val)->data;   // raw buffer bytes
        // Original vm_pktPadDefValue: for string/buffer fields a NON-string/
        // non-buffer value (e.g. int) is treated as EMPTY - it writes a
        // zero-length prefix and no payload (LABEL_21/LABEL_67 paths).  dba's
        // cmd_read_content sends send_message(17409, cookie, -1, 0) with an
        // int 0 content for a missing record; encoding it as anything but an
        // empty payload made ccs's filed operate_done see got_result="0" and
        // read_content returned "0" instead of -1, so callers like shop_mgrd's
        // load_ad_infos wrote save_string("")="\"\"" into the DB.
        // else if (val.index() == 2) ... -> intentionally NOT encoded
        else if (val.index() == 6) s = to_string(val, false);
        int llen = f.fixedLen ? f.fixedLen : 1;
        uint32_t slen = static_cast<uint32_t>(s.size());
        for (int k = llen - 1; k >= 0; --k) out.push_back(static_cast<char>((slen >> (8 * k)) & 0xFF));
        out += s;
    } else {
        // encapsulated named type - encode as a mapping of fields
        auto dit = pktState().defs.find(f.type);
        if (dit != pktState().defs.end()) {
            for (const auto& sf : dit->second) {
                Value v;
                if (val.index() == 6) {
                    auto mp = std::get<std::shared_ptr<MappingValue>>(val);
                    const Value* fv = mp->find(make_string(sf.name));
                    if (fv) v = *fv;
                } else if (val.index() == 5) {
                    auto arr = std::get<std::shared_ptr<ArrayValue>>(val);
                    size_t fi = 0;
                    for (size_t k = 0; k < dit->second.size(); ++k) if (&dit->second[k] == &sf) { fi = k; break; }
                    if (fi < arr->items.size()) v = arr->items[fi];
                }
                encodeField(sf, v, out);
            }
        }
    }
}

} // namespace

std::string pktPadInterface(uint16_t msgNo, const Value& args) {
    if (msgNo == 0x4401 || msgNo == 0x2403)
        std::cerr << "[pad-enter] msgNo=0x" << std::hex << msgNo << std::dec
                  << " tid=" << std::this_thread::get_id() << "\n";
    std::lock_guard<std::recursive_mutex> lk(pktMutex());
    if (msgNo == 0x4401 || msgNo == 0x2403)
        std::cerr << "[pad-locked] msgNo=0x" << std::hex << msgNo << std::dec
                  << " tid=" << std::this_thread::get_id() << "\n";
    auto it = pktState().interfaces.find(msgNo);
    if (it == pktState().interfaces.end()) {
        if (msgNo == 0x4401 || msgNo == 0x2403)
            std::cerr << "[pad] iface 0x" << std::hex << msgNo << std::dec << " NOT FOUND\n";
        return "";
    }
    const PktInterface& pi = it->second;
    if (msgNo == 0x4401 || msgNo == 0x2403)
        std::cerr << "[pad] iface 0x" << std::hex << msgNo << std::dec
                  << " name=" << pi.name << " encap=" << pi.encapType
                  << " fields=" << pi.fields.size()
                  << " argsType=" << static_cast<int>(args.index()) << "\n";
    std::string out;
    // vm_pktPadUserDefinedValue: encapType 0 -> single value (mapping of
    // name->value), 1 -> positional array of values.
    Value values = args;
    if (pi.encapType == 1) {
        // args is an array of field values in order
        std::vector<Value> items;
        if (args.index() == 5) items = std::get<std::shared_ptr<ArrayValue>>(args)->items;
        for (size_t i = 0; i < pi.fields.size() && i < items.size(); ++i)
            encodeField(pi.fields[i], items[i], out);
    } else {
        // args is a mapping name->value
        for (const auto& f : pi.fields) {
            Value v;
            if (args.index() == 6) {
                auto mp = std::get<std::shared_ptr<MappingValue>>(args);
                const Value* fv = mp->find(make_string(f.name));
                if (fv) v = *fv;
            }
            encodeField(f, v, out);
        }
    }
    return out;
}

void pktInitAnalyser(int maxInterfaces, int) {
    std::lock_guard<std::recursive_mutex> lk(pktMutex());
    pktState().inited = true;
    pktState().maxInterfaces = maxInterfaces;
    pktState().interfaces.clear();
    pktState().defs.clear();
    pktState().nameToCmdNo.clear();
}

void pktAddDef(const std::string& name, const std::string& signature) {
    std::lock_guard<std::recursive_mutex> lk(pktMutex());
    std::vector<PktField> fields;
    deriveArguments(signature, fields);
    pktState().defs[name] = std::move(fields);
}

void pktAddInterface(int cmdNo, const std::string& signature, const Value& func, int encapType, uint32_t flag) {
    std::lock_guard<std::recursive_mutex> lk(pktMutex());
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
    std::lock_guard<std::recursive_mutex> lk(pktMutex());
    auto it = pktState().interfaces.find(cmdNo);
    return it == pktState().interfaces.end() ? nullptr : &it->second;
}

int pktAnalyse(int clientNo, uint32_t attrib, uint16_t cmdNo,
               const uint8_t* body, size_t len) {
    std::unique_lock<std::recursive_mutex> lk(pktMutex());
    if (cmdNo == 0x3400 || cmdNo == 0x4401 || cmdNo == 0x3411 || cmdNo == 0x3415 ||
        cmdNo == 0x4419 || cmdNo == 0x4417 || cmdNo == 0x3A01 || cmdNo == 0x3A02) {
        std::string hex;
        static const char* hx = "0123456789ABCDEF";
        for (size_t i = 0; i < len && i < 64; ++i) { hex += hx[body[i] >> 4]; hex += hx[body[i] & 15]; }
        std::cerr << "[pkt-filed] cmd=0x" << std::hex << cmdNo << std::dec
                  << " client=" << clientNo << " attrib=" << attrib
                  << " len=" << len << " tid=" << std::this_thread::get_id()
                  << " body=" << hex << "\n";
    }
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
    if (std::getenv("LPCVM_TRACE") && pi.name == "cmd_l_account") {
        std::string hex;
        static const char* hx = "0123456789ABCDEF";
        for (size_t i = 0; i < len; ++i) { hex += hx[(body[i] >> 4) & 0xF]; hex += hx[body[i] & 0xF]; }
        std::cerr << "[pkt] cmd_l_account body(" << len << ")=" << hex << "\n";
    }
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
    if (std::getenv("LPCVM_TRACE") &&
        (pi.name == "cmd_cfg_get_aaa_list" || pi.name == "cmd_internal_auth" ||
         pi.name == "cmd_cfg_get_my_info" || pi.name == "cmd_cfg_get_dba_list")) {
        std::string dbg;
        for (size_t i = 0; i < fieldVals.size(); ++i)
            dbg += (i ? "," : "") + to_string(fieldVals[i], false);
        std::cerr << "[pkt] " << pi.name << " fields=[" << dbg << "]\n";
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
    if (std::getenv("LPCVM_TRACE")) {
        std::string fdbg;
        for (size_t i = 0; i < pi.fields.size() && i < fieldVals.size(); ++i) {
            if (i) fdbg += " | ";
            fdbg += pi.fields[i].name + "=" + to_string(fieldVals[i], false);
        }
        std::cerr << "[pkt] analyse dispatching " << pi.name
                  << " client=" << clientNo << " argc=" << args.size()
                  << " encap=" << pi.encapType
                  << " fieldCount=" << pi.fields.size()
                  << " vals=" << fieldVals.size()
                  << " bodyLen=" << len
                  << " fields=[" << fdbg << "]\n";
    }
    if (cmdNo == 0x3A01 || cmdNo == 0x3A02)
        std::cerr << "[pkt-filed] dispatch " << pi.name << " funcType=" << static_cast<int>(pi.func.index()) << "\n";
    // Release the interface-table lock BEFORE invoking the LPC callback: the
    // callback may run on a DIFFERENT thread (cross-object call) and itself
    // call pkt_pad_interface -> pktPadInterface, which re-locks pktMutex.  A
    // non-recursive-owner deadlock results if we hold it across the callback.
    Value func = pi.func;
    lk.unlock();
    if (func.index() == 8) {
        auto cl = std::get<std::shared_ptr<ClosureValue>>(func);
        if (std::getenv("LPCVM_TRACE"))
            std::cerr << "[pkt] calling closure " << cl->functionName
                      << " boundArgs=" << cl->boundArgs.size() << "\n";
        std::vector<Value> all = cl->boundArgs;
        all.insert(all.end(), args.begin(), args.end());
        if (cl->functionName.empty()) return 1;
        applyClosureOnOwner(func, args);
        if (cmdNo == 0x3A01 || cmdNo == 0x3A02)
            std::cerr << "[pkt-filed] closure " << cl->functionName << " returned\n";
        return 1;
    }
    if (func.index() == 4) {
        if (std::getenv("LPCVM_TRACE"))
            std::cerr << "[pkt] calling string fn " << std::get<std::string>(func) << "\n";
        callMasterProgramFunction(std::get<std::string>(func), args, lpc::masterSearchRoot());
        if (cmdNo == 0x3A01 || cmdNo == 0x3A02)
            std::cerr << "[pkt-filed] string fn " << std::get<std::string>(func) << " returned\n";
        return 1;
    }
    if (std::getenv("LPCVM_TRACE"))
        std::cerr << "[pkt] func type=" << static_cast<int>(func.index()) << " not callable\n";
    return 1;
}

void pktLoadCommDesc(const std::string& content,
                     const std::function<void(int, const std::string&, const Value&, int)>&) {
    (void)content;
    // The actual registration is driven from LPC (load_comm_desc calls
    // pkt_add_interface per line); this helper is not used directly.
}

} // namespace lpc
