#include <winsock2.h>
#include <ws2tcpip.h>

#include <windows.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "interpreter.h"
#include "lexer.h"
#include "lzss.h"
#include "builtins.h"
#include "mysql_client.h"
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
    interp.runThreads(5);
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

int opStats(const std::string& path) {
    lpc::OriginalObject obj = lpc::loadOriginalObject(path);
    std::map<int, int> global, funcs;
    for (const auto& in : obj.globalCode) global[in.op]++;
    for (const auto& fn : obj.functions)
        for (const auto& in : fn.code) funcs[in.op]++;
    std::cout << "object: " << path << "\n";
    std::cout << "opcode histogram (global code):\n";
    for (auto& [op, n] : global)
        std::cout << "  " << op << " " << lpc::actionName(static_cast<uint8_t>(op)) << " x" << n << "\n";
    std::cout << "opcode histogram (functions):\n";
    for (auto& [op, n] : funcs)
        std::cout << "  " << op << " " << lpc::actionName(static_cast<uint8_t>(op)) << " x" << n << "\n";
    return 0;
}

int disasmFunction(const std::string& path, const std::string& fnName) {
    lpc::OriginalObject obj = lpc::loadOriginalObject(path);
    for (const auto& fn : obj.functions) {
        if (fn.name == fnName) {
            std::cout << "function: " << fn.name << " args=" << fn.argCount
                      << " locals=" << fn.localCount << " nodes=" << fn.code.size() << "\n";
            for (size_t j = 0; j < fn.code.size(); ++j) {
                const auto& in = fn.code[j];
                std::cout << "  " << j << ": op=" << static_cast<int>(in.op) << " "
                          << lpc::actionName(in.op) << " par16=" << static_cast<int16_t>(in.a)
                          << " par8=" << static_cast<int>(in.b) << "\n";
            }
            return 0;
        }
    }
    std::cout << "function not found: " << fnName << "\n";
    return 1;
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
    interp.runThreads(3);

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


int startSimpleTcpServer(int port) {
    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return 1;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<u_short>(port));
    if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "bind port " << port << " failed: " << WSAGetLastError() << "\n";
        return 1;
    }
    listen(s, 16);
    std::cout << "AAA service listening on 0.0.0.0:" << port << "\n";
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

// Debug helper: run one SQL statement against the configured MySQL and print
// the result as a table (reads [ADB]/[MDB] from linux/aaa/aaa/aaa.ini).
int sqlDebug(const std::string& sql, const std::string& which) {
    lpc::MysqlClient cl;
    std::string err;
    // same defaults assure_connection uses (aaa.ini [ADB])
    std::string host = "192.168.200.128";
    std::string user = "root";
    std::string pass = "123456";
    std::string db = (which == "mdb") ? "dl_mdb_all" : "dl_adb_all";
    int port = 3306;
    lpc::MysqlClient::Result res;
    if (!cl.connect(host, port, user, pass, db, err)) {
        std::cerr << "connect failed: " << err << "\n";
        return 1;
    }
    if (!cl.execute(sql, res, err)) {
        std::cerr << "query failed: " << err << "\n";
        return 1;
    }
    if (!res.hasResultSet) {
        std::cout << "affected rows: " << res.affectedRows << "\n";
        return 0;
    }
    for (size_t i = 0; i < res.cols.size(); ++i) {
        if (i) std::cout << " | ";
        std::cout << res.cols[i];
    }
    std::cout << "\n";
    for (const auto& row : res.rows) {
        for (size_t i = 0; i < row.size(); ++i) {
            if (i) std::cout << " | ";
            const auto& c = row[i];
            if (c.isNull) std::cout << "NULL";
            else if (c.isNumber) std::cout << c.num;
            else std::cout << c.str;
        }
        std::cout << "\n";
    }
    return 0;
}

