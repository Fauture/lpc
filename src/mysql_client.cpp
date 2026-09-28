#include "mysql_client.h"

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <mutex>
#include <string>

#pragma comment(lib, "ws2_32.lib")

namespace lpc {

namespace {

std::mutex& wsaMutex() {
    static std::mutex m;
    return m;
}
void ensureWsa() {
    static bool done = [] {
        WSADATA wsa{};
        WSAStartup(MAKEWORD(2, 2), &wsa);
        return true;
    }();
    (void)done;
}

// ---- SHA-1 (FIPS 180-1) ------------------------------------------------
void sha1(const uint8_t* data, size_t len, uint8_t out[20]) {
    uint32_t h[5] = {0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u, 0xC3D2E1F0u};
    size_t padded = ((len + 8) / 64 + 1) * 64;
    std::vector<uint8_t> buf(padded, 0);
    std::memcpy(buf.data(), data, len);
    buf[len] = 0x80;
    uint64_t bits = static_cast<uint64_t>(len) * 8;
    for (int i = 0; i < 8; ++i) buf[padded - 1 - i] = static_cast<uint8_t>(bits >> (8 * i));
    for (size_t off = 0; off < padded; off += 64) {
        uint32_t w[80];
        for (int i = 0; i < 16; ++i)
            w[i] = (static_cast<uint32_t>(buf[off + i * 4]) << 24) |
                   (static_cast<uint32_t>(buf[off + i * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(buf[off + i * 4 + 2]) << 8) |
                   static_cast<uint32_t>(buf[off + i * 4 + 3]);
        for (int i = 16; i < 80; ++i) {
            uint32_t v = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
            w[i] = (v << 1) | (v >> 31);
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i < 20) { f = (b & c) | ((~b) & d); k = 0x5A827999u; }
            else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1u; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDCu; }
            else { f = b ^ c ^ d; k = 0xCA62C1D6u; }
            uint32_t t = ((a << 5) | (a >> 27)) + f + e + k + w[i];
            e = d; d = c; c = (b << 30) | (b >> 2); b = a; a = t;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
    }
    for (int i = 0; i < 5; ++i) {
        out[i * 4] = static_cast<uint8_t>(h[i] >> 24);
        out[i * 4 + 1] = static_cast<uint8_t>(h[i] >> 16);
        out[i * 4 + 2] = static_cast<uint8_t>(h[i] >> 8);
        out[i * 4 + 3] = static_cast<uint8_t>(h[i]);
    }
}

void sha1(const std::string& s, uint8_t out[20]) {
    sha1(reinterpret_cast<const uint8_t*>(s.data()), s.size(), out);
}

// ---- length-encoded integers / strings ---------------------------------
struct Cursor {
    const uint8_t* base;
    const uint8_t* p;
    size_t end;
    bool ok = true;
    explicit Cursor(const std::vector<uint8_t>& b) : base(b.data()), p(b.data()), end(b.size()) {}
    size_t remaining() const { return end - static_cast<size_t>(p - base); }
    uint8_t u8() {
        if (!ok || remaining() < 1) { ok = false; return 0; }
        return *p++;
    }
    uint8_t peek() {
        if (!ok || remaining() < 1) { ok = false; return 0; }
        return *p;
    }
    uint16_t u16le() {
        uint16_t v = u8();
        v |= static_cast<uint16_t>(u8()) << 8;
        return v;
    }
    uint32_t u32le() {
        uint32_t v = u8();
        v |= static_cast<uint32_t>(u8()) << 8;
        v |= static_cast<uint32_t>(u8()) << 16;
        v |= static_cast<uint32_t>(u8()) << 24;
        return v;
    }
    uint64_t u64le() {
        uint64_t v = 0;
        for (int i = 0; i < 8; ++i) v |= static_cast<uint64_t>(u8()) << (8 * i);
        return v;
    }
    // length-encoded integer (0xFB = NULL marker, handled by the caller)
    uint64_t lenenc() {
        uint8_t b = u8();
        if (b < 0xFB) return b;
        if (b == 0xFC) return u16le();
        if (b == 0xFD) {
            uint64_t v = u8();
            v |= static_cast<uint64_t>(u8()) << 8;
            v |= static_cast<uint64_t>(u8()) << 16;
            return v;
        }
        if (b == 0xFE) return u64le();
        ok = false;  // 0xFB/0xFF should not appear here
        return 0;
    }
    std::string lenencStr() {
        uint64_t n = lenenc();
        if (!ok || remaining() < n) { ok = false; return {}; }
        std::string s(reinterpret_cast<const char*>(p), static_cast<size_t>(n));
        p += n;
        return s;
    }
    std::string nullTerm() {
        const uint8_t* q = p;
        while (q < base + end && *q != 0) ++q;
        if (q >= base + end) { ok = false; return {}; }
        std::string s(reinterpret_cast<const char*>(p), static_cast<size_t>(q - p));
        p = q + 1;
        return s;
    }
};

void appendLenenc(std::vector<uint8_t>& out, uint64_t v) {
    if (v < 0xFB) { out.push_back(static_cast<uint8_t>(v)); }
    else if (v <= 0xFFFF) { out.push_back(0xFC); out.push_back(static_cast<uint8_t>(v)); out.push_back(static_cast<uint8_t>(v >> 8)); }
    else if (v <= 0xFFFFFF) { out.push_back(0xFD); out.push_back(static_cast<uint8_t>(v)); out.push_back(static_cast<uint8_t>(v >> 8)); out.push_back(static_cast<uint8_t>(v >> 16)); }
    else { out.push_back(0xFE); for (int i = 0; i < 8; ++i) out.push_back(static_cast<uint8_t>(v >> (8 * i))); }
}
void appendLenencStr(std::vector<uint8_t>& out, const std::string& s) {
    appendLenenc(out, s.size());
    out.insert(out.end(), s.begin(), s.end());
}

bool isNumericColumn(uint8_t type) {
    switch (type) {
        case 1:   // TINY
        case 2:   // SHORT
        case 3:   // LONG
        case 4:   // FLOAT
        case 5:   // DOUBLE
        case 8:   // LONGLONG
        case 9:   // INT24
        case 16:  // BIT
        case 246: // NEWDECIMAL
            return true;
        default:
            return false;
    }
}

}  // namespace

MysqlClient::MysqlClient() {}
MysqlClient::~MysqlClient() { close(); }

bool parseMysqlHost(const std::string& s, std::string& host, int& port) {
    port = 3306;
    host.clear();
    if (s.empty()) return false;
    // "port : host" form produced by dbd's load_db_config ("%s : %s", port, host)
    size_t sep = s.find(" : ");
    if (sep != std::string::npos) {
        std::string portStr = s.substr(0, sep);
        host = s.substr(sep + 3);
        try { port = std::stoi(portStr); } catch (...) { port = 3306; }
        return !host.empty();
    }
    // "host:port"
    size_t colon = s.rfind(':');
    if (colon != std::string::npos) {
        host = s.substr(0, colon);
        try { port = std::stoi(s.substr(colon + 1)); } catch (...) { port = 3306; }
        return !host.empty();
    }
    host = s;
    return true;
}

bool MysqlClient::connect(const std::string& host, int port, const std::string& user,
                          const std::string& password, const std::string& database,
                          std::string& error) {
    close();
    ensureWsa();

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) { error = "socket() failed"; return false; }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<u_short>(port));
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
        // resolve hostname
        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        addrinfo* res = nullptr;
        if (getaddrinfo(host.c_str(), nullptr, &hints, &res) != 0 || !res) {
            closesocket(s);
            error = "cannot resolve host " + host;
            return false;
        }
        std::memcpy(&addr.sin_addr, &reinterpret_cast<sockaddr_in*>(res->ai_addr)->sin_addr, 4);
        freeaddrinfo(res);
    }

    // non-blocking connect with a timeout
    u_long mode = 1;
    ioctlsocket(s, FIONBIO, &mode);
    int rc = ::connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (rc != 0) {
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK || err == WSAEINPROGRESS) {
            fd_set wf;
            FD_ZERO(&wf);
            FD_SET(s, &wf);
            timeval tv{5, 0};
            rc = select(0, nullptr, &wf, nullptr, &tv);
            if (rc <= 0) {
                closesocket(s);
                error = "connect timeout to " + host + ":" + std::to_string(port);
                return false;
            }
            int soerr = 0;
            int len = sizeof(soerr);
            getsockopt(s, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&soerr), &len);
            if (soerr != 0) {
                closesocket(s);
                error = "connect failed to " + host + ":" + std::to_string(port);
                return false;
            }
        } else {
            closesocket(s);
            error = "connect failed to " + host + ":" + std::to_string(port);
            return false;
        }
    }
    mode = 0;
    ioctlsocket(s, FIONBIO, &mode);
    sock_ = static_cast<uintptr_t>(s);
    seq_ = 0;

    // ---- long-uptime hardening: socket read/write timeouts ----------------
    // Every query ran on a BLOCKING socket with no timeout.  After hours of
    // uptime a connection can go half-open (MySQL's own net_read_timeout=30 /
    // net_write_timeout=60 killing the session mid-statement, a NAT/firewall
    // drop, a server restart): recv() then never returns, the DBA's single ddb
    // service thread blocks FOREVER, and every later filed command is
    // dispatched but never executed or answered - which is exactly the
    // "gs 长时间运行后客户端选线路提示通讯错误(9030)" failure (_login10.py caught
    // it at gs uptime 529 min: the DBA received the runtime read 3x, dispatched
    // it, and sent no reply; its last reply to the gs was at 337 min).
    // A timeout turns that into a normal error, and mysqlExecReal's reconnect
    // then reopens the connection.
    {
        DWORD tv = 20000;   // 20 s
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));
        setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));
    }

    // Disable Nagle.  The MySQL protocol is strictly request/response with small
    // packets (a query is one send, the reply arrives after a full round trip),
    // so Nagle + delayed ACK added up to ~40ms to EVERY statement.  Measured on
    // a boot: ~2800 filed RPCs (gs -> dba -> MySQL -> gs) at p50 30ms / p90
    // 141ms, which is what made startup take 13-20 minutes; with the writes sent
    // immediately the same RPC should cost a few ms.
    {
        BOOL nodelay = TRUE;
        setsockopt(s, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&nodelay),
                   sizeof(nodelay));
    }

    if (!handshake(user, password, database, error)) {
        close();
        return false;
    }
    return true;
}

