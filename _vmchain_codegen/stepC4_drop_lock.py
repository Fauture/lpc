#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Step C4: delete the per-VM execution lock machinery.

    python -X utf8 _vmchain_codegen/stepC4_drop_lock.py

WHY IT IS SAFE NOW (all measured, not assumed):
  * `noteVmEntry` probe: 40/40 entries during boot and every entry under real
    client load reported `schedulerThread=1`, 0 off-thread entries (section 11);
  * the telnet daemon became a scheduler process in C2, and C3 removed the last
    pending-thread worker pools, so this file no longer creates a thread that can
    enter a VM (section 12);
  * `create_thread`, timers, constructions and service replies all go through
    `scheduler().spawn*`, and network packets through `execOnMain`.

WHAT GOES AWAY
  `execMutex_`, `execDepth_`, `tlsExecLocksHeld()`, `ExecGuard`, the six guard
  instantiations, and the release/re-take loops inside `runDetached` (which
  existed only to drop and re-acquire that mutex around a cross-object call).

WHAT STAYS
  `runDetached`'s real work: moving the caller's stack/frames into
  `es_->detachedStack` (and back), so a nested cross-object call gets a clean
  stack - that part is about STATE, not about threads, and is still needed.
"""
import io
import re

SRC = r'src\original_vm.cpp'

GUARD_OLD_START = '    // How many times THIS thread actually holds a VM execution lock.'
GUARD_OLD_END = '        ExecGuard& operator=(const ExecGuard&) = delete;\n    };'


def main() -> int:
    raw = open(SRC, 'rb').read()
    crlf = raw.count(b'\r\n')
    lf = raw.count(b'\n')
    sep = '\r\n' if crlf * 2 >= lf else '\n'
    t = raw.decode('utf-8')
    did = []

    # 1. drop the ExecGuard struct + tls counter
    i = t.find(GUARD_OLD_START.replace('\n', sep))
    j = t.find(GUARD_OLD_END.replace('\n', sep), i)
    if i < 0 or j < 0:
        print('ExecGuard block not found (%d/%d)' % (i, j))
        return 1
    j += len(GUARD_OLD_END.replace('\n', sep))
    # eat the following newline
    j = t.find(sep, j) + len(sep)
    t = t[:i] + t[j:]
    did.append('ExecGuard')

    # 2. drop the member declarations
    member_block = '''    // Serialize execution per VM: a VM (especially the shared "<main>" copy) can
    // be entered by several threads, so one recursive mutex guards it (recursive
    // because one VM's execute() may re-enter itself through closures/calls).
    mutable std::recursive_mutex execMutex_;
    // Recursion depth of execMutex_ for the CURRENT holder thread (only the
    // holder touches it).  runDetached() releases all of these to make a
    // cross-object call without holding this VM's lock, avoiding the AB-BA
    // deadlock between two threads calling into each other's objects.
    int execDepth_ = 0;'''
    for cand in (member_block, member_block.replace('    // Serialize execution per VM: a VM (especially the shared "<main>" copy) can\n',
                                                    '')):
        o = cand.replace('\n', sep)
        if t.count(o) == 1:
            t = t.replace(o, '', 1)
            did.append('members')
            break
    else:
        # fall back to the two declaration lines only
        for line in ('    mutable std::recursive_mutex execMutex_;', '    int execDepth_ = 0;'):
            o = line.replace('\n', sep)
            if t.count(o) == 1:
                t = t.replace(o, '', 1)
                did.append('member:' + line.strip()[:24])

    # 3. runInit's lock
    o = '        std::lock_guard<std::recursive_mutex> lock(execMutex_);\n'
    if t.count(o.replace('\n', sep)) == 1:
        t = t.replace(o.replace('\n', sep), '', 1)
        did.append('runInit lock')

    # 4. runDetached: depth bookkeeping + release/re-take loops
    t = t.replace('        int d = execDepth_;' + sep, '', 1)
    t = t.replace('        execDepth_ = 0;' + sep, '', 1)
    t, n1 = re.subn(r'[ \t]*for \(int i = 0; i < d; \+\+i\) \{ if \(tlsExecLocksHeld\(\) > 0\) \{ --tlsExecLocksHeld\(\); execMutex_\.unlock\(\); \} \}' + re.escape(sep),
                    '', t)
    t, n2 = re.subn(r'[ \t]*for \(int i = 0; i < d; \+\+i\) \{ if \(i < d && tlsExecLocksHeld\(\) < i \+ 1\) \{ execMutex_\.lock\(\); \+\+tlsExecLocksHeld\(\); \} \}' + re.escape(sep),
                    '', t)
    t, n3 = re.subn(r'[ \t]*execDepth_ = d;' + re.escape(sep), '', t)
    did.append('runDetached loops (%d unlock, %d relock, %d depth)' % (n1, n2, n3))

    # 5. the six ExecGuard instantiations
    t, n4 = re.subn(r'[ \t]*ExecGuard lock\(execMutex_, execDepth_, [^\n]*\);' + re.escape(sep), '', t)
    did.append('guards (%d)' % n4)

    open(SRC, 'wb').write(t.encode('utf-8'))
    left = t.count('execMutex_') + t.count('execDepth_') + t.count('ExecGuard')
    print('OK removed: %s; references left: %d' % (', '.join(did), left))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
