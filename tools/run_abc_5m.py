# -*- coding: utf-8 -*-
# Boot aaa+dba+ccs (NOT gs), let them run RUN_SECONDS (default 300 = 5 min),
# then kill everything and summarize every [efun-stub] line seen per service:
# unique efun name, total logged calls (counts are rate-limited: first 20 then
# every 500, so "calls" is a lower bound on real invocations), and the distinct
# obj::fn callers that invoked it.
import subprocess, time, os, sys, io, socket, re, codecs
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
RUN_SECONDS = int(sys.argv[1]) if len(sys.argv) > 1 else 300
env = dict(os.environ)
for k in ['LPCVM_TRACE', 'LPCVM_INHINIT', 'LPCVM_EVENT_TRACE', 'LPCVM_TRACE_C8', 'LPCVM_NO_CONST']:
    env.pop(k, None)
exe = 'build/Debug/lpcvm.exe'
procs = []
logs = {}
for name in ['aaa', 'dba', 'ccs']:
    logf = open('logsA_%s.txt' % name, 'wb')
    logs[name] = 'logsA_%s.txt' % name
    p = subprocess.Popen([exe, 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=env)
    procs.append((name, p, logf))
    time.sleep(3)
print('started aaa dba ccs (%ds window)' % RUN_SECONDS, flush=True)
def portup(port):
    s = socket.socket(); s.settimeout(0.4)
    try:
        s.connect(('127.0.0.1', port)); s.close(); return True
    except Exception:
        return False
start = time.time()
last = {}
while time.time() - start < RUN_SECONDS:
    time.sleep(5)
    marks = []
    if portup(8001): marks.append('AAA8001')
    if portup(8120): marks.append('DBA8120')
    if portup(8110): marks.append('CCS8110')
    sizes = ' '.join('%s=%.1fM' % (n[0], os.path.getsize(f) / 1048576.0) for n, f in logs.items() if os.path.exists(f))
    print('[%4ds] %s %s' % (int(time.time() - start), ' '.join(marks), sizes), flush=True)
    # early stop if all three ports up and no log growth for 60s
    for f in logs.values():
        try:
            last[f] = os.path.getsize(f)
        except Exception:
            pass
print('window over, killing', flush=True)
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()

rx = re.compile(r'\[efun-stub\] (\S+) @([^ ]*) args=(\d+).*calls=(\d+)')
print('\n===== EFUN-STUB SUMMARY (rate-limited logs) =====', flush=True)
for name in ['aaa', 'dba', 'ccs']:
    f = logs[name]
    agg = {}
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
        ename, ctx, args, calls = m.group(1), m.group(2), int(m.group(3)), int(m.group(4))
        agg[ename] = agg.get(ename, 0) + calls
        callers.setdefault(ename, set()).add(ctx)
    if not agg:
        print('%s: no [efun-stub] lines' % name, flush=True)
        continue
    print('\n--- %s ---' % name, flush=True)
    for ename, calls in sorted(agg.items(), key=lambda kv: -kv[1]):
        cs = sorted(callers[ename])[:4]
        print('  %-28s logcalls=%-8d callers: %s' % (ename, calls, ' | '.join(cs)), flush=True)
print('\n===== done =====', flush=True)
