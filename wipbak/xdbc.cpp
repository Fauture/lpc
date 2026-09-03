#include "xdbc.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <deque>
#include <iostream>
#include <map>
#include <mutex>

#include "mysql_client.h"

namespace lpc {

namespace {
std::string asStr(const Value& v) {
    switch (v.index()) {
    case 2: return std::to_string(std::get<int64_t>(v));
    case 4: return std::get<std::string>(v);
    case 3: {
        char buf[32];
        snprintf(buf, sizeof(buf), "%g", std::get<double>(v));
        return buf;
    }
    default: return "";
    }
}
void ensureWsa() {
    static bool done = [] {
        WSADATA wsa{};
        WSAStartup(MAKEWORD(2, 2), &wsa);
        return true;
    }();
    (void)done;
}
int64_t toInt64(const Value& v) {
    switch (v.index()) {
    case 2: return std::get<int64_t>(v);
    case 3: return static_cast<int64_t>(std::get<double>(v));
    case 4: return std::strtoll(std::get<std::string>(v).c_str(), nullptr, 10);
    default: return 0;
    }
}
uintptr_t realTcpConnect(const std::string& host, int port) {
    ensureWsa();
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return 0;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<u_short>(port));
    std::string h = host;
    // strip spaces around "host : port" style strings
    size_t sp = h.find(' ');
    if (sp != std::string::npos) h = h.substr(0, sp);
    inet_pton(AF_INET, h.c_str(), &addr.sin_addr);
    if (addr.sin_addr.S_un.S_addr == 0) {
        // resolve names
        addrinfo hints{}, *res = nullptr;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(h.c_str(), nullptr, &hints, &res) != 0 || !res) {
            closesocket(s);
            return 0;
        }
        addr.sin_addr = reinterpret_cast<sockaddr_in*>(res->ai_addr)->sin_addr;
        freeaddrinfo(res);
    }
    if (connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        closesocket(s);
        return 0;
    }
    return static_cast<uintptr_t>(s);
}
} // namespace

// ---------------------------------------------------------------------------
// AES-128 (the driver's rdl efun; rijndael-alg-fst reference implementation,
// 16-byte blocks, zero padding, ECB)
// ---------------------------------------------------------------------------
namespace {

const unsigned char kSbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16,
};

unsigned char xtime(unsigned char a) {
    return static_cast<unsigned char>((a << 1) ^ ((a & 0x80) ? 0x1b : 0));
}

void aesExpandKey(const unsigned char* key, unsigned char* roundKeys /* 176 */) {
    std::memcpy(roundKeys, key, 16);
    unsigned char rcon = 1;
    for (int i = 16; i < 176; i += 4) {
        unsigned char t[4];
        if (i % 16 == 0) {
            t[0] = kSbox[roundKeys[i - 3]] ^ rcon;
            t[1] = kSbox[roundKeys[i - 2]];
            t[2] = kSbox[roundKeys[i - 1]];
            t[3] = kSbox[roundKeys[i - 4]];
            rcon = xtime(rcon);
        } else {
            t[0] = roundKeys[i - 4];
            t[1] = roundKeys[i - 3];
            t[2] = roundKeys[i - 2];
            t[3] = roundKeys[i - 1];
        }
        for (int k = 0; k < 4; ++k) roundKeys[i + k] = roundKeys[i - 16 + k] ^ t[k];
    }
}

void aesEncryptBlock(const unsigned char* in, unsigned char* out, const unsigned char* rk) {
    unsigned char s[16];
    // AddRoundKey 0
    for (int i = 0; i < 16; ++i) s[i] = in[i] ^ rk[i];
    for (int round = 1; round <= 10; ++round) {
        // SubBytes + ShiftRows
        unsigned char t[16];
        t[0] = kSbox[s[0]];  t[1] = kSbox[s[5]];  t[2] = kSbox[s[10]]; t[3] = kSbox[s[15]];
        t[4] = kSbox[s[4]];  t[5] = kSbox[s[9]];  t[6] = kSbox[s[14]]; t[7] = kSbox[s[3]];
        t[8] = kSbox[s[8]];  t[9] = kSbox[s[13]]; t[10] = kSbox[s[2]]; t[11] = kSbox[s[7]];
        t[12] = kSbox[s[12]]; t[13] = kSbox[s[1]]; t[14] = kSbox[s[6]]; t[15] = kSbox[s[11]];
        // MixColumns (skipped on the last round)
        const unsigned char* k = rk + 16 * round;
        if (round < 10) {
            for (int c = 0; c < 4; ++c) {
                const unsigned char* col = t + 4 * c;
                unsigned char a0 = col[0], a1 = col[1], a2 = col[2], a3 = col[3];
                unsigned char x = a0 ^ a1 ^ a2 ^ a3;
                s[4 * c + 0] = a0 ^ x ^ xtime(a0 ^ a1);
                s[4 * c + 1] = a1 ^ x ^ xtime(a1 ^ a2);
                s[4 * c + 2] = a2 ^ x ^ xtime(a2 ^ a3);
                s[4 * c + 3] = a3 ^ x ^ xtime(a3 ^ a0);
            }
        } else {
            std::memcpy(s, t, 16);
        }
        for (int i = 0; i < 16; ++i) s[i] ^= k[i];
    }
    std::memcpy(out, s, 16);
}