void MysqlClient::close() {
    if (sock_) {
        closesocket(static_cast<SOCKET>(sock_));
        sock_ = 0;
    }
}

bool MysqlClient::readPacket(std::vector<uint8_t>& body, std::string& error) {
    if (!sock_) { error = "not connected"; return false; }
    SOCKET s = static_cast<SOCKET>(sock_);
    uint8_t hdr[4];
    size_t got = 0;
    while (got < 4) {
        int n = recv(s, reinterpret_cast<char*>(hdr) + got, 4 - static_cast<int>(got), 0);
        if (n <= 0) {
            // n == 0 -> closed; SOCKET_ERROR with a timeout -> the peer is gone
            // (see the SO_RCVTIMEO note in connect()).  Either way the socket is
            // dead, so drop it: isConnected() then reports false and the caller
            // reconnects instead of blocking this service thread forever.
            error = (n == 0) ? "connection closed during packet header"
                             : ("read timeout/error during packet header (wsa=" +
                                std::to_string(WSAGetLastError()) + ")");
            close();
            return false;
        }
        got += static_cast<size_t>(n);
    }
    uint32_t len = static_cast<uint32_t>(hdr[0]) | (static_cast<uint32_t>(hdr[1]) << 8) | (static_cast<uint32_t>(hdr[2]) << 16);
    seq_ = hdr[3];
    body.resize(len);
    got = 0;
    while (got < len) {
        int n = recv(s, reinterpret_cast<char*>(body.data()) + got, len - static_cast<int>(got), 0);
        if (n <= 0) {
            error = (n == 0) ? "connection closed during packet body"
                             : ("read timeout/error during packet body (wsa=" +
                                std::to_string(WSAGetLastError()) + ")");
            close();
            return false;
        }
        got += static_cast<size_t>(n);
    }
    return true;
}

