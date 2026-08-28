// Dump function tables of .o files: name, index, code size, argCount.
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <map>

#include "o_loader.h"
#include "pak_loader.h"

using namespace lpc;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("usage: dump_funcs <file.o> [file2.o ...]\n");
        return 1;
    }
    for (int a = 1; a < argc; ++a) {
        OriginalObject obj;
        try {
            obj = loadOriginalObject(argv[a]);
        } catch (const std::exception& e) {
            std::printf("ERROR loading %s: %s\n", argv[a], e.what());
            continue;
        }
        std::printf("=== %s ===\n", argv[a]);
        std::printf("  functions=%zu inherits=%zu globalVarCount=%u\n",
                    obj.functions.size(), obj.inherits.size(), obj.globalVarCount);
        std::map<int, const OriginalFunction*> byIndex;
        for (const auto& fn : obj.functions) byIndex[fn.index] = &fn;
        for (const auto& kv : byIndex) {
            const OriginalFunction& fn = *kv.second;
            std::printf("  fn[%4d] name=%-32s code=%5zu args=%d\n",
                        kv.first, fn.name.c_str(), fn.code.size(), fn.argCount);
        }
        std::printf("  --- inherits ---\n");
        for (size_t i = 0; i < obj.inherits.size(); ++i)
            std::printf("    inherit[%zu] path=%s baseFunOffset=%d baseVarOffset=%d\n",
                        i, obj.inherits[i].path.c_str(),
                        obj.inherits[i].baseFunOffset, obj.inherits[i].baseVarOffset);
    }
    return 0;
}
