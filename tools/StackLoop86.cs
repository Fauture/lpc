// Frame-pointer (EBP) stack sampler for the 32-bit lpcvm.exe, run from a
// 32-bit process.  Debug builds keep frame pointers, so walk the EBP chain
// per sample: tally (caller-relative) return addresses at each depth and
// print the most common 2-frame prefixes.
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

public static class StackLoop86
{
    [DllImport("kernel32.dll")] static extern IntPtr OpenProcess(uint access, bool inh, int pid);
    [DllImport("kernel32.dll")] static extern IntPtr OpenThread(uint access, bool inh, uint tid);
    [DllImport("kernel32.dll")] static extern bool GetThreadTimes(IntPtr h, out long c, out long e, out long k, out long u);
    [DllImport("kernel32.dll")] static extern uint SuspendThread(IntPtr h);
    [DllImport("kernel32.dll")] static extern uint ResumeThread(IntPtr h);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
    [DllImport("kernel32.dll")] static extern bool GetThreadContext(IntPtr h, ref CONTEXT ctx);
    [DllImport("kernel32.dll")] static extern bool ReadProcessMemory(IntPtr h, IntPtr addr, byte[] buf, int size, out int read);
    [DllImport("kernel32.dll")] static extern IntPtr CreateToolhelp32Snapshot(uint flags, uint pid);
    [DllImport("kernel32.dll")] static extern bool Thread32First(IntPtr snap, ref THREADENTRY32 e);
    [DllImport("kernel32.dll")] static extern bool Thread32Next(IntPtr snap, ref THREADENTRY32 e);
    [DllImport("psapi.dll")] static extern bool EnumProcessModulesEx(IntPtr h, IntPtr[] mods, uint cb, out uint needed, uint flags);
    [DllImport("psapi.dll")] static extern uint GetModuleBaseName(IntPtr h, IntPtr m, StringBuilder sb, uint size);

    [StructLayout(LayoutKind.Sequential)]
    struct THREADENTRY32 { public uint dwSize; public uint cntUsage; public uint th32ThreadID; public uint th32OwnerProcessID; public int tpBasePri; public int tpDeltaPri; public uint dwFlags; }

    [StructLayout(LayoutKind.Sequential)]
    struct FLOAT_SAVE
    {
        public uint ControlWord, StatusWord, TagWord, ErrorOffset, ErrorSelector, DataOffset, DataSelector;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 80)] public byte[] RegisterArea;
        public uint Cr0NpxState;
    }

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

    static uint GetModuleBase(int pid, IntPtr proc)
    {
        IntPtr[] mods = new IntPtr[256];
        uint needed;
        if (EnumProcessModulesEx(proc, mods, (uint)(mods.Length * IntPtr.Size), out needed, 0))
        {
            int n = (int)(needed / IntPtr.Size);
            for (int i = 0; i < n && i < mods.Length; i++)
            {
                StringBuilder sb = new StringBuilder(260);
                GetModuleBaseName(proc, mods[i], sb, 260);
                if (sb.ToString().Equals("lpcvm.exe", StringComparison.OrdinalIgnoreCase))
                    return (uint)mods[i];
            }
        }
        return 0;
    }

    public static void Run(int pid, int samples, int intervalMs)
    {
        IntPtr proc = OpenProcess(0x1F0FFF, false, pid);
        if (proc == IntPtr.Zero) { Console.WriteLine("OpenProcess fail " + Marshal.GetLastWin32Error()); return; }
        uint baseAddr = GetModuleBase(pid, proc);
        Console.WriteLine("lpcvm base=0x{0:X8}", baseAddr);

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
                CloseHandle(h);
            }
            else if (h != IntPtr.Zero) CloseHandle(h);
        }
        CloseHandle(snap);
        if (bestTid == 0) { Console.WriteLine("no thread"); return; }
        Console.WriteLine("sampling tid=" + bestTid);
        IntPtr th = OpenThread(0x1F03FF, false, bestTid);
        if (th == IntPtr.Zero) { Console.WriteLine("OpenThread fail " + Marshal.GetLastWin32Error()); return; }

        // pairs of (eip, ret@frame0) tallies; plus full-path histogram depth6
        var pairs = new Dictionary<ulong, int>();
        var paths = new Dictionary<string, int>();
        CONTEXT ctx = new CONTEXT();
        ctx.ContextFlags = 0x00010007;
        byte[] buf = new byte[8];
        for (int i = 0; i < samples; i++)
        {
            if (SuspendThread(th) == 0xFFFFFFFF) break;
            if (GetThreadContext(th, ref ctx))
            {
                uint eip = ctx.Eip, ebp = ctx.Ebp;
                ulong key = ((ulong)(eip - baseAddr) << 32) | (uint)((ebp >= baseAddr) ? ebp - baseAddr : 0);
                int c; pairs.TryGetValue(key, out c); pairs[key] = c + 1;
                // walk up to 8 frames
                uint[] rets = new uint[8];
                uint cur = ebp; int nf = 0;
                while (nf < 8 && cur >= 0x10000 && cur < 0x80000000)
                {
                    int rd;
                    if (!ReadProcessMemory(proc, (IntPtr)(cur + 4), buf, 4, out rd) || rd != 4) break;
                    uint ra = BitConverter.ToUInt32(buf, 0);
                    if (ra >= 0x10000 && ra < 0x80000000) rets[nf++] = ra;
                    if (!ReadProcessMemory(proc, (IntPtr)cur, buf, 4, out rd) || rd != 4) break;
                    uint nb = BitConverter.ToUInt32(buf, 0);
                    if (nb <= cur) break;   // guard against loops
                    cur = nb;
                }
                if (nf >= 1)
                {
                    StringBuilder sb = new StringBuilder(160);
                    sb.Append((eip - baseAddr).ToString("X6"));
                    for (int f = 0; f < nf && f < 5; f++) sb.Append(" <- " + (rets[f] - baseAddr).ToString("X6"));
                    int pc; paths.TryGetValue(sb.ToString(), out pc); paths[sb.ToString()] = pc + 1;
                }
            }
            ResumeThread(th);
            if (intervalMs > 0) System.Threading.Thread.Sleep(intervalMs);
        }
        ResumeThread(th);
        CloseHandle(th);

        Console.WriteLine("== top frame pairs (eip, ebp_ret) ==");
        var pl = new List<KeyValuePair<ulong, int>>(pairs);
        pl.Sort((a, b) => b.Value.CompareTo(a.Value));
        for (int i = 0; i < pl.Count && i < 15; i++)
            Console.WriteLine("{0,5}x  eip={1:X6}  ebpRet={2:X6}", pl[i].Value, pl[i].Key >> 32, (uint)(pl[i].Key & 0xFFFFFFFF));

        Console.WriteLine("== top path prefixes ==");
        var pt = new List<KeyValuePair<string, int>>(paths);
        pt.Sort((a, b) => b.Value.CompareTo(a.Value));
        for (int i = 0; i < pt.Count && i < 25; i++)
            Console.WriteLine("{0,5}x  {1}", pt[i].Value, pt[i].Key);
        CloseHandle(proc);
    }
}