int runPakOriginal(const std::string& path) {
    // Faithful driver emulation of "runaaa":
    //   ./magic_Linux32 /r ./ /e /aaa/pack_data/lib_aaa32.pak/aaa/start_aaa.o
    //                    /b /aaa/pack_data/lib_aaa32.pak ...
    // The pak is mounted as the binary directory (any ".pak/" path is a
    // packed volume, vm_isInPackedVolume), the boot object comes from inside
    // the pak, and object loading is driven entirely by the LPC startup code
    // in start_aaa.o.
    std::cout << "PDB is initialized by vmmemlib.\n";   // vmmemlib static init (0x816eb20)
    std::error_code ec;
    if (!fs::exists(path, ec)) {
        std::cerr << "Missing pak file: " << path << "\n";
        return 1;
    }
    std::string absPak = fs::absolute(fs::path(path)).string();
    std::string pakFile = fs::path(path).filename().string();

    // Per-project profile (aaa / dba / ccs / gs).  The boot object, the
    // extracted .o tree, the run directory and the pak mount prefix all derive
    // from the pak filename: lib_<prefix>32.pak -> <prefix>.
    std::string objPrefix = "aaa";
    {
        std::string pf = pakFile;
        size_t p = pf.rfind("32.pak");
        if (p != std::string::npos) pf = pf.substr(0, p);
        if (pf.rfind("lib_", 0) == 0) pf = pf.substr(4);
        if (!pf.empty()) objPrefix = pf;
    }
    std::string bootRel = objPrefix + "/start_" + objPrefix + ".o";
    std::string vfsRoot = "linux/" + objPrefix + "/pack_data/lib_" + objPrefix + "32";
    std::string dataRoot = "linux";
    std::string etcDir = "linux/" + objPrefix + "/pack_data/etc";
    std::string pakPrefix = "/" + objPrefix + "/pack_data/";
    std::string prefix = pakPrefix + pakFile;
    lpc::setVfsProfile(vfsRoot, dataRoot, etcDir, pakPrefix, objPrefix);
    lpc::setPakMount(absPak, prefix);

    // The original boot scripts pass /D CONFIG=<prefix>/<prefix>.ini (rungs:
    // /D CONFIG=gs/gs1.ini).  start_<prefix>.o resolves INI_FILE via
    // expand_macro("CONFIG") with a "/<prefix>/game_server.ini" fallback that
    // only aaa/dba/ccs happen to match; inject the real config file so gs
    // reads gs1.ini (and the others read their own ini) instead of falling
    // back to the missing game_server.ini.
    std::string iniRel = objPrefix + "/" + objPrefix + ".ini";
    if (objPrefix == "gs") iniRel = "gs/gs1.ini";  // rungs: /D CONFIG=gs/gs1.ini
    lpc::injectDefine("CONFIG", iniRel);

    // Boot object: <pak>/<objPrefix>/start_<objPrefix>.o (runaaa/rundba -e).
    lpc::OriginalObject obj;
    std::vector<uint8_t> bootBytes;
    bool fromPak = lpc::pakReadFile(prefix + "/" + bootRel, bootBytes);
    if (fromPak) {
        obj = lpc::loadOriginalObjectFromBytes(bootBytes);
        std::cout << "Loaded boot object from pak: " << prefix << "/" << bootRel << "\n";
    } else {
        std::string oPath = vfsRoot + "/" + bootRel;
        if (!fs::exists(oPath, ec)) {
            std::cerr << "Missing boot object (pak has no " << bootRel
                      << " and no " << oPath << ")\n";
            return 1;
        }
        obj = lpc::loadOriginalObject(oPath);
        std::cout << "Loaded boot object from extracted tree: " << oPath << "\n";
    }
    std::cout << "  strings=" << obj.strings.size()
              << " functions=" << obj.functions.size()
              << " globalNodes=" << obj.globalCode.size() << "\n";
    // The extracted tree stays as the fallback data root (it mirrors the pak
    // contents for any path the pak itself cannot serve).
    std::string root = vfsRoot;
    int rc = lpc::runOriginalObject(obj, std::cout, root);
    std::cout.flush();

    if (rc != 0) return rc;
    // Keep the driver alive: the cooperative scheduler drives daemon
    // processes, timers and queue/event wakeups (stage B).  It returns only
    // when no work remains (all processes finished / parked forever).
    lpc::schedulerRun();
    return 0;
}

