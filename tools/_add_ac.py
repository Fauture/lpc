# -*- coding: utf-8 -*-
"""补起 aaa/ccs（不清库、不动 gs/dba）。日志目录必须是 tmp/ 下全新目录。"""
import os, subprocess, sys, io
from _svc_args import svc_args  # original-style driver command line per service

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
LOG_DIR = sys.argv[1]
if not LOG_DIR.replace('\\', '/').startswith('tmp/'):
    print('!! log dir must live under tmp/'); sys.exit(2)
os.makedirs(LOG_DIR, exist_ok=True)

env = dict(os.environ)
for k in [k for k in list(env) if k.startswith('LPCVM_')]:
    env.pop(k, None)
env['LPCVM_TRACE'] = '0'

exe = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                   '..', 'build', 'Release', 'lpcvm.exe'))
for name in ('aaa', 'ccs'):
    f = open(os.path.join(LOG_DIR, 'live_%s.txt' % name), 'wb', buffering=0)
    subprocess.Popen([exe] + svc_args(name),
                     stdout=f, stderr=subprocess.STDOUT, env=env, cwd=os.getcwd())
    print('started', name, flush=True)
