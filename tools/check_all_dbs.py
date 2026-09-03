import pymysql

# 按 dba.ini 的配置检查所有库
for db in ['dl_ddb_1', 'dl_ddb_1_test', 'dl_ldb_1', 'dl_mdb_1', 'dl_adb_all_test', 'dl_mdb_all']:
    try:
        conn = pymysql.connect(host='192.168.200.128', user='root', password='123456',
                               database=db, charset='utf8')
        cur = conn.cursor()
        cur.execute("select table_name, table_rows from information_schema.tables where table_schema=%s order by table_rows desc", (db,))
        tables = cur.fetchall()
        print(f"===== {db} : {len(tables)} tables =====")
        total = 0
        for t, r in tables[:20]:
            print(f"  {t}: {r}")
        conn.close()
    except Exception as e:
        print(f"===== {db} : ERROR {e} =====")
