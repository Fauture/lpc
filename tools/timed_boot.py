# -*- coding: utf-8 -*-
# Timed gs boot sampler: every 5s print elapsed, ports, gs Load count + last
# Load + size, and the first-seen wall time of key boot markers.
import subprocess, time, os, sys, io, socket, re
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
WINDOW = int(sys.argv[1]) if len(sys.argv) > 1 else 150
env = dict(os.environ)
for k in ['LPCVM_TRACE', 'LPCVM_INHINIT', 'LPCVM_EVENT_TRACE', 'LPCVM_TRACE_C8', 'LPCVM_NO_CONST',
          'LPCVM_LOCK_TRACE', 'LPCVM_STUCK_TRACE', 'LPCVM_EFUN_PROF', 'LPCVM_STEP_PROF',
          'LPCVM_CLONE_PROF', 'LPCVM_LOAD_PROF', 'LPCVM_SVC_TRACE']:
    env.pop(k, None)
exe = 'build/Debug/lpcvm.exe'
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    logf = open('timed_%s.txt' % name, 'wb')
    p = subprocess.Popen([exe, 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=env)
    procs.append((name, p, logf))
    time.sleep(2)
t0 = time.time()
print('started, sampling every 5s', flush=True)

def portup(port):
    s = socket.socket(); s.settimeout(0.25)
    try:
        s.connect(('127.0.0.1', port)); s.close(); return True
    except Exception:
        return False

MARKERS = ['Load all daemons', 'daemons/basic_objectd.c... OK',
           'zone/taoliulin.c... OK', 'Load all init scripts', 'Invoke registered',
           'Please config strategy_war_server', 'Driver load GS ok']
seen_mark = {}
last = ''
while time.time() - t0 < WINDOW:
    time.sleep(5)
    el = time.time() - t0
    ports = [p for p in (8001, 8120, 8110, 8160) if portup(p)]
    sz = os.path.getsize('timed_gs.txt') if os.path.exists('timed_gs.txt') else 0
    loads = 0
    try:
        with open('timed_gs.txt', 'rb') as f:
            data = f.read().decode('utf-8', errors='replace')
        loads = len(re.findall(r'^Load: ', data, re.M))
        for m in re.finditer(r'^Load: (\S+\.c)\.\.\. OK', data, re.M):
            last = m.group(1)
        for mk in MARKERS:
            if mk not in seen_mark and mk in data:
                seen_mark[mk] = el
    except Exception:
        pass
    note = ' | '.join('%s@%.0fs' % (m.rsplit('/', 1)[-1], t) for m, t in sorted(seen_mark.items(), key=lambda x: x[1]))
    print('[%5.0fs] ports=%s gsMB=%.2f loads=%d last=%s%s' % (el, ports, sz / 1048576.0, loads, last,
          (' | ' + note) if note else ''), flush=True)
print('window over', flush=True)
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()
