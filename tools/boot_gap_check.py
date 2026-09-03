# -*- coding: utf-8 -*-
"""Boot 4 services with LPCVM_EFUN_PROF+LPCVM_FMT_CHECK, track per-Load gaps,
report slow loads + fmt-check parity lines + efun dumps around the gap.
Usage: python tools/boot_gap_check.py [window_s]"""
import io, os, re, subprocess, sys, time
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
WINDOW = int(sys.argv[1]) if len(sys.argv) > 1 else 300
env = dict(os.environ)
for k in ['LPCVM_TRACE', 'LPCVM_INHINIT', 'LPCVM_EVENT_TRACE', 'LPCVM_TRACE_C8',
          'LPCVM_NO_CONST', 'LPCVM_LOCK_TRACE', 'LPCVM_STUCK_TRACE', 'LPCVM_STEP_PROF',
          'LPCVM_SLOW_OP', 'LPCVM_CLONE_PROF']:
    env.pop(k, None)
env['LPCVM_EFUN_PROF'] = '1'
env['LPCVM_FMT_CHECK'] = '1'
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    logf = open('regress/gap_%s.txt' % name, 'wb')
    p = subprocess.Popen(['build/Debug/lpcvm.exe', 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=env)
    procs.append((name, p, logf))
    time.sleep(2)
t0 = time.time()
last_load = ''
last_t = None
gaps = []
def gslog():
    try:
        return open('regress/gap_gs.txt', 'rb').read().decode('utf-8', errors='replace')
    except Exception:
        return ''
while time.time() - t0 < WINDOW:
    time.sleep(2)
    el = time.time() - t0
    txt = gslog()
    m = re.findall(r'^Load: (\S+\.c)\.\.\. OK', txt, re.M)
    if m:
        cur = m[-1]
        if cur != last_load:
            if last_t is not None and el - last_t > 1.5:
                gaps.append((el - last_t, last_load, cur))
            last_load = cur
            last_t = el
    if int(el) % 30 == 0:
        print('[%4ds] loads=%d last=%s' % (int(el), len(m), last_load.rsplit('/', 1)[-1]), flush=True)
print('SLOW LOAD GAPS (>=1.5s):', flush=True)
for g, a, b in sorted(gaps, reverse=True)[:12]:
    print('  %7.1fs  %s  (after %s)' % (g, b, a.rsplit('/', 1)[-1]), flush=True)
print('FMT-CHECK LINES:', flush=True)
for name in ['aaa', 'dba', 'ccs', 'gs']:
    try:
        for ln in open('regress/gap_%s.txt' % name, encoding='utf-8', errors='replace'):
            if 'fmt-check' in ln:
                print('%s: %s' % (name, ln.strip()), flush=True)
    except Exception:
        pass
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()
print('done', flush=True)
