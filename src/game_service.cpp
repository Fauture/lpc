// Game client service - faithful port of vm_efun_magic.c / vm_clientSocketCallback
// from the original magic_Linux32x driver (port 8101).
//
// Wire protocol (all multi-byte fields BIG-ENDIAN on the wire):
//   header: 'M' 'Z'  checkSum(be:u16)  tickCount(be:u32)  msgLen(be:u16)  cmdNo(be:u16)
//   body:   msgLen - 12 bytes of command-specific data
//
// The receiver keeps a per-client state machine (LOST_SYNC -> SYNCED_1 ->
// WAIT_HEADER -> WAIT_BODY).  Once a full packet is assembled the command is
// looked up in cmdIndex[] and, after the attrib permission check, dispatched
// to the matching C command function, which parses the body and applies the
// corresponding LPC command on the master program (vm_apply).

#include "game_service.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <atomic>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "builtins.h"
#include "original_vm.h"
#include "pkt_protocol.h"
#include "value.h"
#include "value.h"

namespace lpc {

namespace {

constexpr uint16_t kSyncMagic = 0x5A4D;  // 'M''Z' read as little-endian word

void ensureWsaGame() {
    static bool done = [] {
        WSADATA wsa{};
        WSAStartup(MAKEWORD(2, 2), &wsa);
        return true;
    }();
    (void)done;
}

// attrib bits (vm_magicInit predefines)
constexpr uint32_t ATTRIB_CLIENT_USER = 1;     // CLIENT_USER
constexpr uint32_t ATTRIB_INTERNAL_COMM = 2;   // INTERNAL_COMM
constexpr uint32_t ATTRIB_FROM_SERVER = 4;     // FROM_SERVER

std::mutex& gameMutex() {
    static std::mutex m;
    return m;
}
std::unordered_map<int, GameClient>& gameClients() {
    static std::unordered_map<int, GameClient> clients;
    return clients;
}
std::atomic<int>& nextGameClientNo() {
    static std::atomic<int> n{1};
    return n;
}

uint16_t rd16be(const uint8_t* p) {
    return static_cast<uint16_t>((p[0] << 8) | p[1]);
}
uint32_t rd32be(const uint8_t* p) {
    return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
}
void wr16be(uint8_t* p, uint16_t v) { p[0] = static_cast<uint8_t>(v >> 8); p[1] = static_cast<uint8_t>(v & 0xFF); }
void wr32be(uint8_t* p, uint32_t v) {
    p[0] = static_cast<uint8_t>(v >> 24); p[1] = static_cast<uint8_t>((v >> 16) & 0xFF);
    p[2] = static_cast<uint8_t>((v >> 8) & 0xFF); p[3] = static_cast<uint8_t>(v & 0xFF);
}

// vm_calculateCheckSum (0x804cfb4): v += byte * ((v & 0x7F) + 1)
uint16_t calculateCheckSum(const uint8_t* data, uint32_t len, uint16_t init = 0) {
    uint16_t v = init;
    for (uint32_t i = 0; i < len; ++i)
        v = static_cast<uint16_t>(v + data[i] * ((v & 0x7F) + 1));
    return v;
}

// Dispatch entry lookup table built from the original cmdList.  Only the
// commands whose C handler is implemented below are listed; unknown commands
// are logged and dropped (matching the original "*** Bad command" path).
struct CmdEntry {
    uint16_t cmdNo;
    uint16_t type;
    const char* lpcFun;      // LPC function applied with (clientNo, ...args)
    int argSpec;             // argument encoding, see handlers
};
const CmdEntry kCmdList[] = {
    // serverCmdList entries (Vm_Command_List_T, .data:083C8E40):
    //   <5351h, 4, vm_msgLAuth>      - MSG_L_AUTH
    //   <0F9D3h, 4, vm_msgLoginDone>
    { 0x5351, ATTRIB_CLIENT_USER | ATTRIB_INTERNAL_COMM | ATTRIB_FROM_SERVER, "msg_l_auth", 0 }, // vm_msgLAuth
    { 0xF9D3, ATTRIB_CLIENT_USER | ATTRIB_INTERNAL_COMM | ATTRIB_FROM_SERVER, "msg_login_done", 0 }, // vm_msgLoginDone
};
constexpr size_t kCmdCount = sizeof(kCmdList) / sizeof(kCmdList[0]);

const CmdEntry* findCmd(uint16_t cmdNo) {
    for (size_t i = 0; i < kCmdCount; ++i)
        if (kCmdList[i].cmdNo == cmdNo) return &kCmdList[i];
    return nullptr;
}

// Apply an LPC function on the master program with a variable argument list.
Value applyLpc(const char* fun, const std::vector<Value>& args) {
    // The master VM is registered under "<main>" with the pak's search root
    // (e.g. "linux/aaa - bak/<main>"); find it regardless of the exact root.
    // callMasterProgramFunction is exposed by original_vm.h; pass the root of
    // whichever <main> entry exists.
    return callMasterProgramFunction(fun, args, lpc::masterSearchRoot());
}

// ---- command handlers -----------------------------------------------------

// vm_msgLAuth (0x804ca10): body = result(be:u32) authKey(be:u32) nameLen(be:u16) name
// -> vm_apply("msg_l_auth", clientNo, result, authKey, message)
//   (original pushes: funName, nArgs=4, 0x58,clientNo, 0x58,result, 0x58,authKey, 0x165,message)
void handleMsgLAuth(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 4) return;
    int32_t result = static_cast<int32_t>(rd32be(body));
    int32_t authKey = 0;
    std::string name;
    if (len >= 8) authKey = static_cast<int32_t>(rd32be(body + 4));
    if (len >= 10) {
        uint16_t nlen = rd16be(body + 8);
        if (nlen <= len - 10) name.assign(reinterpret_cast<const char*>(body + 10), nlen);
    }
    if (std::getenv("LPCVM_TRACE"))
        std::cerr << "[game] msg_l_auth client=" << cl.clientNo << " result=" << result
                  << " authKey=" << authKey << " name='" << name << "'\n";
    applyLpc("msg_l_auth", {make_int(cl.clientNo), make_int(result), make_int(authKey), make_string(name)});
}

// Dispatcher: run the C handler for a parsed packet.
// Original logic (vm_clientSocketCallback LABEL_42):
//   if (vm_pktAnalyse(cmdNo, body, len) != 1) {
//     ... cmdIndex lookup + cmdFunc fallback ...
//   }
void dispatchCommand(GameClient& cl, uint16_t cmdNo, const uint8_t* body, size_t len) {
    // Primary path: packet protocol analyser (comm_desc.txt interfaces).
    int pa = pktAnalyse(cl.clientNo, cl.attrib, cmdNo, body, len);
    if (pa == 1) return;   // handled by the protocol interface
    if (pa < 0) {          // parse error
        ++cl.recvBadPkts;
        return;
    }
    // Fallback: cmdList/cmdIndex dispatch.
    const CmdEntry* e = findCmd(cmdNo);
    if (!e) {
        if (std::getenv("LPCVM_TRACE"))
            std::cerr << "[game] bad command 0x" << std::hex << cmdNo << std::dec
                      << " from client " << cl.clientNo << " (len=" << len << ")\n";
        ++cl.recvBadPkts;
        return;
    }
    // attrib permission check: (attrib & type & 7) != 0
    if ((cl.attrib & e->type & 7) == 0) {
        if (std::getenv("LPCVM_TRACE"))
            std::cerr << "[game] denied cmd 0x" << std::hex << cmdNo << std::dec
                      << " attrib=" << cl.attrib << "\n";
        ++cl.recvBadPkts;
        return;
    }
    switch (cmdNo) {
    case 0x5351: handleMsgLAuth(cl, body, len); break;
    default:
        if (std::getenv("LPCVM_TRACE"))
            std::cerr << "[game] cmd 0x" << std::hex << cmdNo << std::dec
                      << " handler not implemented\n";
        break;
    }
}

// Feed bytes into the client's packet state machine (vm_clientSocketCallback).
// Status values mirror the original: 1=LOST_SYNC 2=SYNCED_1 3=WAIT_HEADER
// 4=WAIT_BODY.  The receiver reads the 12-byte header (Code/checkSum/tickCount/
// msgLen/cmdNo), then msgLen-12 body bytes, then dispatches.
void feedClient(GameClient& cl, const char* data, size_t n) {
    if (std::getenv("LPCVM_TRACE"))
        std::cerr << "[game] feed client=" << cl.clientNo << " n=" << n
                  << " status=" << static_cast<int>(cl.status) << "\n";
    size_t off = 0;
    while (off < n) {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(data) + off;
        size_t avail = n - off;
        if (cl.status == 1 || cl.status == 2) {
            // LOST_SYNC / SYNCED_1: scan for the MZ sync code.
            size_t i = 0;
            bool found = false;
            while (i < avail) {
                if (p[i] == 'M') {
                    if (i + 1 < avail) {
                        if (p[i + 1] == 'Z') { found = true; break; }
                        // 'M' but not 'MZ': advance one (the 'M' might be part
                        // of a later code after the next byte).
                        i += 1;
                        continue;
                    }
                    // 'M' is the last byte; keep it for the next chunk.
                    cl.hdrBuf.assign(1, 'M');
                    cl.status = 2;
                    off += avail;
                    break;
                }
                i += 1;
            }
            if (!found) {
                if (cl.status == 2 && cl.hdrBuf.size() == 1 && avail > 0) {
                    // previous chunk ended with 'M'; check this chunk's first byte
                    if (p[0] == 'Z') { found = true; i = 0; cl.hdrBuf.clear(); }
                    else { cl.hdrBuf.clear(); cl.status = 1; }
                    if (!found) { off += avail; break; }
                } else {
                    off += avail;
                    break;
                }
            }
            if (found) {
                off += i;  // consume up to (not including) 'M'
                cl.status = 3;
                cl.hdrBuf.clear();
                continue;
            }
            break;
        }
        if (cl.status == 3) {
            // WAIT_HEADER: accumulate 12 bytes.
            size_t need = 12 - cl.hdrBuf.size();
            size_t take = avail < need ? avail : need;
            cl.hdrBuf.append(data + off, take);
            off += take;
            if (cl.hdrBuf.size() < 12) break;
            const uint8_t* h = reinterpret_cast<const uint8_t*>(cl.hdrBuf.data());
            if (h[0] != 'M' || h[1] != 'Z') {
                cl.status = 1;
                cl.hdrBuf.clear();
                ++cl.recvBadPkts;
                continue;
            }
            cl.pendingLen = rd16be(h + 8);
            cl.pendingCmd = rd16be(h + 10);
            cl.bodyGot = 0;
            cl.body.clear();
            if (cl.pendingLen <= 12) {
                // No body: dispatch the command now.
                dispatchCommand(cl, cl.pendingCmd, nullptr, 0);
                cl.status = 3;
                cl.hdrBuf.clear();
                continue;
            }
            cl.status = 4;
            cl.body.reserve(cl.pendingLen - 12);
            continue;
        }
        if (cl.status == 4) {
            // WAIT_BODY
            size_t need = cl.pendingLen - 12 - cl.bodyGot;
            size_t take = avail < need ? avail : need;
            cl.body.insert(cl.body.end(), p, p + take);
            cl.bodyGot += take;
            off += take;
            if (cl.bodyGot >= cl.pendingLen - 12) {
                dispatchCommand(cl, cl.pendingCmd, cl.body.data(), cl.body.size());
                cl.status = 3;
                cl.hdrBuf.clear();
            }
            break;
        }
        break;
    }
}

} // namespace