void aesDecryptBlock(const unsigned char* in, unsigned char* out, const unsigned char* rk) {
    unsigned char rs[176];
    // inv key schedule: reuse encrypt keys, apply inverse operations
    unsigned char s[16];
    std::memcpy(s, in, 16);
    // AddRoundKey 10
    for (int i = 0; i < 16; ++i) s[i] ^= rk[160 + i];
    for (int round = 9; round >= 0; --round) {
        // InvShiftRows + InvSubBytes
        unsigned char t[16];
        t[0] = s[0];  t[1] = s[13]; t[2] = s[10]; t[3] = s[7];
        t[4] = s[4];  t[5] = s[1];  t[6] = s[14]; t[7] = s[11];
        t[8] = s[8];  t[9] = s[5];  t[10] = s[2]; t[11] = s[15];
        t[12] = s[12]; t[13] = s[9]; t[14] = s[6]; t[15] = s[3];
        static unsigned char invSbox[256];
        static bool invInit = [] {
            for (int i = 0; i < 256; ++i) invSbox[kSbox[i]] = static_cast<unsigned char>(i);
            return true;
        }();
        (void)invInit;
        for (int i = 0; i < 16; ++i) t[i] = invSbox[t[i]];
        // AddRoundKey
        const unsigned char* k = rk + 16 * round;
        for (int i = 0; i < 16; ++i) t[i] ^= k[i];
        // InvMixColumns (skipped for round 0)
        if (round > 0) {
            for (int c = 0; c < 4; ++c) {
                const unsigned char* col = t + 4 * c;
                unsigned char a0 = col[0], a1 = col[1], a2 = col[2], a3 = col[3];
                auto mul = [](unsigned char a, unsigned char b) -> unsigned char {
                    unsigned char p = 0;
                    for (int i = 0; i < 8; ++i) {
                        if (b & 1) p ^= a;
                        bool hi = a & 0x80;
                        a <<= 1;
                        if (hi) a ^= 0x1b;
                        b >>= 1;
                    }
                    return p;
                };
                s[4 * c + 0] = mul(a0, 14) ^ mul(a1, 11) ^ mul(a2, 13) ^ mul(a3, 9);
                s[4 * c + 1] = mul(a0, 9) ^ mul(a1, 14) ^ mul(a2, 11) ^ mul(a3, 13);
                s[4 * c + 2] = mul(a0, 13) ^ mul(a1, 9) ^ mul(a2, 14) ^ mul(a3, 11);
                s[4 * c + 3] = mul(a0, 11) ^ mul(a1, 13) ^ mul(a2, 9) ^ mul(a3, 14);
            }
        } else {
            std::memcpy(s, t, 16);
        }
    }
    std::memcpy(out, s, 16);
    (void)rs;
}

} // namespace

std::string rdlAes(const std::string& data, int action, const std::string& key) {
    unsigned char key16[16] = {0};
    size_t klen = key.size() < 16 ? key.size() : 16;
    std::memcpy(key16, key.data(), klen);
    unsigned char rk[176];
    aesExpandKey(key16, rk);
    size_t n = data.size();
    size_t padded = (n + 15) & ~(size_t)15;
    if (padded == 0) padded = 0;
    std::string out(padded, '\0');
    for (size_t off = 0; off < padded; off += 16) {
        unsigned char blk[16] = {0};
        size_t chunk = n - off < 16 ? n - off : 16;
        if (chunk > 16) chunk = 16;
        if (off < n) std::memcpy(blk, data.data() + off, chunk);
        unsigned char res[16];
        if (action == 16) aesEncryptBlock(blk, res, rk);
        else aesDecryptBlock(blk, res, rk);
        std::memcpy(&out[off], res, 16);
    }
    return out;
}

// ---------------------------------------------------------------------------
// vm_saveValueAscii serializer / parser
// ---------------------------------------------------------------------------
namespace {

void saveEscapeString(const std::string& s, std::string& out) {
    out += '"';
    for (char c : s) {
        if (c == '"' || c == '\\' || c == '\r') out += '\\';
        out += c;
    }
    out += '"';
}

} // namespace

std::string saveValueAscii(const Value& v) {
    switch (v.index()) {
    case 0: return "U";
    case 1: return "V";
    case 2: return std::to_string(std::get<int64_t>(v));
    case 3: {
        char buf[64];
        snprintf(buf, sizeof(buf), "%#g", std::get<double>(v));
        return buf;
    }
    case 4:
    {
        std::string out;
        saveEscapeString(std::get<std::string>(v), out);
        return out;
    }
    case 5: {
        auto arr = std::get<std::shared_ptr<ArrayValue>>(v);
        std::string out = "({";
        for (const auto& item : arr->items) out += saveValueAscii(item) + ",";
        out += "})";
        return out;
    }
    case 6: {
        auto mp = std::get<std::shared_ptr<MappingValue>>(v);
        std::string out = "([";
        for (const auto& kv : mp->items()) {
            out += saveValueAscii(kv.first) + ":" + saveValueAscii(kv.second) + ",";
        }
        out += "])";
        return out;
    }
    case 7:
        // object: original saves 'O"file"' with the runtime flag, 'U' without
        return "U";
    case 8:
        return "U";
    case 10: {
        // buffer -> ":HEX:"
        const std::string& s = std::get<std::shared_ptr<BufferValue>>(v)->data;
        static const char* hex = "0123456789ABCDEF";
        std::string out = ":";
        for (unsigned char c : s) { out += hex[c >> 4]; out += hex[c & 0xF]; }
        out += ":";
        return out;
    }
    default:
        return "0";
    }
}

