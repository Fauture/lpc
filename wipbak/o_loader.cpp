#include "o_loader.h"

#include <fstream>
#include <stdexcept>
#include <sstream>

namespace lpc {

namespace {

struct OriginalCode {
    std::vector<OriginalInsn> insns;
    std::vector<uint8_t> meta;
};

struct Reader {
    const std::vector<uint8_t>& data;
    size_t pos = 0;

    uint8_t u8() {
        if (pos + 1 > data.size()) throw std::runtime_error("o_loader: unexpected end (u8)");
        return data[pos++];
    }
    uint16_t u16() {
        if (pos + 2 > data.size()) throw std::runtime_error("o_loader: unexpected end (u16)");
        uint16_t v = static_cast<uint16_t>(data[pos] | (data[pos + 1] << 8));
        pos += 2;
        return v;
    }
    uint32_t u32() {
        if (pos + 4 > data.size()) throw std::runtime_error("o_loader: unexpected end (u32)");
        uint32_t v = static_cast<uint32_t>(data[pos]) |
                     (static_cast<uint32_t>(data[pos + 1]) << 8) |
                     (static_cast<uint32_t>(data[pos + 2]) << 16) |
                     (static_cast<uint32_t>(data[pos + 3]) << 24);
        pos += 4;
        return v;
    }
    std::vector<uint8_t> bytes(size_t n) {
        if (pos + n > data.size()) throw std::runtime_error("o_loader: unexpected end (bytes)");
        std::vector<uint8_t> b(data.begin() + static_cast<long long>(pos), data.begin() + static_cast<long long>(pos + n));
        pos += n;
        return b;
    }
    std::string cstr(size_t n) {
        std::vector<uint8_t> b = bytes(n);
        std::string s(b.begin(), b.end());
        size_t z = s.find('\0');
        if (z != std::string::npos) s.resize(z);
        return s;
    }
    std::string raw(size_t n) {
        std::vector<uint8_t> b = bytes(n);
        return std::string(b.begin(), b.end());
    }
    std::string tempString() {
        uint32_t n = u32();
        return cstr(n);
    }
    OriginalCode codeSegment() {
        uint32_t symbolsCount = u32();
        for (uint32_t i = 0; i < symbolsCount; ++i) {
            uint8_t flag = u8();
            if (flag & 1) tempString();
            if (flag & 2) u32();
        }
        uint32_t nodesCount = u32();
        std::vector<uint8_t> nodeBytes;
        if (nodesCount) nodeBytes = bytes(4ULL * nodesCount);
        uint32_t metaLen = u32();
        std::vector<uint8_t> meta;
        if (metaLen) meta = bytes(metaLen);
        uint32_t runtimeCount = u32();
        if (runtimeCount) bytes(4ULL * runtimeCount);
        OriginalCode code;
        code.insns.reserve(nodesCount);
        for (uint32_t i = 0; i < nodesCount; ++i) {
            OriginalInsn ins;
            ins.a = static_cast<uint16_t>(nodeBytes[i * 4] | (nodeBytes[i * 4 + 1] << 8));
            ins.b = nodeBytes[i * 4 + 2];
            ins.op = nodeBytes[i * 4 + 3];
            code.insns.push_back(ins);
        }
        code.meta = std::move(meta);
        return code;
    }
    OriginalFunction function() {
        OriginalFunction fn;
        bytes(64); // Vm_Fun_T raw struct
        fn.name = tempString(); // name is always stored, even when keepSymbol is off
        uint16_t argCount = u16();
        fn.argCount = argCount;
        for (uint16_t i = 0; i < argCount; ++i) {
            bytes(20);
            tempString();
        }
        uint16_t localCount = u16();
        fn.localCount = localCount;
        for (uint16_t i = 0; i < localCount; ++i) {
            bytes(20);
            tempString();
        }
        OriginalCode code = codeSegment();
        fn.code = std::move(code.insns);
        fn.metaData = std::move(code.meta);
        return fn;
    }
};

std::vector<uint8_t> readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open file: " + path);
    in.seekg(0, std::ios::end);
    std::streamoff len = in.tellg();
    in.seekg(0, std::ios::beg);
    std::vector<uint8_t> data(static_cast<size_t>(len));
    if (len > 0) in.read(reinterpret_cast<char*>(data.data()), len);
    return data;
}

} // namespace

