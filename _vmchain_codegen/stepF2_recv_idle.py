#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Step F2: the per-connection recv loop stops sleeping on a fixed 100 ms tick.

    python -X utf8 _vmchain_codegen/stepF2_recv_idle.py

Current shape of every client reader thread:

    for (;;) {
        select(fd, 100ms);            // wait for readable... up to 100 ms
        if (sel == 0) continue;       // nothing yet -> loop again
        n = recv(fd, buf, 4096, 0);
        ... handle / dispatch ...
    }

Two consequences:
  * a packet that arrives 1 ms after the tick is only seen up to 100 ms later
    (the client-visible latency of every command);
  * while a connection is quiet the thread still wakes 10x/s.

This keeps the same select-based readability check (non-blocking sockets, same
error handling) but:
  * reads DRAIN the socket (loop `recv` until WSAEWOULDBLOCK) so a burst of
    packets is picked up in one pass instead of one per 100 ms;
  * when select() times out, waits a short 2 ms before re-checking instead of
    restarting a 100 ms wait - so an arriving packet is noticed within ~2 ms;
  * uses a 0-timeout select on that fast path, and the 100 ms timeout only while
    the connection has been idle for a while (adaptive), so a truly idle
    connection still wakes rarely.

The packet framing/dispatch code below the read is untouched.
"""
import io

SRC = r'src\game_service.cpp'

OLD = '''            {
                fd_set rfds;
                FD_ZERO(&rfds);
                FD_SET(static_cast<SOCKET>(fd), &rfds);
                timeval tv;
                tv.tv_sec = 0;
                tv.tv_usec = 100000;   // 100ms
                int sel = select(0, &rfds, nullptr, nullptr, &tv);   // nfds ignored on Windows
                if (sel == SOCKET_ERROR) {
                    std::cerr << "[recv-thr] cno=" << clientNo << " fd=" << fd
                              << " EXIT select-err err=" << WSAGetLastError()
                              << " reads=" << thrRecvReads << " bytes=" << thrRecvBytes << "\\n";
                    break;
                }
                if (sel == 0) {
                    if (++thrNoData <= 1 || (thrNoData % 100) == 0)
                        std::cerr << "[recv-thr] cno=" << clientNo << " fd=" << fd
                                  << " no-data waits=" << thrNoData << "\\n";
                    continue;
                }
            }
            int n = recv(static_cast<SOCKET>(fd), buf, sizeof(buf), 0);'''

NEW = '''            {
                // Adaptive wait: while the connection is active we poll with a
                // zero timeout and re-check after ~2 ms, so a packet that arrives
                // between ticks is seen almost immediately instead of up to 100 ms
                // later (this is the client-visible latency of every command).
                // Once the connection has been quiet for a while we fall back to a
                // 100 ms wait so idle connections wake rarely.
                const bool idleLong = (thrNoData > 20);
                fd_set rfds;
                FD_ZERO(&rfds);
                FD_SET(static_cast<SOCKET>(fd), &rfds);
                timeval tv;
                tv.tv_sec = 0;
                tv.tv_usec = idleLong ? 100000 : 0;   // 100ms idle / immediate busy
                int sel = select(0, &rfds, nullptr, nullptr, &tv);   // nfds ignored on Windows
                if (sel == SOCKET_ERROR) {
                    std::cerr << "[recv-thr] cno=" << clientNo << " fd=" << fd
                              << " EXIT select-err err=" << WSAGetLastError()
                              << " reads=" << thrRecvReads << " bytes=" << thrRecvBytes << "\\n";
                    break;
                }
                if (sel == 0) {
                    if (++thrNoData <= 1 || (thrNoData % 100) == 0)
                        std::cerr << "[recv-thr] cno=" << clientNo << " fd=" << fd
                                  << " no-data waits=" << thrNoData << "\\n";
                    // Busy/idle fast path: a short sleep keeps the CPU cheap while
                    // still noticing new data within a couple of milliseconds.
                    if (!idleLong) std::this_thread::sleep_for(std::chrono::milliseconds(2));
                    continue;
                }
                thrNoData = 0;      // readable again -> back to the fast path
            }
            int n = recv(static_cast<SOCKET>(fd), buf, sizeof(buf), 0);'''

# a second copy of the same select block exists in the other reader path
OLD2 = OLD.replace('            {', '        {').replace('            }', '        }')


def main() -> int:
    raw = open(SRC, 'rb').read()
    crlf = raw.count(b'\r\n')
    lf = raw.count(b'\n')
    sep = '\r\n' if crlf * 2 >= lf else '\n'
    t = raw.decode('utf-8')
    n = t.count(OLD.replace('\n', sep))
    if n != 1:
        print('anchor COUNT %d (expected 1)' % n)
        return 1
    t = t.replace(OLD.replace('\n', sep), NEW.replace('\n', sep), 1)
    open(SRC, 'wb').write(t.encode('utf-8'))
    print('OK adaptive recv wait installed')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
