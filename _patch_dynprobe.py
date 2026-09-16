# -*- coding: utf-8 -*-
"""Make the probes I need on a LIVE server runtime-toggleable.

Replaces the once-cached `std::getenv` checks with `dynProbeFlag(...)` slots,
which the watcher thread re-reads once a second so

    type nul > _probe_on\\LPCVM_ECHO

arms the probe without an 11-minute reboot.
"""
import io
import re

# --------------------------------------------------------------------------
# original_vm.cpp
# --------------------------------------------------------------------------
p = 'src/original_vm.cpp'
d = io.open(p, encoding='utf-8', errors='replace').read()
nl = '\r\n' if '\r\n' in d else '\n'


def sub(old, new):
    global d
    o, n = old.replace('\n', nl), new.replace('\n', nl)
    assert o in d, 'anchor not found: %r' % old[:70]
    d = d.replace(o, n, 1)


sub('static bool nbProbeOn() { static const bool v = (std::getenv("LPCVM_NEWBIE") != nullptr); return v; }',
    '''static bool nbProbeOn() {
    static const std::atomic<bool>& f = dynProbeFlag("LPCVM_NEWBIE");
    return f.load(std::memory_order_relaxed);
}''')

sub('static bool vmEnvEvDbg() { static const bool v = (std::getenv("LPCVM_EVDBG") != nullptr); return v; }',
    '''static bool vmEnvEvDbg() {
    static const std::atomic<bool>& f = dynProbeFlag("LPCVM_EVDBG");
    return f.load(std::memory_order_relaxed);
}''')

sub('static bool vmEnvSvcTrace() { static const bool v = (std::getenv("LPCVM_SVC_TRACE") != nullptr); return v; }',
    '''static bool vmEnvSvcTrace() {
    static const std::atomic<bool>& f = dynProbeFlag("LPCVM_SVC_TRACE");
    return f.load(std::memory_order_relaxed);
}''')

sub('''                        if (std::getenv("LPCVM_TAIL") || std::getenv("LPCVM_QUEUE_TRACE")) {''',
    '''                        static const std::atomic<bool>& qSendTrace =
                            dynProbeFlag("LPCVM_QUEUE_TRACE");
                        if (envFlagTail() || qSendTrace.load(std::memory_order_relaxed)) {''')

sub('''                        const bool qTraceOn =
                            (std::getenv("LPCVM_QUEUE_TRACE") != nullptr);''',
    '''                        static const std::atomic<bool>& qTraceFlag =
                            dynProbeFlag("LPCVM_QUEUE_TRACE");
                        const bool qTraceOn = qTraceFlag.load(std::memory_order_relaxed);''')

io.open(p, 'w', encoding='utf-8', newline='').write(d)

# --------------------------------------------------------------------------
# game_service.cpp
# --------------------------------------------------------------------------
q = 'src/game_service.cpp'
g = io.open(q, encoding='utf-8', errors='replace').read()
gnl = '\r\n' if '\r\n' in g else '\n'
helper = '''namespace lpc {

namespace {

// Runtime-toggleable SVC_TRACE (see lpc::dynProbeFlag): the once-cached env
// read made a probe impossible to arm on a running server.
inline bool svcTraceOn() {
    static const std::atomic<bool>& f = dynProbeFlag("LPCVM_SVC_TRACE");
    return f.load(std::memory_order_relaxed);
}
'''
anchor = 'namespace lpc {' + gnl + gnl + 'namespace {' + gnl
assert anchor in g, 'game_service namespace anchor not found'
g = g.replace(anchor, helper.replace('\n', gnl), 1)

n_echo = g.count('static const bool echoProbe = (std::getenv("LPCVM_ECHO") != nullptr);')
g = g.replace('static const bool echoProbe = (std::getenv("LPCVM_ECHO") != nullptr);',
              'static const std::atomic<bool>& echoFlag = dynProbeFlag("LPCVM_ECHO");' + gnl +
              '    const bool echoProbe = echoFlag.load(std::memory_order_relaxed);')
n_svc = g.count('std::getenv("LPCVM_SVC_TRACE")')
g = g.replace('std::getenv("LPCVM_SVC_TRACE")', 'svcTraceOn()')
io.open(q, 'w', encoding='utf-8', newline='').write(g)
print('patched original_vm.cpp; game_service: echo sites=%d svc sites=%d' % (n_echo, n_svc))