int startGameService(int port) {
    ensureWsaGame();
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
    if (listen(s, 2104) == SOCKET_ERROR) {
        closesocket(s);
        return 0;
    }
    std::thread([s, port]() {
        std::cout << "Service started on port " << port << "\n";
        while (true) {
            SOCKET c = accept(s, nullptr, nullptr);
            if (c == INVALID_SOCKET) {
                if (WSAGetLastError() == WSAEINTR) continue;
                break;
            }
            std::thread([c]() {
                int clientNo = nextGameClientNo().fetch_add(1);
                if (std::getenv("LPCVM_TRACE"))
                    std::cerr << "[game] accepted client no=" << clientNo << "\n";
                GameClient cl;
                cl.fd = static_cast<int>(c);
                cl.clientNo = clientNo;
                cl.attrib = ATTRIB_CLIENT_USER;  // CLIENT_USER initially
                cl.status = 1;
                {
                    std::lock_guard<std::mutex> lk(gameMutex());
                    gameClients()[clientNo] = std::move(cl);
                }
                // vm_gameListenSocketCallback: after binding the client slot,
                // notify the LPC master: vm_apply("cmd_new_connection", clientNo)
                // -> cmd_new_connection -> threadd dispatch -> post_new_connection
                // (creates/registers the client agent).
                try {
                    callMasterProgramFunction("cmd_new_connection", {make_int(clientNo)},
                                              lpc::masterSearchRoot());
                } catch (...) {
                }
                char buf[4096];
                for (;;) {
                    int n = recv(c, buf, sizeof(buf), 0);
                    if (n <= 0) break;
                    GameClient* cur;
                    {
                        std::lock_guard<std::mutex> lk(gameMutex());
                        auto it = gameClients().find(clientNo);
                        if (it == gameClients().end()) break;
                        cur = &it->second;
                    }
                    feedClient(*cur, buf, static_cast<size_t>(n));
                }
                closesocket(c);
                {
                    std::lock_guard<std::mutex> lk(gameMutex());
                    gameClients().erase(clientNo);
                }
            }).detach();
        }
        closesocket(s);
    }).detach();
    return 1;
}

