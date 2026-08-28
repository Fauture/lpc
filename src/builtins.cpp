#include "builtins.h"
#include "efun_table.h"

#include "o_loader.h"
#include "original_vm.h"
#include "mysql_client.h"
#include "pak_loader.h"
#include "lzss.h"
#include "xdbc.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

#include "interpreter.h"
#include "lexer.h"
#include "parser.h"

namespace lpc {

namespace {

int64_t as_int(const Value& v) {
    switch (v.index()) {
    case 0: return 0;
    case 1: return 0;
    case 2: return std::get<int64_t>(v);
    case 3: return static_cast<int64_t>(std::get<double>(v));
    case 4: return std::get<std::string>(v).empty() ? 0 : 1;
    default: return 0;
    }
}

std::string as_string(const Value& v) { return to_string(v, false); }

std::string formatLpc(const std::string& fmt, const std::vector<Value>& args) {
    std::string out;
    size_t argi = 0;
    for (size_t i = 0; i < fmt.size(); ++i) {
        if (fmt[i] != '%' || i + 1 >= fmt.size()) {
            out += fmt[i];
            continue;
        }
        char spec = fmt[++i];
        if (spec == '%') {
            out += '%';
        } else if (spec == 's') {
            out += argi < args.size() ? as_string(args[argi++]) : "";
        } else if (spec == 'd' || spec == 'i') {
            out += argi < args.size() ? std::to_string(as_int(args[argi++])) : "0";
        } else if (spec == 'f') {
            if (argi < args.size()) {
                const Value& v = args[argi++];
                if (v.index() == 3) out += std::to_string(std::get<double>(v));
                else out += std::to_string(static_cast<double>(as_int(v)));
            } else {
                out += "0";
            }
        } else if (spec == 'x' || spec == 'X') {
            char buf[32];
            if (argi < args.size()) {
                if (spec == 'x') snprintf(buf, sizeof(buf), "%llx", static_cast<unsigned long long>(as_int(args[argi++])));
                else snprintf(buf, sizeof(buf), "%llX", static_cast<unsigned long long>(as_int(args[argi++])));
            } else {
                buf[0] = '0'; buf[1] = 0;
            }
            out += buf;
        } else {
            out += '%';
            out += spec;
        }
    }
    return out;
}

std::unordered_map<std::string, std::string>& simpleDb() {
    static std::unordered_map<std::string, std::string> db;
    return db;
}

void simpleDbLoad(const std::string& path) {
    std::ifstream in(path);
    if (!in) return;
    std::string line;
    while (std::getline(in, line)) {
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        simpleDb()[line.substr(0, eq)] = line.substr(eq + 1);
    }
}

void simpleDbSave(const std::string& path) {
    std::ofstream out(path);
    for (auto& kv : simpleDb()) out << kv.first << '=' << kv.second << '\n';
}

// Result-set state for the mysql_* efun family. The original driver keeps one
// cursor + current record per result handle (the array returned by
// mysql_query); we mirror that with maps keyed by the array instance so
// concurrent LPC threads (configd update daemon, aaaserverd do_init, ...)
// cannot clobber each other's current record.
std::unordered_map<const void*, int>& mysqlCursor() {
    static std::unordered_map<const void*, int> cursors;
    return cursors;
}
std::unordered_map<const void*, Value>& mysqlRecords() {
    static std::unordered_map<const void*, Value> recs;
    return recs;
}
std::mutex& mysqlStateMutex() {
    static std::mutex m;
    return m;
}

// Real MySQL connections, keyed by the small integer handle (cno) that
// mysql_connect returns to the LPC side (dbd's assure_connection stores it
// and passes it back to mysql_query/mysql_execute).  Each connection is
// serialized internally, so concurrent LPC threads are safe.
struct MysqlHandle {
    std::shared_ptr<MysqlClient> client;  // null when offline (fallback)
    std::string db;
    std::shared_ptr<std::mutex> mu = std::make_shared<std::mutex>();  // per-connection lock
};
std::map<int, MysqlHandle>& mysqlHandles() {
    static std::map<int, MysqlHandle> m;
    return m;
}
std::mutex& mysqlHandlesMutex() {
    static std::mutex m;
    return m;
}
int nextMysqlCno() {
    static std::atomic<int> n{1};
    return n++;
}

// Execute a query against the live MySQL server.  Returns true and fills
// out on success (result set or affected rows); false when the server
// rejected the statement (error filled) or the connection is unusable.
bool mysqlExecReal(int cno, const std::string& sql, MysqlClient::Result& out, std::string& error) {
    std::shared_ptr<MysqlClient> client;
    std::shared_ptr<std::mutex> mu;
    {
        std::lock_guard<std::mutex> lk(mysqlHandlesMutex());
        auto it = mysqlHandles().find(cno);
        if (it != mysqlHandles().end()) {
            client = it->second.client;
            mu = it->second.mu;
        }
    }
    if (!client || !client->isConnected()) {
        error = "no live connection for cno " + std::to_string(cno);
        return false;
    }
    std::lock_guard<std::mutex> lk(*mu);
    return client->execute(sql, out, error);
}

std::string vfsRoot() {
    std::error_code ec;
    if (fs::exists("linux/aaa - bak", ec)) return "linux/aaa - bak";
    if (fs::exists("out/game", ec)) return "out/game";
    return "out/lib_aaa32";
}

// The driver's run directory.  The original driver was started from the game
// directory (see linux/aaa/runaaa: "./magic_Linux32 /r ./ ..."), so virtual
// paths like "/aaa/aaa.ini" and "/aaa/pack_data/etc.pak" resolve relative to
// that directory, NOT to the extracted .o tree (linux/aaa - bak) which only
// holds object files.  "/aaa/x" -> <runDir>/aaa/x.
std::string dataRoot() {
    std::error_code ec;
    if (fs::exists("linux/aaa/magic_Linux32", ec) || fs::exists("linux/aaa/runaaa", ec))
        return "linux/aaa";
    if (fs::exists("linux/aaa/aaa", ec)) return "linux/aaa";
    return vfsRoot();
}

bool hasSuffix(const std::string& s, const std::string& suffix);   // defined below

// The unpacked etc directory (sibling of the game dir).  The original driver
// read etc files from inside etc.pak; this is the same content already
// unpacked, so both are supported.
std::string etcRoot() {
    std::error_code ec;
    if (fs::exists("linux/etc", ec)) return "linux/etc";
    return "";
}

std::string normalizeSlashes(std::string s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '/' && !out.empty() && out.back() == '/') continue;
        out += s[i];
    }
    return out;
}

// vm_matchString (0x80eaeb0): '*' matches any run, '?' one char, '\\' escapes
// the next pattern char.  str must be fully consumed for a match.
bool matchString(const std::string& str, const std::string& pat) {
    size_t si = 0, pi = 0;
    while (si < str.size()) {
        char pc = pi < pat.size() ? pat[pi] : '\0';
        if (pc == '?') {
            ++si; ++pi;
            continue;
        }
        if (pc == '*') {
            if (pi + 1 >= pat.size()) return true;
            if (si >= str.size()) return false;
            for (size_t k = si; k < str.size(); ++k) {
                if (matchString(str.substr(k), pat.substr(pi + 1))) return true;
            }
            return false;
        }
        if (pc == '\\') {
            ++pi;
            if (pi >= pat.size() || pat[pi] != str[si]) return false;
            ++si; ++pi;
            continue;
        }
        if (pc != str[si]) return false;
        ++si; ++pi;
    }
    // string consumed: match only if the rest of the pattern can match empty
    while (pi < pat.size() && pat[pi] == '*') ++pi;
    return pi >= pat.size();
}

void ensureWsa() {
    static bool done = [] {
        WSADATA wsa{};
        WSAStartup(MAKEWORD(2, 2), &wsa);
        return true;
    }();
    (void)done;
}

// ---------------------------------------------------------------------------
// Telnet client registry. The original driver keeps one Vm_Client_T per slot
// (max 2104); client numbers are small ints handed to the LPC side through
// telnet_new_client(fd).  We mirror that with a map keyed by client number.
// ---------------------------------------------------------------------------
struct TelnetClient {
    SOCKET sock = INVALID_SOCKET;
    bool echo = true;          // telnet_config echo
    bool auth = false;         // telnet_config auth
    bool nativeProcess = false;
    bool enabled = false;
    bool isInternal = false;
    bool eof = false;          // remote end closed the socket
    int peek = 0;
    std::string id;            // telnet_config id (account)
    std::string recvBuf;       // partial-line input buffer
};

std::unordered_map<int, TelnetClient>& telnetClients() {
    static std::unordered_map<int, TelnetClient> clients;
    return clients;
}

std::mutex& telnetMutex() {
    static std::mutex m;
    return m;
}

std::atomic<int>& nextTelnetClientNo() {
    static std::atomic<int> n{1};   // 0 is reserved (undefined)
    return n;
}

// The client number served by the current thread (set when a telnet daemon
// thread starts).  input_string()/printf() use it to route to the socket.
thread_local int tlsCurrentClient = -1;

TelnetClient* findTelnetClient(int cno) {
    if (cno < 0) return nullptr;
    std::lock_guard<std::mutex> lk(telnetMutex());
    auto it = telnetClients().find(cno);
    return it == telnetClients().end() ? nullptr : &it->second;
}

// Send raw bytes to a telnet client's socket (thread-safe copy of the entry).
void telnetSend(int cno, const std::string& data) {
    SOCKET s = INVALID_SOCKET;
    {
        std::lock_guard<std::mutex> lk(telnetMutex());
        auto it = telnetClients().find(cno);
        if (it == telnetClients().end()) return;
        s = it->second.sock;
    }
    if (std::getenv("LPCVM_TRACE")) {
        std::string hex;
        for (unsigned char c : data) {
            if (hex.size() > 120) { hex += "..."; break; }
            char b[8];
            snprintf(b, sizeof(b), "%02X ", c);
            hex += b;
        }
        std::cerr << "[telnet] send " << cno << " len=" << data.size() << " [" << hex << "]\n";
    }
    if (s != INVALID_SOCKET) send(s, data.data(), static_cast<int>(data.size()), 0);
}

// Translate "\n" to "\r\n" for telnet clients (the driver converts on flush).
std::string telnetLineEnd(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        if (c == '\n') out += "\r\n";
        else out += c;
    }
    return out;
}

// Block until a full line arrives from the client socket; return it without
// the trailing CR/LF.  Empty/EOF returns "" (caller closes the client).
// Telnet IAC negotiation sequences are stripped (and minimally answered).
std::string telnetReadLine(int cno) {
    TelnetClient* cl = findTelnetClient(cno);
    if (!cl) return "";
    char buf[1024];
    while (true) {
        size_t nl = cl->recvBuf.find('\n');
        if (nl != std::string::npos) {
            std::string line = cl->recvBuf.substr(0, nl);
            cl->recvBuf.erase(0, nl + 1);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            return line;
        }
        int n = recv(cl->sock, buf, sizeof(buf), 0);
        if (n <= 0) {
            cl->eof = true;
            // EOF / error: deliver what we have, then let the caller close.
            if (!cl->recvBuf.empty()) {
                std::string line = cl->recvBuf;
                cl->recvBuf.clear();
                if (!line.empty() && line.back() == '\r') line.pop_back();
                return line;
            }
            return "";
        }
        // Strip telnet IAC (0xFF) negotiation: 0xFF <cmd> <opt>.
        std::string reply;
        for (int i = 0; i < n; ++i) {
            unsigned char c = static_cast<unsigned char>(buf[i]);
            if (c == 0xFF && i + 2 < n) {
                unsigned char cmd = static_cast<unsigned char>(buf[i + 1]);
                unsigned char opt = static_cast<unsigned char>(buf[i + 2]);
                if (cmd == 0xFB || cmd == 0xFC) {         // WILL / WONT
                    // Accept ECHO/SGA/NAWS/TTYPE/SUPPRESS; decline the rest.
                    bool ok = opt == 1 || opt == 3 || opt == 31 || opt == 24 || opt == 0x20;
                    reply += "\xFF";
                    reply += static_cast<char>(ok ? 0xFD : 0xFE);  // DO / DONT
                    reply += static_cast<char>(opt);
                } else if (cmd == 0xFD || cmd == 0xFE) {  // DO / DONT
                    reply += "\xFF";
                    reply += static_cast<char>(0xFC);     // WONT
                    reply += static_cast<char>(opt);
                }
                i += 2;
                continue;
            }
            cl->recvBuf += static_cast<char>(c);
        }
        if (!reply.empty()) telnetSend(cno, reply);
    }
}

void closeTelnetClient(int cno) {
    {
        std::lock_guard<std::mutex> lk(telnetMutex());
        auto it = telnetClients().find(cno);
        if (it == telnetClients().end()) return;
        if (it->second.sock != INVALID_SOCKET) closesocket(it->second.sock);
        telnetClients().erase(it);
    }
    if (tlsCurrentClient == cno) tlsCurrentClient = -1;
    if (std::getenv("LPCVM_TRACE"))
        std::cerr << "[telnet] client " << cno << " closed\n";
}

