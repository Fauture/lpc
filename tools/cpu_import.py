# -*- coding: utf-8 -*-
# Clean boot; sample gs CPU at 5s intervals for 30s during import phase.
import subprocess, time, os, sys, io
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
env = dict(os.environ)
for k in ['LPCVM_TRACE', 'LPCVM_INHINIT', 'LPCVM_EVENT_TRACE', 'LPCVM_TRACE_C8', 'LPCVM_NO_CONST',
          'LPCVM_LOCK_TRACE', 'LPCVM_STUCK_TRACE', 'LPCVM_EFUN_PROF', 'LPCVM_STEP_PROF',
          'LPCVM_CLONE_PROF', 'LPCVM_LOAD_PROF', 'LPCVM_SVC_TRACE', 'LPCVM_SLOW_OP']:
    env.pop(k, None)
exe = 'build/Debug/lpcvm.exe'
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    logf = open('cpu_%s.txt' % name, 'wb')
    p = subprocess.Popen([exe, 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=env)
    procs.append((name, p, logf))
    time.sleep(2)
pid = procs[3][1].pid
def cpus():
    return sum(int(t) for t in open('cpu_gs.txt', 'rb') and [0])  # unused
import ctypes
# use Get-Process via powershell is heavy; use psapi via ctypes
class FILETIME(ctypes.Structure):
    _fields_ = [("dwLow", ctypes.c_uint), ("dwHigh", ctypes.c_uint)]
def kern_ut():
    p = ctypes.windll.kernel32.OpenProcess(0x400, False, pid)
    a, b, c, d = FILETIME(), FILETIME(), FILETIME(), FILETIME()
    ctypes.windll.kernel32.GetProcessTimes(p, ctypes.byref(a), ctypes.byref(b), ctypes.byref(c), ctypes.byref(d))
    ctypes.windll.kernel32.CloseHandle(p)
    def f(x): return (x.dwHigh << 32) | x.dwLow
    return f(c) + f(d)  # kernel+user 100ns units
prev = kern_ut()
t0 = time.time()
last_load = ''
import re
while time.time() - t0 < 34:
    time.sleep(5)
    cur = kern_ut()
    cpu = (cur - prev) / 1e7 / 5.0
    prev = cur
    try:
        data = open('cpu_gs.txt', 'rb').read().decode('utf-8', errors='replace')
        m = re.findall(r'^Load: (\S+\.c)\.\.\. OK', data, re.M)
        if m: last_load = m[-1]
    except Exception:
        pass
    print('[%3.0fs] gs CPU=%4.2f core  last=%s' % (time.time() - t0, cpu, last_load), flush=True)
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()