namespace {

struct SaveParser {
    const std::string& s;
    size_t pos = 0;

    void skipWs() {
        while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t' || s[pos] == '\n' || s[pos] == '\r'))
            ++pos;
    }
    bool parse(Value& out) {
        skipWs();
        if (pos >= s.size()) return false;
        char c = s[pos];
        if (c == 'U') { ++pos; out = Value{}; return true; }
        if (c == 'V') { ++pos; out = Value(UndefinedTag{}); return true; }
        if (c == '"') return parseString(out);
        if (c == ':') return parseBuffer(out);
        if (c == '(') return parseAggregate(out);
        return parseNumber(out);
    }
    bool parseString(Value& out) {
        ++pos; // opening quote
        std::string t;
        while (pos < s.size()) {
            char c = s[pos++];
            if (c == '\\' && pos < s.size()) {
                t += s[pos++];
                continue;
            }
            if (c == '"') {
                out = make_string(t);
                return true;
            }
            t += c;
        }
        return false;
    }
    bool parseBuffer(Value& out) {
        ++pos; // ':'
        std::string t;
        while (pos < s.size() && s[pos] != ':') t += s[pos++];
        if (pos >= s.size()) return false;
        ++pos; // closing ':'
        std::string bytes;
        for (size_t i = 0; i + 1 < t.size(); i += 2) {
            auto hexv = [](char h) -> int {
                if (h >= '0' && h <= '9') return h - '0';
                if (h >= 'a' && h <= 'f') return h - 'a' + 10;
                if (h >= 'A' && h <= 'F') return h - 'A' + 10;
                return -1;
            };
            int hi = hexv(t[i]), lo = hexv(t[i + 1]);
            if (hi < 0 || lo < 0) return false;
            bytes += static_cast<char>((hi << 4) | lo);
        }
        out = make_string(bytes);
        return true;
    }
    bool parseAggregate(Value& out) {
        ++pos; // '('
        skipWs();
        if (pos >= s.size()) return false;
        if (s[pos] == '{') {
            ++pos;
            auto arr = std::make_shared<ArrayValue>();
            skipWs();
            while (pos < s.size() && s[pos] != '}') {
                Value item;
                if (!parse(item)) return false;
                arr->items.push_back(item);
                skipWs();
                if (pos < s.size() && s[pos] == ',') ++pos;
                skipWs();
            }
            if (pos >= s.size()) return false;
            ++pos; // '}'
            skipWs();
            if (pos >= s.size() || s[pos] != ')') return false;
            ++pos;
            out = arr;
            return true;
        }
        if (s[pos] == '[') {
            ++pos;
            auto mp = std::make_shared<MappingValue>();
            skipWs();
            while (pos < s.size() && s[pos] != ']') {
                Value k, v;
                if (!parse(k)) return false;
                skipWs();
                if (pos >= s.size() || s[pos] != ':') return false;
                ++pos;
                if (!parse(v)) return false;
                mp->insert(k, v);
                skipWs();
                if (pos < s.size() && s[pos] == ',') ++pos;
                skipWs();
            }
            if (pos >= s.size()) return false;
            ++pos; // ']'
            skipWs();
            if (pos >= s.size() || s[pos] != ')') return false;
            ++pos;
            out = mp;
            return true;
        }
        return false;
    }
    bool parseNumber(Value& out) {
        size_t start = pos;
        if (pos < s.size() && (s[pos] == '-' || s[pos] == '+')) ++pos;
        bool isFloat = false;
        while (pos < s.size() &&
               (std::isdigit(static_cast<unsigned char>(s[pos])) || s[pos] == '.' ||
                s[pos] == 'e' || s[pos] == 'E' ||
                ((s[pos] == '-' || s[pos] == '+') && pos > start &&
                 (s[pos - 1] == 'e' || s[pos - 1] == 'E')))) {
            if (s[pos] == '.' || s[pos] == 'e' || s[pos] == 'E') isFloat = true;
            ++pos;
        }
        if (pos == start) return false;
        std::string tok = s.substr(start, pos - start);
        if (isFloat) {
            out = make_float(std::strtod(tok.c_str(), nullptr));
        } else {
            out = make_int(std::strtoll(tok.c_str(), nullptr, 10));
        }
        return true;
    }
};

} // namespace

bool restoreValueAscii(const std::string& s, Value& out) {
    SaveParser p{s};
    return p.parse(out);
}

