// Offline symbol resolve: load lpcvm.pdb via SymLoadModuleEx, resolve RVAs.
using System;
using System.Runtime.InteropServices;
using System.Text;

public static class OffSym
{
    [DllImport("dbghelp.dll")] static extern bool SymInitialize(IntPtr proc, string path, bool inv);
    [DllImport("dbghelp.dll")] static extern ulong SymSetOptions(ulong o);
    [DllImport("dbghelp.dll")] static extern ulong SymLoadModuleEx(IntPtr proc, IntPtr file, string name, ulong base_, int size, IntPtr data, int flags);
    [DllImport("dbghelp.dll")] static extern bool SymFromAddr(IntPtr proc, ulong addr, out ulong disp, byte[] sym);
    [DllImport("dbghelp.dll", EntryPoint = "SymGetLineFromAddr64")] static extern bool SymGetLineFromAddr64(IntPtr proc, ulong addr, out ulong disp, byte[] line);

    public static void Run(string exe, string pdbDir, params ulong[] rvas)
    {
        IntPtr proc = (IntPtr)1; // pseudo handle
        SymSetOptions(0x2 | 0x100 | 0x800); // UNDNAME + LOAD_LINES + AUTO_PUBLICS
        SymInitialize(proc, pdbDir, false);
        ulong b = SymLoadModuleEx(proc, IntPtr.Zero, exe, 0x00130000, 0x551000, IntPtr.Zero, 0);
        Console.WriteLine("loaded base=0x{0:X}", b);
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
    }
}
