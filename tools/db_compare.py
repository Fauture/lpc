# -*- coding: utf-8 -*-
import pymysql

def load(db):
    c = pymysql.connect(host='192.168.200.128', user='root', password='123456', database=db, charset='latin1')
    cur = c.cursor()
    cur.execute('SELECT COUNT(*) FROM data')
    total = cur.fetchone()[0]
    cur.execute("SELECT path, COUNT(*) FROM data GROUP BY path ORDER BY COUNT(*) DESC")
    rows = dict(cur.fetchall())
    c.close()
    return total, rows

ref_total, ref = load('dl_ddb_1')
test_total, test = load('dl_ddb_1_test')
print('ref total:', ref_total, ' test total:', test_total)
print('\n--- paths in REF but missing/less in TEST ---')
for p, n in sorted(ref.items(), key=lambda kv: -kv[1]):
    tn = test.get(p, 0)
    if tn != n:
        print(f'{p:45s} ref={n:6d} test={tn:6d}')
print('\n--- paths only in TEST ---')
for p, n in sorted(test.items(), key=lambda kv: -kv[1]):
    if p not in ref:
        print(f'{p:45s} test={n:6d}')
