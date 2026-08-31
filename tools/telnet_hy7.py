# -*- coding: utf-8 -*-
"""Check haoyun save conditions: is_*_compete_dist and all_credit_info."""
import socket
import sys
import time

s = socket.create_connection(('127.0.0.1', 9110), timeout=10)
s.settimeout(0.5)

def drain(ms=800):
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

drain(500)
s.sendall(b'123456\n')
drain(600)

cmds = [
    b'"/daemons/configd.c"->is_csl_compete_dist()',
    b'"/daemons/configd.c"->is_csp_compete_dist()',
    b'"/daemons/configd.c"->is_arena_compete_dist()',
    b'"/daemons/configd.c"->is_sddh_compete_dist()',
    b'"/daemons/configd.c"->is_celebrity_compete_dist()',
    b'"/daemons/configd.c"->is_samsung_compete_dist()',
    b'"/daemons/configd.c"->is_citywar_compete_dist()',
    b'"/daemons/configd.c"->is_xgame_compete_dist()',
    b'"/daemons/configd.c"->is_everyone_pk_compete_dist()',
    b'"/daemons/configd.c"->is_activity_compete_dist()',
]
for c in cmds:
    s.sendall(c + b'\n')
    time.sleep(1.2)
    out = drain(2000)
    sys.stdout.buffer.write(c.split(b'"')[1] + b' = ' + out.strip()[:80] + b'\n')
    sys.stdout.buffer.flush()
s.sendall(b'.\n')
drain(300)
s.close()
