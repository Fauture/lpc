# -*- coding: utf-8 -*-
"""Boot the 4 services with an explicit LPCVM_* probe set.

usage: python -X utf8 _run_probe.py <logdir> <seconds> [KEY=VAL;KEY=VAL;...]

NOTE: probe entries are separated by ';' (NOT ','), because several probe
VALUES are themselves comma lists (LPCVM_FNTRACE=a,b,c / LPCVM_FNOBJ=x,y).
An earlier ',' split silently truncated those values to the first name, which
made whole probe sets look like they never fired.

Unlike _run_keep2.py (which unconditionally forces LPCVM_TAIL + LPCVM_MEM2)
this applies exactly the probes given, so the cost of each diagnostic switch
can be measured independently.

  no probes   -> the game's own default (no TAIL, no MEM2)
  TAIL=1      -> the tail/forensic probe set
  MEM2=1      -> the per-file [heap] census + [filed-rtt]/[pkt-filed] traffic
"""
import os
import subprocess, time, os, sys, io, shutil, socket

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
LOG_DIR = sys.argv[1] if len(sys.argv) > 1 else 'probe1'
MAXS = int(sys.argv[2]) if len(sys.argv) > 2 and sys.argv[2].isdigit() else 900
SPEC = sys.argv[3] if len(sys.argv) > 3 else ''
SEP = ';' if ';' in SPEC else ','
shutil.rmtree(LOG_DIR, ignore_errors=True)
os.makedirs(LOG_DIR, exist_ok=True)

env = dict(os.environ)
for k in [k for k in list(env) if k.startswith('LPCVM_')]:
    env.pop(k, None)
for kv in [x.strip() for x in SPEC.split(SEP) if x.strip()]:
    if '=' in kv:
        k, v = kv.split('=', 1)
        env[k.strip()] = v
    else:
        env[kv] = '1'
print('probes: %s' % (SPEC or '<none>'), flush=True)

# Single-instance guard.  Two service sets share ports 8001/8110/8120/8160 AND
# the same MySQL database, and each boot starts with a DB wipe - so a leftover
# set silently corrupts the other one's state (a character's temp dbase can be
# cleared underneath it, menus read back empty, etc.).  Refuse to start while
# anything still holds those ports.
PORTS = (8001, 8110, 8120, 8160)


def port_busy(port):
    s = socket.socket(); s.settimeout(0.6)
    try:
        s.connect(('192.168.200.1', port)); s.close(); return True
    except Exception:
        return False


def kill_existing():
    subprocess.run(['powershell', '-NoProfile', '-Command',
                    'Get-Process lpcvm -ErrorAction SilentlyContinue | Stop-Process -Force'],
                   capture_output=True, text=True)
    time.sleep(3)


kill_existing()
busy = [p for p in PORTS if port_busy(p)]
if busy:
    print('!! ports still held by a foreign process: %s' % busy, flush=True)
    print('!! refusing to start a second instance (it would share the DB).', flush=True)
    sys.exit(3)

subprocess.run([sys.executable, '_dbclear.py'], check=True, capture_output=True, timeout=180)
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    fn = os.path.join(LOG_DIR, 'live_%s.txt' % name)
    f = open(fn, 'wb', buffering=0)
    _exe = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'build', 'Release', 'lpcvm.exe')
    p = subprocess.Popen([_exe, 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=f, stderr=subprocess.STDOUT, env=env, cwd=os.getcwd())
    procs.append((name, p, f))
    time.sleep(3)


def up8160():
    s = socket.socket(); s.settimeout(1.0)
    try:
        s.connect(('192.168.200.1', 8160)); s.close(); return True
    except Exception:
        return False


def ws_of(p):
    try:
        out = subprocess.run(['powershell', '-NoProfile', '-Command',
                              "(Get-Process -Id %d).WorkingSet64" % p.pid],
                             capture_output=True, text=True, timeout=20)
        return int(out.stdout.strip()) / 1048576.0
    except Exception:
        return 0.0


start = time.time()
opened = None
peak = 0.0
while True:
    time.sleep(10)
    el = int(time.time() - start)
    up = up8160()
    if up and opened is None:
        opened = el
        print('[%5ds] *** 8160 OPEN (startup=%ds) probes=%s ***' % (el, el, SPEC or 'none'), flush=True)
    gs = [t for t in procs if t[0] == 'gs'][0]
    ws = ws_of(gs[1])
    peak = max(peak, ws)
    if el % 30 == 0 or up:
        print('[%5ds] 8160=%s gs_ws=%.0fMB' % (el, up, ws), flush=True)
    if el >= MAXS:
        print('[done] opened_at=%s peak=%.0fMB probes=%s' % (opened, peak, SPEC or 'none'), flush=True)
        break
