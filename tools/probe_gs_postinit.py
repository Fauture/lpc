# -*- coding: utf-8 -*-
"""Run 4 services, watch GS post_init progress + tasks top-level load count."""
import subprocess, sys, time, os, io, re
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

env = dict(os.environ)
env['LPCVM_TRACE'] = '1'
env['LPCVM_INHINIT'] = '1'
env['LPCVM_DBG_OBJVAR'] = os.environ.get('LPCVM_DBG_OBJVAR', '1')
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
        tops = len(set(re.findall(r'Load: (/gs/daemons/tasks/[^/]+?\.c)\.\.\. OK', text)))
        errs = len(re.findall(r'\[sched\] process .* error', text))
        lgxy = len(re.findall(r'postinit-sync.*longgong|init_all_npc.*longgong', text))
        return tops, last_pi, errs, len(text), lgxy
    except Exception as e:
        return -1, ('err', str(e)), -1, -1, -1

for i in range(200):
    time.sleep(4)
    nt, pi, errs, nbytes, lgxy = gs_state()
    print('[%4ds] tasksTop=%d errs=%d lgxy=%d postinit=%s(%s) log=%.1fMB' % (
        i*4, nt, errs, lgxy, pi[0], pi[1][:50], nbytes/1048576), flush=True)
    if procs[3][1].poll() is not None:
        print('gs exited', procs[3][1].poll(), flush=True)
        break
    if lgxy >= 1:
        print('LONGONG XIANYI REACHED', flush=True)
        break
for name, p, logf in procs:
    if p.poll() is None: p.kill()
    logf.close()
