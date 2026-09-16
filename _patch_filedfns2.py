# -*- coding: utf-8 -*-
"""Extend the [filed-call] diagnostics to the task chain.

That probe already prints obj / vm / vars / varsOwner / varsIsOwn / ownedFn /
lookup(func|byIndex) / this / frames / chain, and it is what cracked the 9026
bug (a cross-object inline call running with the CALLER's variable block, so the
callee read the wrong globals).  query_task_log reads taskd's global
`all_templates`, so if it runs with the char's variable block the lookup misses
and it returns "" - exactly the observed symptom.
"""
import io

p = 'src/original_vm.cpp'
d = io.open(p, encoding='utf-8', errors='replace').read()
nl = '\r\n' if '\r\n' in d else '\n'

old = ('"fetch_data", "write_data", "delete_content_to",\r\n'
       '                                                     "delete_content", "daemon", "dba_comm", "operate_done",\r\n'
       '                                                     "msg_operate_done"};')
if old not in d:
    old = old.replace('\r\n', '\n')
    assert old in d, 'filedFns anchor not found'
new = ('"fetch_data", "write_data", "delete_content_to",\r\n'
       '                                                     "delete_content", "daemon", "dba_comm", "operate_done",\r\n'
       '                                                     "msg_operate_done", "query_task_log", "query_task_tip",\r\n'
       '                                                     "query_task_desc", "query_task", "get_task_class",\r\n'
       '                                                     "send_task_log"};')
if nl == '\n':
    new = new.replace('\r\n', '\n')
d = d.replace(old, new, 1)
io.open(p, 'w', encoding='utf-8', newline='').write(d)
print('filedFns extended with the task chain')
