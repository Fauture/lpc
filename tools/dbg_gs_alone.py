# -*- coding: utf-8 -*-
# Run ONLY gs with LPCVM_TRACE=1 and LPCVM_STUCK_TRACE=1 so the scheduler
# prints [sched-*]/[idle-wait]/[vm-stuck] lines.  Kill after WINDOW seconds.
import subprocess, time, os, sys, io
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
WINDOW = int(sys.argv[1]) if len(sys.argv) > 1 else 300
env = dict(os.environ)
env['LPCVM_TRACE'] = '1'
env['LPCVM_STUCK_TRACE'] = '1'
for k in ['LPCVM_INHINIT', 'LPCVM_EVENT_TRACE', 'LPCVM_TRACE_C8', 'LPCVM_NO_CONST', 'LPCVM_LOCK_TRACE']:
    env.pop(k, None)
exe = 'build/Debug/lpcvm.exe'
logf = open('dbg_gs_alone.txt', 'wb')
p = subprocess.Popen([exe, 'run', 'linux/gs/pack_data/lib_gs32.pak'],
                     stdout=logf, stderr=logf, env=env)
print('gs started alone (%ds)' % WINDOW, flush=True)
start = time.time()
while time.time() - start < WINDOW:
    time.sleep(10)
    print('[%4ds] gs=%0.1fMB' % (int(time.time() - start),
          os.path.getsize('dbg_gs_alone.txt') / 1048576.0), flush=True)
if p.poll() is None:
    p.kill()
logf.close()
print('killed', flush=True)
