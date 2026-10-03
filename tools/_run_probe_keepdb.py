# -*- coding: utf-8 -*-
"""Boot the 4 services WITHOUT wiping the database.

_run_probe.py always runs _dbclear.py first (every boot starts from an empty
world).  This variant keeps the existing data so a restart does not destroy a
character, which is what you want when the only goal is to get a hung service
set back up.

usage: python -X utf8 _run_probe_keepdb.py <logdir> <seconds> [K=V;K=V;...]

Same probe SPEC syntax as _run_probe.py (';' separated, because several probe
VALUES are themselves comma lists).
"""
import io
import os
import shutil
import socket
import subprocess
import sys
import time

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
LOG_DIR = sys.argv[1] if len(sys.argv) > 1 else 'keep1'
MAXS = int(sys.argv[2]) if len(sys.argv) > 2 and sys.argv[2].isdigit() else 7200
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

print('[keepdb] NOT wiping the database', flush=True)
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    fn = os.path.join(LOG_DIR, 'live_%s.txt' % name)
    f = open(fn, 'wb', buffering=0)
    p = subprocess.Popen(['build/Release/lpcvm.exe', 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=f, stderr=subprocess.STDOUT, env=env, cwd=os.getcwd())
    procs.append((name, p, f))
    time.sleep(3)


def up8160():
    s = socket.socket(); s.settimeout(1.0)
    try:
        s.connect(('192.168.200.1', 8160)); s.close(); return True
    except Exception:
        return False


start = time.time()
opened = None
while True:
    time.sleep(10)
    el = int(time.time() - start)
    up = up8160()
    if up and opened is None:
        opened = el
        print('[%5ds] *** 8160 OPEN (startup=%ds) ***' % (el, el), flush=True)
    if el % 60 == 0 or up:
        print('[%5ds] 8160=%s' % (el, up), flush=True)
    if el >= MAXS:
        print('[done] opened_at=%s' % opened, flush=True)
        break