void gameServiceFeed(int clientNo, const char* data, size_t len) {
    std::lock_guard<std::mutex> lk(gameMutex());
    auto it = gameClients().find(clientNo);
    if (it == gameClients().end()) return;
    feedClient(it->second, data, len);
}

void gameServiceDisconnect(int clientNo) {
    std::lock_guard<std::mutex> lk(gameMutex());
    auto it = gameClients().find(clientNo);
    if (it == gameClients().end()) return;
    if (it->second.fd >= 0) closesocket(static_cast<SOCKET>(it->second.fd));
    gameClients().erase(it);
}

Value gameGetClientInfo(int clientNo) {
    // vm_f_get_client_info (0x804d478): mapping with id/client_no/alive/peek/
    // time_stamp/.../local_address/local_port/remote_address/remote_port/
    // remain_send_bytes; empty mapping when the client slot does not exist.
    auto mp = std::make_shared<MappingValue>();
    GameClient cl;
    bool found = false;
    {
        std::lock_guard<std::mutex> lk(gameMutex());
        auto it = gameClients().find(clientNo);
        if (it != gameClients().end()) { cl = it->second; found = true; }
    }
    if (!found) return mp;
    auto add = [&](const char* k, const Value& v) { mp->insert(make_string(k), v); };
    add("id", make_int(cl.id));
    add("client_no", make_int(cl.clientNo));
    add("alive", make_int(cl.fd > 0 ? 1 : 0));
    add("peek", make_int(cl.peekMsgNum));
    add("time_stamp", make_int(0));
    add("time_receive_echo", make_int(0));
    add("total_receive", make_int(static_cast<int64_t>(cl.recvPkts)));
    add("failed_count", make_int(static_cast<int64_t>(cl.recvBadPkts)));
    add("debug_enabled", make_int(0));
    add("mobile_client", make_int(cl.mobileClient ? 1 : 0));
    if (cl.fd > 0) {
        sockaddr_in sin{};
        int slen = static_cast<int>(sizeof(sin));
        if (getpeername(static_cast<SOCKET>(cl.fd), reinterpret_cast<sockaddr*>(&sin), &slen) == 0) {
            add("remote_address", make_string(inet_ntoa(sin.sin_addr)));
            add("remote_port", make_int(ntohs(sin.sin_port)));
        }
    }
    return mp;
}

void gameServiceDispatchPacket(GameClient& cl) {
    (void)cl;
}

} // namespace lpc
