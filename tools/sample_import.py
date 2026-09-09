# -*- coding: utf-8 -*-
# Clean boot, wait ~16s (change_look stall), EIP-sample 12s, kill.
import subprocess, time, os, sys, io, socket, re
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
env = dict(os.environ)
for k in ['LPCVM_TRACE', 'LPCVM_INHINIT', 'LPCVM_EVENT_TRACE', 'LPCVM_TRACE_C8', 'LPCVM_NO_CONST',
          'LPCVM_LOCK_TRACE', 'LPCVM_STUCK_TRACE', 'LPCVM_EFUN_PROF', 'LPCVM_STEP_PROF',
          'LPCVM_CLONE_PROF', 'LPCVM_LOAD_PROF', 'LPCVM_SVC_TRACE']:
    env.pop(k, None)
exe = 'build/Debug/lpcvm.exe'
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    logf = open('cln_%s.txt' % name, 'wb')
    p = subprocess.Popen([exe, 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=env)
    procs.append((name, p, logf))
    time.sleep(2)
pid = procs[3][1].pid
print('gs pid=%d, waiting 16s' % pid, flush=True)
time.sleep(16)
sam = 'C:/Windows/SysWOW64/WindowsPowerShell/v1.0/powershell.exe'
print('SAMPLING', flush=True)
r = subprocess.run([sam, '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
                    os.path.abspath('tools/sample86.ps1'), '-pid2', str(pid),
                    '-samples', '6000', '-interval', '2'])
print('sample exit', r.returncode, flush=True)
print('done', flush=True)
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()
