# -*- coding: utf-8 -*-
# Boot aaa+dba+ccs+gs with LPCVM_STUCK_TRACE=1 (+ optional LPCVM_TRACE for gs)
# so runLoop prints [vm-stuck] obj/fn/ip every 500k steps, revealing where gs
# spins. Kills everything after WINDOW seconds or once gs log stops growing.
import subprocess, time, os, sys, io, socket, re, signal
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
WINDOW = int(sys.argv[1]) if len(sys.argv) > 1 else 600
TRACE = os.environ.get('GS_TRACE', '0')
env = dict(os.environ)
env['LPCVM_STUCK_TRACE'] = '1'
if TRACE == '1':
    env['LPCVM_TRACE'] = '1'
else:
    env.pop('LPCVM_TRACE', None)
for k in ['LPCVM_INHINIT', 'LPCVM_EVENT_TRACE', 'LPCVM_TRACE_C8', 'LPCVM_NO_CONST', 'LPCVM_LOCK_TRACE']:
    env.pop(k, None)
exe = 'build/Debug/lpcvm.exe'
procs = []
logs = {}
for name in ['aaa', 'dba', 'ccs', 'gs']:
    logf = open('dbg_%s.txt' % name, 'wb')
    logs[name] = 'dbg_%s.txt' % name
    p = subprocess.Popen([exe, 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=env)
    procs.append((name, p, logf))
    time.sleep(3)
print('started aaa dba ccs gs (stuck-trace=%s, trace=%s, window=%ds)' % ('1', TRACE, WINDOW), flush=True)

def portup(port):
    s = socket.socket(); s.settimeout(0.4)
    try:
        s.connect(('127.0.0.1', port)); s.close(); return True
    except Exception:
        return False

start = time.time()
last_size = 0
quiet = 0
while time.time() - start < WINDOW:
    time.sleep(8)
    marks = []
    if portup(8001): marks.append('AAA')
    if portup(8120): marks.append('DBA')
    if portup(8110): marks.append('CCS')
    if portup(8160): marks.append('GS8160')
    sz = os.path.getsize(logs['gs']) if os.path.exists(logs['gs']) else 0
    if sz == last_size:
        quiet += 1
    else:
        quiet = 0
    last_size = sz
    print('[%4ds] %-22s gs=%.1fMB quiet=%d' % (int(time.time() - start), ' '.join(marks), sz / 1048576.0, quiet), flush=True)
    if quiet >= 4 and portup(8110):
        print('GS LOG QUIET for ~32s -> dumping tail & stopping', flush=True)
        break
print('window over, killing', flush=True)
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()
