# -*- coding: utf-8 -*-
"""Create time_list_1 row."""
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

s.sendall(b'"/daemons/filed.c"->create_content("haoyun_choufantian_d", "time_list_1", "", "([1346601599:([\\"end_time\\":1346601599,\\"start_time\\":1346515200,\\"data\\":({}),\\"index\\":\\"time_list_1\\",]),1346515199:([\\"end_time\\":1346515199,\\"start_time\\":1346428800,\\"data\\":({}),\\"index\\":\\"time_list_1\\",]),])")\n')
time.sleep(2.5)
out = drain(4000)
sys.stdout.buffer.write(out[:300] + b'\n')
sys.stdout.buffer.flush()
s.sendall(b'.\n')
drain(300)
s.close()