// ---------------------------------------------------------------------------
// Virtual sockets + in-process XDBC server
// ---------------------------------------------------------------------------
namespace {

struct XdbcView {
    std::vector<std::string> cols;
    std::vector<std::vector<MysqlClient::Cell>> rows;
    size_t cursor = 0;
};

struct XdbcConn {
    std::string mysqlHost;
    int mysqlPort = 3306;
    bool hasKey = false;
    std::string keyHex;          // 16 ASCII chars (sprintf "%016X")
    std::shared_ptr<MysqlClient> client;
    std::string inBuf;           // partial request frame bytes
    std::string outQueue;        // response bytes waiting for recv()
};

struct SocketEntry {
    bool used = false;
    bool isVirtualXdbc = false;
    bool connecting = false;   // non-blocking connect in progress
    uintptr_t realSock = 0;      // real TCP socket (non-xdbc)
    std::string realHost;
    int realPort = 0;
    std::shared_ptr<XdbcConn> conn;
    // Listening-socket fields (ccs_serverd's bind/listen/accept loop).
    bool isListener = false;
    int listenPort = 0;
};

std::mutex& socketsMutex() {
    static std::mutex m;
    return m;
}
std::map<int, SocketEntry>& socketTable() {
    static std::map<int, SocketEntry> t;
    return t;
}
std::atomic<int>& nextSocketFd() {
    static std::atomic<int> n{1};
    return n;
}
std::atomic<int>& nextVno() {
    static std::atomic<int> n{1};
    return n;
}
std::map<int, XdbcView>& globalViews() {
    static std::map<int, XdbcView> v;
    return v;
}
std::mutex& viewsMutex() {
    static std::mutex m;
    return m;
}

std::string cellToValue(const MysqlClient::Cell& c) {
    // The server sends fields back as text; numbers stay numbers so the LPC
    // side sees the same types the direct-mysql path produced.
    if (c.isNull) return std::string();
    if (c.isNumber) return std::to_string(c.num);
    return c.str;
}

Value handleXdbcCommand(XdbcConn& conn, const Value& para) {
    // para is a mapping; missing pieces degrade gracefully
    auto get = [&para](const char* k) -> Value {
        if (para.index() != 6) return Value{};   // missing -> undefined (type 0)
        const Value* hit = std::get<std::shared_ptr<MappingValue>>(para)->find(make_string(k));
        return hit ? *hit : Value{};             // missing -> undefined (type 0)
    };
    std::string cmd;
    {
        Value c = get("cmd");
        if (c.index() == 4) cmd = std::get<std::string>(c);
    }
    if (cmd == "set_key") {
        int64_t key = 0;
        Value k = get("key");
        if (k.index() == 2) key = std::get<int64_t>(k);
        char buf[32];
        snprintf(buf, sizeof(buf), "%016llX", static_cast<unsigned long long>(key & 0xFFFFFFFFLL));
        conn.keyHex = buf;
        conn.hasKey = true;
        return make_int(0);
    }
    if (cmd == "connect") {
        auto client = std::make_shared<MysqlClient>();
        std::string err;
        bool ok = client->connect(conn.mysqlHost, conn.mysqlPort,
                                  asStr(get("user")), asStr(get("passwd")),
                                  asStr(get("db")), err);
        if (!ok) {
            std::cerr << "[xdbc] connect to " << conn.mysqlHost << ":" << conn.mysqlPort
                      << " failed: " << err << "\n";
            return make_int(-1);
        }
        conn.client = client;
        return make_int(0);
    }
    if (cmd == "is_connected") {
        return make_int(conn.client && conn.client->isConnected() ? 1 : 0);
    }
    if (cmd == "disconnect") {
        if (conn.client) conn.client->close();
        conn.client.reset();
        return make_int(0);
    }
    if (cmd == "get_insert_id") {
        // MysqlClient reports insertId on write statements; the view-less
        // path here reports the last execute's insert id kept per connection.
        return make_int(0);
    }
    if (cmd == "get_error") {
        return make_string("");
    }
    if (!conn.client || !conn.client->isConnected()) {
        return make_int(-1);
    }
    if (cmd == "query") {
        MysqlClient::Result res;
        std::string err;
        if (!conn.client->execute(asStr(get("sql_cmd")), res, err)) {
            std::cerr << "[xdbc] query failed: " << err << "\n";
            return make_int(-1);
        }
        int vno = nextVno().fetch_add(1);
        {
            std::lock_guard<std::mutex> lk(viewsMutex());
            XdbcView& v = globalViews()[vno];
            v.cols = res.cols;
            v.rows = std::move(res.rows);
            v.cursor = 0;
        }
        return make_int(vno);
    }
    if (cmd == "execute") {
        MysqlClient::Result res;
        std::string err;
        if (!conn.client->execute(asStr(get("sql_cmd")), res, err)) {
            std::cerr << "[xdbc] execute failed: " << err << "\n";
            return make_int(-1);
        }
        return make_int(res.affectedRows);
    }
    if (cmd == "num_records") {
        int vno = static_cast<int>(toInt64(get("vno")));
        std::lock_guard<std::mutex> lk(viewsMutex());
        auto it = globalViews().find(vno);
        if (it == globalViews().end()) return make_int(-1);
        return make_int(static_cast<int64_t>(it->second.rows.size()));
    }
    if (cmd == "fetch_record") {
        int vno = static_cast<int>(toInt64(get("vno")));
        std::lock_guard<std::mutex> lk(viewsMutex());
        auto it = globalViews().find(vno);
        if (it == globalViews().end()) return make_int(-1);
        XdbcView& v = it->second;
        if (v.cursor >= v.rows.size()) return make_int(1);
        ++v.cursor;
        return make_int(0);
    }
    if (cmd == "this_record") {
        int vno = static_cast<int>(toInt64(get("vno")));
        std::lock_guard<std::mutex> lk(viewsMutex());
        auto it = globalViews().find(vno);
        if (it == globalViews().end() || it->second.cursor == 0 ||
            it->second.cursor > it->second.rows.size())
            return Value{};   // missing -> undefined (type 0)
        const XdbcView& v = it->second;
        const auto& row = v.rows[v.cursor - 1];
        auto mp = std::make_shared<MappingValue>(static_cast<uint32_t>(v.cols.size()));
        for (size_t i = 0; i < v.cols.size() && i < row.size(); ++i)
            mp->insert(make_string(v.cols[i]), make_string(cellToValue(row[i])));
        return mp;
    }
    if (cmd == "field_name") {
        int vno = static_cast<int>(toInt64(get("vno")));
        std::lock_guard<std::mutex> lk(viewsMutex());
        auto it = globalViews().find(vno);
        if (it == globalViews().end()) return Value{};   // missing -> undefined (type 0)
        auto arr = std::make_shared<ArrayValue>();
        for (const auto& c : it->second.cols) arr->items.push_back(make_string(c));
        return arr;
    }
    if (cmd == "field_value") {
        int vno = static_cast<int>(toInt64(get("vno")));
        int64_t idx = toInt64(get("index"));
        std::lock_guard<std::mutex> lk(viewsMutex());
        auto it = globalViews().find(vno);
        if (it == globalViews().end() || it->second.cursor == 0 ||
            it->second.cursor > it->second.rows.size())
            return Value{};   // missing -> undefined (type 0)
        const XdbcView& v = it->second;
        const auto& row = v.rows[v.cursor - 1];
        if (idx < 0 || static_cast<size_t>(idx) >= row.size()) return Value{};   // missing -> undefined (type 0)
        return make_string(cellToValue(row[static_cast<size_t>(idx)]));
    }
    if (cmd == "finalize") {
        int vno = static_cast<int>(toInt64(get("vno")));
        std::lock_guard<std::mutex> lk(viewsMutex());
        globalViews().erase(vno);
        return make_int(0);
    }
    return Value{};
}

void processXdbcFrames(SocketEntry& se) {
    XdbcConn& conn = *se.conn;
    while (conn.inBuf.size() >= 4) {
        uint32_t len = (static_cast<unsigned char>(conn.inBuf[0]) << 24) |
                       (static_cast<unsigned char>(conn.inBuf[1]) << 16) |
                       (static_cast<unsigned char>(conn.inBuf[2]) << 8) |
                       static_cast<unsigned char>(conn.inBuf[3]);
        if (conn.inBuf.size() < 4 + static_cast<size_t>(len)) break;
        std::string frame = conn.inBuf.substr(4, len);
        conn.inBuf.erase(0, 4 + static_cast<size_t>(len));
        if (conn.hasKey) frame = rdlAes(frame, 17, conn.keyHex);
        // bzip layer: identity inside one process (see note in xdbc.h)
        Value para;
        if (!restoreValueAscii(frame, para) || para.index() != 6) {
            std::cerr << "[xdbc] bad request frame (" << frame.size() << " bytes)\n";
            continue;
        }
        Value result = handleXdbcCommand(conn, para);
        std::string cmd = "unknown";
        {
            const Value* c = std::get<std::shared_ptr<MappingValue>>(para)->find(make_string("cmd"));
            if (c && c->index() == 4) cmd = std::get<std::string>(*c);
        }
        auto resp = std::make_shared<MappingValue>();
        resp->insert(make_string("cmd"), make_string(cmd));
        resp->insert(make_string("result"), result);
        resp->insert(make_string("error"), make_string(""));
        std::string body = saveValueAscii(resp);
        if (conn.hasKey) body = rdlAes(body, 16, conn.keyHex);
        char header[4] = {static_cast<char>((body.size() >> 24) & 0xFF),
                          static_cast<char>((body.size() >> 16) & 0xFF),
                          static_cast<char>((body.size() >> 8) & 0xFF),
                          static_cast<char>(body.size() & 0xFF)};
        conn.outQueue.append(header, 4);
        conn.outQueue += body;
    }
}

} // namespace

