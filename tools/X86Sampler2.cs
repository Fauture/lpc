// x86 sampler with correct DWORD64 signatures (32-bit helper).
using System;
using System.Runtime.InteropServices;
using System.Text;

public static class X86Sampler2
{
    [DllImport("kernel32.dll")] static extern IntPtr OpenProcess(uint access, bool inh, int pid);
    [DllImport("kernel32.dll")] static extern IntPtr OpenThread(uint access, bool inh, uint tid);
    [DllImport("kernel32.dll")] static extern bool GetThreadTimes(IntPtr h, out long c, out long e, out long k, out long u);
    [DllImport("kernel32.dll")] static extern uint SuspendThread(IntPtr h);
    [DllImport("kernel32.dll")] static extern uint ResumeThread(IntPtr h);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
    [DllImport("kernel32.dll")] static extern bool GetThreadContext(IntPtr h, ref CONTEXT86 ctx);
    [StructLayout(LayoutKind.Sequential)] struct TE32 { public uint dwSize, cntUsage, th32ThreadID, th32OwnerProcessID; public int tpBasePri, tpDeltaPri; public uint dwFlags; }
    [DllImport("kernel32.dll")] static extern IntPtr CreateToolhelp32Snapshot(uint f, uint p);
    [DllImport("kernel32.dll")] static extern bool Thread32First(IntPtr s, ref TE32 e);
    [DllImport("kernel32.dll")] static extern bool Thread32Next(IntPtr s, ref TE32 e);
    [StructLayout(LayoutKind.Sequential)] struct FSAVE { public uint a, b, c, d, e, f, g; [MarshalAs(UnmanagedType.ByValArray, SizeConst = 80)] public byte[] ra; public uint cr; }
    [StructLayout(LayoutKind.Sequential)] struct CONTEXT86 { public uint ContextFlags, Dr0, Dr1, Dr2, Dr3, Dr6, Dr7; public FSAVE fs; public uint SegGs, SegFs, SegEs, SegDs, Edi, Esi, Ebx, Edx, Ecx, Eax, Ebp, Eip, SegCs, EFlags, Esp, SegSs; }

    [StructLayout(LayoutKind.Sequential)] struct ADDR64 { public ulong Offset; public uint Segment; public uint Mode; }
    [StructLayout(LayoutKind.Sequential)] struct STACKFRAME64 { public ADDR64 AddrPC, AddrRet, AddrFrame, AddrStack, AddrBStore; public IntPtr FuncTbl; public ulong P0, P1, P2, P3; public IntPtr Far; public int Virtual; public ulong R0; public IntPtr KdHelp; }
    [DllImport("dbghelp.dll")] static extern bool StackWalk64(uint machine, IntPtr proc, IntPtr thread, ref STACKFRAME64 f, byte[] ctx, IntPtr readMem, IntPtr funcTbl, IntPtr sym, IntPtr modList);
    [DllImport("dbghelp.dll")] static extern bool SymInitialize(IntPtr proc, string path, bool inv);
    [DllImport("dbghelp.dll")] static extern ulong SymSetOptions(ulong o);
    [DllImport("dbghelp.dll")] static extern ulong SymLoadModuleEx(IntPtr proc, IntPtr file, string name, ulong base_, int size, IntPtr data, int flags);
    [DllImport("dbghelp.dll")] static extern bool SymFromAddr(IntPtr proc, ulong addr, out ulong disp, byte[] sym);
    [DllImport("dbghelp.dll", EntryPoint = "SymGetLineFromAddr64")] static extern bool SymGetLineFromAddr64(IntPtr proc, ulong addr, out ulong disp, byte[] line);

    public static void Run(int pid)
    {
        IntPtr snap = CreateToolhelp32Snapshot(4, 0);
        TE32 e = new TE32(); e.dwSize = (uint)Marshal.SizeOf(typeof(TE32));
        uint best = 0; long bu = -1;
        for (bool ok = Thread32First(snap, ref e); ok; ok = Thread32Next(snap, ref e))
        {
            if (e.th32OwnerProcessID != (uint)pid) continue;
            IntPtr h = OpenThread(0x1F03FF, false, e.th32ThreadID);
            long c, x, k, u;
            if (h != IntPtr.Zero && GetThreadTimes(h, out c, out x, out k, out u)) { if (u > bu) { bu = u; best = e.th32ThreadID; } CloseHandle(h); }
            else if (h != IntPtr.Zero) CloseHandle(h);
        }
        CloseHandle(snap);
        Console.WriteLine("busiest tid=" + best + " userMs=" + (bu / 10000));
        IntPtr proc = OpenProcess(0x1F0FFF, false, pid);
        SymSetOptions(0x2 | 0x100);
        SymInitialize(proc, null, true);
        SymLoadModuleEx(proc, IntPtr.Zero, @"C:\Users\Administrator\Desktop\lpc\build\Debug\lpcvm.exe", 0x00130000, 0x551000, IntPtr.Zero, 0);
        IntPtr th = OpenThread(0x1F03FF, false, best);
        SuspendThread(th);
        CONTEXT86 c86 = new CONTEXT86(); c86.ContextFlags = 0x10007;
        if (!GetThreadContext(th, ref c86)) { Console.WriteLine("ctx fail " + Marshal.GetLastWin32Error()); return; }
        Console.WriteLine("EIP=0x{0:X8} ESP=0x{1:X8} EBP=0x{2:X8}", c86.Eip, c86.Esp, c86.Ebp);
        byte[] ctxBuf = new byte[Marshal.SizeOf(typeof(CONTEXT86))];
        IntPtr p = Marshal.AllocHGlobal(ctxBuf.Length);
        Marshal.StructureToPtr(c86, p, false);
        Marshal.Copy(p, ctxBuf, 0, ctxBuf.Length);
        Marshal.FreeHGlobal(p);
        STACKFRAME64 f = new STACKFRAME64();
        f.AddrPC.Offset = c86.Eip; f.AddrPC.Mode = 0; f.AddrFrame.Offset = c86.Ebp; f.AddrStack.Offset = c86.Esp;
        byte[] sym = new byte[4096]; byte[] line = new byte[2048];
        for (int i = 0; i < 64; i++)
        {
            if (!StackWalk64(0x014C, proc, th, ref f, ctxBuf, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero)) break;
            if (f.AddrPC.Offset == 0) break;
            ulong disp;
            string nm = "<nosym>";
            if (SymFromAddr(proc, f.AddrPC.Offset, out disp, sym))
                nm = Encoding.ASCII.GetString(sym, 16, BitConverter.ToInt32(sym, 8)).TrimEnd('\0');
            string ln = "";
            if (SymGetLineFromAddr64(proc, f.AddrPC.Offset, out disp, line))
                ln = Encoding.ASCII.GetString(line, 12, BitConverter.ToInt32(line, 8)).TrimEnd('\0') + ":" + BitConverter.ToInt32(line, 4);
            Console.WriteLine("{0,2}: 0x{1:X8} {2} {3}", i, (uint)f.AddrPC.Offset, nm, ln);
        }
        ResumeThread(th);
        CloseHandle(th);
        CloseHandle(proc);
    }
}
