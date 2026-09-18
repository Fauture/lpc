# -*- coding: utf-8 -*-
"""Restart the services with diagnostics, wait for the gs, run the self-id menu
test and print the [near-var] evidence (what is_visible read for my_x/my_y).

usage: python -X utf8 _diag_run.py [logdir]
"""
import io
import os
import socket
import subprocess
import sys
import time

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
ROOT = os.path.dirname(os.path.abspath(__file__))
LOGDIR = sys.argv[1] if len(sys.argv) > 1 else 'diag1'
SPEC = 'LPCVM_NEAR=1;LPCVM_IIDDBG=1;LPCVM_MENU=1'


def port_open(p, t=1.0):
    s = socket.socket()
    s.settimeout(t)
    try:
        s.connect(('192.168.200.1', p))
        return True
    except Exception:
        return False
    finally:
        s.close()


def main():
    subprocess.run(['powershell', '-NoProfile', '-Command',
                    'Get-Process lpcvm -ErrorAction SilentlyContinue | Stop-Process -Force'],
                   capture_output=True)
    time.sleep(3)
    ps = ("Invoke-CimMethod -ClassName Win32_Process -MethodName Create -Arguments "
          "@{CommandLine='python -X utf8 _run_probe.py %s 21600 %s'; CurrentDirectory='%s'}"
          % (LOGDIR, SPEC, ROOT))
    subprocess.run(['powershell', '-NoProfile', '-Command', ps], capture_output=True)
    print('server starting (%s)' % LOGDIR, flush=True)
    t0 = time.time()
    while time.time() - t0 < 2400:
        if port_open(8160, 0.6):
            print('gs ready after %ds' % int(time.time() - t0), flush=True)
            break
        time.sleep(5)
    else:
        print('TIMEOUT waiting for 8160')
        return 1
    r = subprocess.run([sys.executable, '-X', 'utf8', '_autotest.py', '3', '3', 'awesrd3', 'selfid'],
                       cwd=ROOT, capture_output=True, text=True, encoding='utf-8', errors='replace')
    print((r.stdout or '').strip()[-2000:], flush=True)
    log = os.path.join(ROOT, LOGDIR, 'live_gs.txt')
    lines = io.open(log, encoding='utf-8', errors='replace').read().splitlines()
    hits = [l for l in lines if '[near-var]' in l]
    print('--- [near-var] lines: %d' % len(hits), flush=True)
    for l in hits[-12:]:
        print('  ' + l[:190], flush=True)
    return 0


if __name__ == '__main__':
    sys.exit(main())
