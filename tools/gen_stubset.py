# -*- coding: utf-8 -*-
# Generate src/stub_efuns.inc - the COMPLETE list of efuns the REFERENCE
# driver registers but this engine has no real dispatch branch for.
#
# Source of truth for "which efuns exist": the reference binary
# (Desktop/magic_Linux32x) itself:
#   * ELF .symtab  -> every vm_f_* efun implementation (604 symbols)
#   * its efun registration tables in .data (28-byte entries:
#     name@+0, fptr@+20, nargs@+24) -> the 561 efuns actually registered
#     (the 75 entries in the four VM-opcode tables are excluded).
#   _efun_real.json / _efun_report.txt hold that scan.
# Then subtract every efun this engine dispatches:
#   builtins.cpp / original_vm.cpp / interpreter.cpp / *.inc:
#     name == "x", nm == "x", names[efun] == "x"
# Anything left over is genuinely unimplemented; the stub tracker logs it
# when the running game calls it (so nothing stays silent).
# Run after adding/removing any efun branch:
#   python tools/gen_stubset.py
import re, os, io, sys, json
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def read(p):
    with open(os.path.join(ROOT, p), 'r', encoding='utf-8', errors='replace') as f:
        return f.read()

# 1) authoritative efun names
real_json = os.path.join(ROOT, '_efun_real.json')
if os.path.exists(real_json):
    names = sorted(json.load(open(real_json, encoding='utf-8')))
    src_desc = 'reference driver efun tables (_efun_real.json)'
else:
    tbl = read('src/efun_table.h')
    names = sorted(set(re.findall(r'^\s*"([a-z_0-9]+)",\s*$', tbl, re.M)))
    src_desc = 'src/efun_table.h (fallback)'
assert len(names) > 400, len(names)

def names_in(p, pat):
    return set(re.findall(pat, read(p)))

impl = set()
# NOTE the lookbehind: without it, identifiers such as funcName/fname/fileName
# would match "...name == \"x\"" and mark an LPC function name as an
# implemented efun, silently hiding a genuinely missing efun from the list.
for f in ('src/builtins.cpp', 'src/original_vm.cpp', 'src/interpreter.cpp',
          'src/efun_batch1.inc', 'src/efun_batch2.inc', 'src/map_module.cpp'):
    if not os.path.exists(os.path.join(ROOT, f)):
        continue
    impl |= names_in(f, r'(?<![A-Za-z0-9_])(?:name|nm|efun)\s*[=!]=\s*"([A-Za-z_][A-Za-z0-9_]*)"')
    impl |= names_in(f, r'names\[efun\]\s*[=!]=\s*"([A-Za-z_][A-Za-z0-9_]*)"')
impl &= set(names)
stub = [n for n in names if n not in impl]

# sanity: these must NOT be stubs (they have real implementations)
must_impl = ['sql_open_db', 'sql_query', 'sql_step', 'sql_finalize', 'sql_close_db',
             'std_life', 'std_mana', 'std_attack', 'std_defense', 'std_speed',
             'std_dodge', 'std_tao', 'dup', 'int32', 'expand_macro', 'mysql_query',
             'sleep', 'load_object', 'clone_object', 'set_as_daemon', 'relinquish',
             'move_object', 'find_reachable', 'sscanf', 'strwidth', 'hash',
             'net8', 'net16', 'net32', 'csv_parse', 'empty_value', 'to_unsigned_str']
missing = [m for m in must_impl if m in stub]
print('efun source: %s' % src_desc)
print('efuns: %d, implemented: %d, stub: %d' % (len(names), len(impl), len(stub)))
if missing:
    print('SANITY FAIL - these should NOT be stubs:', missing)

inc = os.path.join(ROOT, 'src', 'stub_efuns.inc')
with open(inc, 'w', encoding='utf-8') as f:
    for n in stub:
        f.write('            "%s",\n' % n)
print('wrote %s (%d entries)' % (inc, len(stub)))
