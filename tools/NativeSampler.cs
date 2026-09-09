// Native stack sampler for lpcvm.exe (x64). Usage: powershell Add-Type from file
// then Sampler.Run(pid). Walks the busiest thread's stack via dbghelp StackWalk64.
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

public static class Sampler
{
    [DllImport("kernel32.dll")] static extern IntPtr OpenProcess(uint access, bool inh, int pid);
    [DllImport("kernel32.dll")] static extern IntPtr OpenThread(uint access, bool inh, uint tid);
    [DllImport("kernel32.dll")] static extern bool GetThreadTimes(IntPtr h, out long c, out long e, out long k, out long u);
    [DllImport("kernel32.dll")] static extern bool SuspendThread(IntPtr h);
    [DllImport("kernel32.dll")] static extern uint ResumeThread(IntPtr h);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
    [DllImport("kernel32.dll")] static extern bool ReadProcessMemory(IntPtr h, IntPtr addr, byte[] buf, int size, out int read);
    [DllImport("kernel32.dll", EntryPoint = "GetCurrentProcess")] static extern IntPtr GetCurrentProcess();

    [StructLayout(LayoutKind.Sequential)]
    struct THREADENTRY32 { public uint dwSize; public uint cntUsage; public uint th32ThreadID; public uint th32OwnerProcessID; public int tpBasePri; public int tpDeltaPri; public uint dwFlags; }
    [DllImport("kernel32.dll")] static extern IntPtr CreateToolhelp32Snapshot(uint flags, uint pid);
    [DllImport("kernel32.dll")] static extern bool Thread32First(IntPtr snap, ref THREADENTRY32 e);
    [DllImport("kernel32.dll")] static extern bool Thread32Next(IntPtr snap, ref THREADENTRY32 e);

    [StructLayout(LayoutKind.Sequential)]
    struct CONTEXT {
        public ulong P1Home, P2Home, P3Home, P4Home, P5Home, P6Home;
        public uint ContextFlags, MxCsr;
        public ushort SegCs, SegDs, SegEs, SegFs, SegGs, SegSs, EFlags, Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
        public ulong Rax, Rcx, Rdx, Rbx, Rsp, Rbp, Rsi, Rdi, R8, R9, R10, R11, R12, R13, R14, R15;
        public ulong Rip;
    }
    [DllImport("kernel32.dll")] static extern bool GetThreadContext(IntPtr h, ref CONTEXT ctx);
    [DllImport("kernel32.dll")] static extern bool SetThreadContext(IntPtr h, ref CONTEXT ctx);

    [StructLayout(LayoutKind.Sequential)]
    struct STACKFRAME64 {
        public ulong AddrPC, AddrReturn, AddrFrame, AddrStack;
        public ulong AddrBStore;
        public IntPtr FuncTableEntry;
        public ulong Params0, Params1, Params2, Params3;
        public IntPtr Far;
        public int Virtual;
        public ulong Reserved0, Reserved1;
        public IntPtr KdHelp;
    }
    [DllImport("dbghelp.dll")] static extern bool StackWalk64(uint machine, IntPtr proc, IntPtr thread,
        ref STACKFRAME64 frame, ref CONTEXT ctx, IntPtr readMem, IntPtr funcTbl, IntPtr sym, IntPtr modList);
    [DllImport("dbghelp.dll")] static extern bool SymInitialize(IntPtr proc, string path, bool inv);
    [DllImport("dbghelp.dll")] static extern ulong SymLoadModuleEx(IntPtr proc, IntPtr file, string name,
        ulong baseAddr, int size, IntPtr data, int flags);
    [DllImport("dbghelp.dll")] static extern bool SymGetModuleInfo64(IntPtr proc, ulong addr, out IMAGEHLP_MODULE64 mi);
    [DllImport("dbghelp.dll")] static extern bool SymFromAddr(IntPtr proc, ulong addr, out ulong disp, byte[] symInfo);
    [DllImport("dbghelp.dll", EntryPoint = "SymGetLineFromAddr64")] static extern bool SymGetLineFromAddr64(IntPtr proc, ulong addr, out ulong disp, byte[] lineInfo);
    [DllImport("dbghelp.dll")] static extern ulong SymSetOptions(ulong opts);

    [StructLayout(LayoutKind.Sequential)]
    struct IMAGEHLP_MODULE64 {
        public int SizeOfStruct;
        public ulong BaseOfImage;
        public int ImageSize;
        public int TimeDateStamp;
        public int CheckSum;
        public int NumSyms;
        public int SymType;
        public IntPtr ModuleName;
        public IntPtr ImageName;
        public IntPtr LoadedImageName;
    }

