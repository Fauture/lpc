# -*- coding: utf-8 -*-
# Poll dl_ddb_1_test for game data while gs runs. Prints row counts per table
# and gs port/memory/process state every N seconds.
import subprocess, time, sys, os
import pymysql

OUT = 'db_poll_out.txt'
INTERVAL = 20
MAX = 90

def db_state():
    try:
        c = pymysql.connect(host='192.168.200.128', user='root', password='123456', charset='latin1', connect_timeout=6)
        cur = c.cursor()
        cur.execute('SHOW TABLES FROM dl_ddb_1_test')
        tabs = [r[0] for r in cur.fetchall()]
        counts = {}
        interesting = [t for t in tabs if any(k in t for k in ('lgxy', 'douchong', 'time_info', 'zhengdaodian'))]
        for t in interesting:
            try:
                cur.execute('SELECT COUNT(*) FROM dl_ddb_1_test.`%s`' % t)
                counts[t] = cur.fetchone()[0]
            except Exception:
                pass
        # path categories in data table (if exists)
        cur.execute("SELECT COUNT(*) FROM dl_ddb_1_test.data WHERE path LIKE '%%douchong%%'")
        dc = cur.fetchone()[0]
        cur.execute("SELECT COUNT(*) FROM dl_ddb_1_test.data WHERE path LIKE '%%lgxy%%' OR path LIKE '%%longgong%%'")
        lg = cur.fetchone()[0]
        cur.execute('SELECT COUNT(*) FROM dl_ddb_1_test.data')
        tot = cur.fetchone()[0]
        c.close()
        return counts, dc, lg, tot
    except Exception as e:
        return {}, -1, -1, -1

def gs_state():
    port = 0
    try:
        out = subprocess.check_output(['powershell', '-NoProfile', '-Command',
            "(Get-NetTCPConnection -State Listen -LocalPort 8160 -ErrorAction SilentlyContinue).Count"], text=True)
        port = int(out.strip() or 0)
    except Exception:
        pass
    ws = -1
    try:
        out = subprocess.check_output(['powershell', '-NoProfile', '-Command',
            "Get-CimInstance Win32_Process -Filter \"Name='lpcvm.exe'\" | Where-Object { $_.CommandLine -match 'lib_gs32' } | ForEach-Object { [math]::Round($_.WorkingSetSize/1MB) }"], text=True)
        vals = [int(x) for x in out.split() if x.strip().lstrip('-').isdigit()]
        if vals:
            ws = vals[-1]
    except Exception:
        pass
    return port, ws

start = time.time()
with open(OUT, 'a', encoding='utf-8') as o:
    for i in range(MAX):
        time.sleep(INTERVAL)
        counts, dc, lg, tot = db_state()
        port, ws = gs_state()
        size = 0
        try:
            size = os.path.getsize('logs_gsAX_w.txt')
        except Exception:
            pass
        line = '[%4ds] port8160=%d ws=%sMB gslog=%.0fMB data_total=%d douchong=%d lgxy=%d %s' % (
            int(time.time() - start), port, ws, size / 1048576, tot, dc, lg,
            ' '.join('%s=%d' % (k[:18], v) for k, v in counts.items()))
        print(line, flush=True)
        o.write(line + '\n')
        o.flush()
        if dc >= 900:
            print('DOUCHONG >=900 REACHED', flush=True)
            break
