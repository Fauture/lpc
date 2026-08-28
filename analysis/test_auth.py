import socket, struct, time

HOST = "127.0.0.1"
PORT = 8101

def checksum(data):
    v = 0
    for b in data:
        v += b * ((v & 0x7F) + 1)
    return v & 0xFFFFFFFF

def make_packet(cmd_no, body):
    total = 12 + len(body)
    # original vm_clientSocketCallback: checkSum computed over cmdNo(2B) then body
    ck = checksum(struct.pack('>H', cmd_no))
    ck = checksum(body)
    hdr = b'MZ' + struct.pack('>I', ck) + struct.pack('>H', 0) + struct.pack('>H', total) + struct.pack('>H', cmd_no)
    return hdr + body

def main():
    s = socket.create_connection((HOST, PORT), timeout=5)
    print("connected")
    # msg_l_auth: body = result(u32) auth_key(u32) name_len(u16) name
    name = b"test"
    body = struct.pack('>I', 0) + struct.pack('>I', 12345) + struct.pack('>H', len(name)) + name
    pkt = make_packet(0x5351, body)
    s.sendall(pkt)
    print("sent msg_l_auth", len(pkt), "bytes")
    time.sleep(1)
    try:
        data = s.recv(4096)
        print("recv", len(data), "bytes:", data[:64].hex())
    except Exception as e:
        print("recv err", e)
    s.close()

if __name__ == "__main__":
    main()
