# -*- coding: utf-8 -*-
"""Boot aaa+dba+ccs with the default FULL trace set (trace.h: instruction-level
+ semantic-level, all on by default).  Log dir must be a FRESH unique tmp/ path.

usage: python -X utf8 tools/_run_alltrace.py <tmp_logdir> <seconds>
"""
import os
import subprocess, time, sys, io, shutil, socket

# Windows: detach from the launching shell's Job Object — when a background
# task ends, its Job kills the whole process tree (services died at 23:01/23:03
# exactly when their monitor tasks completed).  CREATE_BREAKAWAY_FROM_JOB +
# DETACHED_PROCESS makes lpcvm survive task/session teardown.
_BREAKAWAY = 0x01000000          # CREATE_BREAKAWAY_FROM_JOB
_DETACHED = 0x00000008           # DETACHED_PROCESS
_CREATE_FLAGS = _BREAKAWAY | _DETACHED

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
LOG_DIR = sys.argv[1]
MAXS = int(sys.argv[2]) if len(sys.argv) > 2 and sys.argv[2].isdigit() else 600

if not LOG_DIR.replace('\\', '/').startswith('tmp/'):
    print('!! log dir must live under tmp/'); sys.exit(2)
shutil.rmtree(LOG_DIR, ignore_errors=True)
os.makedirs(LOG_DIR, exist_ok=True)

env = dict(os.environ)
for k in [k for k in list(env) if k.startswith('LPCVM_')]:
    env.pop(k, None)
# trace level: default = semantic-only (fast boot); pass e.g. "2" for full
if 'LPCVM_TRACE' not in env:
    env['LPCVM_TRACE'] = '1'
print('probes: LPCVM_TRACE=%s (semantic-level)' % env['LPCVM_TRACE'], flush=True)

PORTS = (8001, 8110, 8120, 8160)


def port_busy(port):
    s = socket.socket(); s.settimeout(0.6)
    try:
        s.connect(('192.168.200.1', port)); s.close(); return True
    except Exception:
        return False


subprocess.run(['powershell', '-NoProfile', '-Command',
                'Get-Process lpcvm -ErrorAction SilentlyContinue | Stop-Process -Force'],
               capture_output=True, text=True)
time.sleep(3)
busy = [p for p in PORTS if port_busy(p)]
if busy:
    print('!! ports still held: %s' % busy, flush=True)
    sys.exit(3)

subprocess.run([sys.executable, 'tools/_dbclear.py'], check=True, capture_output=True,
               timeout=180)

procs = []
for name in ['aaa', 'dba', 'ccs']:   # gs joins later, after ccs boots
    fn = os.path.join(LOG_DIR, 'live_%s.txt' % name)
    f = open(fn, 'wb', buffering=0)
    _exe = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                        '..', 'build', 'Release', 'lpcvm.exe')
    p = subprocess.Popen([os.path.abspath(_exe), 'run',
                          'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=f, stderr=subprocess.STDOUT, env=env, cwd=os.getcwd(),
                         creationflags=_CREATE_FLAGS)
    procs.append((name, p, f))
    print('[start] %s pid=%d -> %s' % (name, p.pid, fn), flush=True)
    time.sleep(3)


def ports_up():
    res = {}
    for p in (8001, 8110, 8120):
        s = socket.socket(); s.settimeout(0.8)
        try:
            s.connect(('192.168.200.1', p)); s.close(); res[p] = True
        except Exception:
            res[p] = False
    return res


start = time.time()
while True:
    time.sleep(15)
    el = int(time.time() - start)
    st = ports_up()
    sizes = {n: os.path.getsize(os.path.join(LOG_DIR, 'live_%s.txt' % n)) // 1024
             for n, _, _ in procs}
    print('[%5ds] 8001=%s 8110=%s 8120=%s KB=%s' %
          (el, st[8001], st[8110], st[8120], sizes), flush=True)
    for name, p, _ in procs:
        if p.poll() is not None:
            print('[!!] %s EXITED rc=%s' % (name, p.returncode), flush=True)
    if el >= MAXS:
        print('[done]', flush=True)
        break
