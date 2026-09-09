// Native stack sampler for 32-bit lpcvm.exe running on x64 Windows.
// Uses Wow64GetThreadContext + StackWalk64 with IMAGE_FILE_MACHINE_I386.
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

public static class Sampler86
{
    [DllImport("kernel32.dll")] static extern IntPtr OpenProcess(uint access, bool inh, int pid);
    [DllImport("kernel32.dll")] static extern IntPtr OpenThread(uint access, bool inh, uint tid);
    [DllImport("kernel32.dll")] static extern bool GetThreadTimes(IntPtr h, out long c, out long e, out long k, out long u);
    [DllImport("kernel32.dll")] static extern uint SuspendThread(IntPtr h);
    [DllImport("kernel32.dll")] static extern uint ResumeThread(IntPtr h);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
    [DllImport("kernel32.dll")] static extern bool Wow64GetThreadContext(IntPtr h, ref WOW64CONTEXT ctx);
    [DllImport("kernel32.dll")] static extern bool ReadProcessMemory(IntPtr h, IntPtr addr, byte[] buf, int size, out int read);

    [StructLayout(LayoutKind.Sequential)]
    struct THREADENTRY32 { public uint dwSize; public uint cntUsage; public uint th32ThreadID; public uint th32OwnerProcessID; public int tpBasePri; public int tpDeltaPri; public uint dwFlags; }
    [DllImport("kernel32.dll")] static extern IntPtr CreateToolhelp32Snapshot(uint flags, uint pid);
    [DllImport("kernel32.dll")] static extern bool Thread32First(IntPtr snap, ref THREADENTRY32 e);
    [DllImport("kernel32.dll")] static extern bool Thread32Next(IntPtr snap, ref THREADENTRY32 e);

