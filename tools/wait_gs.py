# -*- coding: utf-8 -*-
# Boot aaa dba ccs gs and WAIT for GS port 8160 (or timeout). No quiet killer.
import subprocess, time, os, sys, io, socket
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
WINDOW = int(sys.argv[1]) if len(sys.argv) > 1 else 3600
env = dict(os.environ)
for k in ['LPCVM_TRACE', 'LPCVM_INHINIT', 'LPCVM_EVENT_TRACE', 'LPCVM_TRACE_C8', 'LPCVM_NO_CONST',
          'LPCVM_LOCK_TRACE', 'LPCVM_STUCK_TRACE']:
    env.pop(k, None)
exe = 'build/Debug/lpcvm.exe'
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    logf = open('wait_%s.txt' % name, 'wb')
    p = subprocess.Popen([exe, 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=env)
    procs.append((name, p, logf))
    time.sleep(3)
print('started (no trace env)', flush=True)

def portup(port):
    s = socket.socket(); s.settimeout(0.3)
    try:
        s.connect(('127.0.0.1', port)); s.close(); return True
    except Exception:
        return False
start = time.time()
last = {}
while time.time() - start < WINDOW:
    time.sleep(10)
    marks = []
    if portup(8001): marks.append('AAA8001')
    if portup(8120): marks.append('DBA8120')
    if portup(8110): marks.append('CCS8110')
    if portup(8160): marks.append('GS8160')
    sz = os.path.getsize('wait_gs.txt') if os.path.exists('wait_gs.txt') else 0
    tail = ''
    try:
        with open('wait_gs.txt', 'rb') as f:
            f.seek(max(0, sz - 8000))
            tail = f.read().decode('utf-8', errors='replace')
    except Exception:
        pass
    key = ''
    for pat in ['Driver load GS ok', 'Start service on port', 'Game server allows user to login',
                'try_to_sync_time', 'sync.*OK', 'init_service_module', 'Driver startup']:
        import re
        if re.search(pat, tail): key = pat
    print('[%4ds] %-28s gs=%0.1fMB key=%s' % (int(time.time() - start), ' '.join(marks), sz / 1048576.0, key), flush=True)
    if portup(8160):
        print('GS8160 UP after %ds' % int(time.time() - start), flush=True)
        break
print('window over / reached, killing', flush=True)
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()
