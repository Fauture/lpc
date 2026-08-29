# -*- coding: utf-8 -*-
"""Connect to the ORIGINAL aaa telnet shell and probe efun behaviour."""
import sys
sys.path.insert(0, r'C:\Users\Administrator\Desktop\lpc\tools')
import gameshell_rpc

rpc = gameshell_rpc.GameShellRpcClient(host='192.168.200.128', port=1024,
                                       password='123456', timeout=15.0)
banner, welcome = rpc.connect()
print('CONNECTED')
print('banner:', banner.strip())
print('welcome:', welcome.strip())

tests = [
    ('ctime fmt3', r'write(ctime(time(), 3) + "\n")'),
    ('ctime fmt2', r'write(ctime(time(), 2) + "\n")'),
    ('ctime fmt1', r'write(ctime(time(), 1) + "\n")'),
    ('ctime default', r'write(ctime(time()) + "\n")'),
    ('save ({}):', r'write(save_string(({})) + "\n")'),
    ('save ({({})}):', r'write(save_string(({({})})) + "\n")'),
    ('save ({([])}):', r'write(save_string(({([ ])})) + "\n")'),
    ('save ([]):', r'write(save_string(([ ])) + "\n")'),
    ('save 0:', r'write(save_string(0) + "\n")'),
    ('save undef:', r'mixed x; write(save_string(x) + "\n")'),
    ('save "ab":', r'write(save_string("ab") + "\n")'),
    ('make32(a,b):', r'write(sprintf("%O\n", make32(0x11223344, 0x55667788)))'),
    ('make16(a,b):', r'write(sprintf("%O\n", make16(0x1122, 0x3344)))'),
    ('make8(a,b):', r'write(sprintf("%O\n", make8(0x11, 0x22)))'),
    ('make32i(a):', r'write(sprintf("%O\n", make32i(0x11223344)))'),
    ('expand_macro CONFIG:', r'write(sprintf("%O\n", expand_macro("CONFIG")))'),
    ('expand_macro LV_NONE:', r'write(sprintf("%O\n", expand_macro("LV_NONE")))'),
    ('expand_macro MAX_CONNECTIONS:', r'write(sprintf("%O\n", expand_macro("MAX_CONNECTIONS")))'),
    ('sizeof buffer:', r'write(sprintf("%O\n", sizeof("abc")))'),
    ('if ({}):', r'if (({})) { write("array truthy\n"); } else { write("array FALSY\n"); }'),
    ('if ([]):', r'if (([ ])) { write("map truthy\n"); } else { write("map FALSY\n"); }'),
    ('if ("")', r'if ("") { write("empty string truthy\n"); } else { write("empty string FALSY\n"); }'),
    ('get_timer scheme:', r'write(sprintf("%O\n", get_timer("scheme_timer")))'),
    ('get_timer stat:', r'write(sprintf("%O\n", get_timer("stat_cmds")))'),
]

for title, expr in tests:
    print('\n==== %s ====' % title)
    print('>>>', expr)
    try:
        out = rpc.call(expr, wait=6.0)
        print('=>', gameshell_rpc.fix_mojibake(out))
    except Exception as e:
        print('ERR:', e)

rpc.close()
print('\nDONE')
