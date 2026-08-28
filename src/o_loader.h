#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace lpc {

struct OriginalInsn {
    uint8_t op = 0;
    uint16_t a = 0;
    uint8_t b = 0;
};

struct OriginalFunction {
    std::string name;
    int index = -1;
    int argCount = 0;
    int localCount = 0;
    std::vector<OriginalInsn> code;
    std::vector<uint8_t> metaData;
    // String table of the program this function belongs to.  Shared-string
    // indexes inside the bytecode resolve against THIS table (inherited
    // functions keep their own program's strings, not the inheritor's).
    std::vector<std::string> strings;
    // Accumulated baseVarOffset of the constitution this function belongs to.
    // push_obj_var/set_obj_var/push_obj_vptr add it to the variable index
    // (vm_pushObjVar: pObjectValues[valueNo + consititution->baseVarOffset]).
    int varOffset = 0;
};

struct OriginalInherit {
    std::string path;       // e.g. "/feature/daemon/register.c"
    int baseFunOffset = 0;  // inherited functions are mapped to
                            // baseFunOffset + <function index in the parent>
    int baseVarOffset = 0;  // inherited instance variables are mapped to
                            // baseVarOffset + <variable index in the parent>
                            // (vm_pushObjVar/setObjVar add the current
                            // constitution's accumulated baseVarOffset)
};

struct OriginalObject {
    uint32_t magic = 0;
    uint32_t version = 0;
    uint32_t functionBC = 0;
    uint32_t globalVarCount = 0;
    int baseFunOffset = 0;
    std::string sourceName;   // originating file (diagnostics)
    std::vector<std::string> strings;
    std::vector<std::string> names;
    // Preprocessor defines from the .o binary (name -> expansion string).
    // The runtime can look these up via expand_macro / vm_lookupDefine (the
    // packet protocol analyser resolves command numbers from them).
    std::vector<std::pair<std::string, std::string>> defines;
    std::vector<OriginalFunction> functions;
    std::vector<OriginalInsn> globalCode;
    std::vector<uint8_t> globalMeta;
    std::vector<OriginalInherit> inherits;
};

OriginalObject loadOriginalObject(const std::string& path);
OriginalObject loadOriginalObjectFromBytes(const std::vector<uint8_t>& data);
std::string disassembleOriginalObject(const OriginalObject& obj);
std::string actionName(uint8_t op);

} // namespace lpc
