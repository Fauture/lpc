# -*- coding: utf-8 -*-
# Step-by-step AAA(:8001) login driver against the local engine.
# Flow (comm_desc + 登录流程包.txt): echo -> [antibot] -> check_user_data
# (get cookie) -> cmd_l_account (password DES with cookie) -> msg_l_auth ->
# get_server_list -> client_connect_agent -> connect returned ip:port ->
# cmd_login 0x4124.
# Encryption uses tools/d3des.dll gfEncryptDynamic (same as the real client).
import socket, struct, sys, time, ctypes
from ctypes import c_ubyte, c_uint, c_char_p, c_int, POINTER, byref

AAA_HOST = '127.0.0.1'
AAA_PORT = 8001

# d3des semantics reproduced with pycryptodome (same algorithm as src/d3des.c):
#   makekey(keyStr) hashes the key text into an 8-byte DES key using
#   3DES-EDE2(Df_Key1, Df_Key2) rounds; gfEncryptDynamic then does single-DES
#   ECB over zero-padded blocks.
from Crypto.Cipher import DES, DES3

DF_KEY1 = bytes([0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef])
DF_KEY2 = bytes([0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10])


def d3des_makekey(ktext: bytes) -> bytes:
    c = DES3.new(DF_KEY1 + DF_KEY2, DES3.MODE_ECB)  # 16B key => EDE2(k1,k2,k1)
    kptr = bytearray(DF_KEY1)
    i = 0
    first = True
    while i < len(ktext) or first:
        for j in range(8):
            if i < len(ktext):
                kptr[j] ^= ktext[i] & 0x7F
                i += 1
            else:
                break
        kptr = bytearray(c.encrypt(bytes(kptr)))
        first = False
    return bytes(kptr)


