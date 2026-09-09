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

# 1) find the wulongshan room object (alias) and its npc list size/content
print('=== room via alias map (named or zone) ===')
print(q('"/gs/daemons/named.c"->find_room_ob("五龙山")'))
print(q('object_name("/gs/daemons/named.c"->find_room_ob("五龙山"))'))
print('npc attr size:', q('sizeof(("/gs/daemons/named.c"->find_room_ob("五龙山"))->query("npc"))'))
print('npc attr:', q('save_string(("/gs/daemons/named.c"->find_room_ob("五龙山"))->query("npc"))'))
# 2) gods locations summary via import-based full npc data (import/npc or the registered NPCs list)
print('=== npc_ob map sizes ===')
print(q('sizeof(("/gs/daemons/named.c"->query_all_entries()))'))
try:
    c.close()
except Exception:
    pass
