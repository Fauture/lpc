#include <winsock2.h>
#include <ws2tcpip.h>

#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#include <tlhelp32.h>
#include <winternl.h>

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
#include <unordered_map>
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

// Main-thread id (a crash report says whether the dying thread is the main one).
DWORD g_mainTid = 0;

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

// ---------------------------------------------------------------------------
// Original-style driver options (vm_analyseArguments 0x80b9a64).
//
// The original launchers (linux/gs/rungs, linux/aaa/runaaa, ...) pass:
//   ./magic_Linux32 /r ./ /e /gs/pack_data/lib_gs32.pak/gs/start_gs.o
//                   /b /gs/pack_data/lib_gs32.pak -m gs/gsmem.cfg
//                   /D _RELEASE /D CONFIG=gs/gs1.ini /D AUTH= ...
//                   --enable-socket-daemon 0 --stack-size 1024
// Single-char options accept BOTH '-' and '/' prefixes (the parser rejects
// anything else: `v5 != 45 && v5 != 47` -> "Bad syntax"); the value may be a
// separate argv token (rungs form) or attached ("/r./").  -D declares a
// predefine (vm_addPredefine 0x8080128) and may repeat, with or without '='
// (e.g. _RELEASE / AUTH= / CONFIG=gs/gs1.ini).
// ---------------------------------------------------------------------------
struct DriverArgs {
    std::string rootDir = "./";     // -r <dir>   (VM root; informational here)
    std::string startScript;        // -e <script>   REQUIRED: boot .o, may live inside -b pak
    std::string binaryDir;          // -b <pak>      REQUIRED: binary/pak directory
    std::string memoryConfig;       // -m <cfg>   (vmmemlib config; informational here)
    std::string telnetPassword;     // -w <pwd>
    int telnetPort = 0;             // -p <port>
    int serviceThreads = 0;         // -s <n>     (informational)
    // -D NAME[=VALUE] predefines; NAME without '=' defines an empty macro.
    std::vector<std::pair<std::string, std::string>> defines;
    bool hasStartScript = false;
    bool hasBinaryDir = false;
};

namespace {

bool isOptionToken(const std::string& s) {
    return s.size() >= 2 && (s[0] == '-' || s[0] == '/') && s != "--";
}

// Value of an option token: attached ("/r./" -> "./") or the next argv token
// ("/r ./" -> "./", the original takes argv[i+1] unconditionally).
bool optionValue(int argc, char** argv, size_t& i, std::string& out) {
    const std::string tok = argv[i];
    if (tok.size() > 2) {
        out = tok.substr(2);
        return true;
    }
    if (i + 1 < static_cast<size_t>(argc)) {
        out = argv[i + 1];
        ++i;
        return true;
    }
    std::cerr << "Miss argument for " << tok << "\n";
    return false;
}

bool parseDriverArgs(int argc, char** argv, DriverArgs& out) {
    for (size_t i = 1; i < static_cast<size_t>(argc); ++i) {
        std::string tok = argv[i];
        if (!isOptionToken(tok)) {
            std::cerr << "Bad syntax - " << tok << "\n";
            return false;
        }
        std::string val;
        switch (tok[1]) {
            case 'r':
                if (!optionValue(argc, argv, i, val)) return false;
                out.rootDir = val;
                break;
            case 'e':
                if (!optionValue(argc, argv, i, val)) return false;
                out.startScript = val;
                out.hasStartScript = true;
                break;
            case 'b':
                if (!optionValue(argc, argv, i, val)) return false;
                out.binaryDir = val;
                out.hasBinaryDir = true;
                break;
            case 'm':
                if (!optionValue(argc, argv, i, val)) return false;
                out.memoryConfig = val;
                break;
            case 'D':
                if (!optionValue(argc, argv, i, val)) return false;
                {
                    size_t eq = val.find('=');
                    if (eq == std::string::npos)
                        out.defines.emplace_back(val, std::string());
                    else
                        out.defines.emplace_back(val.substr(0, eq), val.substr(eq + 1));
                }
                break;
            case 'w':
                if (!optionValue(argc, argv, i, val)) return false;
                out.telnetPassword = val;
                break;
            case 'p':
                if (!optionValue(argc, argv, i, val)) return false;
                out.telnetPort = std::atoi(val.c_str());
                break;
            case 's':
                if (!optionValue(argc, argv, i, val)) return false;
                out.serviceThreads = std::atoi(val.c_str());
                break;
            default:
                // Long options used by the original launchers
                // (--enable-socket-daemon 0 / --max-process 128 /
                // --stack-size 1024 / ...) are accepted and skipped together
                // with their value; anything else is "Bad option" like the
                // original (which also just keeps going).
                if (tok.compare(0, 2, "--") == 0) {
                    if (i + 1 < static_cast<size_t>(argc) && !isOptionToken(argv[i + 1])) ++i;
                    break;
                }
                std::cerr << "Bad option -- " << tok << "\n";
                break;
        }
    }
    return true;
}

} // namespace

