// Loop EIP sampler for the 32-bit lpcvm.exe, run from a 32-bit process
// (SysWOW64 powershell): repeated SuspendThread/GetThreadContext/ResumeThread,
// tally EIPs, then resolve the top addresses via 32-bit dbghelp SymFromAddr.
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

public static class Sampler86Loop
{
    [DllImport("kernel32.dll")] static extern IntPtr OpenProcess(uint access, bool inh, int pid);
    [DllImport("kernel32.dll")] static extern IntPtr OpenThread(uint access, bool inh, uint tid);
    [DllImport("kernel32.dll")] static extern bool GetThreadTimes(IntPtr h, out long c, out long e, out long k, out long u);
    [DllImport("kernel32.dll")] static extern uint SuspendThread(IntPtr h);
    [DllImport("kernel32.dll")] static extern uint ResumeThread(IntPtr h);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
    [DllImport("kernel32.dll")] static extern bool GetThreadContext(IntPtr h, ref CONTEXT ctx);
    [DllImport("kernel32.dll")] static extern bool SetThreadContext(IntPtr h, ref CONTEXT ctx);
    [DllImport("kernel32.dll")] static extern bool ReadProcessMemory(IntPtr h, IntPtr addr, byte[] buf, int size, out int read);
    [DllImport("kernel32.dll")] static extern IntPtr CreateToolhelp32Snapshot(uint flags, uint pid);
    [DllImport("kernel32.dll")] static extern bool Thread32First(IntPtr snap, ref THREADENTRY32 e);
    [DllImport("kernel32.dll")] static extern bool Thread32Next(IntPtr snap, ref THREADENTRY32 e);

    [StructLayout(LayoutKind.Sequential)]
    struct THREADENTRY32 { public uint dwSize; public uint cntUsage; public uint th32ThreadID; public uint th32OwnerProcessID; public int tpBasePri; public int tpDeltaPri; public uint dwFlags; }

