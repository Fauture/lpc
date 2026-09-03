# -*- coding: utf-8 -*-
"""Sample EIP of every thread of a target process for N seconds and tally RVAs.
Usage: python tools/eip_sampler.py <pid> <seconds> [out.txt]
Then resolve RVAs against build/Debug/lpcvm.map with tools/rva_resolve.py.
"""
import ctypes, sys, time
from ctypes import wintypes

TH32CS_SNAPTHREAD = 0x4
TH32CS_SNAPMODULE = 0x8
THREAD_SUSPEND_RESUME = 0x0002
THREAD_GET_CONTEXT = 0x0008
THREAD_QUERY_INFORMATION = 0x0040
CONTEXT_CONTROL = 0x00010001
PROCESS_ALL = 0x001FFFFF

pid = int(sys.argv[1])
seconds = int(sys.argv[2])
out = sys.argv[3] if len(sys.argv) > 3 else 'regress/eip_tally.txt'

kernel32 = ctypes.windll.kernel32
class MODULEENTRY32(ctypes.Structure):
    _fields_ = [('dwSize', wintypes.DWORD), ('th32ModuleID', wintypes.DWORD),
                ('th32ProcessID', wintypes.DWORD), ('GlblcntUsage', wintypes.DWORD),
                ('ProccntUsage', wintypes.DWORD), ('modBaseAddr', ctypes.POINTER(ctypes.c_byte)),
                ('modBaseSize', wintypes.DWORD), ('hModule', wintypes.HMODULE),
                ('szModule', ctypes.c_char * 256), ('szExePath', ctypes.c_char * 260)]
def module_base(pid):
    snap = kernel32.CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid)
    if snap == -1: return 0
    me = MODULEENTRY32(); me.dwSize = ctypes.sizeof(MODULEENTRY32)
    base = 0
    ok = kernel32.Module32First(snap, ctypes.byref(me))
    while ok:
        if b'lpcvm' in me.szModule:
            base = me.modBaseAddr and ctypes.cast(me.modBaseAddr, ctypes.c_void_p).value or 0
            break
        ok = kernel32.Module32Next(snap, ctypes.byref(me))
    kernel32.CloseHandle(snap)
    return base
class THREADENTRY32(ctypes.Structure):
    _fields_ = [('dwSize', wintypes.DWORD), ('cntUsage', wintypes.DWORD),
                ('th32ThreadID', wintypes.DWORD), ('th32OwnerProcessID', wintypes.DWORD),
                ('tpBasePri', wintypes.LONG), ('tpDeltaPri', wintypes.LONG),
                ('dwFlags', wintypes.DWORD)]
class CONTEXT(ctypes.Structure):
    _fields_ = [('ContextFlags', wintypes.DWORD),
                ('Dr0', wintypes.DWORD), ('Dr1', wintypes.DWORD),
                ('Dr2', wintypes.DWORD), ('Dr3', wintypes.DWORD),
                ('Dr6', wintypes.DWORD), ('Dr7', wintypes.DWORD),
                ('FloatSave', ctypes.c_byte * 112),
                ('SegGs', wintypes.DWORD), ('SegFs', wintypes.DWORD),
                ('SegEs', wintypes.DWORD), ('SegDs', wintypes.DWORD),
                ('Edi', wintypes.DWORD), ('Esi', wintypes.DWORD),
                ('Ebx', wintypes.DWORD), ('Edx', wintypes.DWORD),
                ('Ecx', wintypes.DWORD), ('Eax', wintypes.DWORD),
                ('Ebp', wintypes.DWORD), ('Eip', wintypes.DWORD),
                ('SegCs', wintypes.DWORD), ('EFlags', wintypes.DWORD),
                ('Esp', wintypes.DWORD), ('SegSs', wintypes.DWORD),
                ('ExtendedRegisters', ctypes.c_byte * 512)]

def threads_of(pid):
    snap = kernel32.CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0)
    if snap == -1: return []
    te = THREADENTRY32(); te.dwSize = ctypes.sizeof(THREADENTRY32)
    out = []
    ok = kernel32.Thread32First(snap, ctypes.byref(te))
    while ok:
        if te.th32OwnerProcessID == pid:
            out.append(te.th32ThreadID)
        ok = kernel32.Thread32Next(snap, ctypes.byref(te))
    kernel32.CloseHandle(snap)
    return out

hproc = kernel32.OpenProcess(PROCESS_ALL, False, pid)
if not hproc:
    print('cannot open process'); sys.exit(1)

use_wow64 = ctypes.sizeof(ctypes.c_void_p) == 8   # 64-bit python -> wow64 child
print('python64=%s' % use_wow64)

tids = threads_of(pid)
print('sampling pid=%d threads=%s for %ds' % (pid, tids, seconds), flush=True)
handles = {}
for t in tids:
    h = kernel32.OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, False, t)
    if h: handles[t] = h
from collections import Counter
tally = Counter()
stacks = Counter()
t0 = time.time()
n = 0
def in_range(x, base):
    return base and (base + 0x1000) <= x < (base + 0x2000000)
while time.time() - t0 < seconds:
    for t, h in handles.items():
        if kernel32.SuspendThread(h) == 0xFFFFFFFF: continue
        ctx = CONTEXT(); ctx.ContextFlags = CONTEXT_CONTROL
        ok = kernel32.Wow64GetThreadContext(h, ctypes.byref(ctx)) if use_wow64 \
             else kernel32.GetThreadContext(h, ctypes.byref(ctx))
        if ok:
            tally[ctx.Eip] += 1
            # capture a call stack every 100 samples (frame pointer walk)
            if n % 100 == 0 and ctx.Ebp:
                mb = module_base(pid)
                ebp = ctx.Ebp
                chain = []
                # read process memory for the frame chain
                buf = (ctypes.c_ubyte * 8)()
                nread = ctypes.c_size_t()
                for _ in range(20):
                    if kernel32.ReadProcessMemory(hproc, ctypes.c_void_p(ebp), buf, 8, ctypes.byref(nread)) != 1 or nread.value != 8:
                        break
                    nxt = int.from_bytes(bytes(buf[0:4]), 'little')
                    ret = int.from_bytes(bytes(buf[4:8]), 'little')
                    if not in_range(ret, mb):
                        break
                    chain.append(ret - mb)
                    if nxt <= ebp or nxt > ebp + 0x40000:
                        break
                    ebp = nxt
                if chain:
                    stacks[tuple(chain)] += 1
        kernel32.ResumeThread(h)
    n += 1
    time.sleep(0.002)
for h in handles.values():
    kernel32.CloseHandle(h)
kernel32.CloseHandle(hproc)
print('samples=%d distinct=%d stacks=%d' % (n, len(tally), len(stacks)), flush=True)
mb = module_base(pid)
with open(out, 'w') as f:
    f.write('base %x\n' % (mb or 0))
    for eip, cnt in tally.most_common():
        f.write('%08x %d\n' % (eip, cnt))
    f.write('--stacks--\n')
    for chain, cnt in stacks.most_common(120):
        f.write('%d ' % cnt + ' '.join('%08x' % r for r in chain) + '\n')
print('module base=%x written %s' % (mb, out), flush=True)
