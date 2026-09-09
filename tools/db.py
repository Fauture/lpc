import sys
import pymysql

HOST = '192.168.200.128'
USER = 'root'
PWD = '123456'

DB_ADB = 'dl_adb_all_test'
DB_DDB_TEST = 'dl_ddb_1_test'


def connect(db=None):
    return pymysql.connect(host=HOST, user=USER, password=PWD, charset='utf8', db=db)


def tables(cur, db):
    cur.execute('select table_name from information_schema.tables where table_schema=%s', (db,))
    return [r[0] for r in cur.fetchall()]


def count(db):
    c = connect()
    cur = c.cursor()
    total = 0
    for t in tables(cur, db):
        try:
            cur.execute('select count(*) from `%s`.`%s`' % (db, t))
            n = cur.fetchone()[0]
        except Exception as e:
            n = 'ERR ' + str(e)[:60]
        if isinstance(n, int) and n > 0:
            print('%-45s %d' % (t, n))
        total += (n if isinstance(n, int) else 0)
    print('total rows', db, total)
    c.close()


def reset(dbs):
    c = connect()
    cur = c.cursor()
    for db in dbs:
        for t in tables(cur, db):
            cur.execute('set foreign_key_checks=0')
            cur.execute('truncate table `%s`.`%s`' % (db, t))
        print('reset', db, 'done')
    c.commit()
    c.close()


def rows(db, tbl):
    c = connect()
    cur = c.cursor()
    cur.execute('select count(*) from `%s`.`%s`' % (db, tbl))
    print('%s.%s rows=%d' % (db, tbl, cur.fetchone()[0]))
    c.close()


if __name__ == '__main__':
    cmd = sys.argv[1] if len(sys.argv) > 1 else 'count-adb'
    if cmd == 'count-adb':
        count(DB_ADB)
    elif cmd == 'count-ddb':
        count(DB_DDB_TEST)
    elif cmd == 'reset':
        reset(sys.argv[2:])
    elif cmd == 'rows':
        rows(sys.argv[2], sys.argv[3])
