#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Step C1: delete the debug-only thread that entered LPC off the scheduler.

    python -X utf8 _vmchain_codegen/stepC1_drop_diag_thread.py

The block guarded by `static std::atomic<bool> dumped{false}` spawned a detached
`std::thread`, slept 1.5 s, and then called into the object registry
(`callOriginalObjectFunction`, `objectRegistry()`) from that thread.  It is a
one-shot debug dump (`[treg]`, added while chasing task templates) and it is the
only remaining place in this file where a non-scheduler thread touches VM state -
precisely what `execMutex_`/`execDepth_`/`runDetached` exist to survive, and what
stands in the way of deleting them.

Deleting it is behaviour-neutral for the server (the dump only printed diagnostic
lines) and removes the last such entry point in this translation unit.
"""
import io

SRC = r'src\original_vm.cpp'

GUARD = 'static std::atomic<bool> dumped{false};'
BLOCK_OPEN = 'if (!dumped.exchange(true)) {'


def main() -> int:
    raw = open(SRC, 'rb').read()
    crlf = raw.count(b'\r\n')
    lf = raw.count(b'\n')
    sep = '\r\n' if crlf * 2 >= lf else '\n'
    t = raw.decode('utf-8')
    lines = t.split(sep)

    gi = next((i for i, l in enumerate(lines) if GUARD in l), -1)
    if gi < 0:
        print('guard not found (already removed?)')
        return 0
    # find the enclosing `if (es_->frames.back().fn && ... query_task_log ...` line
    start = gi
    while start > 0 and 'query_task_log' not in lines[start]:
        start -= 1
    # walk back to the comment line that introduces it
    while start > 1 and lines[start - 1].lstrip().startswith('//'):
        start -= 1
    # find the closing brace of the `if (!dumped...)` block: first line whose
    # stripped content is '}' after gi
    end = gi
    while end < len(lines) and lines[end].strip() != '}':
        end += 1
    # and the closing brace of the outer `if (es_->frames...)` block
    end += 1
    while end < len(lines) and lines[end].strip() != '}':
        end += 1
    print('deleting lines %d..%d' % (start + 1, end + 1))
    for k in range(start, min(end + 1, start + 4)):
        print('   -', lines[k].strip()[:90])
    del lines[start:end + 1]
    open(SRC, 'wb').write(sep.join(lines).encode('utf-8'))
    print('OK diagnostic thread removed')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