// The driver-side entry point for a new telnet connection.  The original
// driver allocates a client slot and applies the master object's
// "telnet_new_client" function with the client number.
void acceptTelnetClient(SOCKET c) {
    int cno = nextTelnetClientNo().fetch_add(1);
    {
        std::lock_guard<std::mutex> lk(telnetMutex());
        TelnetClient& tc = telnetClients()[cno];
        tc.sock = c;
    }
    if (std::getenv("LPCVM_TRACE"))
        std::cerr << "[telnet] new client no=" << cno << "\n";
    // Notify the LPC master program: telnet_new_client(fd) decides between
    // the LPC telnet_daemon (create_thread) and the builtin telnet shell.
    try {
        Value r = callMasterProgramFunction("telnet_new_client", {make_int(cno)}, vfsRoot());
        (void)r;
        // telnet_new_client usually calls create_thread(0, (: telnet_daemon, fd :));
        // spawn that runtime thread now.
        drainPendingOriginalThreads(vfsRoot());
    } catch (const std::exception& e) {
        std::cerr << "[telnet] master telnet_new_client error: " << e.what() << "\n";
        closeTelnetClient(cno);
    }
}

void telnetServerLoop(SOCKET listener) {
    while (true) {
        SOCKET c = accept(listener, nullptr, nullptr);
        if (c == INVALID_SOCKET) {
            if (WSAGetLastError() == WSAEINTR) continue;
            break;
        }
        acceptTelnetClient(c);
    }
}

// Driver-level telnet shell (enabled only by the -w/-p command line options,
// like the original driver).  It asks for the configured password and then
// runs an interactive shell where execute_data() evaluates LPC expressions.
// Implemented at lpc scope (see below) so main.cpp can call it.

std::string resolveVfsPath(const std::string& p) {
    std::string path = p;
    if (!path.empty() && path.front() == '/') path.erase(path.begin());
    // Data files (ini/config/pak/etc) live in the real game directory, not in
    // the extracted .o tree.  The .o tree has no such files, so trying the
    // data root first is unambiguous for them.
    if (hasSuffix(path, ".ini") || path.rfind("pack_data/", 0) == 0 ||
        path.rfind("aaa/pack_data/", 0) == 0) {
        std::string d = dataRoot() + "/" + path;
        std::error_code ec;
        if (fs::exists(d, ec)) {
            return d;
        }
    }
    return vfsRoot() + "/" + path;
}

// Read the bytes of a virtual path, with the same fallbacks the original
// driver uses:
//   - /aaa/etc/<file>            -> the unpacked etc dir (etcRoot)
//   - /aaa/pack_data/<pak>/<f>   -> entry <f> inside the real <pak> file
//                                   (searched under the data root and the
//                                   object root)
//   - anything else              -> plain vfs path
std::string readVfsFile(const std::string& vpath) {
    std::error_code ec;
    fs::path plain = resolveVfsPath(vpath);
    if (fs::is_regular_file(plain, ec)) {
        std::ifstream in(plain, std::ios::binary);
        if (in) {
            std::ostringstream ss;
            ss << in.rdbuf();
            return ss.str();
        }
    }
    // /aaa/etc/<file> fallback: the unpacked etc dir (same content as the
    // original etc.pak).
    if (vpath.rfind("/aaa/etc/", 0) == 0) {
        std::string name = vpath.substr(10);
        std::string etc = etcRoot();
        if (!etc.empty()) {
            fs::path alt = etc + "/" + name;
            if (fs::is_regular_file(alt, ec)) {
                std::ifstream in(alt, std::ios::binary);
                if (in) {
                    std::ostringstream ss;
                    ss << in.rdbuf();
                    return ss.str();
                }
            }
        }
    }
    // /aaa/pack_data/<pak>/<entry> fallback: read from the real pak file in
    // the game data root.
    static const std::string pakPrefix = "/aaa/pack_data/";
    if (vpath.rfind(pakPrefix, 0) == 0) {
        std::string rest = vpath.substr(pakPrefix.size());
        size_t slash = rest.find('/');
        if (slash != std::string::npos) {
            std::string pakName = rest.substr(0, slash);
            std::string entry = rest.substr(slash + 1);
            std::vector<std::string> candidates = {
                dataRoot() + "/aaa/pack_data/" + pakName,
                dataRoot() + "/pack_data/" + pakName,
                vfsRoot() + "/pack_data/" + pakName,
                vfsRoot() + "/aaa/pack_data/" + pakName,
            };
            for (const auto& pakPath : candidates) {
                if (!fs::is_regular_file(pakPath, ec)) continue;
                try {
                    lpc::PakFile pak = lpc::loadPak(pakPath);
                    const lpc::PakEntry* e = lpc::findPakEntry(pak, entry);
                    if (!e || e->isDir) continue;
                    std::ifstream in(pakPath, std::ios::binary);
                    if (!in) continue;
                    in.seekg(0, std::ios::end);
                    std::streamoff len = in.tellg();
                    in.seekg(0, std::ios::beg);
                    std::vector<uint8_t> data(static_cast<size_t>(len));
                    if (len > 0) in.read(reinterpret_cast<char*>(data.data()), len);
                    if (static_cast<size_t>(e->offset) + e->compSize > data.size()) continue;
                    std::vector<uint8_t> src(data.begin() + e->offset,
                                             data.begin() + e->offset + e->compSize);
                    std::vector<uint8_t> out;
                    if (lzssDecompress(src, e->origSize, out)) {
                        return std::string(out.begin(), out.end());
                    }
                    if (e->compSize == e->origSize) {
                        return std::string(src.begin(), src.end());
                    }
                } catch (...) {
                    continue;
                }
            }
        }
    }
    return "";
}

bool hasSuffix(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// ---------------------------------------------------------------------------
// Minimal regex for the patterns the game uses (char classes, '*', '+', '?',
// '.', '^', '$', groups, '|').  Returns matched group ranges as
// ({g0start, g0end, g1start, g1end, ...}) like the original vm_f_regexp.
// ---------------------------------------------------------------------------
namespace mini_regex {

struct Piece {
    enum Kind { kChar, kClass, kAny, kStart, kEnd, kGroup } kind = kChar;
    char ch = 0;
    bool negate = false;
    std::vector<std::pair<char, char>> ranges;
    int min = 1, max = 1;
    std::vector<Piece> inner;   // for kGroup (parsed inner sequence)
};

Piece parseSingle(const std::string& s, size_t& i);
std::vector<Piece> parseGroupBody(const std::string& s, size_t& i);

// Match a sequence of pieces starting at pos; sets endPos to the position
// after the match (or -1).  Greedy with backtracking on quantifiers.
bool matchSeq(const std::vector<Piece>& ps, const std::string& str, size_t pos, int& endPos);
bool matchSeqFrom(const std::vector<Piece>& ps, size_t pi, const std::string& str, size_t pos, int& endPos);

std::vector<Piece> parseGroupBody(const std::string& s, size_t& i) {
    std::vector<Piece> out;
    int depth = 0;
    while (i < s.size()) {
        char c = s[i];
        if (c == '(') ++depth;
        if (c == ')') {
            if (depth == 0) { ++i; break; }
            --depth;
        }
        out.push_back(parseSingle(s, i));
    }
    return out;
}

Piece parseSingle(const std::string& s, size_t& i) {
    char c = s[i];
    Piece p;
    p.kind = Piece::kChar;
    p.ch = c;
    if (c == '[') {
        p.kind = Piece::kClass;
        ++i;
        if (i < s.size() && s[i] == '^') { p.negate = true; ++i; }
        while (i < s.size() && s[i] != ']') {
            char lo = s[i++];
            if (i + 1 < s.size() && s[i] == '-' && s[i + 1] != ']') {
                char hi = s[i + 1];
                i += 2;
                p.ranges.emplace_back(lo, hi);
            } else {
                p.ranges.emplace_back(lo, lo);
            }
        }
        if (i < s.size() && s[i] == ']') ++i;
    } else if (c == '.') {
        p.kind = Piece::kAny;
        ++i;
    } else if (c == '^') {
        p.kind = Piece::kStart;
        ++i;
    } else if (c == '$') {
        p.kind = Piece::kEnd;
        ++i;
    } else if (c == '(') {
        p.kind = Piece::kGroup;
        ++i;
        p.inner = parseGroupBody(s, i);
    } else if (c == '\\' && i + 1 < s.size()) {
        p.ch = s[i + 1];
        i += 2;
    } else {
        ++i;
    }
    // quantifier
    if (i < s.size() && (s[i] == '*' || s[i] == '+' || s[i] == '?')) {
        char q = s[i++];
        p.min = q == '+' ? 1 : 0;
        p.max = q == '?' ? 1 : -1;
    }
    return p;
}

std::vector<Piece> parse(const std::string& s) {
    std::vector<Piece> out;
    size_t i = 0;
    while (i < s.size()) out.push_back(parseSingle(s, i));
    return out;
}

// Match one repetition of a single piece against str at pos.  Returns the
// number of chars consumed or -1.  (Anchors return 0 and check position.)
int matchOne(const Piece& p, const std::string& str, size_t pos, int& consume) {
    switch (p.kind) {
        case Piece::kChar:
            if (pos < str.size() && str[pos] == p.ch) { consume = 1; return 0; }
            return -1;
        case Piece::kAny:
            if (pos < str.size()) { consume = 1; return 0; }
            return -1;
        case Piece::kClass: {
            if (pos >= str.size()) return -1;
            char c = str[pos];
            bool hit = false;
            for (auto& r : p.ranges)
                if (c >= r.first && c <= r.second) { hit = true; break; }
            if (p.negate) hit = !hit;
            if (hit) { consume = 1; return 0; }
            return -1;
        }
        case Piece::kStart:
            consume = 0;
            return pos == 0 ? 0 : -1;
        case Piece::kEnd:
            consume = 0;
            return pos == str.size() ? 0 : -1;
        case Piece::kGroup: {
            // Match the inner sequence greedily; consume = matched length.
            int endPos = -1;
            if (matchSeq(p.inner, str, pos, endPos) && endPos >= 0) {
                consume = endPos - static_cast<int>(pos);
                return 0;
            }
            return -1;
        }
    }
    return -1;
}

// Match a sequence of pieces starting at pos; sets endPos to the position
// after the match (or -1).  Greedy with backtracking on quantifiers.
bool matchSeq(const std::vector<Piece>& ps, const std::string& str, size_t pos, int& endPos) {
    if (ps.empty()) { endPos = static_cast<int>(pos); return true; }
    // Split at top-level '|' alternatives.
    // (Parsing keeps '|' out of Piece; treat the whole vector as one branch.)
    return matchSeqFrom(ps, 0, str, pos, endPos);
}

bool matchSeqFrom(const std::vector<Piece>& ps, size_t pi, const std::string& str, size_t pos, int& endPos) {
    if (pi >= ps.size()) { endPos = static_cast<int>(pos); return true; }
    const Piece& p = ps[pi];
    int maxReps = p.max < 0 ? static_cast<int>(str.size() - pos) + 1 : p.max;
    // Try the maximum number of repetitions first (greedy).
    for (int reps = maxReps; reps >= p.min; --reps) {
        size_t q = pos;
        bool ok = true;
        for (int r = 0; r < reps; ++r) {
            int consume = 0;
            if (matchOne(p, str, q, consume) != 0) { ok = false; break; }
            q += static_cast<size_t>(consume);
        }
        if (!ok) continue;
        int restEnd = -1;
        if (matchSeqFrom(ps, pi + 1, str, q, restEnd)) {
            endPos = restEnd;
            return true;
        }
    }
    return false;
}

Value regexpMatch(const std::string& str, const std::string& pattern) {
    std::vector<Piece> ps = parse(pattern);
    int endPos = -1;
    if (!matchSeq(ps, str, 0, endPos)) return make_array();   // no match
    if (endPos < 0) return make_array();
    // The game's callers expect group 0 to cover [0, end) and a size of 2
    // when there is a single (whole-match) group.
    auto arr = std::make_shared<ArrayValue>();
    arr->items.push_back(make_int(0));
    arr->items.push_back(make_int(endPos));
    return arr;
}

} // namespace mini_regex

// ---------------------------------------------------------------------------
// In-memory SQL dump database (linux/new_sql/*.sql, Navicat dumps). The real
// server keeps MySQL on a separate host; we never open a network connection
// to it. Dumps are parsed once into memory tables:
//   CREATE TABLE `t` ( `c1` type ..., `c2` ... ) ENGINE=...;
//   INSERT INTO `t` VALUES ('a', 1, null), ('b', 2, 'x');
// Only adb.sql carries data rows in this tree; the other dumps are schema.
// ---------------------------------------------------------------------------
struct SqlCol {
    std::string name;
    bool numeric = false;
};
struct SqlTable {
    std::vector<SqlCol> cols;
    std::vector<std::vector<Value>> rows;
};

static std::unordered_map<std::string, SqlTable>& sqlDb() {
    static std::unordered_map<std::string, SqlTable> db;
    return db;
}
static std::mutex& sqlDbMutex() {
    static std::mutex m;
    return m;
}

static std::string sqlLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}
static std::string sqlTrim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return std::string();
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}
static bool sqlIsWord(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '$';
}

