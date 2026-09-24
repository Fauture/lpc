#pragma once
#include <cstdint>
#include <memory>
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
    // FNV-1a 64 of `name`, filled once by the loader.  findFunctionAny() scans
    // every function of the constitution twice per name lookup; comparing this
    // hash first turns ~all of those std::string compares into an integer
    // compare (measured 43us -> a few us per lookup) without per-VM containers.
    uint64_t nameHash = 0;
    int index = -1;
    int argCount = 0;
    int localCount = 0;
    std::vector<OriginalInsn> code;
    std::vector<uint8_t> metaData;
    // String table of the program this function belongs to.  Shared-string
    // indexes inside the bytecode resolve against THIS table (inherited
    // functions keep their own program's strings, not the inheritor's).
    // Shared pointer: the master program copies ALL its functions into one
    // shared table (copying the whole table per function cost ~500MB for
    // start_gs.o's 1308 functions x 2978 strings).
    std::shared_ptr<const std::vector<std::string>> strings;
    // Accumulated baseVarOffset of the constitution this function belongs to.
    // push_obj_var/set_obj_var/push_obj_vptr add it to the variable index
    // (vm_pushObjVar: pObjectValues[valueNo + consititution->baseVarOffset]).
    int varOffset = 0;
    // Accumulated baseFunOffset of the constitution this function belongs to.
    // call_in_user_fun resolves local function indexes as
    // consititution->baseFunOffset + index (vm_callInUserFun 0x8094080).
    int funOffset = 0;
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
    // Per-constitution PROG_VARS records (the original driver's pProgVars): they
    // map a variable NAME to its slot index (valueNo + baseVarOffset), which is
    // exactly what get_obj_var / set_obj_var / save_object / restore_object need.
    // These records used to be parsed and thrown away, so every name-based
    // instance-variable access was a no-op and an object's dbase could never be
    // restored: login's "chars" stayed undefined, the character list sent after
    // a create came back empty (0xF061 count=0) and the account link
    // (basic_char_info) was never written.
    struct ProgVarRec {
        uint8_t raw[16] = {0};
        std::string name;      // pVarName
    };
    std::vector<ProgVarRec> progVars;
};

// FNV-1a 64 over the bytes of `s` (the efun dispatcher uses the same
// construction, so a missing hash is easy to spot: 0 means "not computed").
uint64_t lpcNameHash(const std::string& s);

OriginalObject loadOriginalObject(const std::string& path);
OriginalObject loadOriginalObjectFromBytes(const std::vector<uint8_t>& data);
std::string disassembleOriginalObject(const OriginalObject& obj);
std::string actionName(uint8_t op);

} // namespace lpc
