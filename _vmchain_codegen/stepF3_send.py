#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Step F3: the send path waits for writability instead of Sleep(1)-polling.

    python -X utf8 _vmchain_codegen/stepF3_send.py

`gameServiceSend` writes the frame in a loop and, when the socket reports
WSAEWOULDBLOCK, does `Sleep(1)`.  On Windows Sleep(1) is really the timer
granularity (often ~15 ms), so a partially full send buffer delays the reply by
tens of milliseconds - and the reply path is exactly what decides whether the
client keeps re-sending its create packet (each re-send can hit the LPC
re-entrancy guard and produce 9101).

Instead of sleeping blind, wait for the socket to become WRITABLE with a bounded
select().  Behaviour on a healthy socket is unchanged (first send() returns > 0),
only the blocked-buffer case gets faster and still bounded (200 ms, then the old
failure path).

Note (measured in F2): making the RECV path faster made 9101 worse.  This changes
the SEND path only - it gets replies out sooner rather than consuming client
packets sooner, so it is a different lever; the same acceptance test decides.
"""
import io

SRC = r'src\game_service.cpp'

OLD = '''    int sent = 0;
    while (sent < static_cast<int>(data.size())) {
        int n = send(static_cast<SOCKET>(fd), data.data() + sent,
                     static_cast<int>(data.size()) - sent, 0);
        if (n <= 0) {
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK || err == WSAEINTR) {   // non-blocking socket
                Sleep(1);
                continue;
            }'''

NEW = '''    int sent = 0;
    int wouldBlockWaits = 0;
    while (sent < static_cast<int>(data.size())) {
        int n = send(static_cast<SOCKET>(fd), data.data() + sent,
                     static_cast<int>(data.size()) - sent, 0);
        if (n <= 0) {
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK || err == WSAEINTR) {   // non-blocking socket
                // Wait for the socket to become writable instead of Sleep(1):
                // Windows' Sleep(1) is the timer granularity (~15ms), and this is
                // the reply path - a delayed reply is what makes the client
                // re-send and collect 9101 dialogs.  Bounded, then give up like
                // the old code did.
                if (++wouldBlockWaits > 200) {
                    liveness().sendFail.fetch_add(1, std::memory_order_relaxed);
                    std::cerr << "[send-fail] cno=" << clientNo << " fd=" << fd
                              << " would-block timeout len=" << data.size()
                              << " t=" << lpc::msSinceStart() << "ms\\n";
                    return false;
                }
                fd_set wfds;
                FD_ZERO(&wfds);
                FD_SET(static_cast<SOCKET>(fd), &wfds);
                timeval tv;
                tv.tv_sec = 0;
                tv.tv_usec = 1000;        // 1ms; select returns as soon as writable
                select(0, nullptr, &wfds, nullptr, &tv);
                continue;
            }'''


def main() -> int:
    raw = open(SRC, 'rb').read()
    crlf = raw.count(b'\r\n')
    lf = raw.count(b'\n')
    sep = '\r\n' if crlf * 2 >= lf else '\n'
    t = raw.decode('utf-8')
    o, n = OLD.replace('\n', sep), NEW.replace('\n', sep)
    if t.count(o) != 1:
        print('anchor COUNT %d' % t.count(o))
        return 1
    open(SRC, 'wb').write(t.replace(o, n, 1).encode('utf-8'))
    print('OK send path waits for writability')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
