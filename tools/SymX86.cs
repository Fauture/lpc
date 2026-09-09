// 32-bit symbol resolver: SymLoadModuleEx on the lpcvm PDB, then SymFromAddr.
using System;
using System.Runtime.InteropServices;
using System.Text;

public static class SymX86
{
    [DllImport("kernel32.dll")] static extern IntPtr OpenProcess(uint access, bool inh, int pid);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
    [DllImport("dbghelp.dll")] static extern bool SymInitialize(IntPtr proc, string path, bool inv);
    [DllImport("dbghelp.dll")] static extern ulong SymSetOptions(ulong opts);
    [DllImport("dbghelp.dll")] static extern ulong SymLoadModuleEx(IntPtr proc, IntPtr file, string name, uint base_, int size, IntPtr data, int flags);
    [DllImport("dbghelp.dll")] static extern bool SymFromAddr(IntPtr proc, uint addr, out uint disp, byte[] sym);
    [DllImport("dbghelp.dll", EntryPoint="SymGetLineFromAddr")] static extern bool SymGetLineFromAddr(IntPtr proc, uint addr, out uint disp, byte[] line);

    public static void Resolve(int pid, uint addr)
    {
        IntPtr proc = OpenProcess(0x1F0FFF, false, pid);
        if (proc == IntPtr.Zero) { Console.WriteLine("open fail"); return; }
        SymSetOptions(0x2 | 0x100);
        SymInitialize(proc, null, true);
        string exe = @"C:\Users\Administrator\Desktop\lpc\build\Debug\lpcvm.exe";
        SymLoadModuleEx(proc, IntPtr.Zero, exe, 0x00130000, 0x551000, IntPtr.Zero, 0);
        uint disp;
        byte[] sym = new byte[4096];
        if (SymFromAddr(proc, addr, out disp, sym))
        {
            string nm = Encoding.ASCII.GetString(sym, 16, BitConverter.ToInt32(sym, 8)).TrimEnd('\0');
            byte[] line = new byte[2048];
            string fl = "";
            if (SymGetLineFromAddr(proc, addr, out disp, line))
                fl = Encoding.ASCII.GetString(line, 12, BitConverter.ToInt32(line, 8)).TrimEnd('\0') + ":" + BitConverter.ToInt32(line, 4);
            Console.WriteLine("0x{0:X8} {1} +0x{2:X} {3}", addr, nm, disp, fl);
        }
        else
            Console.WriteLine("0x{0:X8} no-sym (err {1})", addr, Marshal.GetLastWin32Error());
        CloseHandle(proc);
    }
}