// Find the matching ')' for the '(' at `start`, ignoring backtick-quoted
// identifiers, single-quoted strings (with '' escapes) and nesting.
static size_t sqlFindParenEnd(const std::string& s, size_t start) {
    int depth = 0;
    bool inBq = false, inSq = false;
    for (size_t k = start; k < s.size(); ++k) {
        char c = s[k];
        if (inSq) {
            if (c == '\'') {
                if (k + 1 < s.size() && s[k + 1] == '\'') ++k;
                else inSq = false;
            }
            continue;
        }
        if (inBq) {
            if (c == '`') inBq = false;
            continue;
        }
        if (c == '\'') inSq = true;
        else if (c == '`') inBq = true;
        else if (c == '(') ++depth;
        else if (c == ')') {
            if (--depth == 0) return k;
        }
    }
    return std::string::npos;
}

// Split [begin,end) on top-level commas (depth 0, outside quotes/backticks).
static std::vector<std::string> sqlSplitTop(const std::string& s, size_t begin, size_t end) {
    std::vector<std::string> out;
    size_t seg = begin;
    int depth = 0;
    bool inBq = false, inSq = false;
    for (size_t k = begin; k < end; ++k) {
        char c = s[k];
        if (inSq) {
            if (c == '\'') {
                if (k + 1 < end && s[k + 1] == '\'') ++k;
                else inSq = false;
            }
            continue;
        }
        if (inBq) {
            if (c == '`') inBq = false;
            continue;
        }
        if (c == '\'') inSq = true;
        else if (c == '`') inBq = true;
        else if (c == '(') ++depth;
        else if (c == ')') --depth;
        else if (c == ',' && depth == 0) {
            out.push_back(s.substr(seg, k - seg));
            seg = k + 1;
        }
    }
    out.push_back(s.substr(seg, end - seg));
    return out;
}

static bool sqlTypeNumeric(const std::string& ty) {
    std::string t = sqlLower(sqlTrim(ty));
    static const char* nums[] = {"tinyint", "smallint", "mediumint", "bigint", "int",
                                 "integer", "float",    "double",    "decimal", "numeric",
                                 "real",    "bit",      "year"};
    for (const char* n : nums)
        if (t.compare(0, std::strlen(n), n) == 0) return true;
    return false;
}

// Convert one raw value token (as written in the dump) into a Value.
// Numeric columns become ints even when Navicat quotes them ('8001').
static Value sqlValue(const std::string& raw, bool numeric) {
    std::string t = sqlTrim(raw);
    if (t.empty()) return numeric ? make_int(0) : make_string("");
    std::string low = sqlLower(t);
    if (low == "null" || low == "default") return numeric ? make_int(0) : make_string("");
    if (t.front() == '\'') {
        std::string out;
        for (size_t k = 1; k + 1 < t.size(); ++k) {
            if (t[k] == '\'' && k + 1 < t.size() && t[k + 1] == '\'') {
                out += '\'';
                ++k;
            } else {
                out += t[k];
            }
        }
        if (numeric) {
            char* end = nullptr;
            long long v = std::strtoll(out.c_str(), &end, 10);
            if (end && *end == '\0' && !out.empty()) return make_int(v);
        }
        return make_string(out);
    }
    if (numeric) {
        char* end = nullptr;
        long long v = std::strtoll(t.c_str(), &end, 10);
        if (end && *end == '\0' && !t.empty()) return make_int(v);
    }
    return make_string(t);
}

static void sqlLoadFile(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) return;
    std::stringstream ss;
    ss << in.rdbuf();
    const std::string s = ss.str();
    const size_t n = s.size();
    size_t i = 0;
    while (i < n) {
        // Skip comments and string literals so keywords inside them don't match.
        if (s[i] == '-' && i + 1 < n && s[i + 1] == '-') {  // -- line comment
            while (i < n && s[i] != '\n') ++i;
            continue;
        }
        if (s[i] == '#' && (i == 0 || s[i - 1] == '\n')) {  // # line comment
            while (i < n && s[i] != '\n') ++i;
            continue;
        }
        if (s[i] == '/' && i + 1 < n && s[i + 1] == '*') {  // /* block comment */
            i += 2;
            while (i + 1 < n && !(s[i] == '*' && s[i + 1] == '/')) ++i;
            i += 2;
            continue;
        }
        if (s[i] == '\'') {  // string literal
            ++i;
            while (i < n) {
                if (s[i] == '\'') {
                    if (i + 1 < n && s[i + 1] == '\'') {
                        i += 2;
                        continue;
                    }
                    ++i;
                    break;
                }
                ++i;
            }
            continue;
        }
        // CREATE TABLE `name` ( ... )
        if (i + 11 < n && sqlLower(s.substr(i, 12)) == "create table") {
            size_t j = i + 12;
            while (j < n && (s[j] == ' ' || s[j] == '\t' || s[j] == '\r' || s[j] == '\n')) ++j;
            std::string name;
            if (j < n && s[j] == '`') {
                ++j;
                while (j < n && s[j] != '`') name += s[j++];
                if (j < n) ++j;
            } else {
                while (j < n && sqlIsWord(s[j])) name += s[j++];
            }
            while (j < n && s[j] != '(') ++j;
            if (j < n) {
                size_t end = sqlFindParenEnd(s, j);
                if (end != std::string::npos) {
                    SqlTable t;
                    for (const std::string& piece : sqlSplitTop(s, j + 1, end)) {
                        std::string pc = sqlTrim(piece);
                        if (pc.empty()) continue;
                        if (pc[0] == '`') {
                            size_t e2 = pc.find('`', 1);
                            if (e2 != std::string::npos) {
                                SqlCol c;
                                c.name = pc.substr(1, e2 - 1);
                                std::string rest = sqlTrim(pc.substr(e2 + 1));
                                c.numeric = sqlTypeNumeric(rest.substr(0, rest.find_first_of(" \t\r\n(")));
                                t.cols.push_back(std::move(c));
                            }
                        }
                        // PRIMARY KEY / KEY / UNIQUE / CONSTRAINT ... lines are skipped.
                    }
                    if (!t.cols.empty()) {
                        // First definition wins so adb.sql (which carries the
                        // data rows) is not clobbered by an empty schema later.
                        sqlDb().emplace(sqlLower(name), std::move(t));
                    }
                    i = end + 1;
                    continue;
                }
            }
        }
        // INSERT INTO `name` VALUES (...),(...);
        if (i + 10 < n && sqlLower(s.substr(i, 11)) == "insert into") {
            size_t j = i + 11;
            while (j < n && (s[j] == ' ' || s[j] == '\t' || s[j] == '\r' || s[j] == '\n')) ++j;
            std::string name;
            if (j < n && s[j] == '`') {
                ++j;
                while (j < n && s[j] != '`') name += s[j++];
                if (j < n) ++j;
            } else {
                while (j < n && sqlIsWord(s[j])) name += s[j++];
            }
            while (j + 5 < n && sqlLower(s.substr(j, 6)) != "values") ++j;  // skip optional column list
            if (j + 5 >= n) {
                i = n;
                break;
            }
            j += 6;
            auto it = sqlDb().find(sqlLower(name));
            if (it == sqlDb().end()) {
                i = j;
                continue;
            }
            while (j < n) {
                while (j < n && (s[j] == ' ' || s[j] == '\t' || s[j] == '\r' || s[j] == '\n')) ++j;
                if (j >= n || s[j] != '(') break;
                size_t end = sqlFindParenEnd(s, j);
                if (end == std::string::npos) break;
                const auto& cols = it->second.cols;
                auto parts = sqlSplitTop(s, j + 1, end);
                std::vector<Value> row;
                row.reserve(parts.size());
                for (size_t k = 0; k < parts.size(); ++k)
                    row.push_back(sqlValue(parts[k], k < cols.size() && cols[k].numeric));
                it->second.rows.push_back(std::move(row));
                j = end + 1;
                while (j < n && (s[j] == ' ' || s[j] == '\t' || s[j] == '\r' || s[j] == '\n')) ++j;
                if (j < n && s[j] == ',') {
                    ++j;
                    continue;
                }
                break;  // expect ';' or end of statement
            }
            i = j;
            continue;
        }
        ++i;
    }
}

static void sqlLoadAll() {
    static std::once_flag once;
    std::call_once(once, [] {
        std::lock_guard<std::mutex> lk(sqlDbMutex());
        for (const char* f : {"adb.sql", "mdb.sql", "ldb.sql", "ddb.sql"}) {
            sqlLoadFile(fs::path("linux/new_sql") / f);
            sqlLoadFile(fs::path("new_sql") / f);
        }
        if (std::getenv("LPCVM_TRACE")) {
            std::cerr << "[sql] loaded " << sqlDb().size() << " tables\n";
            for (const auto& kv : sqlDb())
                std::cerr << "  " << kv.first << ": cols=" << kv.second.cols.size()
                          << " rows=" << kv.second.rows.size() << "\n";
        }
    });
}

Value simpleDbSelect(const std::string& table) {
    auto arr = std::make_shared<ArrayValue>();
    sqlLoadAll();
    {
        std::lock_guard<std::mutex> lk(sqlDbMutex());
        auto it = sqlDb().find(sqlLower(table));
        if (it != sqlDb().end()) {
            const SqlTable& t = it->second;
            for (const auto& row : t.rows) {
                auto mp = std::make_shared<MappingValue>(static_cast<uint32_t>(t.cols.size()));
                for (size_t i = 0; i < t.cols.size() && i < row.size(); ++i)
                    mp->insert(make_string(t.cols[i].name), row[i]);
                arr->items.push_back(mp);
            }
        }
    }
    // Tables not present in the SQL dumps return an empty result set.  (The
    // old local-CSV stub produced fabricated columns that did not match the
    // real schema, so it has been removed.)
    return arr;
}

Value get_index(const Value& c, const Value& idx) {
    if (c.index() == 5) {
        auto arr = std::get<std::shared_ptr<ArrayValue>>(c);
        int64_t i = as_int(idx);
        if (i < 0) i += static_cast<int64_t>(arr->items.size());
        if (i >= 0 && i < static_cast<int64_t>(arr->items.size())) return arr->items[static_cast<size_t>(i)];
        return UndefinedTag{};
    }
    if (c.index() == 6) {
        auto mp = std::get<std::shared_ptr<MappingValue>>(c);
        const Value* hit = mp->find(idx);
        if (hit) return *hit;
        return UndefinedTag{};
    }
    return UndefinedTag{};
}

} // namespace

void setThreadTelnetClient(int clientNo) { tlsCurrentClient = clientNo; }
int threadTelnetClient() { return tlsCurrentClient; }

std::string& driverTelnetPassword() {
    static std::string pwd = "123456";
    return pwd;
}

void setDriverTelnetPassword(const std::string& pwd) { driverTelnetPassword() = pwd; }

void driverTelnetShellLoop(SOCKET listener) {
    while (true) {
        SOCKET c = accept(listener, nullptr, nullptr);
        if (c == INVALID_SOCKET) {
            if (WSAGetLastError() == WSAEINTR) continue;
            break;
        }
        std::thread([c]() {
            int cno = nextTelnetClientNo().fetch_add(1);
            {
                std::lock_guard<std::mutex> lk(telnetMutex());
                TelnetClient& tc = telnetClients()[cno];
                tc.sock = c;
                tc.nativeProcess = true;
            }
            setThreadTelnetClient(cno);
            telnetSend(cno, telnetLineEnd("Welcome to LPC driver shell!\r\n"));
            telnetSend(cno, telnetLineEnd("Password: "));
            std::string pwd = telnetReadLine(cno);
            if (pwd != driverTelnetPassword()) {
                telnetSend(cno, telnetLineEnd("\r\nBad password!\r\n"));
                closeTelnetClient(cno);
                setThreadTelnetClient(-1);
                return;
            }
            telnetSend(cno, telnetLineEnd("\r\nLogged in.  Type LPC expressions; \".\" to quit.\r\n"));
            while (findTelnetClient(cno)) {
                telnetSend(cno, telnetLineEnd("shell> "));
                std::string line = telnetReadLine(cno);
                if (line.empty()) break;
                if (line == ".") {
                    telnetSend(cno, telnetLineEnd("logout\r\n"));
                    closeTelnetClient(cno);
                    break;
                }
                Value r = call_builtin(nullptr, "execute_data", {make_string(line), make_int(1)});
                (void)r;
            }
            closeTelnetClient(cno);
            setThreadTelnetClient(-1);
        }).detach();
    }
}

int startDriverTelnet(int port) {
    ensureWsa();
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return 0;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<u_short>(port));
    if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        closesocket(s);
        return 0;
    }
    if (listen(s, 16) == SOCKET_ERROR) {
        closesocket(s);
        return 0;
    }
    std::thread(driverTelnetShellLoop, s).detach();
    std::cout << "Driver telnet shell started on port " << port << "\n";
    return 1;
}

