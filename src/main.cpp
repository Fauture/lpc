#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "interpreter.h"
#include "lexer.h"
#include "o_loader.h"
#include "parser.h"

namespace {

std::string readTextFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open file: " + path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

bool hasSuffix(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

int runLpcSource(const std::string& path) {
    std::string source = readTextFile(path);
    lpc::Lexer lexer(source);
    auto tokens = lexer.tokenize();
    lpc::Parser parser(tokens);
    lpc::Program prog = parser.parseProgram();

    lpc::Interpreter interp(prog);
    interp.runGlobal();
    // In LPC, after loading an object the driver calls create() if present.
    try {
        interp.callFunction("create", {});
    } catch (...) {
        // create is optional
    }
    return 0;
}

int runOriginalObject(const std::string& path) {
    lpc::OriginalObject obj = lpc::loadOriginalObject(path);
    std::cout << "Loaded original .o: " << path << "\n";
    std::cout << "  strings=" << obj.strings.size()
              << " functions=" << obj.functions.size()
              << " globalNodes=" << obj.globalCode.size() << "\n";
    bool trivial = true;
    for (const auto& in : obj.globalCode) {
        if (in.op != 29 && in.op != 46 && in.op != 72) {
            trivial = false;
            break;
        }
    }
    if (trivial) {
        std::cout << "  Global code is trivial (null/finish/return), executed OK.\n";
    } else {
        std::cout << "  Original bytecode execution is not fully implemented; use `disasm` to inspect.\n";
    }
    return 0;
}

int disasmOriginalObject(const std::string& path) {
    lpc::OriginalObject obj = lpc::loadOriginalObject(path);
    std::cout << lpc::disassembleOriginalObject(obj);
    return 0;
}

void usage() {
    std::cout << "Usage:\n"
              << "  lpcvm run <file.c|file.o>\n"
              << "  lpcvm disasm <file.o>\n";
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc < 3) {
            usage();
            return 1;
        }
        std::string cmd = argv[1];
        std::string path = argv[2];
        if (cmd == "run") {
            if (hasSuffix(path, ".o")) return runOriginalObject(path);
            return runLpcSource(path);
        }
        if (cmd == "disasm") {
            return disasmOriginalObject(path);
        }
        usage();
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
