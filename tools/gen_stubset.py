# -*- coding: utf-8 -*-
# Generate src/stub_efuns.inc - the COMPLETE list of efuns that have no real
# dispatch branch anywhere (builtins.cpp call_builtin / original_vm.cpp
# callEfunWithArgs / interpreter.cpp callBuiltin).  Source of truth:
#   efun_table.h  : every efun index the driver/PAK can reference (573 names)
#   builtins.cpp  : if (name == "x" ...) branches = implemented there
#   original_vm.cpp: if (nm == "x" ...) branches = handled by the VM directly
#   interpreter.cpp: callBuiltin name == branches
# Anything left over is genuinely unimplemented; the stub tracker logs it when
# the running game calls it.  Run this after adding/removing any efun branch:
#   python tools/gen_stubset.py
import re, os, io, sys
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def read(p):
    with open(os.path.join(ROOT, p), 'r', encoding='utf-8', errors='replace') as f:
        return f.read()

# 1) full efun table names
tbl = read('src/efun_table.h')
names = re.findall(r'^\s*"([a-z_0-9]+)",\s*$', tbl, re.M)
assert len(names) > 400, len(names)

def names_in(p, pat):
    src = read(p)
    return set(re.findall(pat, src))

builtin_impl = names_in('src/builtins.cpp', r'name == "([a-z_0-9]+)"')
vm_impl = names_in('src/original_vm.cpp', r'(?:nm|names\[efun\]|names\[index\]|name) == "([a-z_0-9]+)"')
vm_impl |= names_in('src/original_vm.cpp', r'names\[efun\] == "([a-z_0-9]+)"')
interp_impl = names_in('src/interpreter.cpp', r'name == "([a-z_0-9]+)"')

impl = builtin_impl | vm_impl | interp_impl
# only names that appear in the efun table matter
impl &= set(names)
stub = [n for n in names if n not in impl]

# sanity: these must NOT be stubs (they have real implementations)
must_impl = ['sql_open_db', 'sql_query', 'sql_step', 'sql_finalize', 'sql_close_db',
             'std_life', 'std_mana', 'std_attack', 'std_defense', 'std_speed',
             'std_dodge', 'std_tao', 'dup', 'int32', 'expand_macro', 'mysql_query',
             'sleep', 'load_object', 'clone_object', 'set_as_daemon', 'relinquish']
missing = [m for m in must_impl if m in stub]
print('efun table: %d, implemented: %d, stub: %d' % (len(names), len(impl), len(stub)))
if missing:
    print('SANITY FAIL - these should NOT be stubs:', missing)

inc = os.path.join(ROOT, 'src', 'stub_efuns.inc')
with open(inc, 'w', encoding='utf-8') as f:
    for n in stub:
        f.write('            "%s",\n' % n)
print('wrote %s (%d entries)' % (inc, len(stub)))
print('--- stub list ---')
for n in stub:
    print(' ', n)
