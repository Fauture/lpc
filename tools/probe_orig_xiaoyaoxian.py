# -*- coding: utf-8 -*-
"""Query the LIVE ORIGINAL GS (port 1027) about the 逍遥仙 NPC: how its basic
object / import / file_id are wired, and what named.npc_ob holds."""
import sys, io, time
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
sys.path.insert(0, 'tools')
from gameshell_rpc import GameShellRpcClient


def connect(port=1027, tries=6):
    for i in range(tries):
        try:
            c = GameShellRpcClient(host='192.168.200.128', port=port,
                                   password='123456', timeout=20)
            b, w = c.connect()
            print('connected')
            return c
        except Exception as e:
            print('try %d fail: %s' % (i, str(e)[:120]))
            time.sleep(4)
    return None


c = connect()
if not c:
    sys.exit(1)


def q(expr, wait=25):
    try:
        r = c.eval(expr, wait=wait)
        return r
    except Exception as e:
        return 'ERROR: %s' % e


# 1. what does named.npc_ob hold for 逍遥仙
print('=== find_npc_ob("逍遥仙") ===')
print(q('"/gs/daemons/named.c"->find_npc_ob("逍遥仙")'))
print('=== object_name of it ===')
print(q('object_name("/gs/daemons/named.c"->find_npc_ob("逍遥仙"))'))
print('=== its import name / file_id / name / basic_name ===')
print(q('"/gs/daemons/named.c"->find_npc_ob("逍遥仙")->get_import_name()'))
print(q('"/gs/daemons/named.c"->find_npc_ob("逍遥仙")->query("file_id")'))
print(q('"/gs/daemons/named.c"->find_npc_ob("逍遥仙")->query("name")'))
print(q('"/gs/daemons/named.c"->find_npc_ob("逍遥仙")->query("basic_name")'))
print('=== template of the placed npc (object_template) ===')
print(q('object_name(object_template("/gs/daemons/named.c"->find_npc_ob("逍遥仙")))'))
print('=== is_basic_object / my_import_name via basic object lookups ===')
print(q('find_basic_object_by_import_name("tianyongcheng/xiaoyaoxian")'))
print(q('object_name(find_basic_object_by_import_name("tianyongcheng/xiaoyaoxian"))'))
print(q('find_basic_object_by_import_name("tianyongcheng/xiaoyaoxian.c")'))
print(q('object_name(find_basic_object_by_import_name("tianyongcheng/xiaoyaoxian.c"))'))
print('=== the basic object identity fields ===')
print(q('find_basic_object_by_import_name("tianyongcheng/xiaoyaoxian")->get_import_name()'))
print(q('find_basic_object_by_import_name("tianyongcheng/xiaoyaoxian")->query("file_id")'))
print(q('find_basic_object_by_import_name("tianyongcheng/xiaoyaoxian")->query("name")'))
print('=== how many named npc_ob entries / does 泰煞守卫 have its own ===')
print(q('"/gs/daemons/named.c"->find_npc_ob("泰煞守卫")'))
print(q('object_name("/gs/daemons/named.c"->find_npc_ob("泰煞守卫"))'))
print('=== does a generic npc basic exist under the key? any xiaoyaoxian file .o? ===')
print(q('get_dir("/gs/clone/npc/tianyongcheng/xiaoyaoxian.o")'))
print(q('find_object("/gs/clone/npc/tianyongcheng/xiaoyaoxian.c")'))
print(q('find_object("/gs/clone/npc/basic.c")'))
try:
    c.close()
except Exception:
    pass
