# -*- coding: utf-8 -*-
import subprocess, time, os, sys, io, socket
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
env = dict(os.environ)
env['LPCVM_TRACE'] = '1'
env['LPCVM_INHINIT'] = '1'
if 'NO_CONST' in sys.argv:
    env['LPCVM_NO_CONST'] = '1'
exe = 'build/Debug/lpcvm.exe'
procs = []
for name in ['aaa', 'dba', 'gs']:   # no ccs: testing apexd storm reachability
    logf = open('logsAB_%s.txt' % name, 'wb')
    p = subprocess.Popen([exe, 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=env)
    procs.append((name, p, logf))
    time.sleep(3)
print('started aaa dba gs', flush=True)
def portup(port):
    s = socket.socket(); s.settimeout(0.4)
    try:
        s.connect(('127.0.0.1', port)); s.close(); return True
    except Exception:
        return False
for i in range(300):
    time.sleep(10)
    st = 'aaa=%s dba=%s' % ('UP' if portup(8001) else '-', 'UP' if portup(8120) else '-')
    gs_size = os.path.getsize('logsAB_gs.txt')
    print('[%4ds] %s gslog=%.0fMB' % (int(time.time() % 100000), st, gs_size / 1048576), flush=True)
    # detect destruct storm
    with open('logsAB_gs.txt', 'rb') as f:
        f.seek(max(0, gs_size - 200000))
        tail = f.read().decode('utf-8', errors='replace')
    if tail.count('in=destruct obj=/gs/daemons/apexd.o') > 500:
        print('APEXD DESTRUCT STORM DETECTED', flush=True)
        break
    if portup(8160):
        print('GS 8160 UP', flush=True)
        break
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()
