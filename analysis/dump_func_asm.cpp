// Disassemble a specific function's bytecode from a .o file.
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <cstring>

#include "o_loader.h"
#include "pak_loader.h"

using namespace lpc;

static const char* opName(uint8_t op) {
    static const char* names[] = {
        "NOOP","DUP","POP","LOD","STR","NUM","OBJ","CLOS","FUN","CALL",
        "CALLR","RET","JMP","JZ","JNZ","SWITCH","SWITCHC","ADD","SUB","MUL",
        "DIV","MOD","NEG","INC","DEC","NOT","GT","LT","GE","LE","EQ","NE",
        "AND","OR","BITAND","BITOR","BITXOR","SHL","SHR","LODV","STOV","LODG",
        "STOG","LODS","STOS","LODA","STOA","PUSHN","PUSHA","ARR","MAP","INDEX",
        "INDEXA","REF","DEREF","MKREF","CALL_OTHER","CALL_OTHER_FUN","CALL_IN_OTHER_FUN",
        "EXPR","INIT","FOREACH","FOREACH_END","LOCAL_FUN","GLOBAL_FUN","EFUN",
        "PUSH_LCL","PUSH_ARG","PUSH_GLOBAL","PUSH_OBJ","PUSH_VAR","PUSH_VPTR",
        "SET_VAR","SET_VPTR","CALL_FUN","CALL_FUN_R","RET_VAL","SPREAD","SPREAD_END",
        "BREAK","CONTINUE","TRY","CATCH","THROW","ASSERT","FORMAT","SPRINTF"
    };
    if (op < 80) return names[op];
    static char buf[32];
    std::snprintf(buf, sizeof(buf), "OP%u", op);
    return buf;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("usage: dump_func_asm <file.o> <funcname>\n");
        return 1;
    }
    OriginalObject obj;
    try {
        obj = loadOriginalObject(argv[1]);
    } catch (const std::exception& e) {
        std::printf("ERROR loading %s: %s\n", argv[1], e.what());
        return 1;
    }
    const OriginalFunction* target = nullptr;
    for (const auto& fn : obj.functions) {
        if (fn.name == argv[2]) { target = &fn; break; }
    }
    if (!target) {
        std::printf("function %s not found in %s\n", argv[2], argv[1]);
        return 1;
    }
    std::printf("=== %s::%s code=%zu args=%d ===\n", argv[1], target->name.c_str(),
                target->code.size(), target->argCount);
    const uint8_t* p = target->code.data();
    size_t n = target->code.size();
    size_t i = 0;
    while (i < n) {
        size_t start = i;
        uint8_t op = p[i++];
        std::printf("%04zu: %-16s", start, opName(op));
        if (op == 40 || op == 41) { // LODG/STOG: global index
            if (i + 2 <= n) { uint16_t idx = p[i] | (p[i+1] << 8); i += 2; std::printf(" g=%u", idx); }
        } else if (op == 36 || op == 37) { // LODV/STOV: var index
            if (i + 2 <= n) { uint16_t idx = p[i] | (p[i+1] << 8); i += 2; std::printf(" v=%u", idx); }
        } else if (op >= 62 && op <= 69) { // push/set var/vptr
            if (i + 2 <= n) { uint16_t idx = p[i] | (p[i+1] << 8); i += 2; std::printf(" v=%u", idx); }
        } else if (op >= 4 && op <= 9) { // LOD/STR/NUM/OBJ/CLOS/FUN
            if (i + 2 <= n) { uint16_t idx = p[i] | (p[i+1] << 8); i += 2; std::printf(" idx=%u", idx); }
        } else if (op == 9 || op == 10 || op == 55 || op == 56 || op == 70 || op == 71) {
            // FUN/CALL/CALLR/LOCAL_FUN/GLOBAL_FUN/CALL_FUN: 2-byte index + flags
            if (i + 2 <= n) { uint16_t idx = p[i] | (p[i+1] << 8); i += 2; std::printf(" f=%u", idx); }
        } else if (op == 12 || op == 13 || op == 14) { // JMP/JZ/JNZ
            if (i + 4 <= n) { int32_t off = p[i] | (p[i+1]<<8) | (p[i+2]<<16) | (p[i+3]<<24); i += 4; std::printf(" ->%d", start + 4 + off); }
        } else if (op == 57 || op == 58 || op == 59 || op == 72) {
            // CALL_OTHER/CALL_OTHER_FUN/CALL_IN_OTHER_FUN/CALL_FUN_R: 2-byte argcount + 2-byte index
            if (i + 4 <= n) { uint16_t ac = p[i] | (p[i+1]<<8); uint16_t idx = p[i+2] | (p[i+3]<<8); i += 4; std::printf(" argc=%u f=%u", ac, idx); }
        } else if (op == 60 || op == 61) { // EFUN: 2-byte index
            if (i + 2 <= n) { uint16_t idx = p[i] | (p[i+1] << 8); i += 2; std::printf(" efun=%u", idx); }
        } else if (op == 75 || op == 76) { // SPREAD/SPREAD_END
        } else {
            std::printf(" ??");
        }
        std::printf("\n");
    }
    return 0;
}
