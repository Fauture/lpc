# -*- coding: utf-8 -*-
import subprocess, time, os, sys, io, socket
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
env = dict(os.environ)
env['LPCVM_TRACE'] = '1'
env['LPCVM_INHINIT'] = '1'
exe = 'build/Debug/lpcvm.exe'
procs = []
for name in ['aaa', 'dba', 'ccs']:
    logf = open('logsAC_%s.txt' % name, 'wb')
    p = subprocess.Popen([exe, 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=env)
    procs.append((name, p, logf))
    time.sleep(3)
print('started aaa dba ccs', flush=True)
def portup(port):
    s = socket.socket(); s.settimeout(0.4)
    try:
        s.connect(('127.0.0.1', port)); s.close(); return True
    except Exception:
        return False
for i in range(240):
    time.sleep(10)
    print('[%4ds] aaa=%s dba=%s ccs8110=%s' % (
        int(time.time() % 100000),
        'UP' if portup(8001) else '-',
        'UP' if portup(8120) else '-',
        'UP' if portup(8110) else '-'), flush=True)
    if portup(8110):
        print('CCS BOUND 8110', flush=True)
        break
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()
