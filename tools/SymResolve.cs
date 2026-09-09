// Resolve EIP/EIP-offset symbols in a 32-bit process from a 64-bit helper.
using System;
using System.Runtime.InteropServices;
using System.Text;

public static class SymResolve86
{
    [DllImport("kernel32.dll")] static extern IntPtr OpenProcess(uint access, bool inh, int pid);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
    [DllImport("kernel32.dll")] static extern bool ReadProcessMemory(IntPtr h, IntPtr addr, byte[] buf, int size, out int read);
    [DllImport("dbghelp.dll")] static extern bool SymInitialize(IntPtr proc, string path, bool inv);
    [DllImport("dbghelp.dll")] static extern ulong SymSetOptions(ulong opts);
    [DllImport("dbghelp.dll")] static extern bool SymFromAddr(IntPtr proc, ulong addr, out ulong disp, byte[] symInfo);
    [DllImport("dbghelp.dll", EntryPoint = "SymGetLineFromAddr64")] static extern bool SymGetLineFromAddr64(IntPtr proc, ulong addr, out ulong disp, byte[] lineInfo);
    [DllImport("dbghelp.dll")] static extern bool SymGetModuleInfo64(IntPtr proc, ulong addr, out byte[] mi);

    public static string Sym(IntPtr proc, uint addr)
    {
        byte[] sym = new byte[2048];
        ulong disp;
        string s = "?";
        if (SymFromAddr(proc, addr, out disp, sym))
            s = Encoding.ASCII.GetString(sym, 16, BitConverter.ToInt32(sym, 8)).TrimEnd('\0');
        byte[] line = new byte[1024];
        string fl = "";
        if (SymGetLineFromAddr64(proc, addr, out disp, line))
        {
            fl = Encoding.ASCII.GetString(line, 12, BitConverter.ToInt32(line, 8)).TrimEnd('\0') + ":" + BitConverter.ToInt32(line, 4);
        }
        return String.Format("0x{0:X8} {1} ({2})", addr, s, fl);
    }

    public static void Run(int pid, uint eip)
    {
        IntPtr proc = OpenProcess(0x1F0FFF, false, pid);
        if (proc == IntPtr.Zero) { Console.WriteLine("OpenProcess fail"); return; }
        SymSetOptions(0x2 | 0x100);
        SymInitialize(proc, null, true);
        Console.WriteLine("EIP: " + Sym(proc, eip));
        // bytes at eip for a manual peek
        byte[] buf = new byte[48];
        int rd;
        if (ReadProcessMemory(proc, (IntPtr)eip, buf, 48, out rd))
            Console.WriteLine("bytes@eip: " + BitConverter.ToString(buf, 0, rd));
        else
            Console.WriteLine("read eip bytes failed " + Marshal.GetLastWin32Error());
        // scan a window of the page for the module the eip belongs to
        CloseHandle(proc);
    }
}
