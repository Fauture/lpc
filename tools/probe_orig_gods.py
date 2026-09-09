# -*- coding: utf-8 -*-
"""Query ORIGINAL GS where 青龙/白虎/朱雀/玄武/雷神/花神 NPC instances live."""
import sys, io, time
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
sys.path.insert(0, 'tools')
from gameshell_rpc import GameShellRpcClient


def connect(tries=6):
    for i in range(tries):
        try:
            c = GameShellRpcClient(host='192.168.200.128', port=1027,
                                   password='123456', timeout=20)
            c.connect()
            return c
        except Exception as e:
            print('try %d fail: %s' % (i, str(e)[:120]))
            time.sleep(4)
    return None


c = connect()
if not c:
    sys.exit(1)


def q(expr, wait=30):
    try:
        return c.eval(expr, wait=wait)
    except Exception as e:
        return 'ERROR: %s' % e


for nm in ['青龙', '白虎', '朱雀', '玄武', '雷神', '花神', '山神']:
    print('###', nm)
    ob = '("/gs/daemons/named.c"->find_npc_ob("%s"))' % nm
    print('obj:', q(ob))
    print('objname:', q('object_name(%s)' % ob))
    print('imp:', q('%s->get_import_name()' % ob))
    print('file_id:', q('%s->query("file_id")' % ob))
    print('env room:', q('environment(%s)' % ob))
    print('env name:', q('object_name(environment(%s))' % ob))
    print('pos:', q('"%d,%d" % ({ %s->query("x"), %s->query("y") })' % (ob, ob)))
    # basic object it was cloned from (its import target)
    print('basic by imp:', q('find_basic_object_by_import_name(%s->get_import_name())' % ob))
try:
    c.close()
except Exception:
    pass