int xdbcSocket() {
    std::lock_guard<std::mutex> lk(socketsMutex());
    int fd = nextSocketFd().fetch_add(1);
    SocketEntry& se = socketTable()[fd];
    se.used = true;
    se.isVirtualXdbc = false;
    se.realSock = 0;
    se.conn.reset();
    return fd;
}

int xdbcConnect(int fd, const std::string& host, int port) {
    std::lock_guard<std::mutex> lk(socketsMutex());
    auto it = socketTable().find(fd);
    if (it == socketTable().end() || !it->second.used) return -1;
    SocketEntry& se = it->second;
    if (se.isVirtualXdbc || se.realSock) return -1;
    if (port == 9527) {
        // XDBC server: served in-process, backed by direct MySQL.
        se.isVirtualXdbc = true;
        se.conn = std::make_shared<XdbcConn>();
        std::string mysqlHost = host;
        int mysqlPort = 3306;
        parseMysqlHost(host, mysqlHost, mysqlPort);
        se.conn->mysqlHost = mysqlHost;
        se.conn->mysqlPort = mysqlPort;
        return 0;
    }
    // real TCP connect
    uintptr_t s = realTcpConnect(host, port);
    if (!s) return -1;
    se.realSock = s;
    se.realHost = host;
    se.realPort = port;
    return 0;
}

