# -*- coding: utf-8 -*-
"""Launch gs only (aaa/dba/ccs already up). Log into the given tmp/ dir.
usage: python -X utf8 tools/_start_gs.py <tmp_logdir> <seconds>
"""
import os
import subprocess, time, sys, io, socket
from _svc_args import svc_args  # original-style driver command line per service

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
LOG_DIR = sys.argv[1]
MAXS = int(sys.argv[2]) if len(sys.argv) > 2 and sys.argv[2].isdigit() else 600

env = dict(os.environ)
for k in [k for k in list(env) if k.startswith('LPCVM_')]:
    if k not in ('LPCVM_TRACE', 'LPCVM_TRACE_FN', 'LPCVM_TRACE_OPS', 'LPCVM_CTRACE'):
        env.pop(k, None)
if 'LPCVM_TRACE' not in env:
    env['LPCVM_TRACE'] = '1'
print('gs trace: LPCVM_TRACE=%s FN=%s' % (
    env.get('LPCVM_TRACE'), env.get('LPCVM_TRACE_FN', '<all>')), flush=True)

fn = os.path.join(LOG_DIR, 'live_gs.txt')
f = open(fn, 'wb', buffering=0)
exe_arg = os.environ.get('LPCVM_EXE')
if exe_arg:
    _exe = os.path.abspath(exe_arg)
else:
    _exe = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                        '..', 'build', 'Release', 'lpcvm.exe'))
# break away from the shell Job so gs survives task/session teardown
p = subprocess.Popen([_exe] + svc_args('gs'),
                     stdout=f, stderr=subprocess.STDOUT, env=env, cwd=os.getcwd())
print('[start] gs pid=%d -> %s' % (p.pid, fn), flush=True)


def up8160():
    s = socket.socket(); s.settimeout(1.0)
    try:
        s.connect(('192.168.200.1', 8160)); s.close(); return True
    except Exception:
        return False


start = time.time()
opened = None
while True:
    time.sleep(15)
    el = int(time.time() - start)
    up = up8160()
    if up and opened is None:
        opened = el
        print('[%5ds] *** 8160 OPEN (gs startup=%ds) ***' % (el, el), flush=True)
    try:
        kb = os.path.getsize(fn) // 1024
    except OSError:
        kb = -1
    print('[%5ds] 8160=%s gs_log=%dKB' % (el, up, kb), flush=True)
    if p.poll() is not None:
        print('[!!] gs EXITED rc=%s' % p.returncode, flush=True)
    if el >= MAXS:
        print('[done] opened_at=%s' % opened, flush=True)
        break