bool MysqlClient::writePacket(const std::vector<uint8_t>& body, std::string& error) {
    if (!sock_) { error = "not connected"; return false; }
    SOCKET s = static_cast<SOCKET>(sock_);
    uint32_t len = static_cast<uint32_t>(body.size());
    uint8_t hdr[4] = {static_cast<uint8_t>(len), static_cast<uint8_t>(len >> 8), static_cast<uint8_t>(len >> 16), seq_};
    ++seq_;
    std::vector<uint8_t> packet;
    packet.reserve(4 + body.size());
    packet.insert(packet.end(), hdr, hdr + 4);
    packet.insert(packet.end(), body.begin(), body.end());
    size_t sent = 0;
    while (sent < packet.size()) {
        int n = send(s, reinterpret_cast<const char*>(packet.data()) + sent, static_cast<int>(packet.size() - sent), 0);
        if (n <= 0) {
            error = "send failed (wsa=" + std::to_string(WSAGetLastError()) + ")";
            close();   // dead socket: let the caller reconnect
            return false;
        }
        sent += static_cast<size_t>(n);
    }
    return true;
}

bool MysqlClient::handshake(const std::string& user, const std::string& password,
                            const std::string& database, std::string& error) {
    // ---- read server greeting ----
    std::vector<uint8_t> pkt;
    if (!readPacket(pkt, error)) return false;
    Cursor c(pkt);
    uint8_t proto = c.u8();
    if (proto != 10) { error = "unsupported protocol version " + std::to_string(proto); return false; }
    c.nullTerm();  // server version
    c.u32le();     // thread id
    std::string salt1;
    for (int i = 0; i < 8; ++i) salt1 += static_cast<char>(c.u8());
    c.u8();        // filler
    uint32_t capsLow = c.u16le();
    c.u8();        // charset
    c.u16le();     // status
    uint32_t capsHigh = c.u16le();
    serverCapabilities_ = capsLow | (capsHigh << 16);
    uint8_t saltLen = c.u8();  // auth_plugin_data_len (may be 0 in old servers)
    // skip 10 bytes reserved
    for (int i = 0; i < 10 && c.ok && c.remaining() > 0; ++i) c.u8();
    // second salt part
    int second = 12;
    if (saltLen > 8) second = saltLen - 8;
    if (second > static_cast<int>(c.remaining())) second = static_cast<int>(c.remaining());
    std::string salt2;
    for (int i = 0; i < second; ++i) salt2 += static_cast<char>(c.u8());
    // strip trailing NUL
    while (!salt2.empty() && salt2.back() == '\0') salt2.pop_back();
    serverSalt_ = salt1 + salt2;
    if (serverSalt_.size() < 8) { error = "bad handshake salt"; return false; }

    // ---- build HandshakeResponse41 ----
    const uint32_t CLIENT_LONG_PASSWORD = 0x00000001;
    const uint32_t CLIENT_LONG_FLAG = 0x00000004;
    const uint32_t CLIENT_CONNECT_WITH_DB = 0x00000008;
    const uint32_t CLIENT_PROTOCOL_41 = 0x00000200;
    const uint32_t CLIENT_TRANSACTIONS = 0x00002000;
    const uint32_t CLIENT_SECURE_CONNECTION = 0x00008000;
    const uint32_t CLIENT_MULTI_RESULTS = 0x00020000;
    uint32_t caps = CLIENT_LONG_PASSWORD | CLIENT_LONG_FLAG | CLIENT_CONNECT_WITH_DB |
                    CLIENT_PROTOCOL_41 | CLIENT_TRANSACTIONS | CLIENT_SECURE_CONNECTION |
                    CLIENT_MULTI_RESULTS;
    caps &= serverCapabilities_;

    std::vector<uint8_t> auth;
    if (!password.empty()) {
        uint8_t h1[20], h2[20], h3[20];
        sha1(password, h1);
        std::string h1s(reinterpret_cast<char*>(h1), 20);
        sha1(h1s, h2);
        std::string salted = serverSalt_ + std::string(reinterpret_cast<char*>(h2), 20);
        sha1(salted, h3);
        auth.resize(20);
        for (int i = 0; i < 20; ++i) auth[i] = h1[i] ^ h3[i];
    }

    std::vector<uint8_t> resp;
    resp.reserve(64 + user.size() + auth.size() + database.size());
    resp.push_back(static_cast<uint8_t>(caps));
    resp.push_back(static_cast<uint8_t>(caps >> 8));
    resp.push_back(static_cast<uint8_t>(caps >> 16));
    resp.push_back(static_cast<uint8_t>(caps >> 24));
    // max packet size
    resp.push_back(0x00); resp.push_back(0x00); resp.push_back(0x00); resp.push_back(0x01);
    resp.push_back(8);  // charset: latin1 (same default the original client used)
    resp.insert(resp.end(), 23, 0);  // reserved
    resp.insert(resp.end(), user.begin(), user.end());
    resp.push_back(0);
    resp.push_back(static_cast<uint8_t>(auth.size()));
    resp.insert(resp.end(), auth.begin(), auth.end());
    if (caps & CLIENT_CONNECT_WITH_DB) {
        resp.insert(resp.end(), database.begin(), database.end());
        resp.push_back(0);
    }

    // The server greeting is packet seq 0; our handshake response must be 1.
    seq_ = 1;
    if (!writePacket(resp, error)) return false;

    // ---- read auth result ----
    std::vector<uint8_t> ack;
    if (!readPacket(ack, error)) return false;
    if (ack.empty()) { error = "empty auth response"; return false; }
    if (ack[0] == 0x00) return true;  // OK
    if (ack[0] == 0xFF) {
        Cursor e(ack);
        e.u8();
        uint16_t code = e.u16le();
        error = "auth failed (" + std::to_string(code) + "): " + e.nullTerm();
        return false;
    }
    error = "unexpected auth response";
    return false;
}