    public static void Run(int pid)
    {
        SymSetOptions(0x00000002 | 0x00000100); // SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES
        IntPtr proc = OpenProcess(0x1F0FFF, false, pid); // PROCESS_ALL_ACCESS
        if (proc == IntPtr.Zero) { Console.WriteLine("OpenProcess failed " + Marshal.GetLastWin32Error()); return; }
        SymInitialize(proc, null, true);
        // enumerate threads, find max user time
        IntPtr snap = CreateToolhelp32Snapshot(4, 0); // TH32CS_SNAPTHREAD
        THREADENTRY32 e = new THREADENTRY32(); e.dwSize = (uint)Marshal.SizeOf(typeof(THREADENTRY32));
        uint bestTid = 0; long bestU = -1;
        for (bool ok = Thread32First(snap, ref e); ok; ok = Thread32Next(snap, ref e))
        {
            if (e.th32OwnerProcessID != (uint)pid) continue;
            IntPtr h = OpenThread(0x1F03FF, false, e.th32ThreadID); // THREAD_QUERY_INFORMATION
            long c, x, k, u;
            if (h != IntPtr.Zero && GetThreadTimes(h, out c, out x, out k, out u))
            {
                if (u > bestU) { bestU = u; bestTid = e.th32ThreadID; }
                Console.WriteLine("tid={0} userMs={1} kernMs={2}", e.th32ThreadID, u / 10000, k / 10000);
                CloseHandle(h);
            }
            else if (h != IntPtr.Zero) CloseHandle(h);
        }
        CloseHandle(snap);
        Console.WriteLine("=== sampling busiest tid=" + bestTid + " ===");
        IntPtr th = OpenThread(0x1F03FF, false, bestTid); // THREAD_SUSPEND_RESUME
        if (th == IntPtr.Zero) { Console.WriteLine("OpenThread fail"); return; }
        SuspendThread(th);
        CONTEXT ctx = new CONTEXT(); ctx.ContextFlags = 0x00100000 | 0x00000001 | 0x00000002; // ALL | CONTROL | INTEGER
        if (!GetThreadContext(th, ref ctx)) { Console.WriteLine("GetThreadContext fail " + Marshal.GetLastWin32Error()); ResumeThread(th); return; }
        STACKFRAME64 sf = new STACKFRAME64();
        sf.AddrPC = ctx.Rip; sf.AddrFrame = ctx.Rbp; sf.AddrStack = ctx.Rsp;
        Console.WriteLine("RIP=0x{0:X} RSP=0x{1:X} RBP=0x{2:X}", ctx.Rip, ctx.Rsp, ctx.Rbp);
        byte[] sym = new byte[512]; byte[] line = new byte[512];
        for (int i = 0; i < 60; i++)
        {
            if (!StackWalk64(0x8664, proc, th, ref sf, ref ctx, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero))
                break;
            ulong disp;
            string name = "<no-sym>";
            if (SymFromAddr(proc, sf.AddrPC, out disp, sym))
            {
                name = Encoding.ASCII.GetString(sym, 16, BitConverter.ToInt32(sym, 8));
                name = name.TrimEnd('\0');
                IMAGEHLP_MODULE64 mi = new IMAGEHLP_MODULE64(); mi.SizeOfStruct = Marshal.SizeOf(typeof(IMAGEHLP_MODULE64));
                string mod = "";
                if (SymGetModuleInfo64(proc, sf.AddrPC, out mi)) mod = Marshal.PtrToStringAnsi(mi.ImageName) ?? "";
                string file = "";
                if (SymGetLineFromAddr64(proc, sf.AddrPC, out disp, line))
                {
                    file = Encoding.ASCII.GetString(line, 12, BitConverter.ToInt32(line, 8));
                    int ln = BitConverter.ToInt32(line, 4);
                    file = file.TrimEnd('\0') + ":" + ln;
                }
                Console.WriteLine("{0,2}: 0x{1:X16} {2} +0x{3:X}  [{4}]", i, sf.AddrPC, name, disp, file);
            }
            else
                Console.WriteLine("{0,2}: 0x{1:X16} {2}", i, sf.AddrPC, name);
        }
        ResumeThread(th);
        CloseHandle(th);
        CloseHandle(proc);
    }
}
