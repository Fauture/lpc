# -*- coding: utf-8 -*-
"""Test haoyun write_content directly via shell."""
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
    b'"/daemons/filed.c"->write_content("haoyun_choufantian_d", "time_list_1", "", "([1346515199:([\\"end_time\\":1346515199,\\"start_time\\":1346428800,\\"data\\":({}),\\"index\\":\\"time_list_1\\",]),])")',
    b'"/daemons/filed.c"->create_content("haoyun_choufantian_d", "time_list_2", "", "([1346860799:([\\"end_time\\":1346860799,\\"start_time\\":1346774400,\\"data\\":({}),\\"index\\":\\"time_list_2\\",]),])")',
]
for c in cmds:
    s.sendall(c + b'\n')
    time.sleep(2.5)
    out = drain(4000)
    sys.stdout.buffer.write(b'CMD: ' + c[:80] + b'...\n')
    sys.stdout.buffer.write(out[:300] + b'\n---\n')
    sys.stdout.buffer.flush()
s.sendall(b'.\n')
drain(300)
s.close()
