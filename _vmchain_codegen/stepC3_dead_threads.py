#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Step C3: delete the dead pending-thread machinery.

    python -X utf8 _vmchain_codegen/stepC3_dead_threads.py

Evidence that it is dead (all verified on this tree):
  * nothing pushes into `pendingThreads()` any more - `create_thread` goes through
    `scheduler().spawn()` / `spawnTelnet()`;
  * `runPendingOriginalThreads` has no caller anywhere in the repository;
  * `drainPendingOriginalThreads` is called only from `builtins.cpp`'s telnet
    accept path;
  * four real-client logs contain `pending-thr=0`, `pending threads=0`,
    `[thread] run=0`.

Removing the two worker pools takes the last `std::thread` instances out of this
translation unit, i.e. the last place where LPC could still run off the scheduler
thread - which is what the exec lock exists to survive.

Each block is removed by matching an exact context (not by "delete until the next
brace"), and every removal is followed by a brace-balance check.
"""
import io

SRC = r'src\original_vm.cpp'
HDR = r'src\original_vm.h'
BUILTINS = r'src\builtins.cpp'


def read(p):
    raw = open(p, 'rb').read()
    crlf = raw.count(b'\r\n')
    lf = raw.count(b'\n')
    return raw, raw.decode('utf-8'), ('\r\n' if crlf * 2 >= lf else '\n')


def cut_block(text, start_marker, end_marker, eol, label, expect_threads=0):
    """Remove from the line holding start_marker through the line holding end_marker."""
    i = text.find(start_marker.replace('\n', eol))
    if i < 0:
        print('  %s: start not found' % label)
        return text, False
    j = text.find(end_marker.replace('\n', eol), i)
    if j < 0:
        print('  %s: end not found' % label)
        return text, False
    j += len(end_marker.replace('\n', eol))
    # extend to the end of that line
    k = text.find(eol, j)
    if k > 0:
        j = k + len(eol)
    removed = text[i:j]
    if removed.count('std::thread(') != expect_threads:
        print('  %s: refusing, %d thread spawns inside' % (label, removed.count('std::thread(')))
        return text, False
    if removed.count('{') != removed.count('}'):
        print('  %s: refusing, unbalanced braces' % (label,))
        return text, False
    print('  %s: removed %d chars' % (label, len(removed)))
    return text[:i] + text[j:], True


def cut_function(text, signature, eol, label, expect_threads=0):
    """Remove a whole function: from `signature` to the line whose brace closes it.

    Brace counting ignores `//` comment tails on each line (the bodies contain
    braces inside comments, which is what made a "next closing brace" search cut
    the wrong range in earlier attempts).
    """
    i = text.find(signature.replace('\n', eol))
    if i < 0:
        print('  %s: signature not found' % label)
        return text, False
    lines = text[i:].split(eol)
    depth = 0
    end_line = -1
    for n, l in enumerate(lines):
        code = l.split('//')[0]
        depth += code.count('{') - code.count('}')
        if n > 0 and depth == 0:
            end_line = n
            break
    if end_line < 0:
        print('  %s: closing brace not found' % label)
        return text, False
    removed = eol.join(lines[:end_line + 1]) + eol
    # thread spawns here are either a std::thread construction or a
    # workers.emplace_back(lambda)
    spawns = removed.count('std::thread(') + removed.count('emplace_back([')
    if spawns != expect_threads:
        print('  %s: refusing, %d thread spawns inside' % (label, spawns))
        return text, False
    print('  %s: removed %d chars (%d lines)' % (label, len(removed), end_line + 1))
    return text[:i] + text[i + len(removed):], True


def main() -> int:
    # ---- original_vm.cpp --------------------------------------------------
    raw, t, eol = read(SRC)
    before_threads = t.count('std::thread(')
    ok = True
    t, r = cut_function(t, 'void runPendingOriginalThreads(const std::string& searchRoot) {',
                        eol, 'runPendingOriginalThreads', expect_threads=1)
    ok &= r
    t, r = cut_function(t, 'void drainPendingOriginalThreads(const std::string& searchRoot) {',
                        eol, 'drainPendingOriginalThreads', expect_threads=1)
    ok &= r
    t, r = cut_function(t, 'std::vector<PendingThread>& pendingThreads() {',
                        eol, 'pendingThreads()', expect_threads=0)
    ok &= r
    t, r = cut_block(t, 'void runPendingOriginalThreads(const std::string& searchRoot);',
                     'void runPendingOriginalThreads(const std::string& searchRoot);\n', eol,
                     'runPendingOriginalThreads decl', expect_threads=0)
    ok &= r
    t, r = cut_block(t, 'std::vector<PendingThread>& pendingThreads();',
                     'std::mutex& pendingThreadsMutex();\n', eol, 'queue accessors decl',
                     expect_threads=0)
    ok &= r
    if ok:
        open(SRC, 'wb').write(t.encode('utf-8'))
        print('original_vm.cpp: std::thread %d -> %d' % (before_threads, t.count('std::thread(')))
    else:
        print('original_vm.cpp: NOT written (a block failed)')
        return 1

    # ---- original_vm.h ---------------------------------------------------
    raw, h, heol = read(HDR)
    if 'drainPendingOriginalThreads' in h:
        i = h.find('drainPendingOriginalThreads')
        i = h.rfind('\n', 0, i) + 1
        j = h.find('\n', i) + 1
        h = h[:i] + h[j:]
        open(HDR, 'wb').write(h.encode('utf-8'))
        print('original_vm.h: declaration removed')

    # ---- builtins.cpp ----------------------------------------------------
    raw, b, beol = read(BUILTINS)
    old = ('        // telnet_new_client usually calls create_thread(0, (: telnet_daemon, fd :));\n'
           '        // spawn that runtime thread now.\n'
           '        drainPendingOriginalThreads(vfsRoot());\n')
    o = old.replace('\n', beol)
    if b.count(o) == 1:
        new = ('        // NOTE: this used to drain the pending-thread queue into OS\n'
               '        // threads.  create_thread spawns a scheduler process directly now\n'
               '        // (Step C2/C3), so there is nothing to drain - LPC only ever runs\n'
               '        // on the scheduler thread.\n')
        b = b.replace(o, new.replace('\n', beol), 1)
        open(BUILTINS, 'wb').write(b.encode('utf-8'))
        print('builtins.cpp: drain call removed')
    else:
        print('builtins.cpp: drain call not found (%d)' % b.count(o))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
