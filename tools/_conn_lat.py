# -*- coding: utf-8 -*-
"""Measure, on a FRESH TCP connection, how long the server takes to answer the
first echo (0x20D2 -> 0xF9C3).

Separates three legs so we can tell WHERE the ~3.6s goes:
  t_connect  : connect() return
  t_send     : sendall() return (right after connect)
  t_reply    : 0xF9C3 received

  python -X utf8 _conn_lat.py [host] [port] [rounds]
"""
import io, socket, struct, sys, time

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
HOST = sys.argv[1] if len(sys.argv) > 1 else '192.168.200.1'
PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 8160
N = int(sys.argv[3]) if len(sys.argv) > 3 else 3


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
        # Wire header is 10 bytes (MZ + checksum u16 + tick u32 + msgLen u16);
        # the msgLen bytes that follow are cmd(2) + body.
        h = recvn(s, 10)
        if h is None or len(h) < 10:
            return None
        ln = struct.unpack('>H', h[8:10])[0]
        payload = b''
        while len(payload) < ln:
            c = s.recv(ln - len(payload))
            if not c:
                return None
            payload += c
        return struct.unpack('>H', payload[:2])[0], payload[2:]
    except socket.timeout:
        return ('timeout', b'')
    except Exception as e:
        return ('err', str(e).encode())


for k in range(N):
    t0 = time.time()
    s = socket.create_connection((HOST, PORT), timeout=10)
    t_conn = (time.time() - t0) * 1000.0
    tick = int(time.time() * 1000) & 0xFFFFFFFF
    s.sendall(b'MZ\x00\x00' + struct.pack('>I', tick) + struct.pack('>H', 10)
              + struct.pack('>H', 0x20D2) + struct.pack('>I', tick) + b'\xFF\xFF\xFF\xFF')
    t_send = (time.time() - t0) * 1000.0
    got = None
    while (time.time() - t0) < 12:
        r = read_pkt(s, 3.0)
        if r is None:
            break
        if r[0] == 'timeout':
            continue
        if r[0] == 0xF9C3:
            got = (time.time() - t0) * 1000.0
            break
    print('round %d: connect=%.1fms send=%.1fms reply=%s' %
          (k + 1, t_conn, t_send, ('%.1fms' % got) if got else 'NONE'), flush=True)
    s.close()
    time.sleep(0.5)