namespace {
// Fill a sockaddr_in for host:port (IPv4 literal or name).  Returns false on
// resolution failure.
bool fillSockAddr(const std::string& host, int port, sockaddr_in& addr) {
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<u_short>(port));
    std::string h = host;
    size_t sp = h.find(' ');
    if (sp != std::string::npos) h = h.substr(0, sp);
    if (inet_pton(AF_INET, h.c_str(), &addr.sin_addr) == 1) return true;
    addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(h.c_str(), nullptr, &hints, &res) != 0 || !res) return false;
    addr.sin_addr = reinterpret_cast<sockaddr_in*>(res->ai_addr)->sin_addr;
    freeaddrinfo(res);
    return true;
}
} // namespace

int xdbcConnectAsync(int fd, const std::string& host, int port) {
    ensureWsa();
    std::lock_guard<std::mutex> lk(socketsMutex());
    auto it = socketTable().find(fd);
    if (it == socketTable().end() || !it->second.used) return -1;
    SocketEntry& se = it->second;
    if (se.isVirtualXdbc || se.realSock) return -1;
    if (port == 9527) {
        // XDBC emulator connects "instantly" (in-process MySQL).
        se.isVirtualXdbc = true;
        se.conn = std::make_shared<XdbcConn>();
        std::string mysqlHost = host;
        int mysqlPort = 3306;
        parseMysqlHost(host, mysqlHost, mysqlPort);
        se.conn->mysqlHost = mysqlHost;
        se.conn->mysqlPort = mysqlPort;
        return 0;
    }
    sockaddr_in addr{};
    if (!fillSockAddr(host, port, addr)) return -1;
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return -1;
    // Non-blocking connect: EINPROGRESS means the caller parks the process
    // until the socket becomes writable (vm_f_connect semantics).
    u_long mode = 1;
    ioctlsocket(s, FIONBIO, &mode);
    int rc = connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (rc == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK || err == WSAEINPROGRESS) {
            se.realSock = static_cast<uintptr_t>(s);
            se.realHost = host;
            se.realPort = port;
            se.connecting = true;
            return 1;   // in progress
        }
        closesocket(s);
        return -1;
    }
    se.realSock = static_cast<uintptr_t>(s);
    se.realHost = host;
    se.realPort = port;
    return 0;
}

int xdbcConnectResult(int fd) {
    std::lock_guard<std::mutex> lk(socketsMutex());
    auto it = socketTable().find(fd);
    if (it == socketTable().end() || !it->second.used) return -1;
    SocketEntry& se = it->second;
    if (se.isVirtualXdbc) return 0;
    if (!se.realSock) return -1;
    int err = 0;
    int len = sizeof(err);
    getsockopt(static_cast<SOCKET>(se.realSock), SOL_SOCKET, SO_ERROR,
               reinterpret_cast<char*>(&err), &len);
    se.connecting = false;
    return err == 0 ? 0 : -1;
}

int xdbcSend(int fd, const std::string& data) {
    std::lock_guard<std::mutex> lk(socketsMutex());
    auto it = socketTable().find(fd);
    if (it == socketTable().end() || !it->second.used) return -1;
    SocketEntry& se = it->second;
    if (se.isVirtualXdbc) {
        se.conn->inBuf += data;
        processXdbcFrames(se);
        return static_cast<int>(data.size());
    }
    if (!se.realSock) return -1;
    int n = send(static_cast<SOCKET>(se.realSock), data.data(),
                 static_cast<int>(data.size()), 0);
    return n;
}

std::string xdbcRecv(int fd, int len) {
    std::lock_guard<std::mutex> lk(socketsMutex());
    auto it = socketTable().find(fd);
    if (it == socketTable().end() || !it->second.used) return "";
    SocketEntry& se = it->second;
    if (se.isVirtualXdbc) {
        std::string& q = se.conn->outQueue;
        size_t n = static_cast<size_t>(len) < q.size() ? static_cast<size_t>(len) : q.size();
        std::string out = q.substr(0, n);
        q.erase(0, n);
        return out;
    }
    if (!se.realSock || len <= 0) return "";
    std::string buf(static_cast<size_t>(len), '\0');
    int n = recv(static_cast<SOCKET>(se.realSock), &buf[0], len, 0);
    if (n <= 0) return "";
    buf.resize(static_cast<size_t>(n));
    return buf;
}

std::string xdbcRecvAsync(int fd, int len, bool& ok) {
    ok = false;
    std::lock_guard<std::mutex> lk(socketsMutex());
    auto it = socketTable().find(fd);
    if (it == socketTable().end() || !it->second.used) { ok = true; return ""; }
    SocketEntry& se = it->second;
    if (se.isVirtualXdbc) {
        ok = true;   // virtual XDBC: whatever is queued is what we get
        std::string& q = se.conn->outQueue;
        size_t n = static_cast<size_t>(len) < q.size() ? static_cast<size_t>(len) : q.size();
        std::string out = q.substr(0, n);
        q.erase(0, n);
        return out;
    }
    if (!se.realSock || len <= 0) { ok = true; return ""; }
    if (se.connecting) {
        // Connect not finished yet: treat as "no data" (parked until the
        // socket becomes writable, then connect result is checked).
        return "";
    }
    // Non-blocking probe: readable sockets return data immediately; an
    // empty result with ok=false means "no data available - park".
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(static_cast<SOCKET>(se.realSock), &rfds);
    timeval tv{};
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    int rc = ::select(static_cast<int>(se.realSock) + 1, &rfds, nullptr, nullptr, &tv);
    if (rc <= 0) return "";   // not readable yet -> park
    std::string buf(static_cast<size_t>(len), '\0');
    int n = recv(static_cast<SOCKET>(se.realSock), &buf[0], len, 0);
    if (n <= 0) {
        // Peer closed / error: report 0 bytes as a terminal state.
        ok = true;
        return "";
    }
    ok = true;
    buf.resize(static_cast<size_t>(n));
    return buf;
}