bool MysqlClient::execute(const std::string& sql, Result& out, std::string& error) {
    // LPCVM_SQLLOG=<file>: append one line per statement to its OWN file, so the
    // MySQL traffic can be inspected without wading through the service logs.
    // Records: wall ms, elapsed us, verdict, rows/affected, and the full SQL.
    // Enabled only when the env var names a file - zero cost otherwise.
    static const char* sqlLogPath = getenv("LPCVM_SQLLOG");
    static std::mutex sqlLogMx;
    const int64_t sqlT0 = sqlLogPath
        ? std::chrono::duration_cast<std::chrono::microseconds>(
              std::chrono::steady_clock::now().time_since_epoch()).count()
        : 0;
    auto sqlLog = [&](const char* verdict, int64_t rows) {
        if (!sqlLogPath) return;
        const int64_t us = std::chrono::duration_cast<std::chrono::microseconds>(
                               std::chrono::steady_clock::now().time_since_epoch()).count() - sqlT0;
        std::lock_guard<std::mutex> lk(sqlLogMx);
        static std::ofstream f(sqlLogPath, std::ios::app);
        if (!f) return;
        std::string one = sql;
        for (char& c : one) if (c == '\n' || c == '\r') c = ' ';
        const int64_t wallMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::system_clock::now().time_since_epoch()).count();
        f << "t=" << wallMs << "ms us=" << us << " " << verdict
          << " rows=" << rows << " err=" << (error.empty() ? "-" : error.substr(0, 60))
          << " sql=" << one.substr(0, 400) << "\n";
        f.flush();
    };

    if (!sock_) { error = "not connected"; sqlLog("NOCONN", -1); return false; }
    std::vector<uint8_t> q;
    q.reserve(1 + sql.size());
    q.push_back(0x03);  // COM_QUERY
    q.insert(q.end(), sql.begin(), sql.end());
    seq_ = 0;  // every new command starts a fresh sequence
    if (!writePacket(q, error)) { sqlLog("WRITEFAIL", -1); return false; }

    std::vector<uint8_t> first;
    if (!readPacket(first, error)) { sqlLog("READFAIL", -1); return false; }
    if (first.empty()) { error = "empty response"; sqlLog("EMPTYRESP", -1); return false; }
    if (first[0] == 0xFF) {
        Cursor e(first);
        e.u8();
        uint16_t code = e.u16le();
        std::string msg;
        if (e.ok && e.remaining() >= 6 && e.p[0] == '#') { e.u8(); e.u8(); e.u8(); e.u8(); e.u8(); e.u8(); }
        msg = e.nullTerm();
        error = "SQL error " + std::to_string(code) + ": " + msg;
        sqlLog("SQLERR", -1);
        return false;
    }
    if (first[0] == 0x00) {
        // OK packet (write statement)
        Cursor o(first);
        o.u8();
        out.affectedRows = static_cast<int64_t>(o.lenenc());
        out.insertId = static_cast<int64_t>(o.lenenc());
        out.hasResultSet = false;
        sqlLog("OK-WRITE", out.affectedRows);
        return true;
    }
    if (first[0] == 0xFE && first.size() < 9) {
        // EOF immediately: empty result set (no columns)
        out.hasResultSet = true;
        return true;
    }

    // ---- result set ----
    Cursor cc(first);
    uint64_t colCount = cc.lenenc();
    if (!cc.ok) { error = "bad column count"; return false; }
    for (uint64_t i = 0; i < colCount; ++i) {
        std::vector<uint8_t> def;
        if (!readPacket(def, error)) return false;
        Cursor d(def);
        d.lenencStr();  // catalog
        d.lenencStr();  // schema
        d.lenencStr();  // table
        d.lenencStr();  // org_table
        std::string name = d.lenencStr();
        d.lenencStr();  // org_name
        if (!d.ok) { error = "bad column definition"; return false; }
        d.lenenc();     // length of fixed fields
        d.u16le();      // charset
        d.u32le();      // column length
        uint8_t type = d.u8();
        d.u16le();      // flags
        d.u8();         // decimals
        out.cols.push_back(name);
        out.colTypes_.push_back(type);
    }
    // EOF after column definitions
    std::vector<uint8_t> eof1;
    if (!readPacket(eof1, error)) return false;
    // rows
    for (;;) {
        std::vector<uint8_t> row;
        if (!readPacket(row, error)) return false;
        if (row.empty()) { error = "empty row packet"; return false; }
        if (row[0] == 0xFE && row.size() < 9) break;  // final EOF
        Cursor r(row);
        std::vector<Cell> cells;
        for (size_t i = 0; i < out.cols.size(); ++i) {
            Cell cell;
            if (r.peek() == 0xFB) {
                r.u8();
                cell.isNull = true;
            } else {
                std::string val = r.lenencStr();
                if (!r.ok) { error = "truncated row value"; return false; }
                if (i < out.colTypes_.size() && isNumericColumn(out.colTypes_[i])) {
                    cell.isNumber = true;
                    cell.num = static_cast<int64_t>(std::strtoll(val.c_str(), nullptr, 10));
                    cell.str = val;  // keep raw text (decimal columns)
                } else {
                    cell.str = val;
                }
            }
            cells.push_back(std::move(cell));
        }
        out.rows.push_back(std::move(cells));
    }
    out.hasResultSet = true;
    sqlLog("OK-READ", static_cast<int64_t>(out.rows.size()));
    return true;
}

}  // namespace lpc
