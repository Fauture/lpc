import socket, struct, time, sys

HOST = "127.0.0.1"
PORT = 8101

def checksum(data):
    v = 0
    for b in data:
        v += b * ((v & 0x7F) + 1)
    return v & 0xFFFFFFFF

def make_packet(cmd_no, body):
    # msgLen counts from the cmdNo field (2 bytes) through the body
    msgLen = 2 + len(body)
    ck = checksum(struct.pack('>H', cmd_no))
    ck = checksum(body)
    hdr = b'MZ' + struct.pack('>I', ck) + struct.pack('>H', 0) + struct.pack('>H', msgLen) + struct.pack('>H', cmd_no)
    return hdr + body

def parse_header(data):
    # Server reply format (vm_buildFullPacket): MZ(2) + ck(2) + tick(4) +
    # msgLen(2) + cmdNo(2) + body.  msgLen counts from the cmdNo field (2)
    # through the body, so bodyLen = msgLen - 2.
    if len(data) < 12 or data[0:2] != b'MZ':
        return None
    ck = struct.unpack('>H', data[2:4])[0]
    tick = struct.unpack('>I', data[4:8])[0]
    msgLen = struct.unpack('>H', data[8:10])[0]
    cmd = struct.unpack('>H', data[10:12])[0]
    return ck, tick, msgLen, cmd

def main():
    # The antibot request from the real client (captured in run_live logs):
    md5_value = "6A40E1A1109E5D93636BF046E73861DE"
    checksum_val = "21A76DE63D74BD47EB1211F382C08F9CF395854EF1FA2765A3E28479A393FDBA"
    body = bytes([len(md5_value)]) + md5_value.encode() + bytes([len(checksum_val)]) + checksum_val.encode()
    pkt = make_packet(0x0B05, body)   # CMD_L_GET_ANTIBOT_QUESTION
    print("packet len:", len(pkt), "msgLen field: 2+%d=%d" % (len(body), 2+len(body)))

    s = socket.create_connection((HOST, PORT), timeout=5)
    print("connected")
    s.sendall(pkt)
    print("sent antibot request, waiting for reply...")
    s.settimeout(8)
    try:
        data = s.recv(65536)
        print("recv", len(data), "bytes")
        if len(data) >= 12:
            ck, tick, msgLen, cmd = parse_header(data)
            print("reply header: cmd=0x%04X msgLen=%d tick=%d ck=%04X" % (cmd, msgLen, tick, ck))
            body = data[12:12+msgLen-2] if len(data) >= 12+msgLen-2 else data[12:]
            print("reply body hex:", body.hex())
            print("reply body ascii:", body.decode(errors='replace'))
        else:
            print("short reply:", data.hex())
    except socket.timeout:
        print("TIMEOUT: server sent no reply")
    s.close()

if __name__ == "__main__":
    main()
