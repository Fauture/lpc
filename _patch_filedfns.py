# -*- coding: utf-8 -*-
"""Extend the [filed-call] probe list with the delete/daemon entry points."""
import io

p = 'src/original_vm.cpp'
d = io.open(p, encoding='utf-8', errors='replace').read()
old = ('"create_content", "create_content_to", "process_request",\r\n'
       '                                                     "fetch_data", "write_data"};')
new = ('"create_content", "create_content_to", "process_request",\r\n'
       '                                                     "fetch_data", "write_data", "delete_content_to",\r\n'
       '                                                     "delete_content", "daemon", "dba_comm", "operate_done",\r\n'
       '                                                     "msg_operate_done"};')
if old not in d:
    old = old.replace('\r\n', '\n')
    new = new.replace('\r\n', '\n')
assert old in d, 'anchor not found'
d = d.replace(old, new, 1)
io.open(p, 'w', encoding='utf-8', newline='').write(d)
print('patched filedFns')
