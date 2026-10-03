# -*- coding: utf-8 -*-
"""End-to-end check of the character save on logout:
  session 1: login/create/enter world  -> close socket
  wait 210s (logout timer is 180s)     -> the char's ddb row must be rewritten
  session 2: re-login                  -> login must succeed and the newbie
                                          story (0x20D1) must NOT re-trigger
"""
import io, re, socket, subprocess, sys, time
import pymysql

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
HOST = '192.168.200.1'


def port_open(p):
    s = socket.socket(); s.settimeout(0.8)
    try:
        s.connect((HOST, p)); s.close(); return True
    except Exception:
        return False


def wait_port(p, minutes=15):
    end = time.time() + minutes * 60
    while time.time() < end:
        if port_open(p):
            return True
        time.sleep(8)
    return False


def run_probe(seconds=12):
    r = subprocess.run([sys.executable, '-X', 'utf8', '_enter_probe.py', HOST, '8001', '1', '1', 'awesrd', str(seconds)],
                       capture_output=True, text=True, encoding='utf-8', errors='replace')
    out = (r.stdout or '') + (r.stderr or '')
    cmds = [l for l in out.splitlines() if 'WINDOW total' in l]
    res = [l for l in out.splitlines() if l.startswith('RESULT')]
    return (cmds[-1] if cmds else ''), (res[-1] if res else ''), out


def user_rows():
    c = pymysql.connect(host='192.168.200.128', user='root', password='123456',
                        database='dl_ddb_1_test', charset='utf8')
    cur = c.cursor()
    cur.execute("SELECT path,name,branch,LENGTH(content),time FROM data "
                "WHERE path='user' ORDER BY time")
    rows = cur.fetchall()
    cur.execute("SELECT gid,name,time FROM basic_char_info")
    chars = cur.fetchall()
    c.close()
    return rows, chars


print('waiting for 8160 ...', flush=True)
if not wait_port(8160):
    print('!! 8160 never opened'); sys.exit(1)
print('port open', flush=True)
time.sleep(15)

print('=== session 1 ===', flush=True)
print(run_probe()[1], flush=True)
time.sleep(210)

print('=== ddb after logout ===', flush=True)
try:
    rows, chars = user_rows()
    for r in rows:
        print('  ', r, flush=True)
    for ch in chars:
        print('   char', ch, flush=True)
except Exception as e:
    print('   db error', e, flush=True)

print('=== session 2 (re-login) ===', flush=True)
cmds, res, out = run_probe()
print(res, flush=True)
print('   0x20d1 (story) present:', '0x20d1' in cmds, flush=True)
print('   0x2603 (task)  present:', '0x2603' in cmds, flush=True)
if 'FAIL' in res:
    print('--- last packets ---', flush=True)
    for l in out.splitlines()[-12:]:
        print('   ', l, flush=True)