namespace {
// Non-blocking probe: is a connection pending on this listening socket?
// Uses select() (readable = pending accept) so the connection is NOT consumed.
bool acceptReady(SOCKET s) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(s, &rfds);
    timeval tv{};
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    int rc = ::select(static_cast<int>(s) + 1, &rfds, nullptr, nullptr, &tv);
    return rc > 0 && FD_ISSET(s, &rfds);
}
} // namespace

int xdbcSelect(std::vector<int>& fds, int timeoutMs) {    std::vector<int> ready;
    {
        std::lock_guard<std::mutex> lk(socketsMutex());
        for (int fd : fds) {
            auto it = socketTable().find(fd);
            if (it == socketTable().end() || !it->second.used) continue;
            SocketEntry& se = it->second;
            if (se.isVirtualXdbc) {
                if (!se.conn->outQueue.empty()) ready.push_back(fd);
            } else if (se.isListener) {
                // A listening socket is "readable" when a connection is
                // pending (accept_daemon polls this).
                if (se.realSock && acceptReady(static_cast<SOCKET>(se.realSock)))
                    ready.push_back(fd);
            }
        }
    }
    // real sockets: use select() on the raw fds
    std::vector<int> realFds;
    for (int fd : fds) {
        std::lock_guard<std::mutex> lk(socketsMutex());
        auto it = socketTable().find(fd);
        if (it != socketTable().end() && it->second.used && it->second.realSock)
            realFds.push_back(fd);
    }
    if (!realFds.empty()) {
        fd_set rfds;
        FD_ZERO(&rfds);
        SOCKET maxS = 0;
        std::map<SOCKET, int> bySock;
        for (int fd : realFds) {
            std::lock_guard<std::mutex> lk(socketsMutex());
            auto it = socketTable().find(fd);
            if (it == socketTable().end() || !it->second.realSock) continue;
            SOCKET s = static_cast<SOCKET>(it->second.realSock);
            FD_SET(s, &rfds);
            bySock[s] = fd;
            if (s > maxS) maxS = s;
        }
        timeval tv{};
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;
        int rc = ::select(static_cast<int>(maxS) + 1, &rfds, nullptr, nullptr, &tv);
        if (rc > 0) {
            for (auto& kv : bySock)
                if (FD_ISSET(kv.first, &rfds)) ready.push_back(kv.second);
        }
    }
    fds = ready;
    return static_cast<int>(ready.size());
}

int xdbcPoll(const std::vector<int>& fds, std::vector<int>& readable,
             std::vector<int>& writable, int timeoutMs) {
    readable.clear();
    writable.clear();
    if (fds.empty()) return 0;
    // WSAPoll has no FD_SETSIZE limit (select() with FD_SET overflows for
    // socket handles >= 64, silently missing connect/recv completion).
    std::vector<WSAPOLLFD> pfd;
    pfd.reserve(fds.size());
    for (int fd : fds) {
        std::lock_guard<std::mutex> lk(socketsMutex());
        auto it = socketTable().find(fd);
        if (it == socketTable().end() || !it->second.used) continue;
        if (it->second.isVirtualXdbc) {
            // Virtual XDBC socket: readable when a reply is queued.
            if (it->second.conn && !it->second.conn->outQueue.empty()) {
                readable.push_back(fd);
                writable.push_back(fd);
            }
            continue;
        }
        if (!it->second.realSock) continue;
        WSAPOLLFD pf{};
        pf.fd = static_cast<SOCKET>(it->second.realSock);
        pf.events = POLLRDNORM | POLLWRNORM;
        pfd.push_back(pf);
    }
    if (pfd.empty()) return static_cast<int>(readable.size());
    int rc = WSAPoll(pfd.data(), static_cast<ULONG>(pfd.size()), timeoutMs);
    if (rc > 0) {
        for (const auto& pf : pfd) {
            // Map the SOCKET back to the virtual fd.
            int vfd = -1;
            {
                std::lock_guard<std::mutex> lk(socketsMutex());
                for (auto& kv : socketTable()) {
                    if (kv.second.used && kv.second.realSock == static_cast<uintptr_t>(pf.fd)) {
                        vfd = kv.first;
                        break;
                    }
                }
            }
            if (vfd < 0) continue;
            if (pf.revents & (POLLRDNORM | POLLIN | POLLHUP | POLLERR))
                readable.push_back(vfd);
            if (pf.revents & (POLLWRNORM | POLLOUT | POLLHUP | POLLERR))
                writable.push_back(vfd);
        }
    }
    return static_cast<int>(readable.size() + writable.size());
}

void xdbcCloseSocket(int fd) {
    std::lock_guard<std::mutex> lk(socketsMutex());
    auto it = socketTable().find(fd);
    if (it == socketTable().end()) return;
    if (it->second.realSock) closesocket(static_cast<SOCKET>(it->second.realSock));
    socketTable().erase(it);
}

