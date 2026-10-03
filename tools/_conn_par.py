# -*- coding: utf-8 -*-
"""Are two fresh connections serialized (~3.1s each) or independent (~3.1s each)?

  python -X utf8 _conn_par.py [host] [port] [gap_ms]
"""
import io, socket, struct, sys, time

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
HOST = sys.argv[1] if len(sys.argv) > 1 else '192.168.200.1'
PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 8160
GAP = float(sys.argv[3]) / 1000.0 if len(sys.argv) > 3 else 0.2


def recvn(s, n):
    d = b''
    while len(d) < n:
        c = s.recv(n - len(d))
        if not c:
            return None
        d += c
    return d


def read_pkt(s, timeout=8.0):
    s.settimeout(timeout)
    try:
        h = recvn(s, 10)
        if h is None or len(h) < 10:
            return None
        ln = struct.unpack('>H', h[8:10])[0]
        p = b''
        while len(p) < ln:
            c = s.recv(ln - len(p))
            if not c:
                return None
            p += c
        return struct.unpack('>H', p[:2])[0], p[2:]
    except socket.timeout:
        return ('timeout', b'')
    except Exception as e:
        return ('err', str(e).encode())


T0 = time.time()


def mark(msg):
    print('%8.1f ms  %s' % ((time.time() - T0) * 1000.0, msg), flush=True)


socks = []
for k in range(2):
    s = socket.create_connection((HOST, PORT), timeout=10)
    tick = int(time.time() * 1000) & 0xFFFFFFFF
    s.sendall(b'MZ\x00\x00' + struct.pack('>I', tick) + struct.pack('>H', 10)
              + struct.pack('>H', 0x20D2) + struct.pack('>I', tick) + b'\xFF\xFF\xFF\xFF')
    socks.append(s)
    mark('conn %d connected+sent' % k)
    if k == 0:
        time.sleep(GAP)

done = [False, False]
while not all(done) and time.time() - T0 < 20:
    for k, s in enumerate(socks):
        if done[k]:
            continue
        r = read_pkt(s, 0.2)
        if r and r[0] == 0xF9C3:
            mark('conn %d got 0xF9C3' % k)
            done[k] = True
for s in socks:
    s.close()