int lpcSscanf(const std::string& input, const std::string& fmt, std::vector<Value>& out,
              int maxOutputs) {
    // Faithful port of vm_interSscanf (0x8109908):
    //   - literal characters match exactly (NO whitespace skipping anywhere),
    //   - %s followed by literal text matches the SHORTEST prefix that lets
    //     the whole literal segment (up to the next '%') match, and may
    //     contain whitespace (sscanf("/a.c : /d.c", "%s:%s") -> 2 matches),
    //   - %s at the end of the format matches the whole remaining input,
    //   - %s directly followed by another conversion skips forward to where
    //     that conversion could start ('%d'/'%f': first digit, '%x': past
    //     leading zeros/0x, '%c': one char, '%%': next '%'),
    //   - when the format is exhausted but input remains (and an output
    //     argument is left), the rest of the input becomes one extra string
    //     match (LABEL_10),
    //   - %d/%u/%x/%f parse with strtol/strtoul/strtof semantics.
    const char* s = input.c_str();
    const char* f = fmt.c_str();
    int matches = 0;
    bool unlimited = maxOutputs < 0;
    auto room = [&]() { return unlimited || static_cast<int>(out.size()) < maxOutputs; };
    auto isDigit = [](char c) { return c >= '0' && c <= '9'; };
    while (*f) {
        if (*f != '%') {            // literal match (exact, no ws skipping)
            if (*s != *f) return matches;
            ++s; ++f;
            continue;
        }
        ++f;                        // past '%'
        if (*f == '%') {            // literal '%'
            if (*s != '%') return matches;
            ++s; ++f;
            continue;
        }
        if (!*f) return matches;    // format ends in '%'
        bool suppress = false;
        if (*f == '*') { suppress = true; ++f; }
        char conv = *f++;
        switch (conv) {
        case 's': {
            if (!*f) {
                // %s at the end of the format: the whole remaining input.
                if (!suppress) out.push_back(make_string(s));
                return matches + 1;
            }
            if (*f != '%') {
                // Literal segment follows: match the shortest prefix before
                // the first occurrence of the whole segment (may contain
                // whitespace; lazy like the original's LABEL_24 scan).
                const char* after = f + 1;
                const char* pct = strchr(after, '%');
                size_t num = pct ? static_cast<size_t>(pct - after + 1) : strlen(after) + 1;
                const char* start = s;
                const char* p = s;
                bool hit = false;
                while (*p) {
                    if (*p == *f && strncmp(p, f, num) == 0) {
                        if (!suppress)
                            out.push_back(make_string(std::string(start, static_cast<size_t>(p - start))));
                        s = p + num;
                        f = pct ? pct : after + strlen(after);
                        ++matches;
                        hit = true;
                        break;
                    }
                    ++p;
                }
                if (!hit) return matches;   // segment never matched
                break;
            }
            // %s directly followed by another conversion: skip forward to
            // where that conversion could start.
            char nextConv = f[1];
            if (nextConv == '*') nextConv = f[2];
            const char* t = s;
            if (nextConv == 'd') {
                while (*t && !isDigit(*t)) {
                    if ((*t == '+' || *t == '-') && isDigit(t[1])) break;
                    ++t;
                }
            } else if (nextConv == 'f') {
                while (*t && !isDigit(*t)) {
                    if ((*t == '+' || *t == '-') && isDigit(t[1])) break;
                    if (*t == '.' && isDigit(t[1])) break;
                    ++t;
                }
            } else if (nextConv == 'x') {
                while (*t) {
                    if (*t != '0') break;
                    if ((t[1] == 'x' || t[1] == 'X') && isxdigit(static_cast<unsigned char>(t[2]))) break;
                    ++t;
                }
            } else if (nextConv == 'c') {
                if (*t) t = s + 1;
            } else if (nextConv == '%') {
                while (*t && *t != '%') ++t;
            } else if (nextConv == 's') {
                return matches;   // adjacent %s is illegal
            }
            if (!suppress) out.push_back(make_string(std::string(s, static_cast<size_t>(t - s))));
            ++matches;
            s = t;
            if (!*t) return matches;
            break;
        }
        case 'd': {
            char* end = nullptr;
            long long v = std::strtoll(s, &end, 10);
            if (end == s) return matches;
            s = end;
            if (!suppress) out.push_back(make_int(static_cast<int64_t>(v)));
            ++matches;
            break;
        }
        case 'u': {
            char* end = nullptr;
            unsigned long long v = std::strtoull(s, &end, 10);
            if (end == s) return matches;
            s = end;
            if (!suppress) out.push_back(make_int(static_cast<int64_t>(v)));
            ++matches;
            break;
        }
        case 'x': {
            char* end = nullptr;
            unsigned long long v = std::strtoull(s, &end, 16);
            if (end == s) return matches;
            s = end;
            if (!suppress) out.push_back(make_int(static_cast<int64_t>(v)));
            ++matches;
            break;
        }
        case 'c': {
            if (!*s) return matches;
            if (!suppress) out.push_back(make_int(static_cast<unsigned char>(*s)));
            ++s;
            ++matches;
            break;
        }
        case 'f': {
            char* end = nullptr;
            double v = std::strtod(s, &end);
            if (end == s) return matches;
            s = end;
            if (!suppress) out.push_back(make_float(v));
            ++matches;
            break;
        }
        default:
            return matches;   // bad conversion
        }
    }
    // Format exhausted; remaining input becomes one extra string match when
    // an output argument is still available (LABEL_10 in vm_interSscanf).
    if (*s && room()) {
        out.push_back(make_string(s));
        ++matches;
    }
    return matches;
}

