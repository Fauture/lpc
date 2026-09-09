# -*- coding: utf-8 -*-
# Boot aaa dba ccs gs with LPCVM_STUCK_TRACE and NO auto-kill until WINDOW.
# Prints service state so an external sampler can attach at the freeze.
import subprocess, time, os, sys, io, socket
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
WINDOW = int(sys.argv[1]) if len(sys.argv) > 1 else 900
env = dict(os.environ)
env['LPCVM_STUCK_TRACE'] = '1'
for k in ['LPCVM_TRACE', 'LPCVM_INHINIT', 'LPCVM_EVENT_TRACE', 'LPCVM_TRACE_C8', 'LPCVM_NO_CONST', 'LPCVM_LOCK_TRACE']:
    env.pop(k, None)
exe = 'build/Debug/lpcvm.exe'
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    logf = open('live_%s.txt' % name, 'wb')
    p = subprocess.Popen([exe, 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=env)
    procs.append((name, p, logf))
    time.sleep(2)
print('started; pids:', {n: p.pid for n, p, f in procs}, flush=True)

def portup(port):
    s = socket.socket(); s.settimeout(0.3)
    try:
        s.connect(('127.0.0.1', port)); s.close(); return True
    except Exception:
        return False
start = time.time()
last = 0
while time.time() - start < WINDOW:
    time.sleep(6)
    marks = []
    if portup(8001): marks.append('AAA')
    if portup(8120): marks.append('DBA')
    if portup(8110): marks.append('CCS')
    if portup(8160): marks.append('GS')
    sz = os.path.getsize('live_gs.txt') if os.path.exists('live_gs.txt') else 0
    print('[%4ds] %s gs=%0.1fMB' % (int(time.time() - start), ' '.join(marks), sz / 1048576.0), flush=True)
    last = sz
print('window over', flush=True)
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()
