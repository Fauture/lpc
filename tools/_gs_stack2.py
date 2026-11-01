# -*- coding: utf-8 -*-
"""Dump all thread stacks of the hung lpcvm process (Win32/x86).

Finds the gs process (largest working set among lpcvm.exe), opens it,
enumerates threads, gets each context, walks the EBP frame chain by
ReadProcessMemory, and resolves every code address through dbghelp
(PDB from build/Debug).  Prints C++ frames of every thread.
"""
import ctypes, ctypes.wintypes as wt, os, re, struct, subprocess, sys, time

TH32CS_SNAPTHREAD = 0x4
THREAD_ALL = 0x1F03FF
CONTEXT_i386 = 0x00010000
CONTEXT_FULL = CONTEXT_i386 | 0x0007   # control|integer|segments

k32 = ctypes.windll.kernel32
psapi = ctypes.windll.psapi
dbh = ctypes.windll.dbghelp


class THREADENTRY32(ctypes.Structure):
    _fields_ = [('dwSize', wt.DWORD), ('cntUsage', wt.DWORD),
                ('th32ThreadID', wt.DWORD), ('th32OwnerProcessID', wt.DWORD),
                ('tpBasePri', ctypes.c_long), ('tpDeltaPri', ctypes.c_long),
                ('dwFlags', wt.DWORD)]


class CONTEXT(ctypes.Structure):
    _fields_ = [('ContextFlags', wt.DWORD),
                ('Dr0', wt.DWORD), ('Dr1', wt.DWORD), ('Dr2', wt.DWORD),
                ('Dr3', wt.DWORD), ('Dr6', wt.DWORD), ('Dr7', wt.DWORD),
                ('FloatSave', ctypes.c_byte * 112),
                ('SegGs', wt.DWORD), ('SegFs', wt.DWORD),
                ('SegEs', wt.DWORD), ('SegDs', wt.DWORD),
                ('Edi', wt.DWORD), ('Esi', wt.DWORD), ('Ebx', wt.DWORD),
                ('Edx', wt.DWORD), ('Ecx', wt.DWORD), ('Eax', wt.DWORD),
                ('Ebp', wt.DWORD), ('Eip', wt.DWORD), ('SegCs', wt.DWORD),
                ('EFlags', wt.DWORD), ('Esp', wt.DWORD), ('SegSs', wt.DWORD),
                ('ExtendedRegisters', ctypes.c_byte * 512)]


class MODULEINFO(ctypes.Structure):
    _fields_ = [('lpBaseOfDll', ctypes.c_void_p),
                ('SizeOfImage', wt.DWORD),
                ('EntryPoint', ctypes.c_void_p)]


class SYMBOL_INFO(ctypes.Structure):
    _fields_ = [('SizeOfStruct', wt.ULONG), ('TypeIndex', wt.ULONG),
                ('Reserved', ctypes.c_uint64 * 2), ('Index', wt.ULONG),
                ('Size', wt.ULONG), ('ModBase', ctypes.c_uint64),
                ('Flags', wt.ULONG), ('Value', ctypes.c_uint64),
                ('Address', ctypes.c_uint64), ('Register', wt.ULONG),
                ('Scope', wt.ULONG), ('Tag', wt.ULONG),
                ('NameLen', wt.ULONG), ('MaxNameLen', wt.ULONG),
                ('Name', ctypes.c_char * 256)]


def find_gs_pid():
    out = subprocess.check_output(
        ['tasklist', '/FI', 'IMAGENAME eq lpcvm.exe', '/FO', 'CSV'],
        text=True, errors='replace')
    best = None
    for line in out.splitlines():
        if 'lpcvm' not in line:
            continue
        parts = [p.strip('"') for p in line.split('","')]
        pid = int(parts[1])
        mem = parts[-1].strip('"\n\r K').replace(',', '').replace('.', '')
        mem = int(re.sub(r'[^0-9]', '', mem) or 0)
        if best is None or mem > best[1]:
            best = (pid, mem)
    return best


def rpm(h, addr, size):
    buf = ctypes.create_string_buffer(size)
    got = ctypes.c_size_t(0)
    if not k32.ReadProcessMemory(h, ctypes.c_void_p(addr), buf, size,
                                 ctypes.byref(got)):
        return None
    return buf.raw[:got.value]


def resolve(h, addr, cache={}):
    if addr in cache:
        return cache[addr]
    si = SYMBOL_INFO()
    si.SizeOfStruct = ctypes.sizeof(SYMBOL_INFO) - 256
    si.MaxNameLen = 256
    disp = ctypes.c_uint64(0)
    name = None
    if dbh.SymFromAddr(h, ctypes.c_uint64(addr), ctypes.byref(disp),
                       ctypes.byref(si)):
        name = si.Name.decode('utf-8', 'replace')
        if disp.value:
            name += '+0x%x' % disp.value
    cache[addr] = name
    return name


