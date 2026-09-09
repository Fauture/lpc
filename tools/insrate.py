# -*- coding: utf-8 -*-
import subprocess, time, os, sys, io, re
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
env = dict(os.environ)
for k in ['LPCVM_TRACE','LPCVM_INHINIT','LPCVM_EVENT_TRACE','LPCVM_TRACE_C8','LPCVM_NO_CONST',
          'LPCVM_LOCK_TRACE','LPCVM_STUCK_TRACE','LPCVM_EFUN_PROF','LPCVM_STEP_PROF',
          'LPCVM_CLONE_PROF','LPCVM_LOAD_PROF','LPCVM_SVC_TRACE','LPCVM_SLOW_OP']:
    env.pop(k, None)
exe = 'build/Debug/lpcvm.exe'
procs = []
for name in ['aaa','dba','ccs','gs']:
    logf = open('ins_%s.txt' % name, 'wb')
    e = dict(env)
    if name == 'gs':
        e['LPCVM_STEP_PROF'] = '1'
    p = subprocess.Popen([exe, 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=e)
    procs.append((name, p, logf))
    time.sleep(2)
time.sleep(14)
# read step-prof cumulative insns
d = open('ins_gs.txt', encoding='utf-8', errors='replace').read().splitlines()
sp = [l for l in d if 'step-prof' in l]
m = [int(re.search(r'insns=(\d+)', l).group(1)) for l in sp]
print('step-prof samples:', len(sp), 'final insns:', m[-1] if m else 'none')
if len(m) > 1:
    dt = 12.0
    print('insns/s ~', int((m[-1]-m[0])/dt))
# and current loads/last
lo = len(re.findall(r'^Load: ', '\n'.join(d), re.M))
last = re.findall(r'^Load: (\S+\.c)\.\.\. OK', '\n'.join(d), re.M)
print('loads=%d last=%s' % (lo, last[-1] if last else '?'))
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()
