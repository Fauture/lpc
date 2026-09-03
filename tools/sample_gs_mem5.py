# -*- coding: utf-8 -*-
"""Run 4 services, sample GS memory + correlate with post_init object + heap."""
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
    if not h: return -1, -1
    pmc = PMC()
    ctypes.windll.psapi.GetProcessMemoryInfo(h, ctypes.byref(pmc), ctypes.sizeof(pmc))
    ctypes.windll.kernel32.CloseHandle(h)
    return pmc.WorkingSetSize / (1024*1024), pmc.PagefileUsage / (1024*1024)

def gs_state():
    try:
        with open('logs_gsAX_err.txt', 'rb') as f:
            data = f.read()
        text = data.decode('utf-8', errors='replace')
        # last postinit-sync object
        pi = [m for m in re.finditer(r'\[postinit-sync\] (\S+?) on (\S+)', text)]
        last_pi = pi[-1].groups() if pi else ('', '')
        # last Load
        loads = re.findall(r'Load: ([^\s]+\.c)\.\.\. OK', text)
        # count of each
        zone = len(re.findall(r'Load: /gs/zone/', text))
        tops = len(set(re.findall(r'Load: (/gs/daemons/tasks/[^/]+?\.c)\.\.\. OK', text)))
        return len(loads), zone, tops, last_pi, len(text)
    except Exception as e:
        return -1, -1, -1, ('err', str(e)), -1

prev = 0
for i in range(150):
    time.sleep(4)
    ws, priv = ws_of(procs[3][1].pid)
    nl, nz, nt, pi, nbytes = gs_state()
    print('[%4ds] WS=%.0fMB Priv=%.0fMB d=%.0f loads=%d zone=%d tasksTop=%d postinit=%s(%s) log=%.1fMB' % (
        i*4, ws, priv, ws-prev, nl, nz, nt, pi[0], pi[1][:40], nbytes/1048576), flush=True)
    prev = ws
    if procs[3][1].poll() is not None:
        print('gs exited', procs[3][1].poll(), flush=True)
        break
for name, p, logf in procs:
    if p.poll() is None: p.kill()
    logf.close()
