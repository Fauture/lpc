# -*- coding: utf-8 -*-
# 20s boot with gs per-efun profiler (LPCVM_EFUN_PROF=1).
import subprocess, time, os, sys, io, socket, re
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
env = dict(os.environ)
for k in ['LPCVM_TRACE', 'LPCVM_INHINIT', 'LPCVM_EVENT_TRACE', 'LPCVM_TRACE_C8', 'LPCVM_NO_CONST',
          'LPCVM_LOCK_TRACE', 'LPCVM_STUCK_TRACE', 'LPCVM_EFUN_PROF', 'LPCVM_STEP_PROF',
          'LPCVM_CLONE_PROF', 'LPCVM_LOAD_PROF', 'LPCVM_SVC_TRACE']:
    env.pop(k, None)
exe = 'build/Debug/lpcvm.exe'
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    logf = open('prof_%s.txt' % name, 'wb')
    e = dict(env)
    if name == 'gs':
        e['LPCVM_EFUN_PROF'] = '1'
    p = subprocess.Popen([exe, 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=e)
    procs.append((name, p, logf))
    time.sleep(2)
t0 = time.time()
print('started', flush=True)
while time.time() - t0 < 22:
    time.sleep(5)
    print('[%ds]' % int(time.time() - t0), flush=True)
print('killing', flush=True)
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()


