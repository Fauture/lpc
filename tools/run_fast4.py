# -*- coding: utf-8 -*-
import subprocess, time, os, sys, io, socket
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
env = dict(os.environ)
for k in ['LPCVM_TRACE', 'LPCVM_INHINIT', 'LPCVM_EVENT_TRACE', 'LPCVM_TRACE_C8', 'LPCVM_NO_CONST']:
    env.pop(k, None)
exe = 'build/Debug/lpcvm.exe'
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    logf = open('logsN_%s.txt' % name, 'wb')
    p = subprocess.Popen([exe, 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=env)
    procs.append((name, p, logf))
    time.sleep(3)
print('started 4 (no trace)', flush=True)
def portup(port):
    s = socket.socket(); s.settimeout(0.4)
    try:
        s.connect(('127.0.0.1', port)); s.close(); return True
    except Exception:
        return False
import pymysql
def dbs():
    try:
        c = pymysql.connect(host='192.168.200.128', user='root', password='123456', charset='latin1', connect_timeout=5)
        cur = c.cursor()
        cur.execute("SELECT COUNT(*) FROM dl_ddb_1_test.data WHERE path LIKE '%%douchong%%' OR path LIKE '%%lgxy%%'")
        n = cur.fetchone()[0]
        cur.execute('SELECT COUNT(*) FROM dl_ddb_1_test.data')
        t = cur.fetchone()[0]
        c.close()
        return n, t
    except Exception:
        return -1, -1
start = time.time()
for i in range(200):
    time.sleep(10)
    n, t = dbs()
    gs_size = 0
    try:
        gs_size = os.path.getsize('logsN_gs.txt')
    except Exception:
        pass
    mark = []
    if portup(8160): mark.append('GS8160')
    if portup(8110): mark.append('CCS8110')
    print('[%4ds] %s gslog=%.1fMB dc/lg=%d tot=%d' % (int(time.time() - start), ' '.join(mark), gs_size / 1048576, n, t), flush=True)
    if portup(8160) and n >= 900:
        print('GS UP + DB WRITES >=900', flush=True)
        break
    if time.time() - start > 1500:
        print('timeout 25min', flush=True)
        break
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()
