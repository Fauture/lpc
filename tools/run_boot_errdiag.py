# -*- coding: utf-8 -*-
# Boot aaa/dba/ccs/gs with LPCVM_ERR_TRACE so the LPC `error()` efun prints a
# call chain.  Watches for exquisited/questiond construct errors, then kills
# everything.
import subprocess, time, os, sys, io, socket, re
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
WINDOW = int(sys.argv[1]) if len(sys.argv) > 1 else 3000
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
env_base = dict(os.environ)
for k in ['LPCVM_TRACE', 'LPCVM_INHINIT', 'LPCVM_EVENT_TRACE', 'LPCVM_TRACE_C8',
          'LPCVM_NO_CONST', 'LPCVM_LOCK_TRACE', 'LPCVM_STUCK_TRACE',
          'LPCVM_EFUN_PROF', 'LPCVM_STEP_PROF', 'LPCVM_CLONE_PROF',
          'LPCVM_LOAD_PROF', 'LPCVM_SVC_TRACE', 'LPCVM_ERR_TRACE']:
    env_base.pop(k, None)
env_base['LPCVM_ERR_TRACE'] = '1'
exe = os.path.join(ROOT, 'build', 'Debug', 'lpcvm.exe')
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    logf = open(os.path.join(ROOT, 'errdiag_%s.txt' % name), 'wb')
    env = dict(env_base)
    p = subprocess.Popen([exe, 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         cwd=ROOT, stdout=logf, stderr=logf, env=env)
    procs.append((name, p, logf))
    time.sleep(3)
print('started 4 services with LPCVM_ERR_TRACE=1', flush=True)

def portup(port):
    s = socket.socket(); s.settimeout(0.4)
    try:
        s.connect(('127.0.0.1', port)); s.close(); return True
    except Exception:
        return False

# Keep tail positions per log to only report NEW interesting lines.
tails = {}
start = time.time()
hits = 0
while time.time() - start < WINDOW:
    time.sleep(6)
    marks = []
    if portup(8001): marks.append('AAA8001')
    if portup(8120): marks.append('DBA8120')
    if portup(8110): marks.append('CCS8110')
    if portup(8160): marks.append('GS8160')
    alive = sum(1 for _, p, _ in procs if p.poll() is None)
    for name, p, logf in procs:
        path = os.path.join(ROOT, 'errdiag_%s.txt' % name)
        try:
            sz = os.path.getsize(path)
        except OSError:
            sz = 0
        pos = tails.get(name, 0)
        if sz > pos:
            with open(path, 'rb') as f:
                f.seek(pos)
                data = f.read(sz - pos)
            tails[name] = sz
            for raw in data.split(b'\n'):
                line = raw.decode('utf-8', errors='replace').rstrip()
                if not line:
                    continue
                if ('[lpc-error]' in line or '[construct-err]' in line
                        or line.startswith('[c2i') or '[lpc-error-stack]' in line
                        or line.startswith('Load: /gs/daemons/exquisited')
                        or line.startswith('Load: /gs/daemons/questiond')
                        or 'Load sub dir: /gs/daemons' in line):
                    print('[%5ds] %s> %s' % (int(time.time() - start), name, line[:400]), flush=True)
                    if '[lpc-error]' in line or '[construct-err]' in line:
                        hits += 1
    print('[%5ds] %-22s alive=%d %s' % (int(time.time() - start), ' '.join(marks), alive, 'HITS=%d' % hits if hits else ''), flush=True)
    if hits >= 6:
        print('enough diagnostics, killing', flush=True)
        break
    if alive == 0:
        print('all services exited early', flush=True)
        break
print('window over, killing all', flush=True)
for name, p, logf in procs:
    if p.poll() is None:
        try:
            p.kill()
        except Exception:
            pass
    logf.close()
