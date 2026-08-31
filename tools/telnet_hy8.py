# -*- coding: utf-8 -*-
"""Check haoyun all_credit_info via get_all_credit_info."""
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

s.sendall(b'"/ccs/daemons/haoyun_choufantiand.c"->get_all_credit_info()\n')
time.sleep(2.0)
out = drain(3000)
sys.stdout.buffer.write(out[:1500] + b'\n')
sys.stdout.buffer.flush()
s.sendall(b'.\n')
drain(300)
s.close()
