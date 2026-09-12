# -*- coding: utf-8 -*-
"""Boot (no heavy probes), wait for 8160, then create a character on a NORMAL
account and open the NPC menu.

GM accounts (a/a) additionally require a character from the "GM必用字符" list,
which is what kept failing (9111).  A normal account only runs the length rule,
which accepts a short name.

usage: python -X utf8 _npc_run.py [acct] [pwd] [char] [maxwait]
"""
import subprocess, sys, time, socket, os, shutil

ACCT = sys.argv[1] if len(sys.argv) > 1 else '1'
PWD = sys.argv[2] if len(sys.argv) > 2 else '1'
CHAR = sys.argv[3] if len(sys.argv) > 3 else 'awesrd'
MAXWAIT = int(sys.argv[4]) if len(sys.argv) > 4 else 720
HOST = '192.168.200.1'
LOGDIR = 'npcrun'

subprocess.run(['powershell', '-NoProfile', '-Command',
                'Get-Process lpcvm -ErrorAction SilentlyContinue | Stop-Process -Force'],
               capture_output=True, text=True)
time.sleep(3)
shutil.rmtree(LOGDIR, ignore_errors=True)
os.makedirs(LOGDIR, exist_ok=True)
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
subprocess.run([sys.executable, '_dbclear.py'], check=True, capture_output=True, timeout=180)

env = dict(os.environ)
for k in [k for k in list(env) if k.startswith('LPCVM_')]:
    env.pop(k, None)
env['LPCVM_MENU'] = '1'
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    f = open(os.path.join(LOGDIR, 'live_%s.txt' % name), 'wb', buffering=0)
    p = subprocess.Popen(['build/Release/lpcvm.exe', 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=f, stderr=subprocess.STDOUT, env=env, cwd=os.getcwd())
    procs.append((name, p, f))
    time.sleep(3)


def up(port):
    s = socket.socket(); s.settimeout(1.0)
    try:
        s.connect((HOST, port)); s.close(); return True
    except Exception:
        return False


t0 = time.time()
while time.time() - t0 < MAXWAIT:
    if up(8160):
        print('[%4ds] 8160 OPEN' % int(time.time() - t0), flush=True)
        break
    time.sleep(5)
else:
    print('!! 8160 never opened'); sys.exit(2)

print('=== login + create (acct=%s char=%s) ===' % (ACCT, CHAR), flush=True)
r = subprocess.run([sys.executable, '-X', 'utf8', '_login_full2.py', HOST, '8001', ACCT, PWD],
                   capture_output=True, text=True, timeout=300)
for l in r.stdout.splitlines():
    if any(k in l for k in ('cookie', 'auth', 'connect game', '0xF061', 'create_new_char',
                            '0x1FE5', 'exists', 'result')):
        print('   ', l[:170], flush=True)

print('=== NPC menu (0x7012 -> 0x2037) ===', flush=True)
r = subprocess.run([sys.executable, '-X', 'utf8', '_talk_test.py', HOST, '8001', ACCT, PWD, CHAR, '5'],
                   capture_output=True, text=True, timeout=300)
print(r.stdout[-7000:], flush=True)
if r.stderr.strip():
    print('stderr:', r.stderr[-500:], flush=True)

print('=== gs: menu content evidence ===', flush=True)
t = open(os.path.join(LOGDIR, 'live_gs.txt'), encoding='utf-8', errors='replace').read()
for k in ['task_newbie/menu_content', 'duowen', 'register_menu_content', '[menu-call]']:
    print('  %-26s %d' % (k, t.count(k)))
