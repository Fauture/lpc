# -*- coding: utf-8 -*-
"""Probe ORIGINAL dba(1025)/ccs(1026) telnet: save_string/var_type semantics."""
import sys
sys.path.insert(0, r'C:\Users\Administrator\Desktop\lpc\tools')
import gameshell_rpc

for port, tag in ((1025, 'DBA'), (1026, 'CCS')):
    print('================ %s :%d ================' % (tag, port))
    try:
        rpc = gameshell_rpc.GameShellRpcClient(host='192.168.200.128', port=port,
                                               password='123456', timeout=10.0)
        rpc.connect()
        print('CONNECTED %s' % tag)
    except Exception as e:
        print('CONNECT FAIL %s: %s' % (tag, e))
        continue
    tests = [
        ('save_string ({})', r'write(sprintf("save=%O\n", save_string(({}))))'),
        ('save_string ([])', r'write(sprintf("save=%O\n", save_string(([ ]))))'),
        ('var_type ({})', r'write(sprintf("vt=%s\n", var_type(({}))))'),
        ('var_type ([])', r'write(sprintf("vt=%s\n", var_type(([ ]))))'),
        ('var_raw_type ({})', r'write(sprintf("vrt=%d\n", var_raw_type(({}))))'),
        ('save_string(1.5)', r'write(sprintf("save=%O\n", save_string(1.5)))'),
        ('save_string(0)', r'write(sprintf("save=%O\n", save_string(0)))'),
        ('sizeof ([])', r'write(sprintf("sz=%d\n", sizeof(([ ]))))'),
    ]
    for title, expr in tests:
        print('\n== %s ==' % title)
        print('>>>', expr)
        try:
            out = rpc.call(expr, wait=6.0)
            print('=>', gameshell_rpc.fix_mojibake(out))
        except Exception as e:
            print('ERR:', e)
    rpc.close()
print('\nDONE')
