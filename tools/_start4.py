# -*- coding: utf-8 -*-
"""One-shot: wipe processes+db, boot all 4 services, exit immediately.
No monitoring loop — the script returns in seconds so the session never
holds a long-lived background task (abort kills the whole process tree).

usage: python -X utf8 tools/_start4.py <tmp_logdir>
"""
import os
import subprocess, sys, io, shutil, time
from _svc_args import svc_args  # original-style driver command line per service

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
LOG_DIR = sys.argv[1]
if not LOG_DIR.replace('\\', '/').startswith('tmp/'):
    print('!! log dir must live under tmp/'); sys.exit(2)
shutil.rmtree(LOG_DIR, ignore_errors=True)
os.makedirs(LOG_DIR, exist_ok=True)

subprocess.run(['powershell', '-NoProfile', '-Command',
                'Get-Process lpcvm -ErrorAction SilentlyContinue | Stop-Process -Force'],
               capture_output=True, text=True)
time.sleep(2)
subprocess.run([sys.executable,
                os.path.join(os.path.dirname(os.path.abspath(__file__)), '_dbclear.py')],
               check=True, capture_output=True, timeout=180)

env = dict(os.environ)
for k in [k for k in list(env) if k.startswith('LPCVM_')]:
    env.pop(k, None)
env['LPCVM_TRACE'] = '0'

_here = os.path.dirname(os.path.abspath(__file__))
_exe = os.path.abspath(os.path.join(_here, '..', 'build', 'Release', 'lpcvm.exe'))
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    fn = os.path.join(LOG_DIR, 'live_%s.txt' % name)
    f = open(fn, 'wb', buffering=0)
    p = subprocess.Popen([_exe] + svc_args(name),
                         stdout=f, stderr=subprocess.STDOUT, env=env, cwd=os.getcwd())
    procs.append((name, p.pid, fn))
    time.sleep(3)

for name, pid, fn in procs:
    print('[booted] %-4s pid=%-6d log=%s' % (name, pid, os.path.abspath(fn)))
print('[done] all 4 services launched; ports 8001/8110/8120/8160 when booted')
