# -*- coding: utf-8 -*-
import subprocess, time, os, sys, io
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
env = dict(os.environ)
env['LPCVM_TRACE'] = '1'
env['LPCVM_INHINIT'] = '1'
if 'C8' in sys.argv:
    env['LPCVM_TRACE_C8'] = '1'
exe = 'build/Debug/lpcvm.exe'
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    logf = open('logs4_%s.txt' % name, 'wb')
    p = subprocess.Popen([exe, 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=env)
    procs.append((name, p, logf))
    time.sleep(3)
print('started 4 services (debug)', flush=True)
# wait for ccs 8110 and print progress
import socket
def portup(port):
    s = socket.socket(); s.settimeout(0.4)
    try:
        s.connect(('127.0.0.1', port)); s.close(); return True
    except Exception:
        return False
for i in range(200):
    time.sleep(5)
    st = ' '.join('%s=%s' % (n, 'UP' if portup(p) else '-') for n, p in
                  [('aaa', 8001), ('dba', 8120), ('ccs', 8110), ('gs', 8160)])
    print('[%4ds] %s' % (int(time.time() % 100000), st), flush=True)
    if portup(8160):
        print('GS 8160 UP', flush=True)
        break
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()
