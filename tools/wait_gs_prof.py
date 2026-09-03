# -*- coding: utf-8 -*-
# Boot aaa dba ccs gs with the per-efun profiler on for gs (LPCVM_EFUN_PROF=1),
# then report Load-line throughput + [efun-prof] dumps every 10s. No quiet killer.
import subprocess, time, os, sys, io, socket, re
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
WINDOW = int(sys.argv[1]) if len(sys.argv) > 1 else 420
env_base = dict(os.environ)
for k in ['LPCVM_TRACE', 'LPCVM_INHINIT', 'LPCVM_EVENT_TRACE', 'LPCVM_TRACE_C8', 'LPCVM_NO_CONST',
          'LPCVM_LOCK_TRACE', 'LPCVM_STUCK_TRACE', 'LPCVM_EFUN_PROF']:
    env_base.pop(k, None)
exe = 'build/Debug/lpcvm.exe'
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    logf = open('wprof_%s.txt' % name, 'wb')
    env = dict(env_base)
    if name == 'gs':
        env['LPCVM_EFUN_PROF'] = '1'
    p = subprocess.Popen([exe, 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=env)
    procs.append((name, p, logf))
    time.sleep(3)
print('started (gs LPCVM_EFUN_PROF=1)', flush=True)

def portup(port):
    s = socket.socket(); s.settimeout(0.3)
    try:
        s.connect(('127.0.0.1', port)); s.close(); return True
    except Exception:
        return False
start = time.time()
while time.time() - start < WINDOW:
    time.sleep(10)
    marks = []
    if portup(8001): marks.append('AAA8001')
    if portup(8120): marks.append('DBA8120')
    if portup(8110): marks.append('CCS8110')
    if portup(8160): marks.append('GS8160')
    sz = os.path.getsize('wprof_gs.txt') if os.path.exists('wprof_gs.txt') else 0
    loads = 0
    prof = []
    try:
        with open('wprof_gs.txt', 'rb') as f:
            data = f.read().decode('utf-8', errors='replace')
        loads = len(re.findall(r'^Load: ', data, re.M))
        for m in re.finditer(r'\[efun-prof\] top[^\n]*\n((?:  [^\n]*\n){1,14})', data):
            prof.append(m.group(0).strip())
    except Exception:
        pass
    print('[%4ds] %-28s gs=%0.1fMB loads=%d%s' % (int(time.time() - start), ' '.join(marks),
          sz / 1048576.0, loads, '' if not prof else ' lastprof=' + prof[-1].splitlines()[0]), flush=True)
    if portup(8160):
        print('GS8160 UP after %ds' % int(time.time() - start), flush=True)
        break
print('window over / reached, killing', flush=True)
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()
