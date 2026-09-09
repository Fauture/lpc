// Offline symbol resolve: SymInitialize with deferred loads; SymFromAddr auto-loads.
using System;
using System.Runtime.InteropServices;
using System.Text;

public static class OffSym2
{
    [DllImport("dbghelp.dll")] static extern bool SymInitialize(IntPtr proc, string path, bool inv);
    [DllImport("dbghelp.dll")] static extern ulong SymSetOptions(ulong o);
    [DllImport("dbghelp.dll")] static extern bool SymFromAddr(IntPtr proc, ulong addr, out ulong disp, byte[] sym);
    [DllImport("dbghelp.dll", EntryPoint = "SymGetLineFromAddr64")] static extern bool SymGetLineFromAddr64(IntPtr proc, ulong addr, out ulong disp, byte[] line);
    [DllImport("dbghelp.dll")] static extern bool SymCleanup(IntPtr proc);
    [DllImport("dbghelp.dll")] static extern ulong SymLoadModuleEx(IntPtr proc, IntPtr file, string name, string modname, ulong base_, int size, IntPtr data, int flags);

    public static void Resolve(params ulong[] rvas)
    {
        IntPtr proc = new IntPtr(-1); // GetCurrentProcess
        string dir = @"C:\Users\Administrator\Desktop\lpc\build\Debug";
        SymSetOptions(0x2 | 0x100 | 0x800 | 0x1000);
        SymInitialize(proc, dir, true);
        string exe = dir + @"\lpcvm.exe";
        SymLoadModuleEx(proc, IntPtr.Zero, exe, null, 0, 0, IntPtr.Zero, 0);
        foreach (ulong rva in rvas)
        {
            ulong addr = 0x00130000 + rva;
            ulong disp;
            byte[] sym = new byte[4096];
            string nm = "<no-sym>";
            if (SymFromAddr(proc, addr, out disp, sym))
                nm = Encoding.ASCII.GetString(sym, 16, BitConverter.ToInt32(sym, 8)).TrimEnd('\0');
            byte[] line = new byte[2048];
            string fl = "";
            if (SymGetLineFromAddr64(proc, addr, out disp, line))
                fl = Encoding.ASCII.GetString(line, 12, BitConverter.ToInt32(line, 8)).TrimEnd('\0') + ":" + BitConverter.ToInt32(line, 4);
            Console.WriteLine("RVA 0x{0:X6} => {1} ({2})", rva, nm, fl);
        }
        SymCleanup(proc);
    }
}
