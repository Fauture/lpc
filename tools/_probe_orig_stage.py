# -*- coding: utf-8 -*-
import sys, io
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
sys.path.insert(0, 'tools')
from gameshell_rpc import GameShellRpcClient
c = GameShellRpcClient(host='192.168.200.128', port=1027, password='123456', timeout=20)
b, w = c.connect()
print('connected')


def q(expr, wait=20):
    try:
        return c.call(expr, wait=wait)
    except Exception as e:
        return 'ERROR: %s' % str(e)[:150]


print('=== A: all_entries keys head 40 ===')
print(q('mixed zq1; mixed ksq1; int iq1; ksq1=keys(zq1="/gs/daemons/zoned.c"->query_entry("混合区域")->query_all_entries()); write(sprintf("n=" + "%d" + "\\n", sizeof(ksq1))); for(iq1=0;iq1<sizeof(ksq1)&&iq1<40;iq1++){ write(ksq1[iq1] + "|") }'))
print('=== B: is a plain child (baiguidi) in entries? value ===')
print(q('mixed zq2; zq2="/gs/daemons/zoned.c"->query_entry("混合区域"); write(sprintf("bgd=" + "%O" + "\\n", zq2->query_entry("百鬼地")));'))
print('=== C: 参观_/挑战_ clones present? ===')
print(q('mixed zq3; zq3="/gs/daemons/zoned.c"->query_entry("混合区域"); write(sprintf("cg=" + "%O" + "\\n", zq3->query_entry("参观_帮派名人堂"))); write(sprintf("tz=" + "%O" + "\\n", zq3->query_entry("挑战_帮派名人堂")));'))
print('=== D: keys containing 参观_ count ===')
print(q('mixed zq4; mixed ksq4; int iq4; int cnt4; cnt4=0; ksq4=keys(zq4="/gs/daemons/zoned.c"->query_entry("混合区域")->query_all_entries()); for(iq4=0;iq4<sizeof(ksq4);iq4++){ if(strsrch(ksq4[iq4], "参观_")>=0) cnt4=cnt4+1 } write(sprintf("cnt=" + "%d" + "\\n", cnt4));'))
try:
    c.close()
except Exception:
    pass
print('DONE')
