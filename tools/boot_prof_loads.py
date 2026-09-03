# -*- coding: utf-8 -*-
"""Boot aaa/dba/ccs/gs with LPCVM_EFUN_PROF=1, track first-seen wall time of
every gs 'Load: ...' line, find the slowest loads, and summarize the
[efun-prof] CPU dumps around those moments.  Window in seconds via argv.
Usage: python tools/boot_prof_loads.py [window_s]
"""
import io, os, re, subprocess, sys, time
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
WINDOW = int(sys.argv[1]) if len(sys.argv) > 1 else 300
OUT = 'regress'
os.makedirs(OUT, exist_ok=True)
env = dict(os.environ)
for k in ['LPCVM_TRACE', 'LPCVM_INHINIT', 'LPCVM_EVENT_TRACE', 'LPCVM_TRACE_C8',
          'LPCVM_NO_CONST', 'LPCVM_LOCK_TRACE', 'LPCVM_STUCK_TRACE']:
    env.pop(k, None)
env['LPCVM_EFUN_PROF'] = '1'
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    logf = open(os.path.join(OUT, 'prof_%s.txt' % name), 'wb')
    p = subprocess.Popen(['build/Debug/lpcvm.exe', 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=env)
    procs.append((name, p, logf))
    time.sleep(2)
t0 = time.time()
print('started, window=%ds' % WINDOW, flush=True)

def gs_tail():
    path = os.path.join(OUT, 'prof_gs.txt')
    try:
        with open(path, 'rb') as f:
            return f.read().decode('utf-8', errors='replace')
    except Exception:
        return ''

load_first = {}      # line -> wall s when first seen
last_pos = 0
last_line = ''
stalls = []          # (gap, line, prev_line)
prev_time = None
efun_dumps = []      # list of dump blocks (time, text)
dump_re = re.compile(r'^\[efun-prof\]', re.M)
last_dump_time = 0
last_dump_end = 0
while time.time() - t0 < WINDOW:
    time.sleep(1)
    txt = gs_tail()
    el = time.time() - t0
    # per-load first-seen times + gaps
    seen = 0
    for m in re.finditer(r'^Load: (\S+\.c)\.\.\. OK', txt, re.M):
        line = m.group(1)
        if line not in load_first:
            load_first[line] = el
            # gap vs previous load
            if prev_time is not None and el - prev_time >= 2:
                stalls.append((el - prev_time, line, last_line))
            prev_time = el
            last_line = line
        seen += 1
    # newest efun-prof dump
    dumps = [(m.start(), m.end()) for m in dump_re.finditer(txt)]
    if dumps and dumps[-1][0] > last_dump_end:
        # capture the whole dump block (up to 16 lines after marker)
        start = dumps[-1][0]
        blk = txt[start:start + 4000]
        efun_dumps.append((el, blk))
        last_dump_end = dumps[-1][0]
    if int(el) % 15 == 0:
        print('[%4ds] loads=%d last=%s' % (int(el), len(load_first), last_line.rsplit('/', 1)[-1]), flush=True)
print('window over; gs loads seen: %d' % len(load_first), flush=True)
print('\n===== SLOWEST LOADS (gap >= 2s) =====', flush=True)
for gap, line, prev in sorted(stalls, reverse=True)[:25]:
    print('  %6.1fs  %s   (after %s)' % (gap, line, prev.rsplit('/', 1)[-1]), flush=True)
print('\n===== EFUN-PROF DUMP SAMPLES =====', flush=True)
# keep a few dumps: one per 30s roughly
sel = []
for t, blk in efun_dumps:
    if not sel or t - sel[-1][0] >= 30:
        sel.append((t, blk))
for t, blk in sel[-14:]:
    print('----- dump @ %6.1fs -----' % t, flush=True)
    print(blk[:3500], flush=True)
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()
print('done', flush=True)