int runPakOriginal(const DriverArgs& opt) {
    // Faithful driver emulation of the original launchers (linux/gs/rungs etc.):
    //   ./magic_Linux32 /r ./ /e /gs/pack_data/lib_gs32.pak/gs/start_gs.o
    //                  /b /gs/pack_data/lib_gs32.pak /D CONFIG=gs/gs1.ini ...
    // -e is the boot script (vm_setStartScriptFile) INSIDE the -b pak
    // (vm_setBinaryDir); both are REQUIRED from the command line - nothing is
    // hard-coded here.  The pak mount, the extracted-tree fallback and the VFS
    // profile all derive from -e/-b.  The ini file comes exclusively from
    // -D CONFIG=... (rungs); when absent, the boot object's own startup code
    // resolves the file internally (start_aaa.o fallback), so nothing is
    // injected on its behalf.
    std::cout << "PDB is initialized by vmmemlib.\n";   // vmmemlib static init (0x816eb20)
    if (!opt.hasStartScript || !opt.hasBinaryDir) {
        std::cerr << "Missing required option: " << (opt.hasStartScript ? "-b <pak>" : "-e <start script>")
                  << " (see linux/gs/rungs for the original form)\n";
        return 1;
    }
    std::string binaryDir = opt.binaryDir;
    if (binaryDir[0] != '/') binaryDir = "/" + binaryDir;
    const std::string& startScript = opt.startScript;

    std::string pakFile = fs::path(binaryDir).filename().string();

    // Per-project profile (aaa / dba / ccs / gs).  The extracted .o tree, the
    // run directory and the pak mount prefix all derive from the pak filename:
    // lib_<prefix>32.pak -> <prefix>.
    std::string objPrefix = "aaa";
    {
        std::string pf = pakFile;
        size_t p = pf.rfind("32.pak");
        if (p != std::string::npos) pf = pf.substr(0, p);
        if (pf.rfind("lib_", 0) == 0) pf = pf.substr(4);
        if (!pf.empty()) objPrefix = pf;
    }

    // Boot object relative path: -e with the -b pak prefix stripped
    // ("/gs/pack_data/lib_gs32.pak/gs/start_gs.o" -> "gs/start_gs.o").
    std::string pakInner = binaryDir + "/";
    std::string bootRel;
    if (startScript.compare(0, pakInner.size(), pakInner) == 0) {
        bootRel = startScript.substr(pakInner.size());
    } else {
        size_t p = startScript.find(".pak/");
        if (p != std::string::npos) bootRel = startScript.substr(p + 5);
    }
    if (bootRel.empty()) {
        std::cerr << "Start script " << startScript << " is not inside binary dir "
                  << binaryDir << "\n";
        return 1;
    }

    // Physical data root comes from -r (vm_setRootDir 0x80bd900 - the original
    // launchers point it at the deployment cwd, "/r ./").  Nothing about the
    // data location is hard-coded here: the launcher decides where the tree
    // lives (this repo passes -r linux, where the original cwd layout is kept).
    std::string dataRoot = opt.rootDir;
    while (dataRoot.size() > 1 && (dataRoot.back() == '/' || dataRoot.back() == '\\'))
        dataRoot.pop_back();
    if (dataRoot.empty()) dataRoot = ".";
    std::string pakPrefix = binaryDir.substr(0, binaryDir.find_last_of('/') + 1);
    std::string vfsRoot = dataRoot + binaryDir.substr(0, binaryDir.size() - 4);  // strip ".pak"
    std::string etcDir = dataRoot + pakPrefix + "etc";
    std::string absPak = fs::absolute(dataRoot + binaryDir).string();
    lpc::setVfsProfile(vfsRoot, dataRoot, etcDir, pakPrefix, objPrefix);
    lpc::setPakMount(absPak, binaryDir);

    // Command-line defines only (vm_addPredefine 0x8080128).  CONFIG, if the
    // launcher passes it, lands here; no CONFIG means the boot object falls
    // back internally - exactly like the original driver.
    for (const auto& [nm, val] : opt.defines) lpc::injectDefine(nm, val);

    // Boot object: inside the pak first, extracted tree as fallback.
    lpc::OriginalObject obj;
    std::vector<uint8_t> bootBytes;
    bool fromPak = lpc::pakReadFile(binaryDir + "/" + bootRel, bootBytes);
    if (fromPak) {
        obj = lpc::loadOriginalObjectFromBytes(bootBytes);
        std::cout << "Loaded boot object from pak: " << binaryDir << "/" << bootRel << "\n";
    } else {
        std::string oPath = vfsRoot + "/" + bootRel;
        std::error_code ec;
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

// Legacy dev shim for "lpcvm run <pak>" / "lpcvm orig <pak>": rebuild the
// original-style args from the pak path alone.  Dev/debug entry only - the
// driver mode above requires -e/-b from the command line.
int runPak(const std::string& path) {
    DriverArgs opt;
    std::string pf = fs::path(path).filename().string();
    size_t p = pf.rfind("32.pak");
    if (p != std::string::npos) pf = pf.substr(0, p);
    if (pf.rfind("lib_", 0) == 0) pf = pf.substr(4);
    std::string objPrefix = pf.empty() ? "aaa" : pf;
    std::string pakVfs = "/" + objPrefix + "/pack_data/lib_" + objPrefix + "32.pak";
    opt.binaryDir = pakVfs;
    opt.hasBinaryDir = true;
    opt.startScript = pakVfs + "/" + objPrefix + "/start_" + objPrefix + ".o";
    opt.hasStartScript = true;
    // Historical dev invocation runs from the repo root; keep the data-root
    // convention the driver mode now takes from -r (original cwd layout).
    opt.rootDir = "linux";
    // The old dev entry injected the ini itself; keep that so historical
    // invocations ("lpcvm run linux/gs/pack_data/lib_gs32.pak") still boot.
    std::string iniRel = objPrefix + "/" + objPrefix + ".ini";
    if (objPrefix == "gs") iniRel = "gs/gs1.ini";
    opt.defines.emplace_back("CONFIG", iniRel);
    return runPakOriginal(opt);
}

// ---- C stack high-water watchdog (LPCVM_STACKPROBE=1) ----------------------
// Nested VM execution recurses through C++ (runLoop -> efun -> runLoop), so a
// runaway load chain eats the thread stack until the guard page raises
// 0xc00000fd - and by then the crash handler has no room left to walk frames
// (it prints 4 frames), which makes the recursion invisible.
//
// This helper samples EVERY thread's stack pointer from a side thread and prints
// the used/total KB plus the LPC ring (last entered LPC functions) whenever the
// deepest stack grows by another 64KB, whenever a thread gets within a quarter
// of its limit, and as a 5s heartbeat (proof the sampler is alive).  The growth
// steps name the recursion and the ring names the LPC code driving it.
static void startStackProbe() {
    if (std::getenv("LPCVM_STACKPROBE") == nullptr) return;
    std::thread([] {
        const DWORD selfTid = GetCurrentThreadId();
        const DWORD pid = GetCurrentProcessId();
        // Exact per-thread stack limits: a suspended thread's TEB is readable
        // in-process and NT_TIB.StackBase/StackLimit are the real bounds
        // (VirtualQuery only sees the currently committed region).
        struct ThreadBasicInfo {
            NTSTATUS ExitStatus;
            PVOID TebBaseAddress;
            CLIENT_ID ClientId;
            KAFFINITY AffinityMask;
            KPRIORITY Priority;
            KPRIORITY BasePriority;
        };
        typedef NTSTATUS(NTAPI * NtQueryInformationThreadFn)(HANDLE, ULONG, PVOID, ULONG, PULONG);
        NtQueryInformationThreadFn ntqit = reinterpret_cast<NtQueryInformationThreadFn>(
            GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationThread"));
        std::cerr << "[stackprobe] armed tid=" << selfTid << " ntqit=" << (ntqit ? "yes" : "no")
                  << std::endl;
        std::unordered_map<DWORD, size_t> reported;
        int64_t lastBeat = 0;
        for (;;) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
            if (snap == INVALID_HANDLE_VALUE) continue;
            size_t worst = 0;
            size_t worstTotal = 0;
            DWORD worstTid = 0;
            THREADENTRY32 te{};
            te.dwSize = sizeof(te);
            if (Thread32First(snap, &te)) {
                do {
                    if (te.th32OwnerProcessID != pid || te.th32ThreadID == selfTid) continue;
                    HANDLE th = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT |
                                               THREAD_QUERY_INFORMATION,
                                           FALSE, te.th32ThreadID);
                    if (!th) continue;
                    if (SuspendThread(th) != static_cast<DWORD>(-1)) {
                        CONTEXT ctx{};
                        ctx.ContextFlags = CONTEXT_CONTROL;
                        if (GetThreadContext(th, &ctx)) {
#if defined(_WIN64)
                            const uintptr_t sp = static_cast<uintptr_t>(ctx.Rsp);
#else
                            const uintptr_t sp = static_cast<uintptr_t>(ctx.Esp);
#endif
                            uintptr_t top = 0, limit = 0;
                            if (ntqit) {
                                ThreadBasicInfo tbi{};
                                ULONG ret = 0;
                                // 0 == ThreadBasicInformation
                                if (ntqit(th, 0, &tbi, sizeof(tbi), &ret) == 0 &&
                                    tbi.TebBaseAddress) {
                                    const NT_TIB* tib =
                                        reinterpret_cast<const NT_TIB*>(tbi.TebBaseAddress);
                                    top = reinterpret_cast<uintptr_t>(tib->StackBase);
                                    limit = reinterpret_cast<uintptr_t>(tib->StackLimit);
                                }
                            }
                            if (!top) {
                                MEMORY_BASIC_INFORMATION mbi{};
                                if (sp && VirtualQuery(reinterpret_cast<LPCVOID>(sp), &mbi,
                                                       sizeof(mbi))) {
                                    top = reinterpret_cast<uintptr_t>(mbi.BaseAddress) +
                                          mbi.RegionSize;
                                    limit = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
                                }
                            }
                            const size_t used = (sp && top > sp) ? top - sp : 0;
                            const size_t total = (top > limit) ? top - limit : 0;
                            if (used > worst) {
                                worst = used;
                                worstTotal = total;
                                worstTid = te.th32ThreadID;
                            }
                        }
                        ResumeThread(th);
                    }
                    CloseHandle(th);
                } while (Thread32Next(snap, &te));
            }
            CloseHandle(snap);
            if (!worst) {
                // No thread sampled at all: say so once so a silent probe is not
                // mistaken for "the stack never grew".
                if (lastBeat == 0) {
                    lastBeat = 1;
                    std::cerr << "[stackprobe] no thread sampled (OpenThread?)" << std::endl;
                }
                continue;
            }
            const int64_t nowMs = static_cast<int64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                    .count());
            const size_t prev = reported.count(worstTid) ? reported[worstTid] : 0;
            const bool grew = worst > prev + 64 * 1024;
            const bool nearlyFull = worstTotal && worst > (worstTotal * 3) / 4;
            if (grew || nearlyFull || nowMs - lastBeat >= 5000) {
                if (grew) reported[worstTid] = worst;
                lastBeat = nowMs;
                const unsigned pos = lpc::g_lpcRingPos.load();
                std::cerr << "[stackprobe]" << (grew ? " grow" : (nearlyFull ? " NEAR" : " hb"))
                          << " tid=" << worstTid << " used=" << (worst / 1024) << "KB/"
                          << (worstTotal / 1024) << "KB last LPC:";
                for (unsigned k = 0; k < lpc::kLpcRingSize; ++k) {
                    const char* nm = lpc::g_lpcRing[(pos + k) % lpc::kLpcRingSize];
                    if (nm) std::cerr << " " << nm;
                }
                std::cerr << std::endl;
            }
        }
    }).detach();
}

