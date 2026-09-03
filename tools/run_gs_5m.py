# -*- coding: utf-8 -*-
# Boot aaa+dba+ccs+gs (gs depends on ccs), window RUN_SECONDS (default 180),
# then kill all and summarize [efun-stub] lines per service: name, caller.
import subprocess, time, os, sys, io, socket, re
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
RUN_SECONDS = int(sys.argv[1]) if len(sys.argv) > 1 else 180
env = dict(os.environ)
for k in ['LPCVM_TRACE', 'LPCVM_INHINIT', 'LPCVM_EVENT_TRACE', 'LPCVM_TRACE_C8', 'LPCVM_NO_CONST',
          'LPCVM_LOCK_TRACE']:
    env.pop(k, None)
exe = 'build/Debug/lpcvm.exe'
procs = []
logs = {}
for name in ['aaa', 'dba', 'ccs', 'gs']:
    logf = open('logsG_%s.txt' % name, 'wb')
    logs[name] = 'logsG_%s.txt' % name
    p = subprocess.Popen([exe, 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=env)
    procs.append((name, p, logf))
    time.sleep(3)
print('started aaa dba ccs gs (%ds window)' % RUN_SECONDS, flush=True)
def portup(port):
    s = socket.socket(); s.settimeout(0.4)
    try:
        s.connect(('127.0.0.1', port)); s.close(); return True
    except Exception:
        return False
start = time.time()
while time.time() - start < RUN_SECONDS:
    time.sleep(5)
    marks = []
    if portup(8001): marks.append('AAA8001')
    if portup(8120): marks.append('DBA8120')
    if portup(8110): marks.append('CCS8110')
    if portup(8160): marks.append('GS8160')
    sizes = ' '.join('%s=%.1fM' % (n[0], os.path.getsize(f) / 1048576.0) for n, f in logs.items() if os.path.exists(f))
    print('[%4ds] %s %s' % (int(time.time() - start), ' '.join(marks), sizes), flush=True)
print('window over, killing', flush=True)
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()
rx = re.compile(r'\[efun-stub\] (\S+) @([^ ]*) args=(\d+).*calls=(\d+)')
print('\n===== EFUN-STUB SUMMARY =====', flush=True)
for name in ['aaa', 'dba', 'ccs', 'gs']:
    f = logs[name]
    rows = {}
    callers = {}
    try:
        raw = open(f, 'rb').read()
    except Exception as e:
        print('%s: cannot read %s' % (name, e), flush=True)
        continue
    text = None
    for enc in ('gbk', 'utf-8'):
        try:
            text = raw.decode(enc)
            break
        except Exception:
            continue
    if text is None:
        text = raw.decode('gbk', errors='replace')
    for m in rx.finditer(text):
        ename, ctx = m.group(1), m.group(2)
        rows[ename] = rows.get(ename, 0) + 1
        callers.setdefault(ename, set()).add(ctx)
    if not rows:
        print('%s: no [efun-stub] lines' % name, flush=True)
        continue
    print('\n--- %s ---' % name, flush=True)
    for ename, n in sorted(rows.items(), key=lambda kv: -kv[1]):
        cs = sorted(callers[ename])[:4]
        print('  %-28s loglines=%-5d callers: %s' % (ename, n, ' | '.join(cs)), flush=True)
print('\n===== done =====', flush=True)
