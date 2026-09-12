# -*- coding: utf-8 -*-
"""Print ONLY the online-mall buy sequence in log order (no noise)."""
import io
import re
import sys

path = sys.argv[1] if len(sys.argv) > 1 else 'mall1/live_gs.txt'
data = io.open(path, 'rb').read().decode('utf-8', 'replace')
seq = re.compile(r'fn=(buy_from_mall|buy_from_mall_vsucc|buy_from_mall_vfail|decrypt|precost_coin|'
                 r'try_lock_oper_coin|unlock_oper_coin|do_after_precost_coin|'
                 r'do_when_got_precost_coin|do_when_precost_timeout|clean_oper_info|send_result|'
                 r'is_legal_barcode|check_condition|open_mall|send_mall_info|get_oper_coin_lock|'
                 r'lock_oper_coin|is_need_input_coin_pwd|find_basic_object_by_import_name)\b')
send = re.compile(r'\[mall-send\].*cmd=0x(fd27|5109|6376|6377|3393|fd67)\b')
n = 0
for l in data.splitlines():
    if '[mall]' in l and seq.search(l):
        tag = 'ENTER' if ' ENTER' in l else 'RET  '
        m = re.search(r'fn=(\S+?) passed', l)
        fn = m.group(1) if m else '?'
        tail = ''
        if '->' in l:
            tail = ' -> ' + l.split('->', 1)[1][:70]
        args = ''
        if '[' in l and ']' in l:
            args = ' [' + l[l.index('[') + 1:l.rindex(']')][:150] + ']'
        print('%s %-26s%s%s' % (tag, fn, tail, args))
        n += 1
    elif send.search(l):
        m = re.search(r'cmd=0x(\w+) body=(\d+) hex=(\w+)', l)
        print('SEND  cmd=0x%s body=%s %s' % (m.group(1), m.group(2), m.group(3)[:60]))
        n += 1
    if n > 400:
        print('... (truncated)')
        break
