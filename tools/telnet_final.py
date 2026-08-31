# -*- coding: utf-8 -*-
"""Recheck is_test and haoyun on current ccs."""
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
    b'is_test()',
    b'"/ccs/daemons/haoyun_choufantiand.c"->get_game_info()',
    b'"/ccs/daemons/sys_purchased.c"->get_orders()',
]
for c in cmds:
    s.sendall(c + b'\n')
    time.sleep(2.0)
    out = drain(3000)
    sys.stdout.buffer.write(b'CMD: ' + c + b'\n')
    sys.stdout.buffer.write(out[:900] + b'\n---\n')
    sys.stdout.buffer.flush()
s.sendall(b'.\n')
drain(300)
s.close()
