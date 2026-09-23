#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Are the legacy pending-thread worker pools ever used?

    python -X utf8 _vmchain_codegen/stepC2_probe.py

`runPendingOriginalThreads` / `drainPendingOriginalThreads` spawn one OS thread
per pending thread and build a fresh VM inside it (with an objVars SNAPSHOT copy),
i.e. LPC runs off the scheduler thread - the reason the exec lock still exists.
They belong to the old "create_thread = OS thread" model.

Before converting them (risky: telnet routing uses a thread-local client id), find
out whether a real server run reaches them at all.  `runOriginalObjectInit` drains
once at startup under LPCVM_TAIL in trace mode, but the question is whether any
thread is actually QUEUED.
"""
import io

SRC = r'src\original_vm.cpp'

OLD = '''void runPendingOriginalThreads(const std::string& searchRoot) {
    std::vector<PendingThread> snapshot;
    {
        std::lock_guard<std::mutex> lk(pendingThreadsMutex());
        snapshot.swap(pendingThreads());
    }'''

NEW = '''void runPendingOriginalThreads(const std::string& searchRoot) {
    std::vector<PendingThread> snapshot;
    {
        std::lock_guard<std::mutex> lk(pendingThreadsMutex());
        snapshot.swap(pendingThreads());
    }
    // Step C2 probe: these workers run LPC off the scheduler thread (each builds
    // its own VM and copies the creator's objVars), which is what keeps the exec
    // lock alive.  Report every use so the conversion can be judged on facts:
    // if a real run never reaches this, the pools are dead code for this server.
    if (!snapshot.empty()) {
        std::cerr << "[pending-thr] drain n=" << snapshot.size() << " funcs=[";
        for (size_t i = 0; i < snapshot.size() && i < 8; ++i)
            std::cerr << (i ? "," : "") << snapshot[i].fun;
        std::cerr << "]\\n";
        std::cerr.flush();
    }'''


def main() -> int:
    raw = open(SRC, 'rb').read()
    crlf = raw.count(b'\r\n')
    lf = raw.count(b'\n')
    sep = '\r\n' if crlf * 2 >= lf else '\n'
    t = raw.decode('utf-8')
    if 'pending-thr' in t:
        print('probe already installed')
        return 0
    o, n = OLD.replace('\n', sep), NEW.replace('\n', sep)
    if t.count(o) != 1:
        print('anchor COUNT %d' % t.count(o))
        return 1
    t = t.replace(o, n, 1)
    open(SRC, 'wb').write(t.encode('utf-8'))
    print('OK pending-thread probe installed')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
