// Resolve symbols by explicit module load (exe + PDB) for a WOW64 target.
using System;
using System.Runtime.InteropServices;
using System.Text;

public static class SymResolveEx
{
    [DllImport("kernel32.dll")] static extern IntPtr OpenProcess(uint access, bool inh, int pid);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
    [DllImport("dbghelp.dll")] static extern bool SymInitialize(IntPtr proc, string path, bool inv);
    [DllImport("dbghelp.dll")] static extern ulong SymSetOptions(ulong opts);
    [DllImport("dbghelp.dll")] static extern ulong SymLoadModuleEx(IntPtr proc, IntPtr file, string name, ulong baseAddr_, int size, IntPtr data, int flags);
    [DllImport("dbghelp.dll")] static extern bool SymFromAddr(IntPtr proc, ulong addr, out ulong disp, byte[] sym);
    [DllImport("dbghelp.dll", EntryPoint="SymGetLineFromAddr64")] static extern bool SymGetLineFromAddr64(IntPtr proc, ulong addr, out ulong disp, byte[] line);

    public static void Run(int pid, ulong addr, ulong baseAddr)
    {
        IntPtr proc = OpenProcess(0x1F0FFF, false, pid);
        SymSetOptions(0x2 | 0x100);
        SymInitialize(proc, null, true);
        string exe = @"C:\Users\Administrator\Desktop\lpc\build\Debug\lpcvm.exe";
        ulong b = SymLoadModuleEx(proc, IntPtr.Zero, exe, baseAddr, 0x551000, IntPtr.Zero, 0);
        Console.WriteLine("loaded base=0x{0:X}", b);
        byte[] sym = new byte[4096];
        ulong disp;
        if (SymFromAddr(proc, addr, out disp, sym))
        {
            string nm = Encoding.ASCII.GetString(sym, 16, BitConverter.ToInt32(sym, 8)).TrimEnd('\0');
            Console.WriteLine("addr 0x{0:X8} => {1} +0x{2:X}", addr, nm, disp);
        }
        else
            Console.WriteLine("SymFromAddr failed {0}", Marshal.GetLastWin32Error());
        byte[] line = new byte[2048];
        if (SymGetLineFromAddr64(proc, addr, out disp, line))
        {
            string fl = Encoding.ASCII.GetString(line, 12, BitConverter.ToInt32(line, 8)).TrimEnd('\0');
            Console.WriteLine("  line: {0}:{1}", fl, BitConverter.ToInt32(line, 4));
        }
        CloseHandle(proc);
    }
}