def des_encrypt(data: bytes, key: str) -> bytes:
    key8 = d3des_makekey(key.encode('gbk', 'replace'))
    n = ((len(data) + 7) // 8) * 8
    padded = data + bytes(n - len(data))
    c = DES.new(key8, DES.MODE_ECB)
    return c.encrypt(padded)


def frame(cmd: int, body: bytes) -> bytes:
    return b'MZ\x00\x00\x00\x00\x00\x00' + struct.pack('>H', 2 + len(body)) + struct.pack('>H', cmd) + body


def recvn(s, n):
    d = b''
    while len(d) < n:
        c = s.recv(n - len(d))
        if not c:
            return None
        d += c
    return d


def read_pkt(s, timeout=8):
    s.settimeout(timeout)
    try:
        h = recvn(s, 10)
        if h is None or len(h) < 10:
            return None
        ln = struct.unpack('>H', h[8:10])[0]
        payload = h[10:10]
        while len(payload) < ln:
            c = s.recv(ln - len(payload))
            if not c:
                return None
            payload += c
        if len(payload) < 2:
            return None
        cmd = struct.unpack('>H', payload[:2])[0]
        return cmd, payload[2:]
    except socket.timeout:
        return ('timeout', b'')
    except Exception:
        return None


def s8(v):  # string field: u8 len + bytes
    if isinstance(v, str):
        b = v.encode('gbk', 'replace')
    else:
        b = v
    return bytes([len(b)]) + b


def s16(v):
    if isinstance(v, str):
        b = v.encode('gbk', 'replace')
    else:
        b = v
    return struct.pack('>H', len(b)) + b


VERSION_AAA = '20544DE38C2160D6EAC9BFE07486242E'  # DES("1.60.0905","abcdef") hex (no md5 tail)
VERSION_GS = '20544DE38C2160D6EAC9BFE07486242E:D41D8CD98F00B204E9800998ECF8427E'  # with anti-cheat md5 tail


def build_account_frame(acct, pwd_plain, cookie, dist=''):
    # The client sends MD5(pwd) hex-UPPER as the "password" (the DB stores
    # md5(account + MD5hex(pwd) + "20070201")), DES-encrypted with the cookie.
    import hashlib
    pwd_hex = hashlib.md5(pwd_plain.encode('gbk', 'replace')).hexdigest().upper()
    enc_input = pwd_hex.encode('ascii')
    enc = des_encrypt(enc_input, cookie)
    hex_field = enc.hex().upper().encode('ascii')
    body = s8(acct)                     # account
    body += s8(hex_field)               # password = DES(MD5hex(pwd)) as hex text
    body += s8('0000000000000000')      # mac_addr
    body += s8('')                      # client_data
    body += s8('')                      # passpod
    body += s8(dist)                    # dist (GBK, e.g. 游戏内置)
    body += bytes([0])                  # moblie_client int8
    body += s8(VERSION_AAA)             # version hex
    body += s8('')                      # qr_id
    return frame(0x2350, body)


G_T0 = [0.0]


def mark(tag):
    print('  [%.3fs] %s' % (time.time() - G_T0[0], tag), flush=True)


def main():
    host = sys.argv[1] if len(sys.argv) > 1 else AAA_HOST
    port = int(sys.argv[2]) if len(sys.argv) > 2 else AAA_PORT
    acct = sys.argv[3] if len(sys.argv) > 3 else 'a'
    pwd = sys.argv[4] if len(sys.argv) > 4 else 'a'
    G_T0[0] = time.time()
    s = socket.create_connection((host, port), timeout=6)
    mark('connected %s:%d account=%s' % (host, port, acct))

    cookie = None

    # 1) echo heartbeat 0x20D2
    tick = int(time.time() * 1000) & 0xFFFFFFFF
    pkt = b'MZ\x00\x00' + struct.pack('>I', tick) + struct.pack('>H', 10) + struct.pack('>H', 0x20D2) + struct.pack('>I', tick) + b'\xFF\xFF\xFF\xFF'
    s.sendall(pkt)
    r = read_pkt(s)
    print('echo ->', None if r is None else ('0x%04X len=%d hex=%s' % (r[0], len(r[1]), r[1].hex()) if r[0] != 'timeout' else 'timeout'), flush=True)
    time.sleep(0.2)

    # 2) cmd_l_get_antibot_question 0x0B05 - replay the EXACT captured frame
    #    (登录流程包.txt line9): md5_value+checksum are self-consistent, the
    #    server only checksum-verifies them, so the captured bytes pass.
    pkt0b05 = bytes.fromhex(
        '4D5A0000010F240C00640B05'
        '2036413430453141313130394535443933363336424630343645373338363144454032314137364445363344373442443437454231323131463338324330384639434633393538353445463146413237363541334532383437394133393346444241')
    s.sendall(pkt0b05)
    r = read_pkt(s)
    if r and r[0] != 'timeout':
        print('antibot(0x0B05) -> 0x%04X len=%d hex=%s' % (r[0], len(r[1]), r[1].hex()), flush=True)
    else:
        print('antibot -> timeout/no-reply', flush=True)
    time.sleep(0.2)

    # 3) cmd_l_check_user_data 0x1B03 - replay captured frame (empty client_data)
    pkt1b03 = bytes.fromhex('4D5A0000010F246A00061B0300000000')
    s.sendall(pkt1b03)
    r = read_pkt(s)
    if r and r[0] == 0x2B04:
        body = r[1]
        result = struct.unpack('>i', body[:4])[0]
        off = 4
        ln = body[off]; off += 1
        cookie = body[off:off+ln].decode('gbk', 'replace')
        off += ln
        ln = body[off]; off += 1
        seed = body[off:off+ln].decode('gbk', 'replace')
        print('check_user_data -> result=%d cookie=%r seed=%r' % (result, cookie, seed), flush=True)
    else:
        print('check_user_data -> timeout/no-reply', flush=True)

    if cookie:
        # 4) cmd_l_account 0x2350 with DES(cookie) encrypted password
        dist = '试剑内测'
        f = build_account_frame(acct, pwd, cookie, dist)
        s.sendall(f)
        r = read_pkt(s, timeout=12)
        if r and r[0] == 0x5351:
            b = r[1]
            result = struct.unpack('>i', b[:4])[0]
            auth_key = struct.unpack('>i', b[4:8])[0] if len(b) >= 8 else 0
            ln = struct.unpack('>H', b[8:10])[0] if len(b) >= 10 else 0
            msg = b[10:10+ln].decode('gbk', 'replace') if len(b) >= 10 + ln else ''
            print('cmd_l_account -> msg_l_auth result=%d auth_key=%d msg=%r' % (result, auth_key, msg), flush=True)
            mark('auth result=%d' % result)
            if result == 1 and auth_key != 0:
                do_server_list(s, acct, auth_key, dist)
        else:
            print('cmd_l_account -> unexpected/no-reply', flush=True)

    s.close()


def do_server_list(s, acct, auth_key, dist):
    # 5) cmd_l_get_server_list 0x3354 (account, auth_key, dist, network_type)
    body = s8(acct) + struct.pack('>i', auth_key) + s8(dist) + bytes([0])
    s.sendall(frame(0x3354, body))
    r = read_pkt(s, timeout=12)
    if not r or r[0] != 0x4355:
        print('get_server_list -> no 0x4355 reply', None if r is None else hex(r[0]), flush=True)
        return
    b = r[1]
    print('msg_l_server_list len=%d hex=%s' % (len(b), b.hex()), flush=True)
    # encap SERVER_HOST[] = u16 count, each: string server, string ip, int16 status
    off = 0
    if len(b) >= 2:
        cnt = struct.unpack('>H', b[:2])[0]
        off = 2
        servers = []
        for k in range(cnt):
            if off + 1 > len(b): break
            sl = b[off]; off += 1
            server = b[off:off+sl].decode('gbk', 'replace'); off += sl
            if off + 1 > len(b): break
            sl = b[off]; off += 1
            ip = b[off:off+sl].decode('gbk', 'replace'); off += sl
            if off + 2 > len(b): break
            status = struct.unpack('>h', b[off:off+2])[0]; off += 2
            servers.append((server, ip, status))
        print('servers:', servers, flush=True)
        if servers:
            server = servers[0][0]
            # 6) cmd_l_client_connect_agent 0x3356
            mark('send 0x3356 agent_connect server=%s' % server)
            body = s8(acct) + struct.pack('>i', auth_key) + s8(server)
            r = None
            for attempt in range(4):
                s.sendall(frame(0x3356, body))
                r = read_pkt(s, timeout=15)
                if r and r[0] == 0x3357:
                    break
                mark('0x3356 attempt %d no 0x3357 (got %s), retrying' % (attempt + 1, hex(r[0]) if r else 'none'))
                time.sleep(1)
            if not (r and r[0] == 0x3357):
                print('client_connect_agent -> giving up', flush=True)
                return
            if r and r[0] == 0x3357:
                b = r[1]
                result = struct.unpack('>i', b[:4])[0]
                privilege = struct.unpack('>h', b[4:6])[0] if len(b) >= 6 else 0
                off = 6
                def rdstr(off):
                    ln = b[off]; off += 1
                    return b[off:off+ln].decode('gbk', 'replace'), off + ln
                ip, off = rdstr(off)
                port = struct.unpack('>h', b[off:off+2])[0] if len(b) >= off + 2 else 0; off += 2
                seed = struct.unpack('>i', b[off:off+4])[0] if len(b) >= off + 4 else 0; off += 4
                msg, off = rdstr(off)
                print('agent_result result=%d priv=%d ip=%s port=%d seed=%d msg=%r'
                      % (result, privilege, ip, port, seed, msg), flush=True)
                if result == 1:
                    connect_game(ip, port, acct, auth_key, seed)
            else:
                print('client_connect_agent -> no 0x3357 reply', flush=True)


def connect_game(ip, port, acct, auth_key, seed):
    # 7) connect the GS port returned by agent_result and send cmd_login 0x4124
    print('connect game %s:%d account=%s authKey=%d seed=%d' % (ip, port, acct, auth_key, seed), flush=True)
    try:
        g = socket.create_connection((ip, port), timeout=8)
    except Exception as e:
        print('game connect failed:', e, flush=True)
        return
    g.settimeout(20)
    tick = int(time.time() * 1000) & 0xFFFFFFFF
    pkt = b'MZ\x00\x00' + struct.pack('>I', tick) + struct.pack('>H', 10) + struct.pack('>H', 0x20D2) + struct.pack('>I', tick) + b'\xFF\xFF\xFF\xFF'
    g.sendall(pkt)
    try:
        r = read_pkt(g, 5)
        print('game echo ->', None if r is None else hex(r[0]), flush=True)
    except Exception:
        pass
    body = bytearray()
    ab = acct.encode('gbk', 'replace')
    body.append(len(ab)); body += ab
    body += struct.pack('>i', auth_key)
    body += struct.pack('>i', seed)
    vb = VERSION_GS.encode('ascii')   # full 1.60.0905 version incl. md5 tail
    body.append(len(vb)); body += vb
    body.append(0)   # client_type
    body.append(0)   # key len
    body.append(0)   # computer_id len
    t_send = time.time()
    g.sendall(frame(0x4124, bytes(body)))
    print('sent 0x4124 at t=%.2f' % t_send, flush=True)
    t0 = time.time()
    got211 = False
    sentCreate = False
    while time.time() - t0 < 90:
        try:
            r = read_pkt(g, 20)
        except Exception:
            break
        if r is None:
            print('game conn closed', flush=True)
            break
        if r[0] == 'timeout':
            print('game recv timeout at %.0fs' % (time.time() - t0), flush=True)
            continue
        print('game recv 0x%04X len=%d hex=%s' % (r[0], len(r[1]), r[1].hex()), flush=True)
        if r[0] == 0x211B:
            mark('GOT 0x211B after 0x4124')
            got211 = True
        if r[0] == 0xF061 and not sentCreate:
            # 0xF061 = login done; wait 15s, then create the char.
            mark('GOT 0xF061 login-done; waiting 15s then create_new_char')
            time.sleep(15)
            name = 'awesrd'
            nb = name.encode('gbk')
            body = bytes([len(nb)]) + nb + bytes([1, 1, 1])
            g.sendall(frame(0x205C, body))
            mark('sent create_new_char #1 %s' % name)
            sentCreate = True
            time.sleep(12)
            # second create of the SAME name must be rejected as "exists".
            g.sendall(frame(0x205C, bytes([len(nb)]) + nb + bytes([1, 1, 1])))
            mark('sent create_new_char #2 (same name, expect exists)')
    g.close()


if __name__ == '__main__':
    main()