void usage() {
    std::cout << "Usage (driver mode, mirrors magic_Linux32 - see linux/gs/rungs):\n"
              << "  lpcvm -r ./ -e /gs/pack_data/lib_gs32.pak/gs/start_gs.o\n"
              << "        -b /gs/pack_data/lib_gs32.pak [-D CONFIG=gs/gs1.ini] ...\n"
              << "Options:\n"
              << "  -r <dir>        VM root / physical data root (e.g. ./ or linux)\n"
              << "  -e <script>     start script (.o), may live inside the -b pak (required)\n"
              << "  -b <pak>        binary/pak directory (required)\n"
              << "  -m <cfg>        memory config (accepted, informational)\n"
              << "  -s <n>          service thread count (informational)\n"
              << "  -w <pwd>        telnet password\n"
              << "  -p <port>       telnet port\n"
              << "  -D NAME[=VALUE] declare a predefine (repeatable)\n"
              << "Dev tools:\n"
              << "  lpcvm run <file.c|file.o|file.pak>\n"
              << "  lpcvm orig|disasm|opstats|disfn|unpack|server ...\n";
}

} // namespace

// SEH ebp-chain walker for the crash filter above.  Kept free of C++ objects
// with destructors so __try/__except is legal (MSVC C2712).
// SEH 层单独一个无 C++ 对象的函数 (C2712): 只读一帧 ebp 链
static bool safeNextFrame(uintptr_t ebp, uintptr_t* ret, uintptr_t* next) {
    __try {
        uintptr_t* f = reinterpret_cast<uintptr_t*>(ebp);
        *ret = f[1];
        *next = f[0];
        return *ret >= 0x10000;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

static void walkEbpChain(uintptr_t ebp) {
    // PDB 符号解析 (build/Release 的 lpcvm.pdb): 直接给函数名+offset,
    // 省去人工 map 换算 —— exe 基址/地址变动时 map 解析会静默错位。
    // [prewarm] dbghelp 必须在 main 早期就绪: 崩溃发生时堆已损坏,
    // SymInitialize 需要分配内存 —— 拖到首次崩溃时初始化会失败
    // (上次 boot 崩溃 ebp-chain 全部退化为 exe+RVA 就是这个原因)。
    static const bool symReady = [] {
        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS |
                      SYMOPT_LOAD_LINES);
        char exePath[MAX_PATH] = {};
        GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        char* bs = strrchr(exePath, '\\');
        if (bs) *bs = 0;
        return SymInitialize(GetCurrentProcess(), exePath, TRUE) != 0;
    }();
    (void)symReady;
    if (!ebp) return;   // prewarm call: init symbols only, print nothing
    HMODULE exeMod = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCSTR>(&walkEbpChain), &exeMod);
    std::cerr << "[crash] ebp-chain:" << std::hex;
    for (int i = 0; i < 16 && ebp; ++i) {
        uintptr_t ret = 0;
        uintptr_t next = 0;
        if (!safeNextFrame(ebp, &ret, &next)) {
            std::cerr << " <bad-ebp>";
            break;
        }
        std::cerr << "  ret=0x" << ret;
        if (symReady) {
            char symBuf[sizeof(SYMBOL_INFO) + 256] = {};
            SYMBOL_INFO* sym = reinterpret_cast<SYMBOL_INFO*>(symBuf);
            sym->SizeOfStruct = sizeof(SYMBOL_INFO);
            sym->MaxNameLen = 255;
            DWORD64 disp = 0;
            if (SymFromAddr(GetCurrentProcess(),
                            static_cast<DWORD64>(ret), &disp, sym)) {
                std::cerr << "(" << sym->Name << "+0x" << disp << ")";
                // [crash-line] 源码行号 —— 精确定位崩溃语句
                IMAGEHLP_LINE64 line{};
                line.SizeOfStruct = sizeof(line);
                DWORD lineDisp = 0;
                if (SymGetLineFromAddr64(GetCurrentProcess(),
                                         static_cast<DWORD64>(ret),
                                         &lineDisp, &line)) {
                    const char* f = strrchr(line.FileName, '\\');
                    std::cerr << "["
                              << (f ? f + 1 : line.FileName) << ":"
                              << line.LineNumber << "]";
                }
            } else if (exeMod) {
                std::cerr << "(exe+0x"
                          << (ret - reinterpret_cast<uintptr_t>(exeMod))
                          << ")";
            }
        } else if (exeMod) {
            std::cerr << "(exe+0x"
                      << (ret - reinterpret_cast<uintptr_t>(exeMod)) << ")";
        }
        std::cerr << "(ebp=0x" << ebp << ")";
        ebp = next;
        if (ebp < 0x10000) break;
    }
    std::cerr << std::dec << "\n";
}

