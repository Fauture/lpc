# -*- coding: utf-8 -*-
"""Run 4 services, watch for guardd DONE + longgong/douchong do_init, sample gs WS."""
import subprocess, sys, time, os, io, re
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

env = dict(os.environ)
env['LPCVM_TRACE'] = '1'
env['LPCVM_INHINIT'] = '1'
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    logf = open('logs_%sAX_err.txt' % name, 'wb')
    p = subprocess.Popen(['build/Release/lpcvm.exe', 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=env)
    procs.append((name, p, logf))
    time.sleep(3)

def gs_state():
    try:
        with open('logs_gsAX_err.txt', 'rb') as f:
            data = f.read()
        text = data.decode('utf-8', errors='replace')
        pi = [m for m in re.finditer(r'\[postinit-sync\] (\S+?) on (\S+)', text)]
        last_pi = pi[-1].groups() if pi else ('', '')
        guardd_done = 'owner-init DONE do_init on /gs/daemons/guardd.o' in text
        lgxy_init = bool(re.search(r'do_init on /gs/daemons/(template/)?longgong', text))
        dc_init = bool(re.search(r'do_init on /gs/daemons/douchong', text))
        # gs process memory
        ws = -1
        try:
            import ctypes
            ps = subprocess.check_output(['powershell', '-NoProfile', '-Command',
                "Get-CimInstance Win32_Process -Filter \"Name='lpcvm.exe'\" | Where-Object { $_.CommandLine -match 'lib_gs32' } | ForEach-Object { [math]::Round($_.WorkingSetSize/1MB) }"])
            ws = int(ps.strip().split()[-1])
        except Exception:
            pass
        return guardd_done, lgxy_init, dc_init, last_pi, len(text), ws
    except Exception as e:
        return False, False, False, ('err', str(e)), -1, -1

start = time.time()
for i in range(400):
    time.sleep(5)
    gd, lg, dc, pi, nbytes, ws = gs_state()
    print('[%4ds] guardd=%s lgxy=%s douchong=%s postinit=%s(%s) log=%.0fMB ws=%sMB' % (
        int(time.time()-start), gd, lg, dc, pi[0], pi[1][:40], nbytes/1048576, ws), flush=True)
    if procs[3][1].poll() is not None:
        print('gs exited', procs[3][1].poll(), flush=True)
        break
    if lg and dc:
        print('LONGONG + DOUCHONG DO_INIT REACHED', flush=True)
        break
for name, p, logf in procs:
    if p.poll() is None: p.kill()
    logf.close()
