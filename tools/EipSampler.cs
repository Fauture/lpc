// Sample EIP repeatedly from busiest thread of a 32-bit process.
using System;
using System.Runtime.InteropServices;

public static class EipSampler86
{
    [DllImport("kernel32.dll")] static extern IntPtr OpenProcess(uint access, bool inh, int pid);
    [DllImport("kernel32.dll")] static extern IntPtr OpenThread(uint access, bool inh, uint tid);
    [DllImport("kernel32.dll")] static extern bool GetThreadTimes(IntPtr h, out long c, out long e, out long k, out long u);
    [DllImport("kernel32.dll")] static extern uint SuspendThread(IntPtr h);
    [DllImport("kernel32.dll")] static extern uint ResumeThread(IntPtr h);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
    [DllImport("kernel32.dll")] static extern bool Wow64GetThreadContext(IntPtr h, ref WOW64C ctx);
    [StructLayout(LayoutKind.Sequential)]
    struct TE32 { public uint dwSize, cntUsage, th32ThreadID, th32OwnerProcessID; public int tpBasePri, tpDeltaPri; public uint dwFlags; }
    [DllImport("kernel32.dll")] static extern IntPtr CreateToolhelp32Snapshot(uint f, uint p);
    [DllImport("kernel32.dll")] static extern bool Thread32First(IntPtr s, ref TE32 e);
    [DllImport("kernel32.dll")] static extern bool Thread32Next(IntPtr s, ref TE32 e);
    [StructLayout(LayoutKind.Sequential)]
    struct WOW64C { public uint ContextFlags, Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
        public FSAVE fs; public uint SegGs, SegFs, SegEs, SegDs, Edi, Esi, Ebx, Edx, Ecx, Eax, Ebp, Eip, SegCs, EFlags, Esp, SegSs; }
    [StructLayout(LayoutKind.Sequential)]
    struct FSAVE { public uint a,b,c,d,e,f,g; [MarshalAs(UnmanagedType.ByValArray, SizeConst=80)] public byte[] ra; public uint cr; }

    public static void Run(int pid, int n)
    {
        IntPtr snap = CreateToolhelp32Snapshot(4, 0);
        TE32 e = new TE32(); e.dwSize = (uint)Marshal.SizeOf(typeof(TE32));
        uint bestTid = 0; long bestU = -1;
        for (bool ok = Thread32First(snap, ref e); ok; ok = Thread32Next(snap, ref e))
        {
            if (e.th32OwnerProcessID != (uint)pid) continue;
            IntPtr h = OpenThread(0x1F03FF, false, e.th32ThreadID);
            long c, x, k, u;
            if (h != IntPtr.Zero && GetThreadTimes(h, out c, out x, out k, out u))
            { if (u > bestU) { bestU = u; bestTid = e.th32ThreadID; } CloseHandle(h); }
            else if (h != IntPtr.Zero) CloseHandle(h);
        }
        CloseHandle(snap);
        Console.WriteLine("busiest tid=" + bestTid);
        IntPtr th = OpenThread(0x1F03FF, false, bestTid);
        for (int i = 0; i < n; i++)
        {
            SuspendThread(th);
            WOW64C c = new WOW64C();
            c.ContextFlags = 0x00010003;
            if (Wow64GetThreadContext(th, ref c))
                Console.WriteLine("sample {0}: EIP=0x{1:X8} ESP=0x{2:X8} EBP=0x{3:X8} EAX=0x{4:X8} ECX=0x{5:X8} EDX=0x{6:X8} ESI=0x{7:X8} EDI=0x{8:X8}",
                    i, c.Eip, c.Esp, c.Ebp, c.Eax, c.Ecx, c.Edx, c.Esi, c.Edi);
            else
                Console.WriteLine("sample {0}: ctx fail {1}", i, Marshal.GetLastWin32Error());
            ResumeThread(th);
            System.Threading.Thread.Sleep(200);
        }
        CloseHandle(th);
    }
}
