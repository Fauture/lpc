# -*- coding: utf-8 -*-
# Boot aaa/dba/ccs/gs, run until the yunxiaogong shop NPC clones have loaded
# (either OK or with construct errors), then kill everything.
import subprocess, time, os, sys, io, socket
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
WINDOW = int(sys.argv[1]) if len(sys.argv) > 1 else 1200
KEEP = os.environ.get('KEEP_ENV')
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
env_base = dict(os.environ)
for k in ['LPCVM_TRACE', 'LPCVM_INHINIT', 'LPCVM_EVENT_TRACE', 'LPCVM_TRACE_C8',
          'LPCVM_NO_CONST', 'LPCVM_LOCK_TRACE', 'LPCVM_STUCK_TRACE', 'LPCVM_SLOT_REF',
          'LPCVM_EFUN_PROF', 'LPCVM_STEP_PROF', 'LPCVM_CLONE_PROF',
          'LPCVM_LOAD_PROF', 'LPCVM_SVC_TRACE', 'LPCVM_ERR_TRACE']:
    env_base.pop(k, None)
env_base['LPCVM_ERR_TRACE'] = '1'
for e in (KEEP or '').split(','):
    if e:
        env_base[e] = '1'
exe = os.path.join(ROOT, 'build', 'Debug', 'lpcvm.exe')
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    logf = open(os.path.join(ROOT, 'ab_%s.txt' % name), 'wb')
    p = subprocess.Popen([exe, 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         cwd=ROOT, stdout=logf, stderr=logf, env=env_base)
    procs.append((name, p, logf))
    time.sleep(3)
print('started; extra env:', env_base.get('LPCVM_SLOT_REF'), flush=True)

tails = {}
start = time.time()
done = False
while time.time() - start < WINDOW:
    time.sleep(6)
    for name, p, logf in procs:
        path = os.path.join(ROOT, 'ab_%s.txt' % name)
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
                if ('yunxiaogong/zahuodian' in line or 'Load all NPCs' in line
                        or '[lpc-error]' in line or '[construct-err]' in line):
                    print('[%5ds] %s> %s' % (int(time.time() - start), name, line[:220]), flush=True)
    # did we see the zahuodian clone outcome (either OK or construct-err)?
    gp = os.path.join(ROOT, 'ab_gs.txt')
    if os.path.exists(gp):
        txt = open(gp, 'rb').read().decode('utf-8', errors='replace')
        zlines = [l for l in txt.split('\n') if 'yunxiaogong/zahuodian' in l]
        if zlines:
            print('zahuodian seen; finishing', flush=True)
            done = True
    if done:
        break
    alive = sum(1 for _, p, _ in procs if p.poll() is None)
    if alive == 0:
        break
print('window over / done, killing', flush=True)
for name, p, logf in procs:
    if p.poll() is None:
        try:
            p.kill()
        except Exception:
            pass
    logf.close()
