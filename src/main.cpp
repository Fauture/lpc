#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "interpreter.h"
#include "lexer.h"
#include "o_loader.h"
#include "original_vm.h"
#include "pak_loader.h"
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
    std::string searchRoot;
    size_t slash = path.find_last_of("/\\");
    if (slash != std::string::npos) searchRoot = path.substr(0, slash);
    return lpc::runOriginalObject(obj, std::cout, searchRoot);
}

int disasmOriginalObject(const std::string& path) {
    lpc::OriginalObject obj = lpc::loadOriginalObject(path);
    std::cout << lpc::disassembleOriginalObject(obj);
    return 0;
}

int runPak(const std::string& path) {
    lpc::PakFile pak = lpc::loadPak(path);
    const lpc::PakEntry* entry = lpc::findPakEntry(pak, "start_aaa.o");
    if (!entry) entry = lpc::findPakEntry(pak, "aaa/start_aaa.o");
    if (!entry || entry->isDir) {
        std::cerr << "Pak does not contain start_aaa.o\n";
        return 1;
    }
    // Extract whole pak to out/<pakname> so the .o can be loaded by the existing loader.
    std::string pakName = path;
    size_t slash = pakName.find_last_of("/\\");
    if (slash != std::string::npos) pakName = pakName.substr(slash + 1);
    size_t dot = pakName.rfind('.');
    if (dot != std::string::npos) pakName = pakName.substr(0, dot);
    std::string outDir = "out/" + pakName;
    lpc::extractPak(path, outDir);
    std::string oPath = outDir + "/" + entry->name;
    std::cout << "Extracted " << entry->name << " to " << oPath << "\n";
    lpc::OriginalObject obj = lpc::loadOriginalObject(oPath);
    std::cout << "Loaded original .o: " << oPath << "\n";
    std::cout << "  strings=" << obj.strings.size()
              << " functions=" << obj.functions.size()
              << " globalNodes=" << obj.globalCode.size() << "\n";
    return lpc::runOriginalObject(obj, std::cout, outDir);
}

void usage() {
    std::cout << "Usage:\n"
              << "  lpcvm run <file.c|file.o|file.pak>\n"
              << "  lpcvm disasm <file.o>\n"
              << "  lpcvm unpack <file.pak> [outdir]\n";
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
            if (hasSuffix(path, ".pak")) return runPak(path);
            if (hasSuffix(path, ".o")) return runOriginalObject(path);
            return runLpcSource(path);
        }
        if (cmd == "disasm") {
            return disasmOriginalObject(path);
        }
        if (cmd == "unpack") {
            std::string outDir = "out";
            if (argc >= 4) outDir = argv[3];
            lpc::extractPak(path, outDir);
            std::cout << "Unpacked " << path << " to " << outDir << "\n";
            return 0;
        }
        usage();
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
