// EnumProcessModulesEx on a WOW64 target process from x64 helper.
using System;
using System.Runtime.InteropServices;

public static class ModList2
{
    [DllImport("kernel32.dll")] static extern IntPtr OpenProcess(uint access, bool inh, int pid);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
    [DllImport("psapi.dll")] static extern bool EnumProcessModulesEx(IntPtr h, [Out] IntPtr[] mods, int cb, out int needed, uint filter);
    [DllImport("psapi.dll")] static extern uint GetModuleBaseName(IntPtr h, IntPtr m, System.Text.StringBuilder name, int size);
    [DllImport("psapi.dll")] static extern bool GetModuleInformation(IntPtr h, IntPtr m, out MODINFO info, int cb);
    [StructLayout(LayoutKind.Sequential)] struct MODINFO { public IntPtr lpBaseOfDll; public uint SizeOfImage; public IntPtr EntryPoint; }

    public static void Run(int pid)
    {
        IntPtr h = OpenProcess(0x1F0FFF, false, pid);
        if (h == IntPtr.Zero) { Console.WriteLine("open fail " + Marshal.GetLastWin32Error()); return; }
        IntPtr[] mods = new IntPtr[1024];
        int needed;
        bool ok = EnumProcessModulesEx(h, mods, 1024 * IntPtr.Size, out needed, 0x03); // LIST_MODULES_32BIT|64BIT
        if (!ok) { Console.WriteLine("enum fail " + Marshal.GetLastWin32Error()); return; }
        int count = needed / IntPtr.Size;
        for (int i = 0; i < count && i < 1024; i++)
        {
            var sb = new System.Text.StringBuilder(512);
            GetModuleBaseName(h, mods[i], sb, 512);
            MODINFO mi;
            if (GetModuleInformation(h, mods[i], out mi, Marshal.SizeOf(typeof(MODINFO))))
                Console.WriteLine("base=0x{0:X8} size=0x{1:X6} {2}", mi.lpBaseOfDll.ToInt64(), mi.SizeOfImage, sb);
        }
        CloseHandle(h);
    }
}
