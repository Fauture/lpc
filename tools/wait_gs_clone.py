# -*- coding: utf-8 -*-
# Boot aaa dba ccs gs with LPCVM_CLONE_PROF=1 for gs: clone phase-cost dump.
import subprocess, time, os, sys, io, socket, re
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
WINDOW = int(sys.argv[1]) if len(sys.argv) > 1 else 420
env_base = dict(os.environ)
for k in ['LPCVM_TRACE', 'LPCVM_INHINIT', 'LPCVM_EVENT_TRACE', 'LPCVM_TRACE_C8', 'LPCVM_NO_CONST',
          'LPCVM_LOCK_TRACE', 'LPCVM_STUCK_TRACE', 'LPCVM_EFUN_PROF', 'LPCVM_STEP_PROF', 'LPCVM_CLONE_PROF']:
    env_base.pop(k, None)
exe = 'build/Debug/lpcvm.exe'
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    logf = open('wclone_%s.txt' % name, 'wb')
    env = dict(env_base)
    if name == 'gs':
        env['LPCVM_CLONE_PROF'] = '1'
    p = subprocess.Popen([exe, 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=env)
    procs.append((name, p, logf))
    time.sleep(3)
print('started (gs LPCVM_CLONE_PROF=1)', flush=True)

def portup(port):
    s = socket.socket(); s.settimeout(0.3)
    try:
        s.connect(('127.0.0.1', port)); s.close(); return True
    except Exception:
        return False
start = time.time()
lastsz = 0
while time.time() - start < WINDOW:
    time.sleep(10)
    marks = []
    if portup(8001): marks.append('AAA8001')
    if portup(8120): marks.append('DBA8120')
    if portup(8110): marks.append('CCS8110')
    if portup(8160): marks.append('GS8160')
    sz = os.path.getsize('wclone_gs.txt') if os.path.exists('wclone_gs.txt') else 0
    cp = ''
    try:
        with open('wclone_gs.txt', 'rb') as f:
            f.seek(max(0, sz - 2500))
            tail = f.read().decode('utf-8', errors='replace')
        m = re.findall(r'\[clone-prof\] [^\n]*', tail)
        if m: cp = m[-1]
    except Exception:
        pass
    print('[%4ds] %-28s gs=%0.1fMB %s' % (int(time.time() - start), ' '.join(marks),
          sz / 1048576.0, cp), flush=True)
    if portup(8160):
        print('GS8160 UP after %ds' % int(time.time() - start), flush=True)
        break
print('window over / reached, killing', flush=True)
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()
