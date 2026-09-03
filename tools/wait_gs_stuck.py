# -*- coding: utf-8 -*-
# Boot aaa dba ccs gs with LPCVM_STUCK_TRACE=1 for gs so the [vm-stuck] frame
# dump (every 500k instructions) shows exactly which LPC function burns CPU.
import subprocess, time, os, sys, io, socket, re
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
WINDOW = int(sys.argv[1]) if len(sys.argv) > 1 else 300
env_base = dict(os.environ)
for k in ['LPCVM_TRACE', 'LPCVM_INHINIT', 'LPCVM_EVENT_TRACE', 'LPCVM_TRACE_C8', 'LPCVM_NO_CONST',
          'LPCVM_LOCK_TRACE', 'LPCVM_STUCK_TRACE', 'LPCVM_EFUN_PROF']:
    env_base.pop(k, None)
exe = 'build/Debug/lpcvm.exe'
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    logf = open('wstuck_%s.txt' % name, 'wb')
    env = dict(env_base)
    if name == 'gs':
        env['LPCVM_STUCK_TRACE'] = '1'
    p = subprocess.Popen([exe, 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=env)
    procs.append((name, p, logf))
    time.sleep(3)
print('started (gs LPCVM_STUCK_TRACE=1)', flush=True)

def portup(port):
    s = socket.socket(); s.settimeout(0.3)
    try:
        s.connect(('127.0.0.1', port)); s.close(); return True
    except Exception:
        return False
start = time.time()
lastsz = 0
while time.time() - start < WINDOW:
    time.sleep(8)
    sz = os.path.getsize('wstuck_gs.txt') if os.path.exists('wstuck_gs.txt') else 0
    stuck = 0
    tail = ''
    try:
        with open('wstuck_gs.txt', 'rb') as f:
            f.seek(max(0, sz - 20000))
            tail = f.read().decode('utf-8', errors='replace')
        stuck = len(re.findall(r'\[vm-stuck\]', tail))
    except Exception:
        pass
    grow = sz - lastsz
    lastsz = sz
    print('[%4ds] gs=%0.1fMB +%dKB stuckInTail=%d' % (int(time.time() - start), sz / 1048576.0,
          grow // 1024, stuck), flush=True)
    if portup(8160):
        print('GS8160 UP after %ds' % int(time.time() - start), flush=True)
        break
print('window over / reached, killing', flush=True)
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()
