# -*- coding: utf-8 -*-
"""Long gs boot watcher: every 5s prints gs CPU%, Load counts, last Load line,
count of FAILED loads, and the tail (repeated lines).  Window s via argv.
Usage: python tools/boot_watch_gs.py [window_s]
"""
import io, os, re, subprocess, sys, time, ctypes
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
WINDOW = int(sys.argv[1]) if len(sys.argv) > 1 else 600
env = dict(os.environ)
for k in ['LPCVM_TRACE', 'LPCVM_INHINIT', 'LPCVM_EVENT_TRACE', 'LPCVM_TRACE_C8',
          'LPCVM_NO_CONST', 'LPCVM_LOCK_TRACE', 'LPCVM_STUCK_TRACE', 'LPCVM_EFUN_PROF',
          'LPCVM_STEP_PROF', 'LPCVM_SLOW_OP', 'LPCVM_CLONE_PROF', 'LPCVM_FMT_CHECK',
          'LPCVM_OP_PROF', 'LPCVM_REG_PROF']:
    env.pop(k, None)
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    logf = open('regress/watch_%s.txt' % name, 'wb')
    p = subprocess.Popen(['build/Debug/lpcvm.exe', 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=env)
    procs.append((name, p, logf))
    time.sleep(2)
pid = procs[3][1].pid
class FILETIME(ctypes.Structure):
    _fields_ = [('dwLow', ctypes.c_uint), ('dwHigh', ctypes.c_uint)]
def cpu_of(handle):
    a, b, c, d = FILETIME(), FILETIME(), FILETIME(), FILETIME()
    ctypes.windll.kernel32.GetProcessTimes(handle, ctypes.byref(a), ctypes.byref(b), ctypes.byref(c), ctypes.byref(d))
    def f(x): return (x.dwHigh << 32) | x.dwLow
    return f(c) + f(d)
h = ctypes.windll.kernel32.OpenProcess(0x400, False, pid)
prev = cpu_of(h)
t0 = time.time()
print('started pid=%d window=%d' % (pid, WINDOW), flush=True)
prev_load = ''
freeze_start = None
while time.time() - t0 < WINDOW:
    time.sleep(5)
    el = time.time() - t0
    cur = cpu_of(h)
    cpu = (cur - prev) / 1e7 / 5.0
    prev = cur
    txt = ''
    try:
        txt = open('regress/watch_gs.txt', 'rb').read().decode('utf-8', errors='replace')
    except Exception:
        pass
    m = re.findall(r'^Load: (\S+\.c)\.\.\. OK', txt, re.M)
    cur_load = m[-1] if m else ''
    nfail = len(re.findall(r'FAILED', txt))
    if cur_load != prev_load:
        if freeze_start is not None:
            print('  ...froze at %s for %.0fs' % (prev_load.rsplit('/', 1)[-1], el - freeze_start), flush=True)
        freeze_start = None
        prev_load = cur_load
    elif freeze_start is None:
        freeze_start = el
    lines = txt.splitlines()
    tail = lines[-1].strip() if lines else ''
    print('[%4ds] cpu=%3.0f%% loads=%4d last=%s fails=%d tail=%s'
          % (int(el), cpu * 100, len(m), cur_load.rsplit('/', 1)[-1], nfail,
             tail[:90]), flush=True)
print('window over', flush=True)
ctypes.windll.kernel32.CloseHandle(h)
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()
print('done', flush=True)
