#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Step C1 (precise): remove the debug thread that ran LPC off the scheduler.

    python -X utf8 _vmchain_codegen/stepC1_precise.py

Last time this deletion took the whole enclosing statement with it (the block is
nested inside a `switch` case, so "delete until the next `}`" cut into the
structure).  This version:

  * locates the `static std::atomic<bool> dumped{false};` guard,
  * walks back to the enclosing `if (...query_task_log...)` line by BRACE MATCHING
    that statement's braces,
  * walks forward with the same brace counter to its true end,
  * verifies that the removed text contains exactly one `std::thread(` and the
    matching `.detach()`,
  * verifies the file's brace balance is unchanged afterwards.
"""
import io

SRC = r'src\original_vm.cpp'


def main() -> int:
    raw = open(SRC, 'rb').read()
    crlf = raw.count(b'\r\n')
    lf = raw.count(b'\n')
    sep = '\r\n' if crlf * 2 >= lf else '\n'
    text = raw.decode('utf-8')
    lines = text.split(sep)

    gi = next((i for i, l in enumerate(lines) if 'static std::atomic<bool> dumped{false};' in l), -1)
    if gi < 0:
        print('guard not found')
        return 0

    # find the enclosing if: the nearest line above with 'query_task_log' that
    # opens a brace
    start = -1
    for i in range(gi, max(0, gi - 12), -1):
        if 'query_task_log' in lines[i] and lines[i].rstrip().endswith('{'):
            start = i
            break
    if start < 0:
        print('enclosing if not found')
        return 1

    # brace matching from that line
    depth = 0
    end = -1
    for i in range(start, len(lines)):
        depth += lines[i].count('{') - lines[i].count('}')
        if depth == 0 and i > start:
            end = i
            break
    if end < 0:
        print('closing brace not found')
        return 1
    # include the trailing comment line above the if, if it belongs to it
    while start > 0 and lines[start - 1].lstrip().startswith('//') and \
            lines[start - 1].strip() != '//':
        start -= 1
        if not lines[start].lstrip().startswith('//'):
            break

    removed = sep.join(lines[start:end + 1])
    if removed.count('std::thread(') != 1 or removed.count('.detach()') != 1:
        print('refusing: removed text has %d thread spawns, %d detaches'
              % (removed.count('std::thread('), removed.count('.detach()')))
        return 1
    bal_before = text.count('{') - text.count('}')
    del lines[start:end + 1]
    out = sep.join(lines)
    bal_after = out.count('{') - out.count('}')
    if bal_before != bal_after:
        print('refusing: brace balance changed %d -> %d' % (bal_before, bal_after))
        return 1
    open(SRC, 'wb').write(out.encode('utf-8'))
    print('OK removed lines %d..%d (brace balance %d unchanged)' % (start + 1, end + 1, bal_after))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
