#include <winsock2.h>
#include <ws2tcpip.h>

#include <windows.h>
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
    std::cout << "Usage:\n"
              << "  lpcvm run <file.c|file.o|file.pak>\n"
              << "  lpcvm server <file.c> [port]\n"
              << "  lpcvm disasm <file.o>\n"
              << "  lpcvm unpack <file.pak> [outdir]\n";
}

} // namespace

int main(int argc, char** argv) {
    g_mainTid = GetCurrentThreadId();
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
        if (cmd == "sscantest") {
            // Quick unit check of lpcSscanf quote handling (coupon .list):
            //   sscanf("\"ABC123\"", "\"%s\"")  should unquote to ABC123.
            std::string in = "\"ABC123\"";
            std::string fmt = "\"%s\"";
            std::vector<lpc::Value> out;
            int m = lpc::lpcSscanf(in, fmt, out, 1);
            std::cout << "case1 match=" << m << " out=" << out.size();
            if (!out.empty() && out[0].index() == 4)
                std::cout << " str(" << std::get<lpc::SharedString>(out[0]).size() << ")"
                          << " [" << std::get<lpc::SharedString>(out[0]).str() << "]";
            std::cout << "\n";
            std::string in2 = "2016-02-free_gift";
            std::string fmt2 = "%d-%d-%s";
            std::vector<lpc::Value> out2;
            int m2 = lpc::lpcSscanf(in2, fmt2, out2, 3);
            std::cout << "case2 match=" << m2 << "\n";
            // case3: GBK bytes quoted (coupon name, 24 bytes inside quotes)
            // 2016年02月份充值送奖励活动 in GBK
            std::string gbk;
            gbk += "\x32\x30\x31\x36\xc4\xea\x30\x32\xd4\xc2\xb7\xdd"; // 2016年02月份
            gbk += "\xb3\xe4\xd6\xb5\xcb\xcd\xbd\xb1\xc0\xf8\xbb\xee\xb6\xaf"; // 充值送奖励活动
            std::string in3 = "\"" + gbk + "\"";
            std::vector<lpc::Value> out3;
            int m3 = lpc::lpcSscanf(in3, fmt, out3, 1);
            std::cout << "case3 match=" << m3 << " out=" << out3.size();
            if (!out3.empty() && out3[0].index() == 4)
                std::cout << " str(" << std::get<lpc::SharedString>(out3[0]).size() << ")";
            std::cout << "\n";
            // case4: same but ASCII inner of identical 24 length
            std::string in4 = "\"" + std::string(24, 'A') + "\"";
            std::vector<lpc::Value> out4;
            int m4 = lpc::lpcSscanf(in4, fmt, out4, 1);
            std::cout << "case4 match=" << m4 << " out=" << out4.size();
            if (!out4.empty() && out4[0].index() == 4)
                std::cout << " str(" << std::get<lpc::SharedString>(out4[0]).size() << ")";
            std::cout << "\n";
            // case5: coupon-name format on the real name
            std::string gbk2;
            gbk2 += "\x32\x30\x31\x36\xc4\xea\x30\x32\xd4\xc2\xb7\xdd";      // 2016年02月份
            gbk2 += "\xb3\xe4\xd6\xb5\xcb\xcd\xbd\xb1\xc0\xf8\xbb\xee\xb6\xaf"; // 充值送奖励活动
            std::string fmt5;
            fmt5 += "%d\xc4\xea";                        // %d年
            fmt5 += "%d\xd4\xc2\xb7\xdd";                // %d月份
            fmt5 += "%s\xb3\xe4\xd6\xb5\xcb\xcd\xbd\xb1\xc0\xf8\xbb\xee\xb6\xaf"; // %s充值送奖励活动
            std::vector<lpc::Value> out5;
            int m5 = lpc::lpcSscanf(gbk2, fmt5, out5, 3);
            std::cout << "case5 match=" << m5 << " out=" << out5.size();
            if (!out5.empty() && out5[0].index() == 2)
                std::cout << " y=" << std::get<int64_t>(out5[0]);
            if (out5.size() > 1 && out5[1].index() == 2)
                std::cout << " m=" << std::get<int64_t>(out5[1]);
            if (out5.size() > 2 && out5[2].index() == 4)
                std::cout << " s=str";
            std::cout << "\n";
            // case6: the 09-month coupon name, and case7 with trailing CR
            std::string gbk6;
            gbk6 += "\x32\x30\x31\x36\xc4\xea\x30\x39\xd4\xc2\xb7\xdd"; // 2016年09月份
            gbk6 += "\xb3\xe4\xd6\xb5\xcb\xcd\xbd\xb1\xc0\xf8\xbb\xee\xb6\xaf"; // 充值送奖励活动
            std::vector<lpc::Value> out6;
            int m6 = lpc::lpcSscanf(gbk6, fmt5, out6, 3);
            std::cout << "case6 match=" << m6 << "\n";
            std::string gbk7 = gbk6 + "\r";
            std::vector<lpc::Value> out7;
            int m7 = lpc::lpcSscanf(gbk7, fmt5, out7, 3);
            std::cout << "case7(CR) match=" << m7 << "\n";
            // case8/9: the newbie-welfare bonus item string that
            // sbonusd::get_bonus_item_info parses:
            //   功能卷轴·报效师门(100)#非卖品
            //   sscanf(text, "%s#非卖品", &t) -> t = 功能卷轴·报效师门(100)
            //   sscanf(t,    "%s(%s)", &name, &extra) -> name, extra="100"
            // If either parse is wrong, explode(extra,"*") has a size the switch
            // rejects, every entry is dropped and a gathered 石头 grants nothing.
            std::string itemText = "\xb9\xa6\xc4\xdc\xbe\xed\xd6\xe1\xa1\xa4\xb1\xa8\xd0\xa7\xca\xa6\xc3\xc5"
                                   "(100)#\xb7\xc7\xc2\xf4\xc6\xb7";
            std::vector<lpc::Value> outA;
            int mA = lpc::lpcSscanf(itemText, "%s#\xb7\xc7\xc2\xf4\xc6\xb7", outA, 1);
            std::cout << "case8 sscanf(%s#非卖品) match=" << mA << " out=" << outA.size();
            if (!outA.empty() && outA[0].index() == 4)
                std::cout << " len=" << std::get<lpc::SharedString>(outA[0]).size();
            std::cout << "\n";
            std::string itemText2 = "\xb9\xa6\xc4\xdc\xbe\xed\xd6\xe1\xa1\xa4\xb1\xa8\xd0\xa7\xca\xa6\xc3\xc5"
                                    "(100)";
            std::vector<lpc::Value> outB;
            int mB = lpc::lpcSscanf(itemText2, "%s(%s)", outB, 2);
            std::cout << "case9 sscanf(%s(%s)) match=" << mB << " out=" << outB.size();
            for (size_t i = 0; i < outB.size(); ++i) {
                if (outB[i].index() == 4)
                    std::cout << " [" << i << "]=" << std::get<lpc::SharedString>(outB[i]).str();
            }
            std::cout << "\n";
            // case10: the failing durability probe must NOT clobber its output.
            std::string keep = itemText2;
            std::vector<lpc::Value> outC;
            int mC = lpc::lpcSscanf(itemText2, "%s#\xd7\xee\xb4\xf3\xc4\xcd\xbe\xc3(%d)", outC, 2);
            std::cout << "case10 sscanf(%s#最大耐久(%d)) match=" << mC << " out=" << outC.size() << "\n";
            (void)keep;
            return 0;
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
