#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Step C1 (statement-level): delete only the debug `std::thread(...).detach();`.

    python -X utf8 _vmchain_codegen/stepC1_stmt.py

The enclosing `if (frame is query_task_log) { if (!dumped) { ...LOGGING... } }` also
does useful trace output, so the statement that must go is just the detached thread
that entered the VM from a non-scheduler thread.  Boundaries are found by paren
depth (statement start `std::thread(` -> the matching `.detach();`), guarded by
assertions: exactly one spawn, one detach, and no other `std::thread(` inside.
"""
import io

SRC = r'src\original_vm.cpp'


def main() -> int:
    raw = open(SRC, 'rb').read()
    crlf = raw.count(b'\r\n')
    lf = raw.count(b'\n')
    sep = '\r\n' if crlf * 2 >= lf else '\n'
    lines = raw.decode('utf-8').split(sep)

    gi = next((i for i, l in enumerate(lines) if 'static std::atomic<bool> dumped{false};' in l), -1)
    if gi < 0:
        print('guard not found; nothing to do')
        return 0
    start = next((i for i in range(gi, min(gi + 12, len(lines))) if 'std::thread(' in lines[i]), -1)
    if start < 0:
        print('thread statement not found')
        return 1
    depth = 0
    end = -1
    for i in range(start, len(lines)):
        depth += lines[i].count('(') - lines[i].count(')')
        if depth <= 0 and '.detach()' in lines[i]:
            end = i
            break
    if end < 0:
        print('detach not found')
        return 1

    removed = sep.join(lines[start:end + 1])
    if removed.count('std::thread(') != 1 or removed.count('.detach()') != 1:
        print('refusing: %d spawns / %d detaches in the removed text'
              % (removed.count('std::thread('), removed.count('.detach()')))
        return 1
    # replace the statement with a comment that records why it is gone
    indent = lines[start][:len(lines[start]) - len(lines[start].lstrip())]
    replacement = [
        indent + '// The one-shot `[treg]` dump used to run here on its OWN thread',
        indent + '(std::thread + 1500 ms sleep + callOriginalObjectFunction): LPC must',
        indent + 'only ever run on the scheduler thread, and this was the last such entry',
        indent + 'point in this file.  The trace lines above stay; the cross-thread dump',
        indent + 'is gone (see docs/scheduler_refactor_plan.md, Step C).',
    ]
    lines[start:end + 1] = replacement
    open(SRC, 'wb').write(sep.join(lines).encode('utf-8'))
    print('OK removed the statement (lines %d..%d) and left a comment' % (start + 1, end + 1))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
