# -*- coding: utf-8 -*-
import subprocess, time, os, sys, io, socket
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
env = dict(os.environ)
env['LPCVM_TRACE'] = '1'
env['LPCVM_INHINIT'] = '1'
exe = 'build/Debug/lpcvm.exe'
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    logf = open('logsF_%s.txt' % name, 'wb')
    p = subprocess.Popen([exe, 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=logf, stderr=logf, env=env)
    procs.append((name, p, logf))
    time.sleep(3)
print('started 4', flush=True)
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
last = 0
for i in range(400):
    time.sleep(10)
    gs_size = os.path.getsize('logsF_gs.txt')
    n, t = dbs()
    mark = []
    if portup(8160): mark.append('GS8160')
    if portup(8110): mark.append('CCS8110')
    tail = ''
    try:
        with open('logsF_gs.txt', 'rb') as f:
            f.seek(max(0, gs_size - 300000))
            tail = f.read().decode('utf-8', errors='replace')
    except Exception:
        pass
    if tail.count('in=destruct obj=/gs/daemons/apexd.o') > 300:
        mark.append('APEXD-STORM')
    for pat in ['create dist start time failed', 'load dist start time failed', 'My configuration', 'load_dist_start_time']:
        if pat in tail:
            mark.append(pat[:12])
    print('[%4ds] %s gs=%.0fMB dc/lg=%d tot=%d' % (int(time.time() % 100000), ' '.join(mark), gs_size / 1048576, n, t), flush=True)
    if 'APEXD-STORM' in mark:
        print('STOP: apexd storm', flush=True)
        break
    if 'GS8160' in mark and n >= 900:
        print('GS READY + DB WRITES', flush=True)
        break
    if gs_size == last and portup(8110):
        # stall detection: no growth 3 polls
        pass
    last = gs_size
for name, p, logf in procs:
    if p.poll() is None:
        p.kill()
    logf.close()