bool xdbcSocketExists(int fd) {
    std::lock_guard<std::mutex> lk(socketsMutex());
    auto it = socketTable().find(fd);
    return it != socketTable().end() && it->second.used;
}

uintptr_t xdbcDetachSocket(int fd) {
    std::lock_guard<std::mutex> lk(socketsMutex());
    auto it = socketTable().find(fd);
    if (it == socketTable().end() || !it->second.used) return 0;
    uintptr_t s = it->second.realSock;
    socketTable().erase(it);
    return s;
}

int xdbcBind(int fd, int port) {
    ensureWsa();
    std::lock_guard<std::mutex> lk(socketsMutex());
    auto it = socketTable().find(fd);
    if (it == socketTable().end() || !it->second.used) return -1;
    SocketEntry& se = it->second;
    if (se.isVirtualXdbc || se.realSock) return -1;
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return -1;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<u_short>(port));
    if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        closesocket(s);
        return -1;
    }
    se.realSock = static_cast<uintptr_t>(s);
    se.isListener = true;
    se.listenPort = port;
    return 0;
}

int xdbcListen(int fd, int backlog) {
    std::lock_guard<std::mutex> lk(socketsMutex());
    auto it = socketTable().find(fd);
    if (it == socketTable().end() || !it->second.used || !it->second.realSock)
        return -1;
    if (listen(static_cast<SOCKET>(it->second.realSock), backlog) == SOCKET_ERROR)
        return -1;
    return 0;
}

int xdbcAccept(int fd) {
    ensureWsa();
    SOCKET listener = INVALID_SOCKET;
    {
        std::lock_guard<std::mutex> lk(socketsMutex());
        auto it = socketTable().find(fd);
        if (it == socketTable().end() || !it->second.used || !it->second.isListener)
            return -1;
        listener = static_cast<SOCKET>(it->second.realSock);
    }
    // Blocking accept, mirroring the original driver's vm_f_accept: the
    // listening thread (e.g. ccs_serverd's accept_daemon) runs
    // `while(1) { if ((fd = accept(...)) < 0) continue; ... }` and relies on
    // accept() PARKING until a connection arrives.  A non-blocking accept
    // returned -1 instantly, turning that loop into a 100%-CPU busy spin
    // (ccs_serverd.o accept_daemon burned ~54% of a core with zero
    // connections).  Blocking accept keeps the LPC loop idle exactly like the
    // original cooperative scheduler did.
    sockaddr_in cli{};
    int cliLen = sizeof(cli);
    SOCKET c = accept(listener, reinterpret_cast<sockaddr*>(&cli), &cliLen);
    if (c == INVALID_SOCKET) return -1;
    int newFd = nextSocketFd().fetch_add(1);
    {
        std::lock_guard<std::mutex> lk(socketsMutex());
        SocketEntry& ne = socketTable()[newFd];
        ne.used = true;
        ne.realSock = static_cast<uintptr_t>(c);
    }
    return newFd;
}

bool xdbcIsListener(int fd) {
    std::lock_guard<std::mutex> lk(socketsMutex());
    auto it = socketTable().find(fd);
    return it != socketTable().end() && it->second.isListener;
}

Value xdbcGetSockInfo(int fd) {
    std::lock_guard<std::mutex> lk(socketsMutex());
    auto it = socketTable().find(fd);
    if (it == socketTable().end() || !it->second.used) return make_mapping();
    const SocketEntry& se = it->second;
    auto mp = std::make_shared<MappingValue>();
    auto addInt = [&](const char* k, int64_t v) { mp->insert(make_string(k), make_int(v)); };
    auto addStr = [&](const char* k, const std::string& v) { mp->insert(make_string(k), make_string(v)); };
    // vm_generateSocketInfo: id (fd+1), vm_fd, local_address/port,
    // remote_address/port, remain_send_data, oper_error.  remote_address is a
    // STRING (inet_ntoa) - ccs_serverd's accept_daemon rejects connections
    // whose remote_address is falsy, so a missing/misnamed key closed every
    // accepted gs connection.
    addInt("id", fd + 1);
    addInt("vm_fd", fd);
    if (se.isListener) {
        addInt("local_port", se.listenPort);
    } else if (se.realSock) {
        sockaddr_in local{};
        int llen = sizeof(local);
        getsockname(static_cast<SOCKET>(se.realSock), reinterpret_cast<sockaddr*>(&local), &llen);
        addInt("local_port", ntohs(local.sin_port));
        char lbuf[INET_ADDRSTRLEN] = {};
        inet_ntop(AF_INET, &local.sin_addr, lbuf, sizeof(lbuf));
        addStr("local_address", lbuf);
        sockaddr_in rem{};
        int rlen = sizeof(rem);
        getpeername(static_cast<SOCKET>(se.realSock), reinterpret_cast<sockaddr*>(&rem), &rlen);
        addInt("remote_port", ntohs(rem.sin_port));
        char rbuf[INET_ADDRSTRLEN] = {};
        inet_ntop(AF_INET, &rem.sin_addr, rbuf, sizeof(rbuf));
        addStr("remote_address", rbuf);
    }
    addInt("remain_send_data", 0);
    return mp;
}

} // namespace lpc