Value call_builtin(Interpreter*, const std::string& name, const std::vector<Value>& args) {
    auto need = [&](size_t n) -> bool { return args.size() >= n; };

    if (name == "write" || name == "printf" || name == "dump_file") {
        if (!args.empty()) {
            std::vector<Value> rest(args.begin() + 1, args.end());
            std::string out = formatLpc(as_string(args[0]), rest);
            if (std::getenv("LPCVM_TRACE") && out.find("Bad line") != std::string::npos)
                std::cerr << "[printf-badline] out=[" << out.substr(0, 80) << "]\n";
            // The original driver writes the formatted string exactly as-is
            // (no auto newline; the LPC format strings carry their own \n).
            int cno = threadTelnetClient();
            if (cno >= 0 && findTelnetClient(cno)) {
                telnetSend(cno, telnetLineEnd(out));
                return make_int(1);
            }
            fputs(out.c_str(), stdout);
            fflush(stdout);
        }
        return make_int(1);
    }
    if (name == "sprintf") {
        if (args.empty()) return make_string("");
        std::vector<Value> rest(args.begin() + 1, args.end());
        return make_string(formatLpc(as_string(args[0]), rest));
    }
    if (name == "replace_string") {
        // replace_string(str, from, to) - replace ALL occurrences.
        if (args.size() >= 3) {
            std::string s = as_string(args[0]);
            std::string from = as_string(args[1]);
            std::string to = as_string(args[2]);
            if (from.empty()) return make_string(s);
            std::string out;
            size_t pos = 0;
            for (;;) {
                size_t p = s.find(from, pos);
                if (p == std::string::npos) {
                    out += s.substr(pos);
                    break;
                }
                out += s.substr(pos, p - pos);
                out += to;
                pos = p + from.size();
            }
            return make_string(out);
        }
        return make_string("");
    }
    if (name == "atoi") {
        if (args.empty()) return make_int(0);
        const Value& v = args[0];
        if (v.index() == 2) return v;
        if (v.index() == 4) {
            try { return make_int(static_cast<int64_t>(std::stoll(sqlTrim(std::get<std::string>(v))))); }
            catch (...) { return make_int(0); }
        }
        return make_int(0);
    }
    if (name == "atof") {
        if (args.empty()) return make_float(0);
        const Value& v = args[0];
        if (v.index() == 3) return v;
        if (v.index() == 2) return make_float(static_cast<double>(std::get<int64_t>(v)));
        if (v.index() == 4) {
            try { return make_float(std::stod(sqlTrim(std::get<std::string>(v)))); }
            catch (...) { return make_float(0); }
        }
        return make_float(0);
    }
    if (name == "sizeof" || name == "size") {
        if (args.empty()) return make_int(0);
        const Value& v = args[0];
        if (v.index() == 5) return make_int(static_cast<int64_t>(std::get<std::shared_ptr<ArrayValue>>(v)->items.size()));
        if (v.index() == 6) return make_int(static_cast<int64_t>(std::get<std::shared_ptr<MappingValue>>(v)->size()));
        if (v.index() == 4) return make_int(static_cast<int64_t>(std::get<std::string>(v).size()));
        return make_int(0);
    }
    if (name == "strlen") return args.empty() ? make_int(0) : make_int(static_cast<int64_t>(as_string(args[0]).size()));
    if (name == "is_undefined") return make_int(!args.empty() && args[0].index() == 1 ? 1 : 0);
    if (name == "is_mapping") return make_int(!args.empty() && args[0].index() == 6 ? 1 : 0);
    if (name == "is_array") return make_int(!args.empty() && args[0].index() == 5 ? 1 : 0);
    if (name == "is_function") return make_int(!args.empty() && args[0].index() == 8 ? 1 : 0);
    if (name == "is_string") return make_int(!args.empty() && args[0].index() == 4 ? 1 : 0);
    if (name == "is_int") return make_int(!args.empty() && args[0].index() == 2 ? 1 : 0);
    if (name == "is_float") return make_int(!args.empty() && args[0].index() == 3 ? 1 : 0);
    if (name == "is_object") return make_int(!args.empty() && args[0].index() == 7 ? 1 : 0);
    if (name == "to_int" || name == "to_number") return args.empty() ? make_int(0) : make_int(as_int(args[0]));
    if (name == "to_string") return args.empty() ? make_string("") : make_string(as_string(args[0]));
    if (name == "to_float") return args.empty() ? make_float(0.0) : make_float(as_int(args[0]));
    if (name == "abs") return args.empty() ? make_int(0) : make_int(std::llabs(as_int(args[0])));
    if (name == "min") return args.empty() ? make_int(0) : make_int(as_int(*std::min_element(args.begin(), args.end(), [](const Value& a, const Value& b) { return as_int(a) < as_int(b); })));
    if (name == "max") return args.empty() ? make_int(0) : make_int(as_int(*std::max_element(args.begin(), args.end(), [](const Value& a, const Value& b) { return as_int(a) < as_int(b); })));
    if (name == "keys") {
        if (args.empty() || args[0].index() != 6) return make_array();
        auto arr = std::make_shared<ArrayValue>();
        for (auto& kv : std::get<std::shared_ptr<MappingValue>>(args[0])->items()) arr->items.push_back(kv.first);
        return arr;
    }
    if (name == "values") {
        if (args.empty() || args[0].index() != 6) return make_array();
        auto arr = std::make_shared<ArrayValue>();
        for (auto& kv : std::get<6>(args[0])->items()) arr->items.push_back(kv.second);
        return arr;
    }
    if (name == "map_delete") {
        if (args.size() >= 2 && args[0].index() == 6) {
            auto mp = std::get<6>(args[0]);
            mp->erase(args[1]);
        }
        return make_int(1);
    }
    if (name == "member_array") {
        if (args.size() >= 2 && args[1].index() == 5) {
            auto arr = std::get<std::shared_ptr<ArrayValue>>(args[1]);
            for (size_t i = 0; i < arr->items.size(); ++i) if (value_equal(arr->items[i], args[0])) return make_int(static_cast<int64_t>(i));
        }
        return make_int(-1);
    }
    if (name == "explode") {
        // vm_f_explode (0x80d5c68): explode(str) splits on "\n" by default;
        // explode(str, sep) splits on the given separator.  Empty separator
        // splits into individual characters.
        auto arr = std::make_shared<ArrayValue>();
        if (args.empty()) return arr;
        std::string s = as_string(args[0]);
        std::string sep = args.size() >= 2 ? as_string(args[1]) : "\n";
        if (std::getenv("LPCVM_TRACE") && std::getenv("LPCVM_TRACE_EXPLODE"))
            std::cerr << "[explode] len=" << s.size() << " sep=" << (args.size() >= 2 ? "explicit" : "newline")
                      << " first8=[" << s.substr(0, 8) << "]\n";
        if (sep.empty()) {
            for (char c : s) arr->items.push_back(make_string(std::string(1, c)));
            return arr;
        }
        size_t start = 0;
        while (true) {
            size_t p = s.find(sep, start);
            if (p == std::string::npos) {
                arr->items.push_back(make_string(s.substr(start)));
                break;
            }
            arr->items.push_back(make_string(s.substr(start, p - start)));
            start = p + sep.size();
        }
        if (std::getenv("LPCVM_TRACE") && std::getenv("LPCVM_TRACE_EXPLODE")) {
            std::string dbg;
            for (size_t i = 0; i < arr->items.size() && i < 4; ++i) {
                if (!dbg.empty()) dbg += "|";
                const std::string& e = std::get<std::string>(arr->items[i]);
                dbg += "[" + e.substr(0, 12) + "]";
            }
            std::string hex0;
            if (!arr->items.empty()) {
                const std::string& e0 = std::get<std::string>(arr->items[0]);
                char b[8];
                for (size_t k = 0; k < e0.size() && k < 8; ++k) { snprintf(b, sizeof(b), "%02X ", (unsigned char)e0[k]); hex0 += b; }
            }
            std::cerr << "[explode] n=" << arr->items.size() << " first=[" << dbg << "] elem0hex=" << hex0 << "\n";
        }
        return arr;
    }
    if (name == "implode") {
        if (args.size() < 2 || args[0].index() != 5) return make_string("");
        auto arr = std::get<std::shared_ptr<ArrayValue>>(args[0]);
        std::string sep = as_string(args[1]), out;
        for (size_t i = 0; i < arr->items.size(); ++i) {
            if (i) out += sep;
            out += as_string(arr->items[i]);
        }
        return make_string(out);
    }
    if (name == "allocate") {
        auto arr = std::make_shared<ArrayValue>();
        if (!args.empty()) {
            int64_t n = as_int(args[0]);
            for (int64_t i = 0; i < n; ++i) arr->items.push_back(make_int(0));
        }
        return arr;
    }
    if (name == "query_temp" || name == "query") return make_int(0);
    if (name == "set_name" || name == "set_temp" || name == "set") return make_int(1);
    if (name == "destruct_object" || name == "to_be_disconnect" || name == "is_clone") return name == "is_clone" ? make_int(0) : make_int(1);
    if (name == "register_daemon_data" || name == "dispatch_service" || name == "send_to_cta") return make_int(1);
    if (name == "find_function") {
        // Return a function value for a builtin efun (the game uses this to
        // build the mysql_connect closure in find_mysql_func).
        if (args.empty()) return make_int(0);
        std::string fnName = as_string(args[0]);
        const auto& names = efunNameTable();
        for (size_t i = 0; i < names.size(); ++i) {
            if (names[i] == fnName) return make_closure(fnName, {});
        }
        return make_int(0);
    }
    if (name == "allocate_array") {
        // allocate_array(n) -> an array with n undefined slots.
        auto arr = std::make_shared<ArrayValue>();
        int64_t n = args.empty() ? 0 : as_int(args[0]);
        if (n < 0) n = 0;
        for (int64_t i = 0; i < n; ++i) arr->items.push_back(UndefinedTag{});
        return arr;
    }
    if (name == "defined") {
        // The original driver is built with CONFIG defined, which selects the
        // direct find_function path in find_mysql_func.
        if (!args.empty() && as_string(args[0]) == "CONFIG") return make_int(1);
        return make_int(0);
    }
    if (name == "expand_macro") {
        // vm_f_expand_macro (0x80d5598) looks up the predefine table.
        // __BIN_DIR__ (vm_getBinDir) returns the -b directory WRAPPED IN
        // DOUBLE QUOTES; the LPC startup strips them with [1..<2].
        if (!args.empty() && args[0].index() == 4) {
            const std::string& key = std::get<std::string>(args[0]);
            if (key == "__BIN_DIR__") {
                if (pakMounted())
                    return make_string("\"" + pakVirtualPrefix() + "\"");
                return make_string("\"\"");
            }
            if (key == "CONFIG") return make_int(0);
        }
        return make_int(0);
    }
    if (name == "set_case_check") {
        // vm_f_set_case_check (0x80db520) -> vm_setCaseCheck(m, flag, obsorb):
        // flag != 0 turns case sensitivity ON, 0 turns it OFF.
        if (args.size() >= 2 && args[0].index() == 6) {
            auto mp = std::get<std::shared_ptr<MappingValue>>(args[0]);
            int flag = static_cast<int>(as_int(args[1]));
            int obsorb = args.size() >= 3 ? static_cast<int>(as_int(args[2])) : 0;
            mp->setCaseCheck(flag != 0, obsorb);
            return make_int(1);
        }
        return make_int(0);
    }
    if (name == "trim") {
        if (args.empty()) return make_string("");
        std::string s = as_string(args[0]);
        if (std::getenv("LPCVM_TRACE") && std::getenv("LPCVM_TRACE_EXPLODE"))
            std::cerr << "[trim] in=[" << s.substr(0, 16) << "] len=" << s.size() << "\n";
        size_t b = s.find_first_not_of(" \t\r\n");
        if (b == std::string::npos) {
            if (std::getenv("LPCVM_TRACE") && std::getenv("LPCVM_TRACE_EXPLODE"))
                std::cerr << "[trim] -> empty\n";
            return make_string("");
        }
        size_t e = s.find_last_not_of(" \t\r\n");
        std::string r = s.substr(b, e - b + 1);
        if (std::getenv("LPCVM_TRACE") && std::getenv("LPCVM_TRACE_EXPLODE"))
            std::cerr << "[trim] -> [" << r.substr(0, 16) << "]\n";
        return make_string(r);
    }
    if (name == "relinquish") return make_int(1);
    if (name == "get_dir_ex" || name == "get_dir") {
        // Original vm_f_get_dir2_in_st (0x80eb5d4):
        //   - paths containing ".pak/" are packed volumes listed through the
        //     pak catalog in CATALOG order (vm_getDirInPackedFs),
        //   - otherwise opendir/readdir order (no sorting),
        //   - the last path component is the wildcard pattern ('*' any run,
        //     '?' one char, '\\' escape - vm_matchString); an empty pattern
        //     behaves like "*",
        //   - flag 0 -> plain names; flag 1/2 -> ({name, size, mtime}) with
        //     size -2 for directories.
        auto arr = std::make_shared<ArrayValue>();
        if (args.empty()) return arr;
        std::string pattern = as_string(args[0]);
        int flag = args.size() >= 2 ? static_cast<int>(as_int(args[1])) : 0;

        // split into dir + pattern at the last '/'
        std::string dirPart, pat;
        {
            size_t slash = pattern.find_last_of('/');
            if (slash == std::string::npos) {
                dirPart = "";
                pat = pattern;
            } else {
                dirPart = pattern.substr(0, slash + 1);
                pat = pattern.substr(slash + 1);
            }
            if (pat.empty()) pat = "*";
        }

        auto pushPlain = [&](const std::string& name, int64_t size, int64_t mtime) {
            if (flag) {
                auto item = std::make_shared<ArrayValue>();
                item->items.push_back(make_string(name));
                item->items.push_back(make_int(size));
                item->items.push_back(make_int(mtime));
                arr->items.push_back(item);
            } else {
                arr->items.push_back(make_string(name));
            }
        };

        // packed volume?
        if (pattern.find(".pak/") != std::string::npos && pakMounted()) {
            std::vector<const PakEntry*> entries;
            if (!pakListDir(dirPart, entries)) return arr;
            for (const PakEntry* e : entries) {
                if (!matchString(e->name, pat)) continue;
                int64_t size = e->isDir ? -2 : static_cast<int64_t>(e->origSize);
                pushPlain(e->name, size, 0);
            }
            return arr;
        }

        // real filesystem: readdir order (NTFS gives name order; the original
        // relied on ext3 readdir order, but on the deployed servers these
        // plain .c directories were empty - everything lived in the pak).
        fs::path dir = resolveVfsPath(dirPart.empty() ? "/" : dirPart);
        std::error_code ec;
        if (!fs::exists(dir, ec)) return arr;
        for (auto& p : fs::directory_iterator(dir, ec)) {
            std::string name = p.path().filename().string();
            if (!matchString(name, pat)) continue;
            std::error_code ec2;
            int64_t size = -2, mtime = 0;
            if (!fs::is_directory(p.path(), ec2)) {
                size = static_cast<int64_t>(fs::file_size(p.path(), ec2));
                auto ft = fs::last_write_time(p.path(), ec2);
                mtime = static_cast<int64_t>(ft.time_since_epoch().count() / 10000000);
            }
            pushPlain(name, size, mtime);
        }
        return arr;
    }
    if (name == "get_sub_dirs_ex") {
        auto arr = std::make_shared<ArrayValue>();
        if (args.empty()) return arr;
        fs::path dir = resolveVfsPath(as_string(args[0]));
        std::error_code ec;
        if (!fs::exists(dir, ec)) return arr;
        std::vector<std::string> names;
        for (auto& p : fs::directory_iterator(dir, ec)) {
            if (p.is_directory()) names.push_back(p.path().filename().string());
        }
        std::sort(names.begin(), names.end());
        for (auto& n : names) {
            auto item = std::make_shared<ArrayValue>();
            item->items.push_back(make_string(n));
            item->items.push_back(make_int(-2));
            arr->items.push_back(item);
        }
        return arr;
    }
    if (name == "safe_load_object" || name == "load_object") {
        // The driver-side object loader: load + initialize the object and
        // register it so find_object / cross-object calls can reach it.
        // The "Load: ... OK" line is printed by the LPC safe_load_object
        // wrapper itself; do not duplicate it here.
        if (args.empty()) return make_int(0);
        std::string vpath = normalizeSlashes(as_string(args[0]));
        if (std::getenv("LPCVM_TRACE"))
            std::cerr << "[load_object] " << vpath << "\n";
        int ok = loadObjectIntoRegistry(vpath, vfsRoot());
        return make_int(ok);
    }
    if (name == "find_object") {
        // find_object(path) - object value if loaded, else 0.
        if (args.empty()) return make_int(0);
        std::string vpath = normalizeSlashes(as_string(args[0]));
        return findObjectInRegistry(vpath, vfsRoot());
    }
    if (name == "object_name") {
        // object_name(ob) - return the object's path.
        if (!args.empty() && args[0].index() == 7) {
            auto ov = std::get<std::shared_ptr<ObjectValue>>(args[0]);
            if (!ov->programName.empty()) return make_string(ov->programName);
        }
        return make_string("");
    }
    if (name == "file_size") {
        if (args.empty()) return make_int(0);
        std::error_code ec;
        auto sz = fs::file_size(resolveVfsPath(as_string(args[0])), ec);
        return make_int(static_cast<int64_t>(sz));
    }
    if (name == "read_file") {
        if (args.empty()) return make_string("");
        std::string vpath = as_string(args[0]);
        if (std::getenv("LPCVM_TRACE") && std::getenv("LPCVM_TRACE_EXPLODE"))
            std::cerr << "[read_file] path=" << vpath << "\n";
        std::string content = readVfsFile(vpath);
        if (std::getenv("LPCVM_TRACE") && std::getenv("LPCVM_TRACE_EXPLODE"))
            std::cerr << "[read_file] -> " << content.size() << " bytes, head=["
                      << content.substr(0, 20) << "]\n";
        // mode "b" (buffer): the original returns a buffer; we represent
        // buffers as byte strings so is_string() sees a string and slicing
        // works.  Content is already raw bytes.
        return make_string(content);
    }
    if (name == "write_file") {
        if (args.size() >= 2) {
            std::ofstream out(resolveVfsPath(as_string(args[0])), std::ios::app);
            out << as_string(args[1]);
        }
        return make_int(1);
    }
    if (name == "telnet_on") {
        ensureWsa();
        int port = args.empty() ? 8001 : static_cast<int>(as_int(args[0]));
        SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s == INVALID_SOCKET) return make_int(0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(static_cast<u_short>(port));
        if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
            closesocket(s);
            return make_int(0);
        }
        if (listen(s, 16) == SOCKET_ERROR) {
            closesocket(s);
            return make_int(0);
        }
        std::thread(telnetServerLoop, s).detach();
        std::cout << "Telnet service started on port " << port << "\n";
        return make_int(1);
    }
    if (name == "telnet_off") return make_int(1);
    if (name == "start_service_on") {
        ensureWsa();
        int port = args.empty() ? 8001 : static_cast<int>(as_int(args[0]));
        SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s == INVALID_SOCKET) return make_int(0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(static_cast<u_short>(port));
        if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
            closesocket(s);
            return make_int(0);
        }
        if (listen(s, 2104) == SOCKET_ERROR) {
            closesocket(s);
            return make_int(0);
        }
        std::thread(telnetServerLoop, s).detach();
        std::cout << "Service started on port " << port << "\n";
        return make_int(1);
    }
    if (name == "mysql_connect") {
        // mysql_connect(host, user, password, database) -> cno (>=1) or -1.
        // The game (dbd's assure_connection) passes host from aaa.ini, which
        // load_db_config rewrites to "port : host" when the port is set.
        if (args.size() >= 4) {
            std::string hostArg = as_string(args[0]);
            std::string user = as_string(args[1]);
            std::string pass = as_string(args[2]);
            std::string db = as_string(args[3]);
            std::string host;
            int port = 3306;
            parseMysqlHost(hostArg, host, port);
            // dbd's assure_connection pings every 60s and may call
            // mysql_connect again; reuse an existing connection for the same
            // target instead of opening a new one each time.
            std::string key = host + ":" + std::to_string(port) + ":" + user + ":" + db;
            {
                std::lock_guard<std::mutex> lk(mysqlHandlesMutex());
                for (auto& kv : mysqlHandles()) {
                    if (kv.second.db == key && kv.second.client && kv.second.client->isConnected()) {
                        if (std::getenv("LPCVM_TRACE"))
                            std::cerr << "[mysql_connect] reuse cno=" << kv.first << " for " << key << "\n";
                        return make_int(kv.first);
                    }
                }
            }
            auto client = std::make_shared<MysqlClient>();
            std::string err;
            int cno = nextMysqlCno();
            bool ok = client->connect(host, port, user, pass, db, err);
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[mysql_connect] host=" << host << ":" << port << " db=" << db
                          << " user=" << user << " ok=" << ok
                          << (ok ? "" : (" err=" + err)) << " cno=" << cno << "\n";
            if (!ok) {
                // Keep an offline handle so the game's connection flow
                // proceeds; queries fall back to the in-memory dump tables.
                std::cerr << "[mysql] connect to " << host << ":" << port << "/" << db
                          << " failed: " << err << " (using in-memory tables)\n";
            }
            {
                std::lock_guard<std::mutex> lk(mysqlHandlesMutex());
                mysqlHandles()[cno] = MysqlHandle{ok ? client : nullptr, key};
            }
            return make_int(cno);
        }
        return make_int(-1);
    }
    if (name == "mysql_is_connected") {
        if (!args.empty()) {
            std::lock_guard<std::mutex> lk(mysqlHandlesMutex());
            auto it = mysqlHandles().find(static_cast<int>(as_int(args[0])));
            if (it != mysqlHandles().end() && it->second.client && it->second.client->isConnected())
                return make_int(1);
        }
        return make_int(0);
    }
    if (name == "mysql_disconnect" || name == "mysql_close_db") {
        if (!args.empty()) {
            std::lock_guard<std::mutex> lk(mysqlHandlesMutex());
            auto it = mysqlHandles().find(static_cast<int>(as_int(args[0])));
            if (it != mysqlHandles().end()) {
                if (it->second.client) it->second.client->close();
                mysqlHandles().erase(it);
            }
        }
        return make_int(1);
    }
    if (name == "mysql_select_db" || name == "mysql_set_auto_commit") return make_int(1);
    if (name == "mysql_query") {
        // mysql_query(cno, sql) -> result handle (array of row mappings) for
        // SELECT; the game's mixed_mysql_select iterates it with
        // mysql_num_records/mysql_fetch_record/mysql_this_record.
        if (args.size() >= 2) {
            int cno = static_cast<int>(as_int(args[0]));
            std::string sql = as_string(args[1]);
            std::string table;
            size_t p = sql.find("from ");
            if (p != std::string::npos) {
                std::string rest = sql.substr(p + 5);
                while (!rest.empty() && (rest.front() == ' ' || rest.front() == '`')) rest.erase(rest.begin());
                size_t e = rest.find_first_of(" ;`");
                table = rest.substr(0, e);
            }
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[mysql_query] cno=" << cno << " sql=" << sql << " table=" << table << "\n";
            MysqlClient::Result res;
            std::string err;
            if (mysqlExecReal(cno, sql, res, err)) {
                if (res.hasResultSet) {
                    auto arr = std::make_shared<ArrayValue>();
                    for (const auto& row : res.rows) {
                        auto mp = std::make_shared<MappingValue>(static_cast<uint32_t>(res.cols.size()));
                        for (size_t i = 0; i < res.cols.size() && i < row.size(); ++i) {
                            const auto& cell = row[i];
                            if (cell.isNull) {
                                mp->insert(make_string(res.cols[i]), make_string(""));
                            } else if (cell.isNumber) {
                                mp->insert(make_string(res.cols[i]), make_int(cell.num));
                            } else {
                                mp->insert(make_string(res.cols[i]), make_string(cell.str));
                            }
                        }
                        arr->items.push_back(mp);
                    }
                    {
                        std::lock_guard<std::mutex> lk(mysqlStateMutex());
                        mysqlCursor()[arr.get()] = -1;   // before the first record
                        mysqlRecords()[arr.get()] = make_int(0);
                    }
                    if (std::getenv("LPCVM_TRACE"))
                        std::cerr << "[mysql_query] " << table << " rows=" << arr->items.size()
                                  << " ptr=" << arr.get() << " (live)\n";
                    return arr;
                }
                // Non-SELECT via mysql_query (game mostly uses mysql_execute
                // for writes): surface affected rows as a plain int.
                return make_int(static_cast<int64_t>(res.affectedRows));
            }
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[mysql_query] error: " << err << " -> in-memory fallback\n";
            // Fall back to the in-memory dump tables (offline or SQL error).
            if (!table.empty()) {
                Value r = simpleDbSelect(table);
                if (r.index() == 5) {
                    auto arr = std::get<std::shared_ptr<ArrayValue>>(r);
                    {
                        std::lock_guard<std::mutex> lk(mysqlStateMutex());
                        mysqlCursor()[arr.get()] = -1;
                        mysqlRecords()[arr.get()] = make_int(0);
                    }
                    if (std::getenv("LPCVM_TRACE"))
                        std::cerr << "[mysql_query] " << table << " rows=" << arr->items.size()
                                  << " ptr=" << arr.get() << " (memory)\n";
                }
                return r;
            }
        }
        return make_array();
    }
    if (name == "mysql_num_records") {
        if (!args.empty() && args[0].index() == 5) {
            auto arr = std::get<std::shared_ptr<ArrayValue>>(args[0]);
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[mysql_num_records] ptr=" << arr.get() << " size=" << arr->items.size() << "\n";
            return make_int(static_cast<int64_t>(arr->items.size()));
        }
        if (std::getenv("LPCVM_TRACE"))
            std::cerr << "[mysql_num_records] NOT-ARRAY index=" << (args.empty() ? -1 : static_cast<int>(args[0].index())) << "\n";
        return make_int(0);
    }
    if (name == "mysql_fetch_record") {
        // Advance the result-set cursor and publish the current record.
        // Returns 0 while a record is available, nonzero past the end.
        if (!args.empty() && args[0].index() == 5) {
            auto arr = std::get<std::shared_ptr<ArrayValue>>(args[0]);
            int cur;
            {
                std::lock_guard<std::mutex> lk(mysqlStateMutex());
                cur = ++mysqlCursor()[arr.get()];
                if (cur < static_cast<int>(arr->items.size()))
                    mysqlRecords()[arr.get()] = arr->items[static_cast<size_t>(cur)];
                else
                    mysqlRecords()[arr.get()] = make_int(0);
            }
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[mysql_fetch_record] ptr=" << arr.get() << " cur=" << cur
                          << " size=" << arr->items.size() << "\n";
            return cur < static_cast<int>(arr->items.size()) ? make_int(0) : make_int(1);
        }
        return make_int(1);
    }
    if (name == "mysql_this_record") {
        // The current record of THIS result set (published by fetch_record).
        if (!args.empty() && args[0].index() == 5) {
            auto arr = std::get<std::shared_ptr<ArrayValue>>(args[0]);
            std::lock_guard<std::mutex> lk(mysqlStateMutex());
            auto it = mysqlRecords().find(arr.get());
            if (it != mysqlRecords().end()) return it->second;
        }
        return make_int(0);
    }
    if (name == "mysql_finalize") {
        if (!args.empty() && args[0].index() == 5) {
            auto arr = std::get<std::shared_ptr<ArrayValue>>(args[0]);
            std::lock_guard<std::mutex> lk(mysqlStateMutex());
            mysqlCursor().erase(arr.get());
            mysqlRecords().erase(arr.get());
        }
        return make_int(1);
    }
    if (name == "mysql_field_value" || name == "mysql_field_name") {
        if (args.size() >= 2 && args[0].index() == 6) {
            auto mp = std::get<std::shared_ptr<MappingValue>>(args[0]);
            const Value* hit = mp->find(args[1]);
            if (hit) return *hit;
        }
        return make_int(0);
    }
    if (name == "mysql_get_error" || name == "mysql_finalize" ||
        name == "mysql_num_fields" || name == "mysql_int_field" ||
        name == "mysql_get_insert_id") return make_int(0);
    if (name == "mysql_execute") {
        // mysql_execute(cno, sql) - run a statement (usually a write) on the
        // live MySQL server; returns affected rows.
        if (args.size() >= 2) {
            int cno = static_cast<int>(as_int(args[0]));
            std::string sql = as_string(args[1]);
            std::string table;
            size_t p = sql.find("from ");
            if (p != std::string::npos) {
                std::string rest = sql.substr(p + 5);
                while (!rest.empty() && (rest.front() == ' ' || rest.front() == '`')) rest.erase(rest.begin());
                size_t e = rest.find_first_of(" ;`");
                table = rest.substr(0, e);
            }
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[mysql_execute] cno=" << cno << " sql=" << sql << " table=" << table << "\n";
            MysqlClient::Result res;
            std::string err;
            if (mysqlExecReal(cno, sql, res, err)) {
                if (res.hasResultSet) {
                    // SELECT via execute: return the rows as an array.
                    auto arr = std::make_shared<ArrayValue>();
                    for (const auto& row : res.rows) {
                        auto mp = std::make_shared<MappingValue>(static_cast<uint32_t>(res.cols.size()));
                        for (size_t i = 0; i < res.cols.size() && i < row.size(); ++i) {
                            const auto& cell = row[i];
                            if (cell.isNull) mp->insert(make_string(res.cols[i]), make_string(""));
                            else if (cell.isNumber) mp->insert(make_string(res.cols[i]), make_int(cell.num));
                            else mp->insert(make_string(res.cols[i]), make_string(cell.str));
                        }
                        arr->items.push_back(mp);
                    }
                    return arr;
                }
                return make_int(static_cast<int64_t>(res.affectedRows));
            }
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[mysql_execute] error: " << err << "\n";
            return make_int(0);
        }
        return make_int(0);
    }
    if (name == "mysql_bind_parameters") {
        // vm_f_mysql_bind_parameters (0x81703b0): replace every '?' in the
        // statement with the next parameter, formatting per type:
        //   undefined/null -> '' (empty quoted string)
        //   int            -> %d  (no quotes)
        //   float          -> %g  (no quotes)
        //   string         -> 'escaped' (single quotes, mysql escaping)
        // The result is a new SQL string (not a prepared statement).
        if (args.empty()) return make_int(0);
        std::string sql = as_string(args[0]);
        std::string out;
        out.reserve(sql.size() + 32);
        size_t param = 1;  // args[0] is the statement
        for (size_t i = 0; i < sql.size(); ++i) {
            char c = sql[i];
            if (c != '?') {
                out += c;
                continue;
            }
            if (param >= args.size()) {
                // Too few parameters: keep the placeholder (original errors).
                out += c;
                continue;
            }
            const Value& p = args[param++];
            switch (p.index()) {
            case 2:  // int
                out += std::to_string(std::get<int64_t>(p));
                break;
            case 3: {  // float
                char buf[64];
                snprintf(buf, sizeof(buf), "%g", std::get<double>(p));
                out += buf;
                break;
            }
            case 4: {  // string, single-quoted with escaping
                const std::string& s = std::get<std::string>(p);
                out += '\'';
                for (char sc : s) {
                    if (sc == '\'' || sc == '\\' || sc == '\n' || sc == '\r' ||
                        sc == '\0' || sc == '\x1a' || sc == '"') {
                        out += '\\';
                    }
                    out += sc;
                }
                out += '\'';
                break;
            }
            default:  // undefined / null / others -> empty quoted string
                out += "''";
                break;
            }
        }
        return make_string(out);
    }
    if (name == "mysql_select") {
        if (args.size() < 2) return make_array();
        int cno = static_cast<int>(as_int(args[0]));
        std::string sql = as_string(args[1]);
        std::string table;
        size_t p = sql.find("from ");
        if (p != std::string::npos) {
            std::string rest = sql.substr(p + 5);
            while (!rest.empty() && (rest.front() == ' ' || rest.front() == '`')) rest.erase(rest.begin());
            size_t e = rest.find_first_of(" ;`");
            table = rest.substr(0, e);
        }
        MysqlClient::Result res;
        std::string err;
        if (mysqlExecReal(cno, sql, res, err) && res.hasResultSet) {
            auto arr = std::make_shared<ArrayValue>();
            for (const auto& row : res.rows) {
                auto mp = std::make_shared<MappingValue>(static_cast<uint32_t>(res.cols.size()));
                for (size_t i = 0; i < res.cols.size() && i < row.size(); ++i) {
                    const auto& cell = row[i];
                    if (cell.isNull) mp->insert(make_string(res.cols[i]), make_string(""));
                    else if (cell.isNumber) mp->insert(make_string(res.cols[i]), make_int(cell.num));
                    else mp->insert(make_string(res.cols[i]), make_string(cell.str));
                }
                arr->items.push_back(mp);
            }
            return arr;
        }
        if (table.empty()) return make_array();
        return simpleDbSelect(table);
    }
    if (name == "mysql_select_single_result") {
        if (args.size() >= 2 && args[1].index() == 5) {
            auto arr = std::get<std::shared_ptr<ArrayValue>>(args[1]);
            if (!arr->items.empty()) return arr->items[0];
        }
        return make_int(0);
    }
    if (name == "read_ini") {
        // read_ini(section, key, file) - read one value from an INI file.
        if (args.size() >= 3) {
            std::string section = as_string(args[0]);
            std::string key = as_string(args[1]);
            std::string path = resolveVfsPath(as_string(args[2]));
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[read_ini] section=" << section << " key=" << key
                          << " file=" << as_string(args[2]) << " -> " << path << "\n";
            std::ifstream in(path);
            if (in) {
                std::string cur;
                std::string line;
                auto trim = [](const std::string& s) {
                    size_t b = s.find_first_not_of(" \t\r\n");
                    if (b == std::string::npos) return std::string();
                    size_t e = s.find_last_not_of(" \t\r\n");
                    return s.substr(b, e - b + 1);
                };
                while (std::getline(in, line)) {
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    size_t s = line.find_first_not_of(" \t");
                    if (s == std::string::npos) continue;
                    if (line[s] == '#' || line[s] == ';') continue;
                    if (line[s] == '[') {
                        size_t e = line.find(']', s);
                        if (e == std::string::npos) continue;
                        cur = trim(line.substr(s + 1, e - s - 1));
                        continue;
                    }
                    if (cur != section) continue;
                    size_t eq = line.find('=', s);
                    if (eq == std::string::npos) continue;
                    if (trim(line.substr(s, eq - s)) == key)
                        return make_string(trim(line.substr(eq + 1)));
                }
            } else {
                std::cout << "read_ini: cannot open " << path << "\n";
            }
        }
        return make_string("");
    }
    if (name == "get_raw_password") {
        // The original driver hashes the raw password; keep the plain text so the
        // stub sql backend can use it directly.
        if (!args.empty()) return args[0];
        return make_string("");
    }
    if (name == "read_whole_ini") {
        // Parse an INI file into { section : { key : value } }.
        if (args.empty()) return make_int(0);
        std::string path = resolveVfsPath(as_string(args[0]));
        std::ifstream in(path);
        if (!in) {
            std::cout << "read_whole_ini: cannot open " << path << "\n";
            return make_int(0);
        }
        auto root = std::make_shared<MappingValue>();
        std::shared_ptr<MappingValue> cur;
        std::string line;
        auto trim = [](const std::string& s) {
            size_t b = s.find_first_not_of(" \t\r\n");
            if (b == std::string::npos) return std::string();
            size_t e = s.find_last_not_of(" \t\r\n");
            return s.substr(b, e - b + 1);
        };
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            size_t s = line.find_first_not_of(" \t");
            if (s == std::string::npos) continue;
            if (line[s] == '#' || line[s] == ';' || line[s] == '/') continue;
            if (line[s] == '[') {
                size_t e = line.find(']', s);
                if (e == std::string::npos) continue;
                std::string sec = trim(line.substr(s + 1, e - s - 1));
                cur = std::make_shared<MappingValue>();
                root->insert(make_string(sec), cur);
                continue;
            }
            if (!cur) continue;
            size_t eq = line.find('=', s);
            if (eq == std::string::npos) continue;
            std::string key = trim(line.substr(s, eq - s));
            std::string val = trim(line.substr(eq + 1));
            cur->insert(make_string(key), make_string(val));
        }
        return root;
    }
    if (name == "db_open") {
        if (!args.empty()) simpleDbLoad(as_string(args[0]));
        return make_int(1);
    }
    if (name == "db_set") {
        if (args.size() >= 2) {
            simpleDb()[as_string(args[0])] = as_string(args[1]);
            simpleDbSave("lpcvm.db");
        }
        return make_int(1);
    }
    if (name == "db_get") {
        if (args.empty()) return make_int(0);
        auto it = simpleDb().find(as_string(args[0]));
        if (it == simpleDb().end()) return make_int(0);
        return make_string(it->second);
    }
    if (name == "db_delete") {
        if (!args.empty()) simpleDb().erase(as_string(args[0]));
        simpleDbSave("lpcvm.db");
        return make_int(1);
    }
    if (name == "db_query") {
        auto arr = std::make_shared<ArrayValue>();
        for (auto& kv : simpleDb()) arr->items.push_back(make_string(kv.first + "=" + kv.second));
        return arr;
    }
    if (name == "sql_execute" || name == "sql_insert" || name == "sql_close_db") return make_int(0);
    // ---- telnet efuns ----
    if (name == "input_string") {
        // input_string() - block until the current thread's telnet client
        // sends a line.  Returns the line (without CR/LF).  When the remote
        // side disconnects, the original driver kills the pended process;
        // we terminate the daemon thread the same way (TelnetClosedError).
        int cno = threadTelnetClient();
        if (cno < 0 || !findTelnetClient(cno)) return Value(UndefinedTag{});
        std::string line = telnetReadLine(cno);
        TelnetClient* cl = findTelnetClient(cno);
        if (cl && cl->eof) {
            closeTelnetClient(cno);
            setThreadTelnetClient(-1);
            throw TelnetClosedError("telnet client closed");
        }
        return make_string(line);
    }
    if (name == "close_telnet") {
        if (!args.empty()) closeTelnetClient(static_cast<int>(as_int(args[0])));
        return make_int(1);
    }
    if (name == "telnet_config") {
        // telnet_config(fd, mapping) - set per-client options.
        if (args.size() >= 2 && args[0].index() == 2 && args[1].index() == 6) {
            int cno = static_cast<int>(std::get<int64_t>(args[0]));
            auto mp = std::get<std::shared_ptr<MappingValue>>(args[1]);
            TelnetClient* cl = findTelnetClient(cno);
            if (cl) {
                auto getInt = [&](const char* k, int def) {
                    const Value* hit = mp->find(make_string(k));
                    if (hit) return static_cast<int>(as_int(*hit));
                    return def;
                };
                if (!mp->empty()) {
                    cl->nativeProcess = getInt("native_process", cl->nativeProcess ? 1 : 0) != 0;
                    cl->echo = getInt("echo", cl->echo ? 1 : 0) != 0;
                    cl->auth = getInt("auth", cl->auth ? 1 : 0) != 0;
                    cl->enabled = getInt("enable", cl->enabled ? 1 : 0) != 0;
                    const Value* idv = mp->find(make_string("id"));
                    if (idv) cl->id = as_string(*idv);
                }
            }
        }
        return make_int(1);
    }
    if (name == "get_telnet_settings") {
        // The game reads get_telnet_settings()["type"]: "L" enables the LPC
        // telnet_daemon login path, "G" enables game-style id logging.
        auto mp = std::make_shared<MappingValue>();
        mp->insert(make_string("type"), make_string("LG"));
        mp->insert(make_string("ip"), make_string("127.0.0.1"));
        return mp;
    }
    if (name == "get_telnet_info") {
        if (!args.empty()) {
            // get_telnet_info(fd) -> ({ native_process, echo, auth })
            auto arr = std::make_shared<ArrayValue>();
            int cno = static_cast<int>(as_int(args[0]));
            TelnetClient* cl = findTelnetClient(cno);
            arr->items.push_back(make_int(cl ? (cl->nativeProcess ? 1 : 0) : 0));
            arr->items.push_back(make_int(cl ? (cl->echo ? 1 : 0) : 0));
            arr->items.push_back(make_int(cl ? (cl->auth ? 1 : 0) : 0));
            return arr;
        }
        return make_int(0);
    }
    if (name == "get_all_client_no") {
        auto arr = std::make_shared<ArrayValue>();
        std::lock_guard<std::mutex> lk(telnetMutex());
        for (auto& kv : telnetClients()) arr->items.push_back(make_int(kv.first));
        return arr;
    }
    if (name == "send_to_client") {
        // send_to_client(client_no, msg_no, data) - write raw bytes.
        if (args.size() >= 3 && args[0].index() == 2) {
            int cno = static_cast<int>(std::get<int64_t>(args[0]));
            std::string data = as_string(args[2]);
            telnetSend(cno, data);
        }
        return make_int(1);
    }
    if (name == "set_client_attrib") {
        if (args.size() >= 2 && args[0].index() == 2) {
            int cno = static_cast<int>(std::get<int64_t>(args[0]));
            TelnetClient* cl = findTelnetClient(cno);
            if (cl && args[1].index() == 6) {
                auto mp = std::get<std::shared_ptr<MappingValue>>(args[1]);
                for (auto& kv : mp->items()) {
                    if (kv.first.index() == 4) {
                        std::string k = std::get<std::string>(kv.first);
                        if (k == "type" && kv.second.index() == 4)
                            cl->isInternal = std::get<std::string>(kv.second) == "internal";
                        if (k == "peek") cl->peek = static_cast<int>(as_int(kv.second));
                    }
                }
            }
        }
        return make_int(1);
    }
    if (name == "telnet_check_password" || name == "telnet_second_password") {
        // The builtin telnet shell checks the configured password; accept
        // "123456" (matching get_telnet_password's stub) so the LPC login can
        // proceed through the aaa.ini configured value as well.
        if (args.size() >= 2) {
            std::string pwd = as_string(args[1]);
            if (pwd == "123456") return make_int(1);
        }
        return make_int(0);
    }
    if (name == "make32" || name == "make32i") {
        // make32(n) -> 4-byte big-endian string; make32i(str[,off]) -> int.
        if (name == "make32") {
            int64_t v = args.empty() ? 0 : as_int(args[0]);
            std::string s(4, '\0');
            for (int i = 0; i < 4; ++i) s[static_cast<size_t>(i)] = static_cast<char>((v >> (24 - 8 * i)) & 0xFF);
            return make_string(s);
        }
        std::string s = args.empty() ? "" : as_string(args[0]);
        size_t off = args.size() >= 2 ? static_cast<size_t>(as_int(args[1])) : 0;
        int64_t v = 0;
        for (int i = 0; i < 4 && off + static_cast<size_t>(i) < s.size(); ++i)
            v = (v << 8) | static_cast<unsigned char>(s[off + static_cast<size_t>(i)]);
        return make_int(v);
    }
    if (name == "regexp") {
        // regexp(str, pattern[, max]) -> array of {start,end} per group.
        if (args.size() >= 2) {
            std::string str = as_string(args[0]);
            std::string pat = as_string(args[1]);
            if (std::getenv("LPCVM_TRACE_CASE"))
                std::cerr << "[regexp] '" << pat << "' on '" << str << "'\n";
            return mini_regex::regexpMatch(str, pat);
        }
        return make_array();
    }
    if (name == "strsrch") {
        // strsrch(str, sub[, reverse]) -> index or -1
        if (args.size() >= 2) {
            std::string str = as_string(args[0]);
            std::string sub = as_string(args[1]);
            size_t pos = str.find(sub);
            if (pos == std::string::npos) return make_int(-1);
            return make_int(static_cast<int64_t>(pos));
        }
        return make_int(-1);
    }
    if (name == "ctime") {
        // ctime([t]) - "Www Mmm dd hh:mm:ss yyyy\n"
        std::time_t t = args.empty() ? std::time(nullptr) : static_cast<std::time_t>(as_int(args[0]));
        std::tm tm{};
        localtime_s(&tm, &t);
        char buf[64];
        strftime(buf, sizeof(buf), "%a %b %d %H:%M:%S %Y\n", &tm);
        return make_string(buf);
    }
    if (name == "execute_data") {
        // execute_data(cmd[, enable_log]) - the driver's telnet shell command
        // executor: the command string is parsed as an LPC expression and
        // evaluated in the VM's shared context (so "help", "obj->func()",
        // "x + 1", etc. all work like the original driver's shell).
        if (args.empty()) return make_int(0);
        std::string cmd = as_string(args[0]);
        if (std::getenv("LPCVM_TRACE"))
            std::cerr << "[execute_data] '" << cmd << "'\n";
        std::string t = cmd;
        size_t b = t.find_first_not_of(" \t\r\n");
        if (b == std::string::npos) return make_int(0);
        t = t.substr(b);
        if (t.empty()) return make_int(0);
        try {
            lpc::Lexer lexer(t);
            std::vector<lpc::Token> toks = lexer.tokenize();
            lpc::Parser parser(toks);
            lpc::ExprPtr expr = parser.parseShellExpression();
            lpc::Program prog;
            lpc::Interpreter interp(prog);
            lpc::Environment env;
            Value result = interp.eval(expr, env);
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[execute_data] => " << to_string(result, false) << "\n";
            // Echo non-trivial results back to the telnet client.
            if (result.index() != 0 && result.index() != 1) {
                telnetSend(threadTelnetClient(),
                           telnetLineEnd(to_string(result, false) + "\r\n"));
            }
            return result;
        } catch (const std::exception& e) {
            telnetSend(threadTelnetClient(),
                       telnetLineEnd("Error: " + std::string(e.what()) + "\r\n"));
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[execute_data] error: " << e.what() << "\n";
            return make_int(0);
        }
    }
    if (name == "log_to_das") return make_int(1);
    if (name == "start_buildin_telnet_shell" || name == "buildin_telnet_daemon") {
        // Driver builtin telnet shell: login with the configured password,
        // then a command loop.  The LPC telnet_daemon path is preferred when
        // get_telnet_settings()["type"] contains "L"; this is the fallback.
        if (!args.empty()) {
            int cno = static_cast<int>(as_int(args[0]));
            setThreadTelnetClient(cno);
            telnetSend(cno, telnetLineEnd("Input password: "));
            std::string pwd = telnetReadLine(cno);
            if (pwd != "123456") {
                telnetSend(cno, telnetLineEnd("Bad password!\r\n"));
                closeTelnetClient(cno);
                setThreadTelnetClient(-1);
                return make_int(0);
            }
            telnetSend(cno, telnetLineEnd("Welcome!\r\n"));
            while (findTelnetClient(cno)) {
                telnetSend(cno, telnetLineEnd("\r\nShell> "));
                std::string line = telnetReadLine(cno);
                if (line.empty() || line == ".") {
                    telnetSend(cno, telnetLineEnd("logout\r\n"));
                    closeTelnetClient(cno);
                    break;
                }
                Value r = call_builtin(nullptr, "execute_data", {make_string(line), make_int(1)});
                (void)r;
            }
            setThreadTelnetClient(-1);
        }
        return make_int(1);
    }
    if (name == "get_telnet_password") {
        // Return the telnet login password configured for a server name.
        if (!args.empty()) {
            std::string server = as_string(args[0]);
            auto it = simpleDb().find("telnet_" + server);
            if (it != simpleDb().end()) return make_string(it->second);
        }
        return make_string("123456");
    }
    if (name == "set_timer") return make_int(1);
    if (name == "sleep") {
        // The driver's sleep() takes seconds and suspends the calling thread.
        int64_t secs = args.empty() ? 0 : as_int(args[0]);
        if (secs > 0) {
            std::chrono::seconds d(secs);
            if (secs > 60) d = std::chrono::seconds(60);   // keep startup responsive
            std::this_thread::sleep_for(d);
        }
        return make_int(1);
    }
    if (name == "wait_event" || name == "send_to_queue") {
        // Original vm_f_wait_event (0x80f097c) blocks the process until an
        // event fires or the timeout (arg 3, seconds; -1 = forever) expires.
        // Our event queue is a stub that never fires: emulate the blocking
        // wait so daemon service loops (which call wait_event in a tight
        // loop) do not spin at 100% CPU.  Positive timeouts sleep for real
        // (capped); 0/-1/absent poll at a low cadence.
        double secs = -1.0;
        if (args.size() >= 4) {
            const Value& t = args[3];
            if (t.index() == 2) secs = static_cast<double>(std::get<int64_t>(t));
            else if (t.index() == 3) secs = std::get<double>(t);
        }
        int64_t ms = 100;  // minimum poll interval
        if (secs > 0.0) {
            if (secs > 60.0) secs = 60.0;
            ms = static_cast<int64_t>(secs * 1000.0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        return make_int(0);
    }
    if (name == "receive_from_queue" || name == "receive_from_queue_ex") {
        // Queue is a stub; the daemon loop is:
        //   msg = receive_from_queue(...)
        //   if (msg == 0) goto loop;     // "if" jumps on non-zero result of
        //                                // (msg==0) comparison, i.e. loops when
        //                                // the call returned a non-zero value
        //   ... dispatch msg ...
        // Sleeping briefly (like the original blocking queue read) prevents a
        // 100% CPU service loop, and returning non-zero keeps the loop running
        // instead of falling into the "Bad message" branch.
        int timeout = args.size() >= 2 ? static_cast<int>(as_int(args[1])) : 0;
        if (timeout <= 0 || timeout > 200) timeout = 100;  // ms
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
        return make_int(1);
    }
    if (name == "get_thread_id") return make_int(-1);
    if (name == "time" || name == "time_desc")
        return make_int(static_cast<int64_t>(std::time(nullptr)));
    if (name == "tick") {
        // tick() - milliseconds since the epoch (the driver uses
        // gettimeofday()*1000 + usec/1000).
        auto now = std::chrono::system_clock::now().time_since_epoch();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        return make_int(static_cast<int64_t>(ms));
    }
    if (name == "get_system_tick" || name == "get_server_tick" || name == "get_bad_tick_times") {
        auto now = std::chrono::system_clock::now().time_since_epoch();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        return make_int(static_cast<int64_t>(ms));
    }
    if (name == "local_time") {
        // local_time([t]) -> ({hour,min,sec,wday,ch_wday,mday,yday,mon,year})
        std::time_t t = args.empty() ? std::time(nullptr) : static_cast<std::time_t>(as_int(args[0]));
        std::tm tm{};
        localtime_s(&tm, &t);
        auto mp = std::make_shared<MappingValue>();
        auto add = [&](const char* k, int64_t v) { mp->insert(make_string(k), make_int(v)); };
        add("hour", tm.tm_hour);
        add("min", tm.tm_min);
        add("sec", tm.tm_sec);
        add("wday", tm.tm_wday);
        add("ch_wday", tm.tm_wday == 0 ? 7 : tm.tm_wday);
        add("mday", tm.tm_mday);
        add("yday", tm.tm_yday);
        add("mon", tm.tm_mon + 1);
        add("year", tm.tm_year + 1900);
        return mp;
    }
    if (name == "mktime") {
        // mktime(mapping) - build an epoch value from a {year,mon,day,hour,min,sec} map.
        if (!args.empty() && args[0].index() == 6) {
            auto mp = std::get<std::shared_ptr<MappingValue>>(args[0]);
            auto get = [&](const char* k, int def) -> int {
                const Value* hit = mp->find(make_string(k));
                if (hit) return static_cast<int>(as_int(*hit));
                return def;
            };
            std::tm t{};
            t.tm_year = get("year", 1970) - 1900;
            t.tm_mon = get("mon", 1) - 1;
            t.tm_mday = get("mday", get("day", 1));
            t.tm_hour = get("hour", 0);
            t.tm_min = get("min", 0);
            t.tm_sec = get("sec", 0);
            std::time_t result = std::mktime(&t);
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[mktime] year=" << t.tm_year + 1900 << " mon=" << t.tm_mon + 1
                          << " mday=" << t.tm_mday << " hour=" << t.tm_hour
                          << " -> " << static_cast<int64_t>(result) << "\n";
            return make_int(static_cast<int64_t>(result));
        }
        return make_int(0);
    }
    if (name == "create_queue" || name == "create_event") {
        // vm_f_create_queue -> vm_createPendObject (0x80a9cc4): queues and
        // events share ONE global object-id table (vmObject[]), ids start at
        // 1 (the first free slot), which is what makes threadd's service
        // numbers match the original run (first queue id 9 in aaa.log).
        static std::atomic<int64_t> pendSeq{0};
        int64_t qid = pendSeq.fetch_add(1, std::memory_order_relaxed) + 1;
        if (std::getenv("LPCVM_TRACE"))
            std::cerr << "[create_" << (name == "create_queue" ? "queue" : "event")
                      << "] id=" << qid << "\n";
        return make_int(qid);
    }
    if (name == "delete_queue" || name == "delete_event") return make_int(1);
    if (name == "raise_event") return make_int(1);
    if (name == "restore_value") {
        // vm_f_restore_value (0x80dae84) -> vm_restoreMixedString: parse one
        // value in the vm_saveValueAscii format.  On failure the original
        // raises (caught by xdbc_wait_command's try/catch); returning the
        // input lets the caller's is_mapping check fail the same way.
        if (!args.empty() && args[0].index() == 4) {
            Value out;
            if (restoreValueAscii(std::get<std::string>(args[0]), out)) return out;
        }
        return args.empty() ? make_int(0) : args[0];
    }
    if (name == "save_buffer") {
        // vm_f_save_buffer (0x80dafe4) -> vm_saveValueToBuffer; we keep the
        // ASCII form (identical grammar to save_string).
        if (args.empty()) return make_string("0");
        return make_string(saveValueAscii(args[0]));
    }
    if (name == "rdl") {
        // vm_f_rdl (0x80e0314): AES-128 ECB, action 16 = encrypt / 17 = decrypt.
        if (!args.empty()) {
            std::string data = as_string(args[0]);
            int action = args.size() >= 2 ? static_cast<int>(as_int(args[1])) : 16;
            std::string key = args.size() >= 3 ? as_string(args[2]) : std::string();
            return make_string(rdlAes(data, action, key));
        }
        return make_int(0);
    }
    if (name == "bzip_compress" || name == "bzip_decompress") {
        // The bzip layer only ever runs between our own efuns and the
        // in-process XDBC emulator (xdbc.cpp), so identity keeps the frame
        // format self-consistent without a bzip2 implementation.
        if (!args.empty() && args[0].index() == 4) return args[0];
        return make_string("");
    }
    if (name == "socket") {
        // vm_f_socket (0x80e3f4c): allocate a socket fd (1-based).
        return make_int(xdbcSocket());
    }
    if (name == "connect") {
        // vm_f_connect (0x80e83b4): connect(fd, host, port) -> 0 / -1.
        if (args.size() >= 3) {
            int fd = static_cast<int>(as_int(args[0]));
            std::string host = as_string(args[1]);
            int port = static_cast<int>(as_int(args[2]));
            return make_int(xdbcConnect(fd, host, port));
        }
        return make_int(-1);
    }
    if (name == "send") {
        // vm_f_send (0x80e7034): send(fd, data) -> bytes sent.
        if (args.size() >= 2) {
            int fd = static_cast<int>(as_int(args[0]));
            return make_int(xdbcSend(fd, as_string(args[1])));
        }
        return make_int(-1);
    }
    if (name == "close_socket") {
        if (!args.empty()) xdbcCloseSocket(static_cast<int>(as_int(args[0])));
        return make_int(0);
    }
    if (name == "error") return make_int(0);
    // ---- LPC value helpers the game uses (verified against the original
    // driver's handlers in IDA) ----
    if (name == "random") {
        // vm_f_random (0x80d9d9c): builds a 31-bit random from three rand()
        // calls, then:
        //   random(n)      -> n <= 0 ? 0 : rand31 % n
        //   random(a, b)   -> a + rand31 % (b - a + 1) (args swapped if a > b)
        //   random()       -> raw 31-bit value
        static std::mt19937 rng(static_cast<uint32_t>(std::time(nullptr)) ^
                                std::random_device{}());
        auto rand31 = [&]() -> int64_t {
            // Original builds a 32-bit value from three 15-bit rand() draws:
            //   v3 = ((r0 & 0x7FFF) >> 13) | (4 * (r1 & 0x7FFF)) | (r2 << 17)
            uint32_t r0 = static_cast<uint32_t>(rng()) & 0x7FFFu;
            uint32_t r1 = static_cast<uint32_t>(rng()) & 0x7FFFu;
            uint32_t r2 = static_cast<uint32_t>(rng()) & 0x7FFFu;
            uint32_t v = ((r0 & 0x7FFFu) >> 13) | (4u * (r1 & 0x7FFFu)) | (r2 << 17);
            return static_cast<int64_t>(v & 0x7FFFFFFFu);
        };
        if (args.size() >= 2) {
            int64_t a = as_int(args[0]), b = as_int(args[1]);
            if (a > b) std::swap(a, b);
            int64_t range = b - a + 1;
            if (range <= 0) return make_int(a);
            return make_int(a + rand31() % range);
        }
        if (args.size() == 1) {
            int64_t n = as_int(args[0]);
            if (n <= 0) return make_int(0);
            return make_int(rand31() % n);
        }
        return make_int(rand31());
    }
    if (name == "allocate_buffer") {
        // vm_f_allocate_buffer (0x80d444c): buffer of n zero bytes.
        int64_t n = args.empty() ? 0 : as_int(args[0]);
        if (n < 0) n = 0;
        return make_string(std::string(static_cast<size_t>(n), '\0'));
    }
    if (name == "is_buffer") {
        // Buffers are represented as byte strings; distinguish them from text
        // by checking for embedded NUL bytes.
        if (!args.empty() && args[0].index() == 4) {
            const std::string& s = std::get<std::string>(args[0]);
            return make_int(s.find('\0') != std::string::npos ? 1 : 0);
        }
        return make_int(0);
    }
    if (name == "save_string") {
        // vm_f_save_string (0x80db0d0) -> vm_saveValueToString: the ASCII
        // save form (vm_saveValueAscii).
        if (args.empty()) return make_string("0");
        return make_string(saveValueAscii(args[0]));
    }
    if (name == "iconv") {
        // vm_f_iconv (0x817af00): converts between encodings.  The game calls
        // iconv(str, "utf-8", "gb2312") when generating captcha pictures.  A
        // full GB2312<->UTF-8 conversion is out of scope here; pass the input
        // through so the rest of the pipeline (verigen_*) still runs.
        if (args.empty()) return make_string("");
        return args[0];
    }
    // Unknown efuns silently return 0 to keep restored scripts running.
    return make_int(0);
}

} // namespace lpc
