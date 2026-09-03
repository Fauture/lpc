# -*- coding: utf-8 -*-
"""Boot 4 services, wait until gs is inside the basic_objectd stall, EIP-sample
the gs process, then resolve against the map.  Usage: python tools/boot_eip.py
"""
import io, os, subprocess, sys, time
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
env = dict(os.environ)
for k in ['LPCVM_TRACE', 'LPCVM_INHINIT', 'LPCVM_EVENT_TRACE', 'LPCVM_TRACE_C8',
          'LPCVM_NO_CONST', 'LPCVM_LOCK_TRACE', 'LPCVM_STUCK_TRACE', 'LPCVM_EFUN_PROF',
          'LPCVM_STEP_PROF', 'LPCVM_SLOW_OP', 'LPCVM_CLONE_PROF']:
    env.pop(k, None)
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    logf = open('regress/eip_%s.txt' % name, 'wb')
    p = subprocess.Popen(['build/Debug/lpcvm.exe', 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=env)
    procs.append((name, p, logf))
    time.sleep(2)
pid = procs[3][1].pid
print('gs pid=%d, waiting 100s to reach the stall' % pid, flush=True)
time.sleep(100)
print('sampling 75s', flush=True)
r = subprocess.run([sys.executable, 'tools/eip_sampler.py', str(pid), '75', 'regress/eip_tally.txt'])
print('sample exit %d' % r.returncode, flush=True)
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()
print('done', flush=True)
