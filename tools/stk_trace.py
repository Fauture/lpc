# -*- coding: utf-8 -*-
# Boot gs with SVC+EVENT+STUCK trace, run SHORT, dump the stall location.
import subprocess, time, os, sys, io, socket, re
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
WINDOW = int(sys.argv[1]) if len(sys.argv) > 1 else 22
env = dict(os.environ)
for k in ['LPCVM_TRACE', 'LPCVM_INHINIT', 'LPCVM_EVENT_TRACE', 'LPCVM_TRACE_C8', 'LPCVM_NO_CONST',
          'LPCVM_LOCK_TRACE', 'LPCVM_STUCK_TRACE', 'LPCVM_EFUN_PROF', 'LPCVM_STEP_PROF',
          'LPCVM_CLONE_PROF', 'LPCVM_LOAD_PROF', 'LPCVM_SVC_TRACE']:
    env.pop(k, None)
exe = 'build/Debug/lpcvm.exe'
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    logf = open('stk_%s.txt' % name, 'wb')
    e = dict(env)
    if name == 'gs':
        e['LPCVM_SVC_TRACE'] = '1'
        e['LPCVM_EVENT_TRACE'] = '1'
        e['LPCVM_STUCK_TRACE'] = '1'
    p = subprocess.Popen([exe, 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=e)
    procs.append((name, p, logf))
    time.sleep(2)
t0 = time.time()
print('started stuck-trace', flush=True)
last = 0
while time.time() - t0 < WINDOW:
    time.sleep(5)
    sz = os.path.getsize('stk_gs.txt') if os.path.exists('stk_gs.txt') else 0
    print('[%4ds] gs=%.2fMB' % (time.time() - t0, sz / 1048576.0), flush=True)
print('done killing', flush=True)
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()
