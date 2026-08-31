# -*- coding: utf-8 -*-
"""Query filed/autorecoverd reads via string call_other on ccs."""
import socket
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
    b'"/daemons/filed.c"->read_content("group_chat_index", "", "")',
    b'"/daemons/filed.c"->read_content("daemons", "SYS_PURCHASE_D", "")',
]
for c in cmds:
    s.sendall(c + b'\n')
    time.sleep(2.0)
    out = drain(3000)
    print('CMD:', c.decode())
    print(out.decode('utf-8', 'replace'))
    print('---')
s.sendall(b'.\n')
drain(300)
s.close()
