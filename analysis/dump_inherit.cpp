// Dump inherit records + variable info from .o files (uses the lpc loader).
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "o_loader.h"
#include "pak_loader.h"

using namespace lpc;

static void dumpOne(const std::string& path) {
    OriginalObject obj;
    try {
        obj = loadOriginalObject(path);
    } catch (const std::exception& e) {
        std::printf("ERROR loading %s: %s\n", path.c_str(), e.what());
        return;
    }
    std::printf("=== %s ===\n", path.c_str());
    std::printf("  magic=%08x version=%u functionBC=%u globalVarCount=%u\n",
                obj.magic, obj.version, obj.functionBC, obj.globalVarCount);
    std::printf("  strings=%zu names=%zu functions=%zu globalNodes=%zu inherits=%zu\n",
                obj.strings.size(), obj.names.size(), obj.functions.size(),
                obj.globalCode.size(), obj.inherits.size());
    for (size_t i = 0; i < obj.inherits.size(); ++i) {
        const auto& inh = obj.inherits[i];
        std::printf("  inherit[%zu] path=%s baseFunOffset=%d\n", i, inh.path.c_str(), inh.baseFunOffset);
    }
    std::printf("  --- variables (names) ---\n");
    for (size_t i = 0; i < obj.names.size() && i < 80; ++i) {
        std::printf("    var[%zu] = %s\n", i, obj.names[i].c_str());
    }
    if (obj.names.size() > 80) std::printf("    ... (%zu more)\n", obj.names.size() - 80);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("usage: dump_inherit <file.o> [file2.o ...]\n");
        return 1;
    }
    for (int i = 1; i < argc; ++i) dumpOne(argv[i]);
    return 0;
}