std::string actionName(uint8_t op) {
    static const char* names[] = {
        "invalid", "add", "and", "assign_value", "assign_value_reverse",
        "call_fun_ptr", "call_in_efun", "call_in_user_fun", "call_in_other_fun_ex",
        "call_os_fun", "case_check_number", "case_check_string", "case_default",
        "cast_type", "caught_error", "check_args_num", "check_is_ref_type",
        "check_is_ref_type_i", "cond_jmp_for_loop", "convert_to_ptr",
        "convert_to_value", "create_argument_array", "create_array",
        "create_function_value", "create_mapping", "div_value",
        "end_thread_catch", "enter_function", "eq_value", "finish_action",
        "ge_value", "goto", "gt_value", "if", "inc_value", "init_for_loop",
        "is_type", "le_value", "lsh_value", "lt_value", "mod_value",
        "mul_value", "ne_value", "neg_value", "new_thread_catch", "not_value",
        "null_action", "op_eq_value", "or_value", "pickup_os_value",
        "pop_n_stack", "pop_stack", "push_arg_num", "push_global_var",
        "push_global_vptr", "push_local_value", "push_local_var_i",
        "push_local_vptr", "push_local_vptr_i", "push_number(S)",
        "push_number(L)", "push_obj_var", "push_obj_vptr",
        "push_real_from_word", "push_shared_string", "push_this_object",
        "push_undefined", "query_member", "query_member_ptr", "query_range",
        "query_range_ptr", "resort_stack", "return_from_fake_fun",
        "return_from_fun", "rev_value", "rsh_value", "set_global_var",
        "set_local_var", "set_obj_var", "sizeof", "step_jmp_for_loop",
        "sub_value", "to_reserve_stack_value", "to_unreserve_stack_value",
        "trace", "xor_value", "null"
    };
    if (op < 86) return names[op];
    return "unknown";
}

OriginalObject loadOriginalObjectFromBytes(const std::vector<uint8_t>& data) {
    Reader r{data};
    OriginalObject obj;
    obj.magic = r.u32();
    obj.version = r.u32();
    obj.functionBC = r.u32();
    uint32_t defineCount = r.u32();
    for (uint32_t i = 0; i < defineCount; ++i) {
        r.u32();       // nargs
        std::string dname = r.tempString();   // name
        std::string dexp = r.tempString();    // expansion
        obj.defines.emplace_back(std::move(dname), std::move(dexp));
    }
    uint32_t stringCount = r.u32();
    for (uint32_t i = 0; i < stringCount; ++i) {
        uint32_t n = r.u32();
        obj.strings.push_back(r.cstr(n));
    }

    uint32_t nameCount = r.u32();
    uint32_t nameSize = r.u32();
    std::string namePool = r.raw(nameSize);
    obj.names.reserve(nameCount);
    {
        size_t start = 0;
        for (uint32_t i = 0; i < nameCount; ++i) {
            size_t end = namePool.find('\0', start);
            obj.names.push_back(namePool.substr(start, end == std::string::npos ? std::string::npos : end - start));
            if (end == std::string::npos) break;
            start = end + 1;
        }
    }

    uint16_t globalVarCount = r.u16();
    obj.globalVarCount = globalVarCount;
    uint16_t progVarCount = r.u16();
    // Object files (functionBC == 0) store per-object PROG_VARS records.
    // Global programs (functionBC != 0) store only the global variable count.
    if (obj.functionBC == 0) {
        for (uint16_t i = 0; i < progVarCount; ++i) {
            r.bytes(16);
            r.tempString();
        }
    }

    uint16_t efunCount = r.u16();
    (void)efunCount;
    uint16_t functionCount = r.u16();
    (void)functionCount;

    uint32_t hiddenCount = r.u32();
    for (uint32_t i = 0; i < hiddenCount; ++i) {
        obj.functions.push_back(r.function());
    }

    // Inherit table is present only for object files (pCurrentProgram != nullptr).
    if (obj.functionBC == 0) {
        uint16_t maxConstitution = r.u16();
        uint32_t inheritCount = r.u32();
        for (uint32_t i = 0; i < inheritCount; ++i) {
            OriginalInherit inh;
            inh.path = r.tempString();
            std::vector<uint8_t> raw = r.bytes(12);
            // 6 u16 (matches vm_loadBinaryInherit):
            //   baseConsititutionOffset, baseFunOffset, baseVarOffset,
            //   expectedSubConsititutionNo, expectedFunctionNo, expectedVariableNo
            if (raw.size() >= 4) {
                inh.baseFunOffset = static_cast<int16_t>(raw[2] | (raw[3] << 8));
            }
            if (raw.size() >= 6) {
                inh.baseVarOffset = static_cast<int16_t>(raw[4] | (raw[5] << 8));
            }
            if (i == 0 && raw.size() >= 4) {
                // Legacy field kept for compatibility.
                obj.baseFunOffset = static_cast<int16_t>(raw[2] | (raw[3] << 8));
            }
            obj.inherits.push_back(std::move(inh));
        }
        (void)maxConstitution;
    }

    uint32_t dltCount = r.u32();
    for (uint32_t i = 0; i < dltCount; ++i) {
        std::vector<uint8_t> dlt = r.bytes(20);
        uint32_t attrib = static_cast<uint32_t>(dlt[4]) | (static_cast<uint32_t>(dlt[5]) << 8) |
                          (static_cast<uint32_t>(dlt[6]) << 16) | (static_cast<uint32_t>(dlt[7]) << 24);
        if (attrib & 0x8000000u) {
            OriginalFunction fn = r.function();
            uint32_t nameOff = static_cast<uint32_t>(dlt[0]) | (static_cast<uint32_t>(dlt[1]) << 8) |
                               (static_cast<uint32_t>(dlt[2]) << 16) | (static_cast<uint32_t>(dlt[3]) << 24);
            if (nameOff < namePool.size()) {
                size_t end = namePool.find('\0', nameOff);
                fn.name = namePool.substr(nameOff, end == std::string::npos ? std::string::npos : end - nameOff);
            }
            // DLT entry +16/+17/+18/+19 store the original function index.
            fn.index = static_cast<int32_t>(dlt[16]) | (static_cast<int32_t>(dlt[17]) << 8) |
                       (static_cast<int32_t>(dlt[18]) << 16) | (static_cast<int32_t>(dlt[19]) << 24);
            obj.functions.push_back(std::move(fn));
        }
    }

    OriginalCode globalCode = r.codeSegment();
    obj.globalCode = std::move(globalCode.insns);
    obj.globalMeta = std::move(globalCode.meta);
    return obj;
}

