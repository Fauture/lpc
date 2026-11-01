import sys, ctypes
from ctypes import wintypes
import os
exe = r"C:/Users/Administrator/Desktop/lpc/build/Debug/lpcvm.exe"
# image base of a 32-bit PE - get from header
import struct
with open(exe,'rb') as f:
    d = f.read(0x400)
pe = struct.unpack_from('<I', d, 0x3c)[0]
machine = struct.unpack_from('<H', d, pe+4)[0]
optoff = pe+24
magic = struct.unpack_from('<H', d, optoff)[0]
imgbase = struct.unpack_from('<I', d, optoff+28)[0]
print("imagebase=0x%x" % imgbase)
dbghelp = ctypes.WinDLL(r"C:/Program Files (x86)/Windows Kits/10/Debuggers/x64/dbghelp.dll") if os.path.exists(r"C:/Program Files (x86)/Windows Kits/10/Debuggers/x64/dbghelp.dll") else ctypes.WinDLL("dbghelp.dll")
hproc = ctypes.c_void_p(-1)  # GetCurrentProcess
dbghelp.SymSetOptions(0x00000010 | 0x00000002 | 0x00000004)  # LOAD_LINES|UNDNAME|DEFERRED
if not dbghelp.SymInitializeW(hproc, None, False):
    print("SymInitialize failed", ctypes.get_last_error()); sys.exit(1)
base = dbghelp.SymLoadModuleExW(hproc, None, exe, None, imgbase, os.path.getsize(exe), None, 0)
if not base:
    print("SymLoadModuleEx failed", ctypes.get_last_error()); sys.exit(1)
class SYMBOL_INFOW(ctypes.Structure):
    _fields_=[("SizeOfStruct",wintypes.ULONG),("TypeIndex",wintypes.ULONG),("Reserved",ctypes.c_uint64*2),
              ("Index",wintypes.ULONG),("Size",wintypes.ULONG),("ModBase",ctypes.c_uint64),("Flags",wintypes.ULONG),
              ("Value",ctypes.c_uint64),("Address",ctypes.c_uint64),("Register",wintypes.ULONG),("Scope",wintypes.ULONG),
              ("Tag",wintypes.ULONG),("NameLen",wintypes.ULONG),("MaxNameLen",wintypes.ULONG),("Name",ctypes.c_wchar*512)]
for arg in sys.argv[1:]:
    rva = int(arg,0) if not arg.startswith("0x") else int(arg,16)
    addr = imgbase + rva
    si = SYMBOL_INFOW(); si.SizeOfStruct=88; si.MaxNameLen=512
    disp = ctypes.c_uint64(0)
    ok = dbghelp.SymFromAddrW(hproc, ctypes.c_uint64(addr), ctypes.byref(disp), ctypes.byref(si))
    print("rva=0x%x -> %s + 0x%x %s" % (rva, si.Name if ok else "?", disp.value if ok else 0, "OK" if ok else "FAIL"))

# line info
class IMAGEHLP_LINEW64(ctypes.Structure):
    _fields_=[("SizeOfStruct",wintypes.ULONG),("Key",ctypes.c_void_p),("FileName",ctypes.c_wchar_p),
              ("LineNumber",wintypes.ULONG),("Address",ctypes.c_uint64)]
dbghelp.SymGetLineFromAddrW64.restype = ctypes.c_bool
dbghelp.SymGetLineFromAddrW64.argtypes = [ctypes.c_void_p, ctypes.c_uint64, ctypes.POINTER(wintypes.ULONG), ctypes.POINTER(IMAGEHLP_LINEW64)]
print("---- lines ----")
for arg in sys.argv[1:]:
    rva = int(arg,16) if arg.startswith("0x") else int(arg,0)
    addr = imgbase + rva
    disp = wintypes.ULONG(0)
    ln = IMAGEHLP_LINEW64(); ln.SizeOfStruct = 32
    if dbghelp.SymGetLineFromAddrW64(hproc, ctypes.c_uint64(addr), ctypes.byref(disp), ctypes.byref(ln)):
        print("rva=0x%x -> %s:%d (+%d)" % (rva, ln.FileName, ln.LineNumber, disp.value))
    else:
        print("rva=0x%x -> no line info" % rva)