    // x86 CONTEXT (CONTEXT_i386 = 0x00010000 | CONTROL 0x01 | INTEGER 0x02 | SEGMENTS 0x04 | FLOATING 0x08 | DEBUG 0x10)
    [StructLayout(LayoutKind.Sequential)]
    struct WOW64CONTEXT
    {
        public uint ContextFlags;
        public uint Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
        public WOW64FLOAT_SAVE FloatSave;
        public uint SegGs, SegFs, SegEs, SegDs;
        public uint Edi, Esi, Ebx, Edx, Ecx, Eax;
        public uint Ebp, Eip, SegCs, EFlags, Esp, SegSs;
    }
    [StructLayout(LayoutKind.Sequential)]
    struct WOW64FLOAT_SAVE
    {
        public uint ControlWord, StatusWord, TagWord, ErrorOffset, ErrorSelector, DataOffset, DataSelector;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 80)] public byte[] RegisterArea;
        public uint Cr0NpxState;
    }

    [StructLayout(LayoutKind.Sequential)]
    struct STACKFRAME64
    {
        public ulong AddrPC, AddrReturn, AddrFrame, AddrStack, AddrBStore;
        public IntPtr FuncTableEntry;
        public ulong Params0, Params1, Params2, Params3;
        public IntPtr Far;
        public int Virtual;
        public ulong Reserved0, Reserved1;
        public IntPtr KdHelp;
    }
    [DllImport("dbghelp.dll")] static extern bool StackWalk64(uint machine, IntPtr proc, IntPtr thread,
        ref STACKFRAME64 frame, ref byte[] ctxBuf, IntPtr readMem, IntPtr funcTbl, IntPtr sym, IntPtr modList);
    [DllImport("dbghelp.dll")] static extern bool SymInitialize(IntPtr proc, string path, bool inv);
    [DllImport("dbghelp.dll")] static extern bool SymFromAddr(IntPtr proc, ulong addr, out ulong disp, byte[] symInfo);
    [DllImport("dbghelp.dll", EntryPoint = "SymGetLineFromAddr64")] static extern bool SymGetLineFromAddr64(IntPtr proc, ulong addr, out ulong disp, byte[] lineInfo);
    [DllImport("dbghelp.dll")] static extern ulong SymSetOptions(ulong opts);

    public static void Run(int pid)
    {
        SymSetOptions(0x00000002 | 0x00000100);
        IntPtr proc = OpenProcess(0x1F0FFF, false, pid);
        if (proc == IntPtr.Zero) { Console.WriteLine("OpenProcess failed " + Marshal.GetLastWin32Error()); return; }
        SymInitialize(proc, null, true);
        IntPtr snap = CreateToolhelp32Snapshot(4, 0);
        THREADENTRY32 e = new THREADENTRY32(); e.dwSize = (uint)Marshal.SizeOf(typeof(THREADENTRY32));
        uint bestTid = 0; long bestU = -1;
        for (bool ok = Thread32First(snap, ref e); ok; ok = Thread32Next(snap, ref e))
        {
            if (e.th32OwnerProcessID != (uint)pid) continue;
            IntPtr h = OpenThread(0x1F03FF, false, e.th32ThreadID);
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
        IntPtr th = OpenThread(0x1F03FF, false, bestTid);
        if (th == IntPtr.Zero) { Console.WriteLine("OpenThread fail " + Marshal.GetLastWin32Error()); return; }
        SuspendThread(th);
        WOW64CONTEXT ctx = new WOW64CONTEXT();
        ctx.ContextFlags = 0x00010007; // CONTEXT_i386|CONTROL|INTEGER|SEGMENTS
        if (!Wow64GetThreadContext(th, ref ctx)) { Console.WriteLine("Wow64GetThreadContext fail " + Marshal.GetLastWin32Error()); ResumeThread(th); return; }
        Console.WriteLine("EIP=0x{0:X8} ESP=0x{1:X8} EBP=0x{2:X8} EAX=0x{3:X8} ECX=0x{4:X8} EDX=0x{5:X8} ESI=0x{6:X8} EDI=0x{7:X8}",
            ctx.Eip, ctx.Esp, ctx.Ebp, ctx.Eax, ctx.Ecx, ctx.Edx, ctx.Esi, ctx.Edi);
        byte[] ctxBuf = new byte[Marshal.SizeOf(typeof(WOW64CONTEXT))];
        IntPtr pCtx = Marshal.AllocHGlobal(ctxBuf.Length);
        Marshal.StructureToPtr(ctx, pCtx, false);
        Marshal.Copy(pCtx, ctxBuf, 0, ctxBuf.Length);
        Marshal.FreeHGlobal(pCtx);
        STACKFRAME64 sf = new STACKFRAME64();
        sf.AddrPC = ctx.Eip; sf.AddrFrame = ctx.Ebp; sf.AddrStack = ctx.Esp;
        byte[] sym = new byte[1024]; byte[] line = new byte[1024];
        for (int i = 0; i < 80; i++)
        {
            if (!StackWalk64(0x014C, proc, th, ref sf, ref ctxBuf, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero))
            {
                Console.WriteLine("stackwalk stopped at frame " + i + " err=" + Marshal.GetLastWin32Error());
                break;
            }
            ulong disp;
            string name = "<no-sym>";
            bool got = SymFromAddr(proc, sf.AddrPC, out disp, sym);
            if (got)
            {
                name = Encoding.ASCII.GetString(sym, 16, BitConverter.ToInt32(sym, 8)).TrimEnd('\0');
                string file = "";
                if (SymGetLineFromAddr64(proc, sf.AddrPC, out disp, line))
                {
                    file = Encoding.ASCII.GetString(line, 12, BitConverter.ToInt32(line, 8)).TrimEnd('\0');
                    int ln = BitConverter.ToInt32(line, 4);
                    file = file + ":" + ln;
                }
                Console.WriteLine("{0,2}: 0x{1:X8} {2} +0x{3:X} {4}", i, (uint)sf.AddrPC, name, disp, file);
            }
            else
                Console.WriteLine("{0,2}: 0x{1:X8} (no symbol)", i, (uint)sf.AddrPC);
        }
        ResumeThread(th);
        CloseHandle(th);
        CloseHandle(proc);
    }
}