int runPak(const std::string& path) {
    // Execute the pak through the original bytecode VM. Network/service startup
    // is expected to be performed by the LPC code itself, not by this C++ launcher.
    return runPakOriginal(path);
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
    // SEH crash catcher: print a backtrace so a segfault inside the VM or a
    // builtin (the current MSVCP140.dll 0xc0000005 crashes) can be located
    // without a debugger attached.
    SetUnhandledExceptionFilter([](EXCEPTION_POINTERS* ep) -> LONG {
        void* frames[32];
        USHORT n = CaptureStackBackTrace(0, 32, frames, nullptr);
        std::cerr << "[crash] code=0x" << std::hex
                  << (ep ? ep->ExceptionRecord->ExceptionCode : 0)
                  << " addr=0x" << (ep ? ep->ExceptionRecord->ExceptionAddress : 0)
                  << std::dec << " frames=" << n << "\n";
        for (USHORT i = 0; i < n; ++i) {
            HMODULE mod = nullptr;
            char modName[MAX_PATH] = "?";
            GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               static_cast<LPCSTR>(frames[i]), &mod);
            if (mod) GetModuleFileNameA(mod, modName, MAX_PATH);
            std::string mn = modName;
            size_t bs = mn.find_last_of('\\');
            if (bs != std::string::npos) mn = mn.substr(bs + 1);
            uintptr_t base = mod ? reinterpret_cast<uintptr_t>(mod) : 0;
            std::cerr << "  #" << i << " 0x" << std::hex << frames[i] << std::dec
                      << " rva=0x" << std::hex
                      << (base ? (reinterpret_cast<uintptr_t>(frames[i]) - base) : 0)
                      << std::dec << " in " << mn << "\n";
        }
        std::cerr.flush();
        return EXCEPTION_CONTINUE_SEARCH;   // let the OS generate the dump
    });
    try {
        // Driver-level telnet options (mirroring the original driver's
        // -w <password> / -p <port>): telnet is only enabled when a port is
        // configured.  Without these options the driver does not listen.
        std::string telnetPwd;
        int telnetPort = 0;
        std::vector<std::string> args;
        for (int i = 1; i < argc; ++i) {
            if (std::strcmp(argv[i], "-w") == 0 && i + 1 < argc) {
                telnetPwd = argv[++i];
            } else if (std::strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
                telnetPort = std::atoi(argv[++i]);
            } else {
                args.push_back(argv[i]);
            }
        }
        if (!telnetPwd.empty()) lpc::setDriverTelnetPassword(telnetPwd);
        if (telnetPort >= 1024) lpc::startDriverTelnet(telnetPort);

        if (args.size() < 2) {
            usage();
            return 1;
        }
        std::string cmd = args[0];
        std::string path = args[1];
        if (cmd == "run") {
            if (hasSuffix(path, ".pak")) return runPak(path);
            if (hasSuffix(path, ".o")) return runOriginalObject(path);
            return runLpcSource(path);
        }
        if (cmd == "orig") {
            if (hasSuffix(path, ".pak")) return runPakOriginal(path);
            return runOriginalObject(path);
        }
        if (cmd == "disasm") {
            return disasmOriginalObject(path);
        }
        if (cmd == "opstats") {
            return opStats(path);
        }
        if (cmd == "disfn") {
            if (args.size() < 3) { usage(); return 1; }
            return disasmFunction(path, args[2]);
        }
        if (cmd == "server") {
            int port = 4000;
            if (args.size() >= 3) port = std::atoi(args[2].c_str());
            return runServer(path, port);
        }
        if (cmd == "unpack") {
            std::string outDir = "out";
            if (args.size() >= 3) outDir = args[2];
            lpc::extractPak(path, outDir);
            std::cout << "Unpacked " << path << " to " << outDir << "\n";
            return 0;
        }
        if (cmd == "sqlcmd") {
            // Debug helper: query the configured MySQL directly.
            //   lpcvm sqlcmd "select ..." [adb|mdb]
            if (args.size() < 2) { usage(); return 1; }
            return sqlDebug(path, args.size() >= 3 ? args[2] : std::string());
        }
        usage();
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Error: unknown (non-std) exception escaped to main\n";
        return 1;
    }
}
