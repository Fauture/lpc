# -*- coding: utf-8 -*-
"""Run all 4 services, sample GS memory + which Load phase it's in."""
import subprocess, sys, time, os, io, re, ctypes
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

env = dict(os.environ)
env['LPCVM_TRACE'] = '1'
env['LPCVM_INHINIT'] = '1'
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    logf = open('logs_%sAX_err.txt' % name, 'wb')
    p = subprocess.Popen(['build/Release/lpcvm.exe', 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=env)
    procs.append((name, p, logf))
    time.sleep(3)

class PMC(ctypes.Structure):
    _fields_ = [('cb', ctypes.c_ulong), ('PageFaultCount', ctypes.c_ulong),
                ('PeakWorkingSetSize', ctypes.c_size_t), ('WorkingSetSize', ctypes.c_size_t),
                ('QuotaPeakPagedPoolUsage', ctypes.c_size_t), ('QuotaPagedPoolUsage', ctypes.c_size_t),
                ('QuotaPeakNonPagedPoolUsage', ctypes.c_size_t), ('QuotaNonPagedPoolUsage', ctypes.c_size_t),
                ('PagefileUsage', ctypes.c_size_t), ('PeakPagefileUsage', ctypes.c_size_t)]

def ws_of(pid):
    h = ctypes.windll.kernel32.OpenProcess(0x1000, False, pid)
    if not h: return -1
    pmc = PMC()
    ctypes.windll.psapi.GetProcessMemoryInfo(h, ctypes.byref(pmc), ctypes.sizeof(pmc))
    ctypes.windll.kernel32.CloseHandle(h)
    return pmc.WorkingSetSize / (1024*1024)

def log_tail_lines(name):
    try:
        with open('logs_%sAX_err.txt' % name, 'rb') as f:
            data = f.read()
        text = data.decode('utf-8', errors='replace')
        # last Load line
        loads = re.findall(r'Load: ([^\s]+\.c)\.\.\. OK', text)
        return len(loads), (loads[-1] if loads else '')
    except Exception as e:
        return -1, str(e)

prev_gs = 0
for i in range(100):
    time.sleep(3)
    gs = ws_of(procs[3][1].pid)
    n, last = log_tail_lines('gs')
    print('[%4ds] gs_WS=%.0fMB d_gs=%+d loads=%d last=%s' % (i*3, gs, gs - prev_gs, n, last[:70]), flush=True)
    prev_gs = gs
    if procs[3][1].poll() is not None:
        print('gs exited', procs[3][1].poll(), flush=True)
        break
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()
