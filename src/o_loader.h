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
};

struct OriginalInherit {
    std::string path;       // e.g. "/feature/daemon/register.c"
    int baseFunOffset = 0;  // inherited functions are mapped to
                            // baseFunOffset + <function index in the parent>
};

struct OriginalObject {
    uint32_t magic = 0;
    uint32_t version = 0;
    uint32_t functionBC = 0;
    uint32_t globalVarCount = 0;
    int baseFunOffset = 0;
    std::vector<std::string> strings;
    std::vector<std::string> names;
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
