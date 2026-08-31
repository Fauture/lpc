# -*- coding: utf-8 -*-
"""Verify get_thread_id(name/int) named-process lookups via the driver shell."""
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


def try_shell():
    s = socket.create_connection(('127.0.0.1', 9110), timeout=5)
    s.settimeout(0.3)
    out = drain(s, 1000)
    return s, out


for attempt in range(8):
    s, out = try_shell()
    txt = out.decode('gb2312', 'replace')
    if 'Password' in txt:
        s.sendall(b'123456\n')
        time.sleep(1.0)
        drain(s, 1000)
        cmds = [
            'get_thread_id("tfiled")',
            'get_thread_id("tdbacomm")',
            'get_thread_id("tthreaduser1")',
            'get_thread_id(9999)',
        ]
        for c in cmds:
            s.sendall((c + '\n').encode('gb2312'))
            time.sleep(2.5)
            r = drain(s, 2500).decode('gb2312', 'replace')
            lines = [l for l in r.splitlines() if l.strip() and 'shell>' not in l]
            print(c, '=>', (lines[-1][:80] if lines else 'NO OUTPUT'))
        s.sendall(b'.\n')
        drain(s, 200)
        s.close()
        break
    else:
        s.close()
        time.sleep(0.3)
