# -*- coding: utf-8 -*-
import sys, io, time
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
sys.path.insert(0, 'tools')
from gameshell_rpc import GameShellRpcClient

c = None
for i in range(6):
    try:
        c = GameShellRpcClient(host='192.168.200.128', port=1027, password='123456', timeout=20)
        c.connect()
        break
    except Exception as e:
        print('try fail', str(e)[:100]); time.sleep(3)
if not c:
    sys.exit(1)


def q(expr, wait=30):
    try:
        return c.eval(expr, wait=wait)
    except Exception as e:
        return 'ERROR: %s' % e


names = ['爵卞玄端服·新郎', '爵卞玄端服·新娘', '十年一梦套装·女', '绫罗幻形镜']
for nm in names:
    print('###', nm)
    print('basic:', q('find_basic_object_by_import_name("%s")' % nm))
    print('gender:', q('find_basic_object_by_import_name("%s")->query("gender")' % nm))
    print('name:', q('find_basic_object_by_import_name("%s")->query("name")' % nm))
    print('file_id:', q('find_basic_object_by_import_name("%s")->query("file_id")' % nm))
    print('dbase keys gender? whole dbase sample:')
    print(q('save_string(find_basic_object_by_import_name("%s")->query_entire_dbase())' % nm)[:400])
try:
    c.close()
except Exception:
    pass