// main 早期调用: 把 dbghelp 的 SymInitialize (遍历加载全部模块符号)
// 提前到堆/服务健康时完成, 崩溃 filter 里 walkEbpChain 直接复用。
void prewarmCrashSymbols() { walkEbpChain(0); }

int main(int argc, char** argv) {
    g_mainTid = GetCurrentThreadId();
    prewarmCrashSymbols();   // [prewarm] 符号初始化须在堆健康时完成
    // Unbuffered stdout/stderr: the service logs are redirected to files and
    // must appear line-by-line like the original driver (Load:/printf lines
    // otherwise sit in the C++ stdio buffer for minutes).
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
    // LPC printf() goes through C stdio, NOT std::cout: with stdout redirected
    // to a file it is block-buffered, so the LPC boot output ("Load: ... OK",
    // service start messages) lagged behind the C++ diagnostics by whatever sat
    // in the 4KB buffer - a stalled boot and a slow one looked identical in the
    // logs, and the last buffered line before a wait was invisible.
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::setvbuf(stderr, nullptr, _IONBF, 0);
    // SEH crash catcher: print a backtrace so a segfault inside the VM or a
    // builtin (the current MSVCP140.dll 0xc0000005 crashes) can be located
    // without a debugger attached.
    SetUnhandledExceptionFilter([](EXCEPTION_POINTERS* ep) -> LONG {
        void* frames[32];
        USHORT n = CaptureStackBackTrace(0, 32, frames, nullptr);
        std::cerr << "[crash] code=0x" << std::hex
                  << (ep ? ep->ExceptionRecord->ExceptionCode : 0)
                  << " addr=0x" << (ep ? ep->ExceptionRecord->ExceptionAddress : 0);
        // [crash-eip] 崩溃指令本身的符号+行号 (ExceptionAddress 即 eip)
        {
            static const bool symReadyEip = [] {
                SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS |
                              SYMOPT_LOAD_LINES);
                char exePath2[MAX_PATH] = {};
                GetModuleFileNameA(nullptr, exePath2, MAX_PATH);
                char* bs2 = strrchr(exePath2, '\\');
                if (bs2) *bs2 = 0;
                return SymInitialize(GetCurrentProcess(), exePath2, TRUE) != 0;
            }();
            (void)symReadyEip;
        }
        if (ep) {
            char symBuf2[sizeof(SYMBOL_INFO) + 256] = {};
            SYMBOL_INFO* sym2 = reinterpret_cast<SYMBOL_INFO*>(symBuf2);
            sym2->SizeOfStruct = sizeof(SYMBOL_INFO);
            sym2->MaxNameLen = 255;
            DWORD64 disp2 = 0;
            if (SymFromAddr(GetCurrentProcess(),
                            reinterpret_cast<DWORD64>(
                                ep->ExceptionRecord->ExceptionAddress),
                            &disp2, sym2)) {
                IMAGEHLP_LINE64 ln{};
                ln.SizeOfStruct = sizeof(ln);
                DWORD lnDisp = 0;
                std::cerr << " eip=" << sym2->Name << "+0x" << disp2;
                if (SymGetLineFromAddr64(
                        GetCurrentProcess(),
                        reinterpret_cast<DWORD64>(
                            ep->ExceptionRecord->ExceptionAddress),
                        &lnDisp, &ln)) {
                    const char* f2 = strrchr(ln.FileName, '\\');
                    std::cerr << "[" << (f2 ? f2 + 1 : ln.FileName) << ":"
                              << ln.LineNumber << "]";
                }
            }
        }
        // An access violation carries the ACCESS TYPE in Information[0] (0 read,
        // 1 write, 8 DEP/execute) and the address it touched in Information[1].
        // Without them a crash whose ExceptionAddress points into the HEAP is
        // indistinguishable from a bad data pointer - and that distinction is
        // exactly what matters: "execute" means we jumped to a garbage function
        // pointer (e.g. a closure's captured OriginalFunction*), which is what
        // the combat sort_by_speed crashes look like.
        if (ep && ep->ExceptionRecord &&
            (ep->ExceptionRecord->ExceptionCode == 0xC0000005) &&
            ep->ExceptionRecord->NumberParameters >= 2) {
            static const char* const kAcc[] = { "read", "write", "?2", "?3", "?4",
                                                "?5", "?6", "?7", "execute" };
            const ULONG_PTR a0 = ep->ExceptionRecord->ExceptionInformation[0];
            std::cerr << " access=" << (a0 < 9 ? kAcc[a0] : "?")
                      << " faultaddr=0x" << ep->ExceptionRecord->ExceptionInformation[1];
        }
        // Where the INSTRUCTION POINTER was: for an "execute" fault this is the
        // bogus code address, not a valid module - so print it separately from
        // the handler's own frames.
        if (ep && ep->ContextRecord) {
            std::cerr << " eip=0x" << static_cast<uintptr_t>(ep->ContextRecord->Eip);
        }
        std::cerr << std::dec << " frames=" << n << "\n";
        // Which thread died and how deep its stack was.  0xc00000fd
        // (EXCEPTION_STACK_OVERFLOW) swallows the frame list, so the depth is
        // the only evidence left: near the full reserve = runaway recursion,
        // shallow = the thread simply has a small stack (e.g. a receiver).
        {
            NT_TIB* tib = reinterpret_cast<NT_TIB*>(NtCurrentTeb());
            char here = 0;
            const uintptr_t sp = reinterpret_cast<uintptr_t>(&here);
            const uintptr_t base = reinterpret_cast<uintptr_t>(tib->StackBase);
            const uintptr_t lim = reinterpret_cast<uintptr_t>(tib->StackLimit);
            const DWORD tid = GetCurrentThreadId();
            std::cerr << "[crash] tid=" << tid << (tid == g_mainTid ? " (main)" : "")
                      << " stack used=" << ((base > sp ? base - sp : 0) / 1024)
                      << "KB reserve=" << ((base > lim ? base - lim : 0) / 1024) << "KB\n";
        }
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
        // CaptureStackBackTrace starts at the HANDLER, so for a fault whose
        // eip is a CFG castguard stub (__castguard_check_failure_os_handled_fptr)
        // the frames above say nothing about the CALL SITE.  Walk the faulting
        // thread's EBP chain from the exception context instead: the stub has
        // no frame of its own, so the first return address is the code that
        // made the guarded indirect call.
        if (ep && ep->ContextRecord) walkEbpChain(ep->ContextRecord->Ebp);
        // Last entered LPC functions: a C++-level recursion (a cyclic value
        // walked by padVariableValue/to_string/saveValueAscii/value_equal)
        // never pushes an LPC frame, so this ring is what says WHERE the
        // process died.
        {
            unsigned pos = lpc::g_lpcRingPos.load();
            std::cerr << "[crash] last LPC functions:";
            for (unsigned k = 0; k < lpc::kLpcRingSize; ++k) {
                const char* nm = lpc::g_lpcRing[(pos + k) % lpc::kLpcRingSize];
                if (nm) std::cerr << " " << nm;
            }
            std::cerr << "\n";
        }
        std::cerr.flush();
        return EXCEPTION_CONTINUE_SEARCH;   // let the OS generate the dump
    });
    startStackProbe();
    try {
        // Dev subcommands keep their historical form; anything else is parsed
        // as the original driver command line (vm_analyseArguments 0x80b9a64).
        static const char* const kDevCmds[] = { "run",   "orig",     "disasm", "opstats",
                                                "disfn", "server", "unpack" };
        const std::string first = argc > 1 ? argv[1] : "";
        const bool devCmd = std::find(std::begin(kDevCmds), std::end(kDevCmds), first) != std::end(kDevCmds);
        if (!devCmd) {
            DriverArgs drv;
            if (!parseDriverArgs(argc, argv, drv)) {
                usage();
                return 1;
            }
            if (!drv.telnetPassword.empty()) lpc::setDriverTelnetPassword(drv.telnetPassword);
            if (drv.telnetPort >= 1024) lpc::startDriverTelnet(drv.telnetPort);
            return runPakOriginal(drv);
        }

        if (argc < 3) {
            usage();
            return 1;
        }
        std::string cmd = argv[1];
        std::string path = argv[2];
        std::vector<std::string> args(argv + 1, argv + argc);
        if (cmd == "run") {
            if (hasSuffix(path, ".pak")) return runPak(path);
            if (hasSuffix(path, ".o")) return runOriginalObject(path);
            return runLpcSource(path);
        }
        if (cmd == "orig") {
            if (hasSuffix(path, ".pak")) return runPak(path);
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
