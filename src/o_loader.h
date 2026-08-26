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
    int argCount = 0;
    int localCount = 0;
    std::vector<OriginalInsn> code;
};

struct OriginalObject {
    uint32_t magic = 0;
    uint32_t version = 0;
    uint32_t functionBC = 0;
    std::vector<std::string> strings;
    std::vector<std::string> names;
    std::vector<OriginalFunction> functions;
    std::vector<OriginalInsn> globalCode;
};

OriginalObject loadOriginalObject(const std::string& path);
std::string disassembleOriginalObject(const OriginalObject& obj);
std::string actionName(uint8_t op);

} // namespace lpc
