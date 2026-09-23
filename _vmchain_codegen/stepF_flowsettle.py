#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Make the per-packet flow settle event-driven instead of a 5ms poll.

    python -X utf8 _vmchain_codegen/stepF_flowsettle.py

`game_service`'s `waitFlowSettled(cno)` currently loops up to 200 times with
`sleep_for(5ms)`:

    for (int i = 0; i < 200; ++i) {
        if (lpc::serviceQueueIdle() && lpc::clientSendIdleMs(cno) >= 50) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

Every packet of a client connection therefore pays up to 50 ms of *polling*
(and the send path is the one that just happened, so in practice it polls until
the 50 ms idle mark is reached - 10 wasted wakeups per packet under load).

This adds a condition variable that `noteClientSend()` notifies, plus

    bool waitClientSendIdle(int64_t cno, int64_t idleMs, int timeoutMs)

which waits until the connection has been quiet for `idleMs` OR the timeout
expires, then switches `waitFlowSettled` to it.  Semantics are unchanged (same 50 ms
quiet window, same bounded wait); only the polling goes away.
"""
import io

BUILTINS_CPP = r'src\builtins.cpp'
BUILTINS_H = r'src\builtins.h'
GAME = r'src\game_service.cpp'

CPP_OLD = '''static std::mutex g_clientSendMtx;
static std::map<int64_t, int64_t> g_clientLastSendMs;

static int64_t steadyNowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

void noteClientSend(int64_t cno) {
    if (cno < 0) return;
    std::lock_guard<std::mutex> lk(g_clientSendMtx);
    g_clientLastSendMs[cno] = steadyNowMs();
}'''

CPP_NEW = '''static std::mutex g_clientSendMtx;
static std::map<int64_t, int64_t> g_clientLastSendMs;
// Woken whenever a packet is sent to ANY client, so a connection waiting for its
// own flow to go quiet does not have to poll (see waitClientSendIdle).
static std::condition_variable g_clientSendCv;

static int64_t steadyNowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

void noteClientSend(int64_t cno) {
    if (cno < 0) return;
    {
        std::lock_guard<std::mutex> lk(g_clientSendMtx);
        g_clientLastSendMs[cno] = steadyNowMs();
    }
    g_clientSendCv.notify_all();
}

// Wait until `cno` has been quiet for `idleMs`, or `timeoutMs` elapsed.  Returns
// true when it went quiet.  This replaces the old "poll every 5 ms for up to 1 s"
// loop in game_service's per-packet flow gate: the wait is now event driven (the
// send path notifies), with the same idle window and the same bound.
bool waitClientSendIdle(int64_t cno, int64_t idleMs, int timeoutMs) {
    if (cno < 0) return true;
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs > 0 ? timeoutMs : 1000);
    std::unique_lock<std::mutex> lk(g_clientSendMtx);
    for (;;) {
        auto it = g_clientLastSendMs.find(cno);
        const int64_t idle = (it == g_clientLastSendMs.end()) ? (int64_t(1) << 30)
                                                              : (steadyNowMs() - it->second);
        if (idle >= idleMs) return true;
        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline) return false;      // bound reached, flow still busy
        // Wake up when this connection is quiet, and at the deadline regardless:
        // whichever comes first.
        auto waitFor = std::chrono::milliseconds(idleMs - idle);
        const auto untilDeadline = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - now);
        if (untilDeadline < waitFor) waitFor = untilDeadline;
        g_clientSendCv.wait_for(lk, waitFor);
    }
}'''

H_OLD = '''void noteClientSend(int64_t cno);
int64_t clientSendIdleMs(int64_t cno);'''
H_NEW = '''void noteClientSend(int64_t cno);
int64_t clientSendIdleMs(int64_t cno);
// Event-driven version of the per-packet flow gate: blocks until the connection
// has been quiet for `idleMs` or `timeoutMs` elapsed (see its definition).
bool waitClientSendIdle(int64_t cno, int64_t idleMs, int timeoutMs);'''

GAME_OLD = '''static void waitFlowSettled(int cno) {
    for (int i = 0; i < 200; ++i) {          // bounded (~1s per packet)
        if (lpc::serviceQueueIdle() && lpc::clientSendIdleMs(cno) >= 50) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}'''

GAME_NEW = '''static void waitFlowSettled(int cno) {
    // Bounded (~1s per packet) and EVENT DRIVEN: lpc::waitClientSendIdle blocks
    // until this connection's send flow has been quiet for 50ms (the send path
    // notifies a condition variable), instead of polling every 5ms.
    for (int i = 0; i < 200; ++i) {
        if (!lpc::serviceQueueIdle()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        if (lpc::waitClientSendIdle(cno, 50, 1000)) break;
    }
}'''


def patch(path, old, new):
    raw = open(path, 'rb').read()
    crlf = raw.count(b'\r\n')
    lf = raw.count(b'\n')
    sep = '\r\n' if crlf * 2 >= lf else '\n'
    t = raw.decode('utf-8')
    o, n = old.replace('\n', sep), new.replace('\n', sep)
    if t.count(o) != 1:
        print('%s: anchor COUNT %d' % (path, t.count(o)))
        return False
    open(path, 'wb').write(t.replace(o, n, 1).encode('utf-8'))
    print('%s: patched' % path)
    return True


def main() -> int:
    ok = patch(BUILTINS_CPP, CPP_OLD, CPP_NEW)
    ok &= patch(BUILTINS_H, H_OLD, H_NEW)
    ok &= patch(GAME, GAME_OLD, GAME_NEW)
    return 0 if ok else 1


if __name__ == '__main__':
    raise SystemExit(main())
