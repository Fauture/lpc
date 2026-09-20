# -*- coding: utf-8 -*-
"""Every N minutes: close the real client, reopen it, try to log in.

Loops until the login stops reaching the game (the reported symptom is
"通讯错误" right after selecting the line) or the time budget runs out.  On the
first failure it dumps the evidence (screenshot + client log + gs log tail) and
exits, so the failure can be worked on with the gs still in its failing state.

usage: python -X utf8 _login10.py [interval_min] [max_hours] [acct] [pwd] [char]
       defaults: 10 min, 8 h, 1/1/awesrd
"""
import ctypes
import ctypes.wintypes as wt
import io
import os
import re
import subprocess
import sys
import time

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

LOGFILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), '_login10.log')
_logfh = io.open(LOGFILE, 'a', encoding='utf-8', newline='\n')


def log(msg):
    """Print AND append to _login10.log (the background job's stdout is only
    readable between reads, the file survives everything)."""
    print(msg, flush=True)
    try:
        _logfh.write(msg + '\n')
        _logfh.flush()
    except Exception:
        pass

INTERVAL = float(sys.argv[1]) if len(sys.argv) > 1 else 10.0
MAXH = float(sys.argv[2]) if len(sys.argv) > 2 else 8.0
ACCT = sys.argv[3] if len(sys.argv) > 3 else '1'
PWD = sys.argv[4] if len(sys.argv) > 4 else '1'
CHAR = sys.argv[5] if len(sys.argv) > 5 else 'awesrd'

ROOT = os.path.dirname(os.path.abspath(__file__))
CLIENT_LOG_DIR = r'E:\bf\BaiduNetdiskDownload\asktao1.60\log'
GS_LOG = os.path.join(ROOT, 'errfix1', 'live_gs.txt')   # set by the boot job
user32 = ctypes.windll.user32

FAIL_WORDS = ('通讯错误', '服务器连接中断', '无法连接', '连接服务器失败')


def run(args, timeout=300):
    return subprocess.run(args, cwd=ROOT, capture_output=True, text=True,
                          encoding='utf-8', errors='replace', timeout=timeout)


def client_titles():
    found = []

    @ctypes.WINFUNCTYPE(ctypes.c_bool, wt.HWND, wt.LPARAM)
    def cb(h, l):
        n = user32.GetWindowTextLengthW(h)
        b = ctypes.create_unicode_buffer(n + 1)
        user32.GetWindowTextW(h, b, n + 1)
        if '问道(1.60' in b.value:
            found.append((h, b.value))
        return True

    user32.EnumWindows(cb, 0)
    return found


def kill_client():
    run(['powershell', '-NoProfile', '-Command',
         'Get-Process asktao.mod -ErrorAction SilentlyContinue | Stop-Process -Force'])
    for _ in range(20):
        if not client_titles():
            return True
        time.sleep(0.5)
    return False


def newest_client_log():
    try:
        files = [os.path.join(CLIENT_LOG_DIR, f) for f in os.listdir(CLIENT_LOG_DIR)
                 if f.endswith('.txt')]
        return max(files, key=os.path.getmtime)
    except Exception:
        return None


def read_new(path, offset):
    try:
        with io.open(path, 'rb') as f:
            f.seek(offset)
            return f.read().decode('gbk', 'replace')
    except Exception:
        return ''


def gs_uptime_min():
    try:
        txt = io.open(GS_LOG, 'rb').read().decode('utf-8', 'replace')
    except Exception:
        return -1.0
    ms = re.findall(r'\[echo\] (?:recv|reply) cno=\d+ tick=\d+ period=\d+ t=(\d+)ms', txt)
    return (int(ms[-1]) / 60000.0) if ms else -1.0


