# -*- coding: utf-8 -*-
"""Broad get_thread_id name check via the driver shell."""
import socket
import time


def drain(s, ms=700):
    end = time.time() + ms / 1000.0
    out = b''
    while time.time() < end:
        try:
            d = s.recv(4096)
            if not d:
                break
            out += d
        except socket.timeout:
            pass
    return out


for attempt in range(8):
    s = socket.create_connection(('127.0.0.1', 9110), timeout=5)
    s.settimeout(0.3)
    out = drain(s, 1000)
    if b'Password' in out:
        s.sendall(b'123456\n')
        time.sleep(1.0)
        drain(s, 1000)
        names = ['tthreaduser1', 'tthreaduser2', 'tthreadsrv1', 'tthreadrpc1',
                 'tthreadcomm1', 'tthreadcs1', 'tthreadweb1', 'tthreadddbsrv1',
                 'tthreadldbsrv1', 'tthreadintersrv1', 'tthreadmasrv1',
                 'tthreadtradesrv1', 'tthreadhttpsrv1']
        for n in names:
            c = 'get_thread_id("%s")' % n
            s.sendall((c + '\n').encode('gb2312'))
            time.sleep(1.5)
            r = drain(s, 1800).decode('gb2312', 'replace')
            lines = [l for l in r.splitlines() if l.strip() and 'shell>' not in l]
            print(c, '=>', (lines[-1][:40] if lines else 'NO OUTPUT'))
        s.sendall(b'.\n')
        drain(s, 200)
        s.close()
        break
    else:
        s.close()
        time.sleep(0.3)