OriginalObject loadOriginalObject(const std::string& path) {
    std::vector<uint8_t> data = readFile(path);
    OriginalObject obj = loadOriginalObjectFromBytes(data);
    obj.sourceName = path;
    return obj;
}

std::string disassembleOriginalObject(const OriginalObject& obj) {
    std::ostringstream os;
    os << "magic=0x" << std::hex << obj.magic << " version=" << obj.version
       << " functionBC=" << std::dec << obj.functionBC
       << " globalVarCount=" << obj.globalVarCount << "\n";
    os << "strings: " << obj.strings.size() << "\n";
    for (size_t i = 0; i < obj.strings.size(); ++i) os << "  [" << i << "] " << obj.strings[i] << "\n";
    os << "names: " << obj.names.size() << "\n";
    for (size_t i = 0; i < obj.names.size(); ++i) os << "  [" << i << "] " << obj.names[i] << "\n";
    os << "inherits: " << obj.inherits.size() << "\n";
    for (size_t i = 0; i < obj.inherits.size(); ++i) {
        os << "  " << i << ": " << obj.inherits[i].path
           << " baseFunOffset=" << obj.inherits[i].baseFunOffset
           << " baseVarOffset=" << obj.inherits[i].baseVarOffset << "\n";
    }
    os << "functions: " << obj.functions.size() << "\n";
    for (size_t i = 0; i < obj.functions.size(); ++i) {
        const auto& fn = obj.functions[i];
        os << "  " << i << ": " << fn.name << " dlt=" << fn.index << " args=" << fn.argCount << " locals=" << fn.localCount
           << " nodes=" << fn.code.size() << "\n";
        for (size_t j = 0; j < fn.code.size(); ++j) {
            const auto& in = fn.code[j];
            os << "    " << j << ": op=" << static_cast<int>(in.op) << " " << actionName(in.op)
               << " a=" << in.a << " b=" << static_cast<int>(in.b) << "\n";
        }
    }
    os << "global nodes: " << obj.globalCode.size() << "\n";
    for (size_t j = 0; j < obj.globalCode.size(); ++j) {
        const auto& in = obj.globalCode[j];
        os << "  " << j << ": op=" << static_cast<int>(in.op) << " " << actionName(in.op)
           << " a=" << in.a << " b=" << static_cast<int>(in.b) << "\n";
    }
    return os.str();
}

} // namespace lpc