def wait_gs(max_min=30.0):
    import socket
    t0 = time.time()
    while time.time() - t0 < max_min * 60:
        s = socket.socket()
        s.settimeout(2)
        try:
            s.connect(('127.0.0.1', 8160))
            s.close()
            return True
        except Exception:
            pass
        finally:
            try:
                s.close()
            except Exception:
                pass
        time.sleep(15)
    return False


def attempt(n):
    tag = 'L%02d' % n
    clog = newest_client_log()
    off = os.path.getsize(clog) if clog and os.path.exists(clog) else 0
    log('== attempt %d: closing client' % n)
    kill_client()
    time.sleep(2)
    r = run([sys.executable, '-X', 'utf8', '_rdp.py', 'start'], timeout=180)
    log('   start: %s' % (r.stdout or '').strip()[:120])
    time.sleep(8)
    r = run([sys.executable, '-X', 'utf8', '_rdp.py', 'auto', ACCT, PWD, CHAR, tag], timeout=420)
    tail = [l for l in (r.stdout or '').splitlines() if 'after' in l or 'state=' in l or 'WARN' not in l]
    log('   auto: %s' % ' | '.join(x.strip() for x in tail[-4:]))
    # Wait for the game world: entering the game can be slow (the reported
    # failure is 通讯错误 right after the line/char choice), so poll the window
    # title and record HOW LONG the entry took - a rising latency is the
    # degradation that precedes the failure.
    t_enter = time.time()
    ingame = False
    title = ''
    while time.time() - t_enter < 60:
        titles = client_titles()
        title = titles[0][1] if titles else '<no client window>'
        if ('[%s]' % CHAR) in title:
            ingame = True
            break
        time.sleep(3)
    enter_s = time.time() - t_enter
    newlog = read_new(clog, off) if clog else ''
    bad = [w for w in FAIL_WORDS if w in newlog]
    shot = os.path.join(ROOT, '_login_%s.jpg' % tag)
    run([sys.executable, '-X', 'utf8', '_shot.py', shot, '700', '55', 'client'], timeout=120)
    up = gs_uptime_min()
    verdict = 'OK  ' if (ingame and not bad) else 'FAIL'
    log('[%s] attempt %d uptime=%.1fmin -> %s ingame=%s enter=%.1fs title=%s bad=%s shot=%s'
        % (time.strftime('%H:%M:%S'), n, up, verdict, ingame, enter_s, title.strip(), bad,
           os.path.basename(shot)))
    if newlog.strip():
        interesting = [l for l in newlog.splitlines()
                       if any(w in l for w in FAIL_WORDS) or l.strip().startswith('09/')]
        for l in interesting[-6:]:
            log('     client-log: %s' % l[:150])
    return verdict == 'OK  ', bad, newlog


def main():
    log('login monitor: every %.0f min, up to %.1f h, account %s char %s'
        % (INTERVAL, MAXH, ACCT, CHAR))
    if not wait_gs(30):
        log('gs 8160 never opened - aborting monitor')
        return 1
    log('gs 8160 is up (uptime %.1f min) - starting attempts' % gs_uptime_min())
    t0 = time.time()
    n = 0
    while time.time() - t0 < MAXH * 3600:
        n += 1
        ok, bad, newlog = attempt(n)
        if not ok:
            log('=== FAILURE: login did not reach the game (bad=%s) ===' % bad)
            try:
                txt = io.open(GS_LOG, 'rb').read().decode('utf-8', 'replace').splitlines()
                log('--- gs log tail ---')
                for l in txt[-25:]:
                    log('   %s' % l[:180])
            except Exception as e:
                log('gs log read failed: %s' % e)
            log('--- client log tail ---')
            for l in (newlog or '').splitlines()[-15:]:
                log('   %s' % l[:180])
            return 3
        left = INTERVAL * 60 - (time.time() - t0 - (n - 1) * INTERVAL * 60)
        time.sleep(max(10.0, INTERVAL * 60 - 60))   # attempt itself takes ~1 min
    log('monitor finished without failure after %.1f h' % MAXH)
    return 0


sys.exit(main())
