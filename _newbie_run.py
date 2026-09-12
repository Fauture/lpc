# -*- coding: utf-8 -*-
"""New-character story/task verification.

  python -X utf8 _newbie_run.py [spec] [logdir]

boots the 4 services with LPCVM_NEWBIE=1 (plus whatever spec is given), waits
for gs 8160, creates the char (1/1 -> awesrd) and enters the world, then dumps
the newbie chain from the gs log:
  logind::enter_world -> set_newbie_task -> post_enter_world ->
  _enter_world_funcs closures -> newbied::do_when_enter_world ->
  taskd("【新手】1-9级新手任务")->do_when_char_born/do_task_instruction ->
  try_create_task / raise_event_ex("初进游戏") / scenario / teleport
"""
import io
import os
import socket
import subprocess
import sys
import time

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

SPEC = sys.argv[1] if len(sys.argv) > 1 else 'LPCVM_NEWBIE=1'
LOG = sys.argv[2] if len(sys.argv) > 2 else 'nb1'
PY = sys.executable
HOST = '192.168.200.1'


def port_open(port, t=1.5):
    s = socket.socket()
    s.settimeout(t)
    try:
        s.connect((HOST, port))
        s.close()
        return True
    except Exception:
        return False


def run(args, timeout):
    try:
        r = subprocess.run([PY, '-X', 'utf8'] + args, capture_output=True, text=True,
                           encoding='utf-8', errors='replace', timeout=timeout)
        return r.stdout or '', r.stderr or ''
    except subprocess.TimeoutExpired as e:
        o = e.stdout.decode('utf-8', 'replace') if isinstance(e.stdout, bytes) else (e.stdout or '')
        return o, 'TIMEOUT after %ss' % timeout


print('=== boot (probes=%s) ===' % SPEC, flush=True)
bf = open(LOG + '_boot.txt', 'wb', buffering=0)
boot = subprocess.Popen([PY, '-X', 'utf8', '_run_probe.py', LOG, '1100', SPEC],
                        stdout=bf, stderr=subprocess.STDOUT, cwd=os.getcwd())
print('boot pid=%d' % boot.pid, flush=True)

t0 = time.time()
up = False
while time.time() - t0 < 1000:
    time.sleep(10)
    if port_open(8160):
        up = True
        break
print('gs 8160 up=%s after %.0fs' % (up, time.time() - t0), flush=True)
if not up:
    print('!! gs never opened - aborting', flush=True)
    subprocess.run(['powershell', '-NoProfile', '-Command',
                    'Get-Process lpcvm -ErrorAction SilentlyContinue | Stop-Process -Force'])
    sys.exit(2)
time.sleep(25)

print('=== login/create (1/1 -> awesrd) ===', flush=True)
so, se = run(['_login_full2.py', HOST, '8001', '1', '1'], 420)
tail = [l for l in so.splitlines() if ('F061' in l or 'create_new_char' in l or '0x1FE5' in l
                                       or '9113' in l or '1FE5' in l)]
print('\n'.join(tail) or so[-1200:], flush=True)
if se.strip():
    print('stderr:', se[-600:], flush=True)

print('=== enter world (select awesrd, wait) ===', flush=True)
so2, se2 = run(['_npc_verbose.py', '1', '1', 'awesrd'], 300)
print('\n'.join(so2.splitlines()[:12]), flush=True)
if se2.strip():
    print('stderr:', se2[-600:], flush=True)

print('=== newbie chain (gs) ===', flush=True)
path = os.path.join(LOG, 'live_gs.txt')
data = io.open(path, 'rb').read().decode('utf-8', 'replace').splitlines()
KEY = ('register_enter_world', 'post_enter_world', 'do_when_enter_world', 'set_newbie_task',
       'do_task_instruction', 'do_when_char_born', 'gift_for_newbie', 'try_create_task',
       'raise_event', 'do_other_thing', 'newbied', 'instruction_1', 'newbie-gift-task',
       'played_drama', '初进游戏', 'send_task_log', 'do_stage_move', 'scenariod')
hits = [l for l in data if any(k in l for k in KEY)]
print('matching lines: %d (of %d)' % (len(hits), len(data)), flush=True)
for l in hits[-260:]:
    print('  ' + l[:250], flush=True)

print('=== stopping services ===', flush=True)
subprocess.run(['powershell', '-NoProfile', '-Command',
                'Get-Process lpcvm -ErrorAction SilentlyContinue | Stop-Process -Force'])
try:
    boot.wait(timeout=20)
except Exception:
    boot.kill()
print('=== DONE ===', flush=True)
