// List modules of a 32-bit process from 64-bit helper.
using System;
using System.Runtime.InteropServices;
using System.Text;

public static class ModList86
{
    [StructLayout(LayoutKind.Sequential)]
    struct MODULEENTRY32 { public uint dwSize; public uint th32ModuleID; public uint th32ProcessID; public uint GlblcntUsage, ProccntUsage; public uint modBaseAddr; public uint modBaseSize; public uint hModule; [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)] public string szModule; [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)] public string szExePath; }
    [DllImport("kernel32.dll")] static extern IntPtr CreateToolhelp32Snapshot(uint f, uint p);
    [DllImport("kernel32.dll")] static extern bool Module32First(IntPtr s, ref MODULEENTRY32 e);
    [DllImport("kernel32.dll")] static extern bool Module32Next(IntPtr s, ref MODULEENTRY32 e);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);

    public static void Run(int pid)
    {
        IntPtr snap = CreateToolhelp32Snapshot(0x18, (uint)pid); // TH32CS_SNAPMODULE32
        if (snap == IntPtr.Zero) { Console.WriteLine("snap fail " + Marshal.GetLastWin32Error()); return; }
        MODULEENTRY32 e = new MODULEENTRY32(); e.dwSize = (uint)Marshal.SizeOf(typeof(MODULEENTRY32));
        for (bool ok = Module32First(snap, ref e); ok; ok = Module32Next(snap, ref e))
            Console.WriteLine("base=0x{0:X8} size=0x{1:X6} {2}", e.modBaseAddr, e.modBaseSize, e.szModule);
        CloseHandle(snap);
    }
}