    [StructLayout(LayoutKind.Sequential)]
    struct FLOAT_SAVE
    {
        public uint ControlWord, StatusWord, TagWord, ErrorOffset, ErrorSelector, DataOffset, DataSelector;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 80)] public byte[] RegisterArea;
        public uint Cr0NpxState;
    }

    // x86 CONTEXT, CONTEXT_i386=0x00010000 | CONTROL=0x01 | INTEGER=0x02 | SEGMENTS=0x04 => 0x10007
    [StructLayout(LayoutKind.Sequential)]
    struct CONTEXT
    {
        public uint ContextFlags;
        public uint Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
        public FLOAT_SAVE FloatSave;
        public uint SegGs, SegFs, SegEs, SegDs;
        public uint Edi, Esi, Ebx, Edx, Ecx, Eax;
        public uint Ebp, Eip, SegCs, EFlags, Esp, SegSs;
    }

    // dbghelp symbol resolution (32-bit dbghelp against the x86 PDB)
    [DllImport("dbghelp.dll")] static extern bool SymInitialize(IntPtr proc, string path, bool inv);
    [DllImport("dbghelp.dll")] static extern ulong SymSetOptions(ulong opts);
    [DllImport("dbghelp.dll", EntryPoint = "SymFromAddr")] static extern bool SymFromAddr(IntPtr proc, ulong addr, out ulong disp, byte[] symInfo);
    [DllImport("dbghelp.dll", EntryPoint = "SymGetLineFromAddr64")] static extern bool SymGetLineFromAddr64(IntPtr proc, ulong addr, out ulong disp, byte[] lineInfo);
    [DllImport("psapi.dll")] static extern bool EnumProcessModulesEx(IntPtr h, IntPtr[] mods, uint cb, out uint needed, uint flags);
    [DllImport("psapi.dll")] static extern uint GetModuleBaseName(IntPtr h, IntPtr m, StringBuilder sb, uint size);

    static uint GetModuleBase(int pid, IntPtr proc)
    {
        // Find the module base via EnumProcessModulesEx (32-bit to 32-bit works).
        IntPtr[] mods = new IntPtr[64];
        uint needed;
        if (EnumProcessModulesEx(proc, mods, (uint)(mods.Length * IntPtr.Size), out needed, 0))
        {
            int n = (int)(needed / IntPtr.Size);
            for (int i = 0; i < n && i < mods.Length; i++)
            {
                StringBuilder sb = new StringBuilder(260);
                GetModuleBaseName(proc, mods[i], sb, 260);
                if (sb.ToString().ToLowerInvariant().Contains("lpcvm"))
                    return (uint)mods[i];
            }
        }
        return 0;
    }

    public static void Run(int pid, int samples, int intervalMs)
    {
        IntPtr proc = OpenProcess(0x1F0FFF, false, pid);
        if (proc == IntPtr.Zero) { Console.WriteLine("OpenProcess fail " + Marshal.GetLastWin32Error()); return; }
        // Print all modules first so the caller can map absolute EIPs offline.
        IntPtr[] allMods = new IntPtr[256];
        uint modNeed;
        if (EnumProcessModulesEx(proc, allMods, (uint)(allMods.Length * IntPtr.Size), out modNeed, 0))
        {
            int mn = (int)(modNeed / IntPtr.Size);
            for (int i = 0; i < mn && i < allMods.Length; i++)
            {
                StringBuilder sb = new StringBuilder(260);
                GetModuleBaseName(proc, allMods[i], sb, 260);
                Console.WriteLine("module 0x{0:X8} {1}", (uint)allMods[i], sb);
            }
        }
        uint baseAddr = GetModuleBase(pid, proc);
        Console.WriteLine("lpcvm module base=0x{0:X8}", baseAddr);

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
        if (bestTid == 0) { Console.WriteLine("no thread found"); return; }
        Console.WriteLine("sampling tid=" + bestTid + " x" + samples);
        IntPtr th = OpenThread(0x1F03FF, false, bestTid);
        if (th == IntPtr.Zero) { Console.WriteLine("OpenThread fail " + Marshal.GetLastWin32Error()); return; }
        CONTEXT ctx = new CONTEXT();
        ctx.ContextFlags = 0x00010007;
        var hist = new Dictionary<uint, int>();
        uint esp0 = 0, ebp0 = 0;
        for (int i = 0; i < samples; i++)
        {
            if (SuspendThread(th) == 0xFFFFFFFF) break;
            if (GetThreadContext(th, ref ctx))
            {
                int c;
                hist.TryGetValue(ctx.Eip, out c);
                hist[ctx.Eip] = c + 1;
                if (esp0 == 0) { esp0 = ctx.Esp; ebp0 = ctx.Ebp; }
            }
            ResumeThread(th);
            if (intervalMs > 0) System.Threading.Thread.Sleep(intervalMs);
        }
        ResumeThread(th);
        CloseHandle(th);
        Console.WriteLine("samples recorded=" + hist.Count + " esp0=0x" + esp0.ToString("X8") + " ebp0=0x" + ebp0.ToString("X8"));

        // sort desc
        var list = new List<KeyValuePair<uint, int>>(hist);
        list.Sort((a, b) => b.Value.CompareTo(a.Value));
        SymSetOptions(0x00000002 | 0x00000100 | 0x00000001);
        SymInitialize(proc, null, true);
        byte[] sym = new byte[4096]; byte[] line = new byte[2048];
        for (int i = 0; i < list.Count && i < 60; i++)
        {
            uint addr = list[i].Key;
            uint rva = addr >= baseAddr ? addr - baseAddr : addr;
            ulong disp; string name = "<no-sym>";
            bool got = SymFromAddr(proc, (ulong)addr, out disp, sym);
            if (got)
            {
                int nlen = BitConverter.ToInt32(sym, 8);
                if (nlen < 0 || 16 + nlen > sym.Length) nlen = 0;
                name = Encoding.ASCII.GetString(sym, 16, nlen).TrimEnd('\0');
            }
            string file = "";
            if (SymGetLineFromAddr64(proc, (ulong)addr, out disp, line))
            {
                int ln = BitConverter.ToInt32(line, 4);
                int fn = BitConverter.ToInt32(line, 8);
                if (fn >= 0 && 12 + fn <= line.Length)
                    file = Encoding.ASCII.GetString(line, 12, fn).TrimEnd('\0') + ":" + ln;
            }
            Console.WriteLine("{0,5}x  0x{1:X8} rva0x{2:X8}  {3} +0x{4:X}  {5}", list[i].Value, addr, rva, name, disp, file);
        }
        CloseHandle(proc);
    }
}
