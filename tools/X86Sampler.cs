// x86 native stack sampler (run under 32-bit powershell).
using System;
using System.Runtime.InteropServices;
using System.Text;

public static class X86Sampler
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
    [StructLayout(LayoutKind.Sequential)] struct FLOATSAVE { public uint a,b,c,d,e,f,g; [MarshalAs(UnmanagedType.ByValArray, SizeConst=80)] public byte[] ra; public uint cr; }
    [StructLayout(LayoutKind.Sequential)] struct CONTEXT86 { public uint ContextFlags, Dr0, Dr1, Dr2, Dr3, Dr6, Dr7; public FLOATSAVE fs; public uint SegGs, SegFs, SegEs, SegDs, Edi, Esi, Ebx, Edx, Ecx, Eax, Ebp, Eip, SegCs, EFlags, Esp, SegSs; }

    [StructLayout(LayoutKind.Sequential)] struct STACKFRAME { public uint AddrPC, AddrRet, AddrFrame, AddrStack; public IntPtr FuncTbl; public uint Params0, Params1, Params2, Params3; public IntPtr Far; public int Virtual; public uint Reserved0; public IntPtr KdHelp; }
    [DllImport("dbghelp.dll")] static extern bool StackWalk(uint machine, IntPtr proc, IntPtr thread, ref STACKFRAME f, ref CONTEXT86 ctx86tx, IntPtr readMem, IntPtr funcTbl, IntPtr sym, IntPtr modList);
    [DllImport("dbghelp.dll")] static extern bool SymInitialize(IntPtr proc, string path, bool inv);
    [DllImport("dbghelp.dll")] static extern bool SymFromAddr(IntPtr proc, uint addr, out uint disp, byte[] sym);
    [DllImport("dbghelp.dll", EntryPoint = "SymGetLineFromAddr")] static extern bool SymGetLineFromAddr(IntPtr proc, uint addr, out uint disp, byte[] line);

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
        SymInitialize(proc, null, true);
        IntPtr th = OpenThread(0x1F03FF, false, best);
        SuspendThread(th);
        CONTEXT86 ctx86 = new CONTEXT86(); ctx86.ContextFlags = 0x10007;
        if (!GetThreadContext(th, ref ctx86)) { Console.WriteLine("ctx fail " + Marshal.GetLastWin32Error()); return; }
        Console.WriteLine("EIP=0x{0:X8} ESP=0x{1:X8} EBP=0x{2:X8}", ctx86.Eip, ctx86.Esp, ctx86.Ebp);
        STACKFRAME f = new STACKFRAME(); f.AddrPC = ctx86.Eip; f.AddrFrame = ctx86.Ebp; f.AddrStack = ctx86.Esp;
        byte[] sym = new byte[2048]; byte[] line = new byte[1024];
        for (int i = 0; i < 64; i++)
        {
            if (!StackWalk(0x14C, proc, th, ref f, ref ctx86, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero)) break;
            uint disp;
            string nm = "<nosym>";
            if (SymFromAddr(proc, f.AddrPC, out disp, sym))
                nm = Encoding.ASCII.GetString(sym, 16, BitConverter.ToInt32(sym, 8)).TrimEnd('\0');
            string ln = "";
            if (SymGetLineFromAddr(proc, f.AddrPC, out disp, line))
                ln = Encoding.ASCII.GetString(line, 12, BitConverter.ToInt32(line, 8)).TrimEnd('\0') + ":" + BitConverter.ToInt32(line, 4);
            Console.WriteLine("{0,2}: 0x{1:X8} {2} {3}", i, f.AddrPC, nm, ln);
        }
        ResumeThread(th);
        CloseHandle(th);
        CloseHandle(proc);
    }
}
