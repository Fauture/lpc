// tools/stackdump.cpp — poor-man's sampling profiler.
// Usage: stackdump.exe <pid> [seconds]
// Samples all threads of <pid> every 40ms, resolves top frames via the PDB
// next to the target exe, prints an aggregate histogram of (module!symbol).
#include <windows.h>
#include <tlhelp32.h>
#include <dbghelp.h>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <map>
#include <string>
#include <vector>

#pragma comment(lib, "dbghelp.lib")

static std::map<std::string, int> g_hist;
static std::map<std::string, int> g_chain;   // full d0<-d1<-d2 chain

static void SampleThread(HANDLE proc, HANDLE th, CONTEXT& ctx) {
    std::vector<std::string> names;
    STACKFRAME64 frame{};
#ifdef _WIN64
    DWORD machine = IMAGE_FILE_MACHINE_AMD64;
    frame.AddrPC.Offset = ctx.Rip; frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = ctx.Rbp; frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = ctx.Rsp; frame.AddrStack.Mode = AddrModeFlat;
#else
    DWORD machine = IMAGE_FILE_MACHINE_I386;
    frame.AddrPC.Offset = ctx.Eip; frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = ctx.Ebp; frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = ctx.Esp; frame.AddrStack.Mode = AddrModeFlat;
#endif
    for (int depth = 0; depth < 8; ++depth) {
        if (!StackWalk64(machine, proc, th, &frame, &ctx, nullptr,
                         SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
            break;
        if (!frame.AddrPC.Offset) break;
        DWORD64 pc = frame.AddrPC.Offset;
        char symBuf[sizeof(SYMBOL_INFO) + 256] = {};
        SYMBOL_INFO* sym = reinterpret_cast<SYMBOL_INFO*>(symBuf);
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen = 255;
        std::string name = "??";
        DWORD64 disp = 0;
        if (SymFromAddr(proc, pc, &disp, sym)) name = sym->Name;
        names.push_back(name);
        if (depth <= 2) {
            char key[512];
            snprintf(key, sizeof(key), "d%d %s", depth, name.c_str());
            g_hist[key]++;
        }
    }
    if (names.size() >= 2) {
        std::string chain = names[0];
        for (size_t i = 1; i < names.size() && i < 5; ++i)
            chain += " <- " + names[i];
        g_chain[chain]++;
    }
}

int main(int argc, char** argv) {
    if (argc < 2) { printf("usage: stackdump <pid> [secs]\n"); return 1; }
    DWORD pid = static_cast<DWORD>(atoi(argv[1]));
    int secs = argc >= 3 ? atoi(argv[2]) : 15;
    HANDLE proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!proc) { printf("OpenProcess(%lu) failed %lu\n", pid, GetLastError()); return 1; }
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    if (!SymInitialize(proc, nullptr, TRUE))
        printf("SymInitialize failed %lu (no symbols)\n", GetLastError());

    int samples = 0;
    DWORD tick0 = GetTickCount();
    while ((GetTickCount() - tick0) < static_cast<DWORD>(secs) * 1000) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snap == INVALID_HANDLE_VALUE) break;
        THREADENTRY32 te; te.dwSize = sizeof(te);
        std::vector<DWORD> tids;
        if (Thread32First(snap, &te)) {
            do {
                if (te.th32OwnerProcessID == pid) tids.push_back(te.th32ThreadID);
            } while (Thread32Next(snap, &te));
        }
        CloseHandle(snap);
        for (DWORD tid : tids) {
            HANDLE th = OpenThread(THREAD_ALL_ACCESS, FALSE, tid);
            if (!th) continue;
            if (SuspendThread(th) == static_cast<DWORD>(-1)) { CloseHandle(th); continue; }
            CONTEXT ctx{};
            ctx.ContextFlags = CONTEXT_FULL;
            if (GetThreadContext(th, &ctx)) { SampleThread(proc, th, ctx); ++samples; }
            ResumeThread(th);
            CloseHandle(th);
        }
        Sleep(40);
    }
    // dump
    printf("samples=%d\n", samples);
    std::vector<std::pair<int, std::string>> v;
    v.reserve(g_hist.size());
    for (auto& kv : g_hist) v.push_back({kv.second, kv.first});
    std::sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.first > b.first; });
    for (auto& kv : v)
        printf("%6d  %s\n", kv.first, kv.second.c_str());
    printf("---- chains (top 25) ----\n");
    std::vector<std::pair<int, std::string>> c;
    c.reserve(g_chain.size());
    for (auto& kv : g_chain) c.push_back({kv.second, kv.first});
    std::sort(c.begin(), c.end(), [](auto& a, auto& b) { return a.first > b.first; });
    for (size_t i = 0; i < c.size() && i < 25; ++i)
        printf("%6d  %s\n", c[i].first, c[i].second.c_str());
    SymCleanup(proc);
    CloseHandle(proc);
    return 0;
}
