# -*- coding: utf-8 -*-
"""Boot 4 services with LPCVM_SLOW_OP=1; every 5s print gs process CPU% and
the gs log tail, so a stall window can be correlated with CPU + slow-op lines.
Usage: python tools/boot_cpu_slow.py [window_s]
"""
import ctypes, io, os, re, subprocess, sys, time
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
WINDOW = int(sys.argv[1]) if len(sys.argv) > 1 else 300
OUT = 'regress'
env = dict(os.environ)
for k in ['LPCVM_TRACE', 'LPCVM_INHINIT', 'LPCVM_EVENT_TRACE', 'LPCVM_TRACE_C8',
          'LPCVM_NO_CONST', 'LPCVM_LOCK_TRACE', 'LPCVM_STUCK_TRACE', 'LPCVM_EFUN_PROF',
          'LPCVM_STEP_PROF']:
    env.pop(k, None)
env['LPCVM_SLOW_OP'] = '1'
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    logf = open(os.path.join(OUT, 'cpu_%s.txt' % name), 'wb')
    p = subprocess.Popen(['build/Debug/lpcvm.exe', 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=env)
    procs.append((name, p, logf))
    time.sleep(2)
pid = procs[3][1].pid

class FILETIME(ctypes.Structure):
    _fields_ = [('dwLow', ctypes.c_uint), ('dwHigh', ctypes.c_uint)]
def kern_ut(handle):
    a, b, c, d = FILETIME(), FILETIME(), FILETIME(), FILETIME()
    ctypes.windll.kernel32.GetProcessTimes(handle, ctypes.byref(a), ctypes.byref(b),
                                           ctypes.byref(c), ctypes.byref(d))
    def f(x): return (x.dwHigh << 32) | x.dwLow
    return f(c) + f(d)
h = ctypes.windll.kernel32.OpenProcess(0x400, False, pid)
prev = kern_ut(h)
t0 = time.time()
loads_seen = {}
print('started pid=%d window=%d' % (pid, WINDOW), flush=True)
while time.time() - t0 < WINDOW:
    time.sleep(5)
    el = time.time() - t0
    cur = kern_ut(h)
    cpu = (cur - prev) / 1e7 / 5.0
    prev = cur
    path = os.path.join(OUT, 'cpu_gs.txt')
    txt = ''
    try:
        with open(path, 'rb') as f:
            txt = f.read().decode('utf-8', errors='replace')
    except Exception:
        pass
    last = ''
    m = re.findall(r'^Load: (\S+\.c)\.\.\. OK', txt, re.M)
    if m: last = m[-1]
    nslow = txt.count('[slow-op]')
    tail = txt.splitlines()
    slowline = ''
    for ln in reversed(tail):
        if '[slow-op]' in ln:
            slowline = ln.strip()
            break
    print('[%4ds] gsCPU=%4.0f%% loads=%d last=%s slowOps=%d %s'
          % (int(el), cpu * 100, len(m), last.rsplit('/', 1)[-1], nslow,
             slowline[:150]), flush=True)
ctypes.windll.kernel32.CloseHandle(h)
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()
print('done', flush=True)
