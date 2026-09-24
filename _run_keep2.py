# -*- coding: utf-8 -*-
# 4-service runner that STAYS UP after 8160 opens (no auto-kill).
import subprocess, time, os, sys, io, shutil, socket
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
LOG_DIR = sys.argv[1] if len(sys.argv) > 1 else 'regressKEEP'
shutil.rmtree(LOG_DIR, ignore_errors=True)
os.makedirs(LOG_DIR, exist_ok=True)

def clear_db():
    subprocess.run([sys.executable, '_dbclear.py'], check=True, capture_output=True, timeout=120)

# --- single-instance guard: refuse to start a second set -------------------
PORTS = (8001, 8110, 8120, 8160)


def _port_busy(port):
    s = socket.socket(); s.settimeout(0.6)
    try:
        s.connect(('192.168.200.1', port)); s.close(); return True
    except Exception:
        return False


def _require_single_instance():
    subprocess.run(['powershell', '-NoProfile', '-Command',
                    'Get-Process lpcvm -ErrorAction SilentlyContinue | Stop-Process -Force'],
                   capture_output=True, text=True)
    time.sleep(3)
    busy = [p for p in PORTS if _port_busy(p)]
    if busy:
        print('!! ports still held by another process: %s' % busy, flush=True)
        print('!! refusing to start a second instance (it would share the DB).', flush=True)
        sys.exit(3)


_require_single_instance()
# ---------------------------------------------------------------------------

env = dict(os.environ)
for k in ['LPCVM_TRACE', 'LPCVM_INHINIT', 'LPCVM_EVENT_TRACE', 'LPCVM_TAIL', 'LPCVM_TRACE_C8',
          'LPCVM_SQL_TRACE', 'LPCVM_ERR_TRACE', 'LPCVM_QUEUE_TRACE', 'LPCVM_TRACE_FN',
          'LPCVM_LOCK_TRACE', 'LPCVM_STUCK_TRACE', 'LPCVM_CONSTRUCT_TRACE', 'LPCVM_TRACE_CREATE',
          'LPCVM_PWD_DIAG']:
    env.pop(k, None)
# LPCVM_QUIET=1: skip the default TAIL/MEM2 chatter.  Boot with this when the
# only probe needed is a targeted one (e.g. LPCVM_INSN_FN=<fn>): those fire
# only for the named function, so the boot log stays small (~300MB of [heap]
# lines otherwise).
if not os.environ.get('LPCVM_QUIET'):
    env['LPCVM_TAIL'] = '1'
    env['LPCVM_MEM2'] = '1'
if os.environ.get('EXTRA_LPCVM'):
    for k in os.environ['EXTRA_LPCVM'].split(','):
        k = k.strip()
        if not k:
            continue
        # KEY=VALUE sets the value verbatim (e.g. LPCVM_TRACE_OBJ=open_menu),
        # a bare KEY sets it to '1'.
        if '=' in k:
            name, val = k.split('=', 1)
            env[name.strip()] = val
        else:
            env[k] = '1'

clear_db()
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    fn = os.path.join(LOG_DIR, 'live_%s.txt' % name)
    f = open(fn, 'wb', buffering=0)
    p = subprocess.Popen(['build/Release/lpcvm.exe', 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=f, stderr=subprocess.STDOUT, env=env, cwd=os.getcwd())
    procs.append((name, p, f))
    time.sleep(3)
print('started 4 services (stay-up mode)', flush=True)

def up8160():
    s = socket.socket(); s.settimeout(1.0)
    try:
        s.connect(('192.168.200.1', 8160)); s.close(); return True
    except Exception:
        return False

start = time.time()
while True:
    time.sleep(15)
    el = int(time.time() - start)
    up = up8160()
    dead = [(nm, p.poll()) for nm, p, _ in procs if p.poll() is not None]
    if dead:
        print('[%5ds] service exited: %s' % (el, dead), flush=True)
    print('[%5ds] 8160=%s' % (el, up), flush=True)
