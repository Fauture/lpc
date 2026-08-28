// Dump the full disassembly of a .o file, then print only functions matching a name.
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "o_loader.h"
#include "pak_loader.h"

using namespace lpc;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("usage: dump_disasm <file.o> [funcname-substr]\n");
        return 1;
    }
    OriginalObject obj;
    try {
        obj = loadOriginalObject(argv[1]);
    } catch (const std::exception& e) {
        std::printf("ERROR loading %s: %s\n", argv[1], e.what());
        return 1;
    }
    std::string out = disassembleOriginalObject(obj);
    if (argc >= 3) {
        std::string want = argv[2];
        // print lines around occurrences of want
        std::vector<std::string> lines;
        std::string cur;
        for (char ch : out) {
            if (ch == '\n') { lines.push_back(cur); cur.clear(); }
            else cur.push_back(ch);
        }
        if (!cur.empty()) lines.push_back(cur);
        for (size_t i = 0; i < lines.size(); ++i) {
            if (lines[i].find(want) != std::string::npos) {
                size_t start = i > 3 ? i - 3 : 0;
                size_t end = i + 25 < lines.size() ? i + 25 : lines.size();
                for (size_t j = start; j < end; ++j)
                    std::printf("%s\n", lines[j].c_str());
                std::printf("---\n");
            }
        }
    } else {
        std::printf("%s", out.c_str());
    }
    return 0;
}
