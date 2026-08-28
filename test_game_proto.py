import socket, struct, time, sys

def rd16be(b, o): return (b[o] << 8) | b[o+1]
def rd32be(b, o): return (b[o] << 24) | (b[o+1] << 16) | (b[o+2] << 8) | b[o+3]

def checksum(data, init=0):
    v = init
    for byte in data:
        v = (v + byte * ((v & 0x7F) + 1)) & 0xFFFF
    return v

# Build a game packet: header(MZ) + body
def make_packet(cmdno, body):
    msgLen = 12 + len(body)
    # checksum = checksum over cmdNo(2 bytes) then body
    ck = checksum(struct.pack('>H', cmdno))
    ck = checksum(body, ck)
    hdr = b'MZ' + struct.pack('>H', ck) + struct.pack('>I', int(time.time())) + struct.pack('>H', msgLen) + struct.pack('>H', cmdno)
    return hdr + body

host, port = '127.0.0.1', 8101
s = socket.create_connection((host, port), timeout=5)
print("connected")

# msg_l_auth: result(u32be) authKey(u32be) nameLen(u16be) name
name = b'test'
body = struct.pack('>I', 0) + struct.pack('>I', 12345) + struct.pack('>H', len(name)) + name
pkt = make_packet(0x5351, body)
print("sending msg_l_auth packet len=%d cmd=0x5351" % len(pkt))
s.sendall(pkt)

time.sleep(2)
try:
    s.settimeout(2)
    data = s.recv(4096)
    print("recv %d bytes: %s" % (len(data), data.hex()))
except socket.timeout:
    print("no response (timeout)")
s.close()
