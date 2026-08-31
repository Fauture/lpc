# -*- coding: utf-8 -*-
"""Deep check haoyun state via shell."""
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
    b'find_object("/ccs/daemons/haoyun_choufantiand.c")',
    b'"/ccs/daemons/haoyun_choufantiand.c"->get_game_info()',
    b'"/ccs/daemons/haoyun_choufantiand.c"->save()',
]
for c in cmds:
    s.sendall(c + b'\n')
    time.sleep(2.5)
    out = drain(4000)
    sys.stdout.buffer.write(b'CMD: ' + c + b'\n')
    sys.stdout.buffer.write(out[:1000] + b'\n---\n')
    sys.stdout.buffer.flush()
s.sendall(b'.\n')
drain(300)
s.close()
