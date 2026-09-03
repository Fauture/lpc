# -*- coding: utf-8 -*-
"""Start aaa/dba/ccs/gs from build/Debug, sample progress for N seconds, then
kill.  Prints a compact per-service report (Load progress, ports up, storms)
and optionally writes a JSON snapshot for later comparison.

Usage: python tools/boot_smoke.py [-w seconds] [-j out.json] [-e exe]
"""
import argparse, io, json, os, re, socket, subprocess, sys, time
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('-w', type=int, default=20)
    ap.add_argument('-j', default='')
    ap.add_argument('-e', default='build/Debug/lpcvm.exe')
    ap.add_argument('-o', default='regress', help='dir for per-service logs')
    a = ap.parse_args()
    os.makedirs(a.o, exist_ok=True)
    env = dict(os.environ)
    for k in ['LPCVM_TRACE', 'LPCVM_INHINIT', 'LPCVM_EVENT_TRACE', 'LPCVM_TRACE_C8',
              'LPCVM_NO_CONST', 'LPCVM_LOCK_TRACE', 'LPCVM_STUCK_TRACE', 'LPCVM_EFUN_PROF',
              'LPCVM_STEP_PROF', 'LPCVM_CLONE_PROF', 'LPCVM_LOAD_PROF', 'LPCVM_SVC_TRACE',
              'LPCVM_DBG_OBJVAR', 'LPCVM_SLOW_OP']:
        env.pop(k, None)
    procs = []
    for name in ['aaa', 'dba', 'ccs', 'gs']:
        logf = open(os.path.join(a.o, 'boot_%s.txt' % name), 'wb')
        p = subprocess.Popen([a.e, 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                             stdout=logf, stderr=logf, env=env)
        procs.append({'name': name, 'p': p, 'f': logf})
        time.sleep(2)
    print('started 4 services (window=%ds)' % a.w, flush=True)

    def portup(port):
        s = socket.socket(); s.settimeout(0.25)
        try:
            s.connect(('127.0.0.1', port)); s.close(); return True
        except Exception:
            return False

    PORTS = {'aaa': 8001, 'dba': 8120, 'ccs': 8110, 'gs': 8160}
    t0 = time.time()
    samples = []
    while time.time() - t0 < a.w:
        time.sleep(2)
        el = time.time() - t0
        row = {'t': round(el, 1)}
        for pr in procs:
            name = pr['name']
            path = os.path.join(a.o, 'boot_%s.txt' % name)
            txt = ''
            try:
                with open(path, 'rb') as f:
                    txt = f.read().decode('utf-8', errors='replace')
            except Exception:
                pass
            row[name] = {
                'loads': len(re.findall(r'^Load: ', txt, re.M)),
                'port': portup(PORTS[name]),
                'alive': pr['p'].poll() is None,
                'mb': round(os.path.getsize(path) / 1048576.0, 3) if os.path.exists(path) else 0,
                'last': '',
            }
            m = re.findall(r'^Load: (\S+\.c)\.\.\. OK', txt, re.M)
            if m:
                row[name]['last'] = m[-1]
        samples.append(row)
        marks = ' '.join('%s%s' % (n, 'U' if row[n]['port'] else '') for n in ['aaa', 'dba', 'ccs', 'gs'])
        print('[%4.0fs] ports:%s loads aaa=%d dba=%d ccs=%d gs=%d' %
              (el, marks, row['aaa']['loads'], row['dba']['loads'],
               row['ccs']['loads'], row['gs']['loads']), flush=True)

    # summary at the end
    summary = {}
    for pr in procs:
        name = pr['name']
        path = os.path.join(a.o, 'boot_%s.txt' % name)
        txt = ''
        try:
            with open(path, 'rb') as f:
                txt = f.read().decode('utf-8', errors='replace')
        except Exception:
            pass
        loads = len(re.findall(r'^Load: ', txt, re.M))
        # most-repeated line (spam/storm detection) over the tail
        lines = txt.splitlines()[-3000:]
        from collections import Counter
        cnt = Counter(lines)
        top = cnt.most_common(3)
        storm = [(l[:90], n) for l, n in top if n >= 50]
        summary[name] = {
            'alive': pr['p'].poll() is None,
            'port': portup(PORTS[name]),
            'loads': loads,
            'mb': round(os.path.getsize(path) / 1048576.0, 3),
            'storms': storm,
        }
        print('%s: %s port=%s loads=%d mb=%.2f %s' %
              (name, 'RUNNING' if summary[name]['alive'] else 'EXITED',
               summary[name]['port'], loads, summary[name]['mb'],
               (' STORM ' + str(storm)) if storm else ''), flush=True)
    try:
        import pymysql
        c = pymysql.connect(host='192.168.200.128', user='root', password='123456',
                            charset='latin1', connect_timeout=5)
        cur = c.cursor()
        cur.execute("SELECT COUNT(*) FROM dl_ddb_1_test.data WHERE path LIKE '%%douchong%%' OR path LIKE '%%lgxy%%'")
        n = cur.fetchone()[0]
        cur.execute('SELECT COUNT(*) FROM dl_ddb_1_test.data')
        t = cur.fetchone()[0]
        c.close()
        summary['db_dc_lg'] = n
        summary['db_total'] = t
        print('DB dl_ddb_1_test.data dc/lg=%d total=%d' % (n, t), flush=True)
    except Exception as ex:
        summary['db'] = 'ERR %s' % ex
        print('DB unavailable: %s' % ex, flush=True)
    for pr in procs:
        if pr['p'].poll() is None:
            pr['p'].kill()
        pr['f'].close()
    if a.j:
        with open(a.j, 'w', encoding='utf-8') as f:
            json.dump({'window': a.w, 'samples': samples, 'summary': summary}, f,
                      ensure_ascii=False, indent=1)
        print('json -> %s' % a.j, flush=True)

main()
