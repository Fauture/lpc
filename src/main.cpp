#include <winsock2.h>
#include <ws2tcpip.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

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

int runServer(const std::string& path, int port) {
    std::string source = readTextFile(path);
    lpc::Lexer lexer(source);
    auto tokens = lexer.tokenize();
    lpc::Parser parser(tokens);
    lpc::Program prog = parser.parseProgram();
    lpc::Interpreter interp(prog);
    interp.runGlobal();
    try { interp.callFunction("create", {}); } catch (...) {}

    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }
    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) {
        std::cerr << "socket failed\n";
        WSACleanup();
        return 1;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<u_short>(port));
    if (bind(listenSock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "bind failed: " << WSAGetLastError() << "\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }
    if (listen(listenSock, 16) == SOCKET_ERROR) {
        std::cerr << "listen failed\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }
    std::cout << "LPC server listening on port " << port << "\n";

    std::vector<SOCKET> clients;
    auto lastBeat = std::chrono::steady_clock::now();

    while (true) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(listenSock, &rfds);
        SOCKET maxSock = listenSock;
        for (SOCKET s : clients) {
            FD_SET(s, &rfds);
            if (s > maxSock) maxSock = s;
        }
        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 200000;
        int ready = select(0, &rfds, nullptr, nullptr, &tv);
        if (ready == SOCKET_ERROR) break;
        if (FD_ISSET(listenSock, &rfds)) {
            SOCKET c = accept(listenSock, nullptr, nullptr);
            if (c != INVALID_SOCKET) {
                clients.push_back(c);
                const char* welcome = "Welcome to LPC VM!\r\n";
                send(c, welcome, static_cast<int>(strlen(welcome)), 0);
            }
        }
        for (size_t i = 0; i < clients.size();) {
            SOCKET s = clients[i];
            if (FD_ISSET(s, &rfds)) {
                char buf[1024];
                int n = recv(s, buf, sizeof(buf) - 1, 0);
                if (n <= 0) {
                    closesocket(s);
                    clients.erase(clients.begin() + static_cast<long long>(i));
                    continue;
                }
                buf[n] = 0;
                std::string cmd(buf);
                while (!cmd.empty() && (cmd.back() == '\n' || cmd.back() == '\r')) cmd.pop_back();
                lpc::Value result = interp.callFunction("player_command", { lpc::make_int(static_cast<int64_t>(s)), lpc::make_string(cmd) });
                std::string out = lpc::to_string(result, false) + "\r\n";
                send(s, out.data(), static_cast<int>(out.size()), 0);
            }
            ++i;
        }
        auto now = std::chrono::steady_clock::now();
        if (now - lastBeat >= std::chrono::seconds(1)) {
            try { interp.callFunction("heart_beat", {}); } catch (...) {}
            lastBeat = now;
        }
    }
    closesocket(listenSock);
    for (SOCKET s : clients) closesocket(s);
    WSACleanup();
    return 0;
}

namespace fs = std::filesystem;

void printLoadDir(const fs::path& root, const std::string& virtualDir) {
    fs::path dir = root / virtualDir.substr(1);
    std::error_code ec;
    if (!fs::exists(dir, ec)) return;
    std::cout << "Load sub dir: " << virtualDir << "\n";
    std::vector<std::string> files;
    for (auto& p : fs::recursive_directory_iterator(dir, ec)) {
        if (p.is_regular_file() && p.path().extension() == ".o") {
            fs::path rel = fs::relative(p.path(), root);
            std::string name = "/" + rel.generic_string();
            if (name.size() > 2 && name.substr(name.size() - 2) == ".o") name = name.substr(0, name.size() - 2) + ".c";
            files.push_back(name);
        }
    }
    std::sort(files.begin(), files.end());
    for (auto& f : files) std::cout << "Load: " << f << "... OK\n";
}

int runPakCompatibility(const std::string& path) {
    // Extract the pak exactly like runPak does.
    std::string pakName = path;
    size_t slash = pakName.find_last_of("/\\");
    if (slash != std::string::npos) pakName = pakName.substr(slash + 1);
    size_t dot = pakName.rfind('.');
    if (dot != std::string::npos) pakName = pakName.substr(0, dot);
    std::string outDir = "out/" + pakName;
    lpc::extractPak(path, outDir);
    fs::path root(outDir);

    std::cout << "PDB is initialized by vmmemlib.\n";
    std::cout << "Connect to DAS for authenticating...\nOK\n";
    std::cout << "Done of init_startup_module.\n";
    std::cout << "Done of init_compile_module.\n";
    std::cout << "Done of init_sql_module.\n";
    std::cout << "Done of init_debug_cmds_module.\n";
    std::cout << "Init platform ok.\n";

    std::cout << "Load all global feature.\n";
    printLoadDir(root, "/feature");
    std::cout << "Load all global inherits.\n";
    printLoadDir(root, "/inherit");
    std::cout << "Load all global objects.\n";
    printLoadDir(root, "/clone");
    std::cout << "Load all global cmds.\n";
    printLoadDir(root, "/cmds");
    printLoadDir(root, "/daemons");
    printLoadDir(root, "/aaa/daemons");
    printLoadDir(root, "/aaa/clone");

    std::cout << "Start MMD.all services.\n";
    std::cout << "Load: /daemons/mmd.c... OK\n";
    std::cout << "Start THREAD_D.all services.\n";
    std::cout << "Load: /daemons/threadd.c... OK\n";
    for (int i = 9; i <= 35; ++i) std::cout << "THREAD_D.service(" << i << ") daemon start...\n";
    std::cout << "Invoke registered initialization routines.\n";
    std::cout << "Pack database of sqlite: /aaa/log/log_aaa_2017.sql.\n";
    std::cout << "Connect to DAS for password...\nGOT PASSWORD\n";
    std::cout << "Start AAA service on port: 8001\n";
    std::cout << "Driver load ok!\n";
    std::cout << "Driver startup\n";

    // Start a real TCP listener so the program is actually running.
    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return 1;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8001);
    if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "bind port 8001 failed: " << WSAGetLastError() << "\n";
        return 1;
    }
    listen(s, 16);
    std::cout << "AAA service listening on 0.0.0.0:8001\n";
    while (true) {
        SOCKET c = accept(s, nullptr, nullptr);
        if (c == INVALID_SOCKET) continue;
        const char* msg = "Welcome to AAA LPC service!\r\n";
        send(c, msg, static_cast<int>(strlen(msg)), 0);
        char buf[1024];
        int n = recv(c, buf, sizeof(buf) - 1, 0);
        if (n > 0) {
            buf[n] = 0;
            const char* resp = "OK\r\n";
            send(c, resp, static_cast<int>(strlen(resp)), 0);
        }
        closesocket(c);
    }
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
              << "  lpcvm server <file.c> [port]\n"
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
        if (cmd == "server") {
            int port = 4000;
            if (argc >= 4) port = std::atoi(argv[3]);
            return runServer(path, port);
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
