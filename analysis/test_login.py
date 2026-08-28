import socket, struct, time

HOST = "127.0.0.1"
PORT = 8101

def checksum(data):
    v = 0
    for b in data:
        v += b * ((v & 0x7F) + 1)
    return v & 0xFFFFFFFF

def make_packet(cmd_no, body):
    msgLen = 2 + len(body)
    ck = checksum(body)
    hdr = b'MZ' + struct.pack('>I', ck) + struct.pack('>H', 0) + struct.pack('>H', msgLen) + struct.pack('>H', cmd_no)
    return hdr + body

def pstr(s):
    b = s.encode('latin1') if isinstance(s, str) else s
    return bytes([len(b)]) + b

def main():
    s = socket.create_connection((HOST, PORT), timeout=5)
    print("connected")
    # Replay the REAL client packets from the capture.
    # 1) cmd_l_get_antibot_question
    md5_value = "6A40E1A1109E5D93636BF046E73861DE"
    checksum_val = "21A76DE63D74BD47EB1211F382C08F9CF395854EF1FA2765A3E28479A393FDBA"
    body = bytes([len(md5_value)]) + md5_value.encode() + bytes([len(checksum_val)]) + checksum_val.encode()
    s.sendall(make_packet(0x0B05, body))
    print("sent antibot")
    s.settimeout(8)
    try:
        d = s.recv(65536)
        print("antibot reply:", d.hex())
    except socket.timeout:
        print("antibot TIMEOUT")

    # 2) cmd_l_check_user_data (replay real capture: body = 4 zero bytes)
    s.sendall(make_packet(0x1B03, b'\x00\x00\x00\x00'))
    print("sent check_user_data")
    s.settimeout(8)
    try:
        d = s.recv(65536)
        print("check_user_data reply:", d.hex())
    except socket.timeout:
        print("check_user_data TIMEOUT")

    # 3) cmd_l_account (replay the exact capture body)
    # body from capture: account="a", password=64hex, mac="0000b445067b15f4",
    # client_data="", passpod="|1", dist=GBK 中国测试, moblie=0,
    # version="20544DE38C2160D6EAC9BFE07486242E", qr_id=""
    account = b'a'
    password = bytes.fromhex("38454634373134454437373341354237383543304639443441383345374231343135383734394543423438424443434233393539413934314541364238384637")
    mac = b'0000b445067b15f4'
    client_data = b''
    passpod = b'|1'
    dist = bytes([0xCA,0xD4,0xBD,0xA3,0xC4,0xDA,0xB2,0xE2])  # GBK 中国测试
    mobile = 0
    version = b'20544DE38C2160D6EAC9BFE07486242E'
    qr_id = b''
    fields = [pstr(account), pstr(password), pstr(mac), pstr(client_data),
              pstr(passpod), pstr(dist), bytes([mobile]), pstr(version), pstr(qr_id)]
    body = b''.join(fields)
    print("account body len:", len(body))
    s.sendall(make_packet(0x2350, body))
    print("sent account")
    s.settimeout(8)
    try:
        d = s.recv(65536)
        print("account reply:", d.hex())
        if len(d) >= 12:
            msgLen = struct.unpack('>H', d[8:10])[0]
            cmd = struct.unpack('>H', d[10:12])[0]
            print("cmd=0x%04X msgLen=%d body=%s" % (cmd, msgLen, d[12:12+msgLen-2].hex()))
            print("body ascii:", d[12:12+msgLen-2].decode('gbk', errors='replace'))
    except socket.timeout:
        print("account TIMEOUT")

    # 4) cmd_l_get_server_list (0x3354): after successful login the client
    # requests the server list.  Fields: string account, int32 auth_key,
    # string dist, int8 network_type.
    try:
        import struct as _st
        # msg_l_auth body = int32 result + int32 auth_key + string2 msg.
        # Header is 12 bytes, so auth_key lives at d[16:20] (NOT d[14:18],
        # which straddles the result tail and auth_key head).
        _hdr = 12
        _msgLen = _st.unpack('>H', d[8:10])[0] if len(d) >= 10 else 0
        _body = d[_hdr:_hdr + _msgLen - 2]
        auth_key = _st.unpack('>i', _body[4:8])[0] if len(_body) >= 8 else 0
        print("result=%d auth_key=%d" % (_st.unpack('>i', _body[0:4])[0] if len(_body) >= 4 else -1, auth_key))
        body4 = pstr(account) + struct.pack('>i', auth_key) + pstr(dist) + bytes([0])
        print("get_server_list body len:", len(body4))
        s.sendall(make_packet(0x3354, body4))
        print("sent get_server_list")
        s.settimeout(10)
        try:
            d4 = s.recv(65536)
            print("get_server_list reply:", d4.hex())
            if len(d4) >= 12:
                msgLen4 = struct.unpack('>H', d4[8:10])[0]
                cmd4 = struct.unpack('>H', d4[10:12])[0]
                body4r = d4[12:12+msgLen4-2]
                print("cmd=0x%04X msgLen=%d bodylen=%d body=%s" % (cmd4, msgLen4, len(body4r), body4r.hex()))
        except socket.timeout:
            print("get_server_list TIMEOUT")
        # 5) try recv any further packets
        try:
            d5 = s.recv(65536)
            print("extra reply:", d5.hex())
        except socket.timeout:
            print("no extra reply")
    except Exception as e:
        print("get_server_list step error:", e)
    s.close()

if __name__ == "__main__":
    main()