def main():
    found = find_gs_pid()
    if not found:
        print('no lpcvm process'); return 1
    pid, mem = found
    print('gs pid=%d ws=%.0fMB' % (pid, mem))
    h = k32.OpenProcess(0x1F0FFF, False, pid)
    if not h:
        print('OpenProcess failed', k32.GetLastError()); return 1

    # symbols: load lpcvm module with the Debug PDB search path
    pdb_dir = os.path.abspath('build/Debug').encode()
    dbh.SymSetOptions(0x10 | 0x2)   # LOAD_LINES? no: SYMOPT_UNDNAME|SYMOPT_DEFERRED
    if not dbh.SymInitializeW(h, ctypes.create_string_buffer(
            ('C:\\Users\\Administrator\\Desktop\\lpc\\build\\Debug;').encode('utf-16-le') if False else b'', 0), False):
        pass
    # SymInitializeW expects wchar; use W variant properly
    dbh.SymInitializeW(h, os.path.abspath('build/Debug'), False)
    # find module base via EnumProcessModulesEx
    LIST_MODULES_ALL = 0x03
    need = wt.DWORD()
    psapi.EnumProcessModulesEx(h, None, 0, ctypes.byref(need), LIST_MODULES_ALL)
    cnt = need.value // ctypes.sizeof(wt.HMODULE)
    arr = (ctypes.c_void_p * max(cnt, 1))()
    psapi.EnumProcessModulesEx(h, arr, ctypes.sizeof(arr), ctypes.byref(need),
                               LIST_MODULES_ALL)
    exe_base = None
    for i in range(need.value // ctypes.sizeof(ctypes.c_void_p)):
        mi = MODULEINFO()
        psapi.GetModuleInformation(h, ctypes.c_void_p(arr[i]),
                                   ctypes.byref(mi), ctypes.sizeof(mi))
        nm = ctypes.create_unicode_buffer(512)
        psapi.GetModuleFileNameExW(h, ctypes.c_void_p(arr[i]), nm, 512)
        if nm.value.lower().endswith('lpcvm.exe'):
            exe_base = mi.lpBaseOfDll
            dbh.SymLoadModuleEx(h, None, nm.value.encode(), None,
                                ctypes.c_uint64(mi.lpBaseOfDll),
                                mi.SizeOfImage, None, 0)
            print('lpcvm base=0x%x size=0x%x' % (mi.lpBaseOfDll, mi.SizeOfImage))

    snap = k32.CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0)
    te = THREADENTRY32(); te.dwSize = ctypes.sizeof(THREADENTRY32)
    ok = k32.Thread32First(snap, ctypes.byref(te))
    rows = []
    while ok:
        if te.th32OwnerProcessID == pid:
            rows.append(te.th32ThreadID)
        ok = k32.Thread32Next(snap, ctypes.byref(te))
    k32.CloseHandle(snap)
    print('threads:', len(rows))

    for tid in rows:
        th = k32.OpenThread(THREAD_ALL, False, tid)
        if not th:
            print('tid=%d open-failed' % tid); continue
        WOW64 = ctypes.sizeof(ctypes.c_void_p) == 8   # 64b python, 32b target
        if WOW64:
            k32.Wow64SuspendThread(th)
        else:
            k32.SuspendThread(th)
        ctx = CONTEXT(); ctx.ContextFlags = CONTEXT_FULL
        got = (k32.Wow64GetThreadContext(th, ctypes.byref(ctx)) if WOW64
               else k32.GetThreadContext(th, ctypes.byref(ctx)))
        if not got:
            k32.ResumeThread(th); k32.CloseHandle(th)
            continue
        frames = []
        eip = ctx.Eip; ebp = ctx.Ebp; n = 0
        frames.append((eip, None))
        while ebp and n < 40:
            data = rpm(h, ebp, 8)
            if not data or len(data) < 8:
                break
            nebp, ret = struct.unpack('<II', data)
            if ret < 0x10000:
                break
            frames.append((ret, None))
            if nebp <= ebp:
                break
            ebp = nebp
            n += 1
        print('--- tid=%d eip=%s esp=0x%x' % (
            tid, resolve(h, eip) or hex(eip), ctx.Esp))
        for a, _ in frames:
            s = resolve(h, a)
            if s:
                print('     %s' % s)
        k32.ResumeThread(th)
        k32.CloseHandle(th)
    return 0


if __name__ == '__main__':
    sys.exit(main())
