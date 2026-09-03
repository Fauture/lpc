# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')

def scan(path, patterns, label, limit=10):
    try:
        with open(path, 'rb') as f:
            data = f.read()
    except Exception as e:
        print(label, 'ERR', e); return
    text = data.decode('utf-8', errors='replace')
    lines = text.splitlines()
    print('=== %s: %d lines ===' % (label, len(lines)))
    hits = 0
    for i, l in enumerate(lines):
        for p in patterns:
            if re.search(p, l):
                print('L%d: %s' % (i+1, l[:160]))
                hits += 1
                break
        if hits >= limit:
            return

scan(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_fast.txt',
     [r'read_etc\b(?!_file)', r'17520', r'9331', r'9329', r'ev_readetcd', r'send_to_client.*17520',
      r'lgxy_server', r'try_to_connect_aaa', r'assure_config_agent'],
     'gs read_etc network')
print()
scan(r'C:\Users\Administrator\Desktop\lpc\logs_aaaAX_fast.txt',
     [r'17520', r'9331', r'9329', r'read_etc', r'cmd_cfg_read', r'msg_cfg_read_etc', r'config', r'send_message'],
     'aaa read_etc handler')
