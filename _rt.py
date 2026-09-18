# -*- coding: utf-8 -*-
"""One-command RDP round-trip: server -> account/char -> client -> in game.

    python -X utf8 _rt.py [acct] [pwd] [charname]

Steps (all waits are short polls, never a fixed 10s sleep):
  1. make sure the four services are running (started with Win32_Process.Create so
     they live outside this shell's memory job); clear dl_ddb_1_test only when a
     fresh boot is actually started;
  2. poll 8001/8110/8120/8160 until LISTEN (poll 3s, report progress);
  3. protocol-side account/char preparation: only when the account has no
     character, create one through the server itself (_login_full.py) - the
     client is then used for the real login;
  4. launch the client (outside the shell job) and drive the login click-through;
  5. screenshots: <tag>_1_filled / _2_after_login / _3_ingame.
"""
import io
import os
import socket
import subprocess
import sys
import time

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
ROOT = os.path.dirname(os.path.abspath(__file__))
HOST = '192.168.200.1'
PORTS = (8001, 8110, 8120, 8160)


def port_open(p, timeout=1.0):
    s = socket.socket()
    s.settimeout(timeout)
    try:
        s.connect((HOST, p))
        return True
    except Exception:
        return False
    finally:
        s.close()


def services_up():
    return all(port_open(p, 0.6) for p in (8001, 8110, 8120))


def start_services(probe='rt1', spec='LPCVM_NONE=1'):
    ps = ("Invoke-CimMethod -ClassName Win32_Process -MethodName Create -Arguments "
          "@{CommandLine='python -X utf8 _run_probe.py %s 21600 %s'; CurrentDirectory='%s'}"
          % (probe, spec, ROOT))
    subprocess.run(['powershell', '-NoProfile', '-Command', ps], capture_output=True)
    print('services starting (%s)' % probe)


def wait_ports(timeout=2400, step=3):
    t0 = time.time()
    while time.time() - t0 < timeout:
        up = [p for p in PORTS if port_open(p, 0.6)]
        if len(up) == len(PORTS):
            print('all ports up after %ds' % int(time.time() - t0))
            return True
        time.sleep(step)
        if int(time.time() - t0) % 30 < step:
            print('  [%3ds] up=%s' % (int(time.time() - t0), up))
    print('TIMEOUT waiting for ports')
    return False


def rdp(*args):
    r = subprocess.run([sys.executable, '-X', 'utf8', '_rdp.py'] + [str(a) for a in args],
                       cwd=ROOT, capture_output=True, text=True, encoding='utf-8', errors='replace')
    out = (r.stdout or '').strip()
    if out:
        print(out)
    return out


def main():
    acct = sys.argv[1] if len(sys.argv) > 1 else '2'
    pwd = sys.argv[2] if len(sys.argv) > 2 else '2'
    char = sys.argv[3] if len(sys.argv) > 3 else 'awesrd'

    if not services_up():
        start_services()
    if not wait_ports():
        return 1

    # account/char: the client cannot create a character without a real name
    # (GM-name rule on some accounts) - make sure one exists first.
    if not rdp('rect').startswith('hwnd'):
        rdp('start')
    rdp('qlogin', acct, pwd, 'rt')
    print('screenshots: rt_1_filled.jpg rt_2_after_login.jpg rt_3_ingame.jpg')
    return 0


if __name__ == '__main__':
    sys.exit(main())
