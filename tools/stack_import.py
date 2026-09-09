# -*- coding: utf-8 -*-
# Clean boot, at ~18s (change_look stall) stack-sample gs 10s, then kill.
import subprocess, time, os, sys, io
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
env = dict(os.environ)
for k in ['LPCVM_TRACE', 'LPCVM_INHINIT', 'LPCVM_EVENT_TRACE', 'LPCVM_TRACE_C8', 'LPCVM_NO_CONST',
          'LPCVM_LOCK_TRACE', 'LPCVM_STUCK_TRACE', 'LPCVM_EFUN_PROF', 'LPCVM_STEP_PROF',
          'LPCVM_CLONE_PROF', 'LPCVM_LOAD_PROF', 'LPCVM_SVC_TRACE']:
    env.pop(k, None)
exe = 'build/Debug/lpcvm.exe'
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    logf = open('st2_%s.txt' % name, 'wb')
    p = subprocess.Popen([exe, 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=env)
    procs.append((name, p, logf))
    time.sleep(2)
pid = procs[3][1].pid
print('gs pid=%d wait 17s' % pid, flush=True)
time.sleep(17)
sam = 'C:/Windows/SysWOW64/WindowsPowerShell/v1.0/powershell.exe'
print('STACK SAMPLING', flush=True)
subprocess.run([sam, '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
                os.path.abspath('tools/stack86.ps1'), '-pid2', str(pid),
                '-samples', '4000', '-interval', '2'])
print('done', flush=True)
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()
