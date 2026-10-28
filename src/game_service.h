#pragma once
// Game client service (port 8101) - faithful port of the original driver's
// vm_efun_magic.c client-socket protocol:
//   - Vm_Packet_Header: MZ sync code, big-endian len/cmd, checksum
//   - clientInfo[2104] connection table with per-client state machine
//   - cmdList[924] / serverCmdList[3] command dispatch via cmdIndex
//   - cmdFunc -> vm_apply -> LPC command handlers
//   - attrib permission check (CLIENT_USER=1, INTERNAL_COMM=2, FROM_SERVER=4)

#include <cstdint>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "value.h"

namespace lpc {

// The driver's main loop owns the listening sockets: the original scans
// them with the connected ones and accepts on the driver thread.  `cb`
// runs on the scheduler thread, receives the accepted socket and must not
// call into the VM; returning false drops the listener.
void registerServiceSocket(int fd,
                           std::function<bool(unsigned int fd)> cb);

// Wire packet header (12 bytes, all multi-byte fields big-endian on the wire).
struct GamePacketHeader {
    uint8_t code[2];       // 'M','Z' (0x4D 0x5A) sync magic
    uint16_t checkSum;     // checksum (big-endian on wire)
    uint32_t tickCount;    // client timestamp (big-endian on wire)
    uint16_t msgLen;       // total message length incl. 12-byte header (BE)
    uint16_t cmdNo;        // command number (big-endian on wire)
};

// Vm_Command_List_T (8 bytes): one dispatch entry.
struct GameCommandEntry {
    uint16_t cmdNo;
    uint16_t type;
    uint32_t cmdFuncAddr;  // original driver's function address (for ref)
};

// One client connection (mirrors Vm_Client_Info, 128 bytes).
struct GameClient {
    int fd = -1;
    int id = 0;
    int clientNo = 0;
    uint32_t attrib = 0;
    uint8_t status = 1;        // 1=LOST_SYNC 2=SYNCED_1 3=WAIT_HEADER 4=WAIT_BODY
    uint8_t mobileClient = 0;
    int peekMsgNum = -1;
    uint32_t maxBodySize = 0;
    // receive state
    std::string hdrBuf;        // partial header bytes
    std::vector<uint8_t> body; // packet body
    size_t bodyGot = 0;
    uint16_t pendingLen = 0;   // full msgLen of the packet being read
    uint16_t pendingCmd = 0;
    // stats
    uint64_t recvPkts = 0, recvBytes = 0, recvBadPkts = 0;
    std::string account;       // authenticated account name (if any)
    // Vm_Client_Info timing fields (vm_bindToClient sets timeStamp and
    // timeRecvEcho = time(0); vm_cmdEcho refreshes timeRecvEcho on each
    // valid echo and tickServer with the reply tick).
    time_t timeStamp = 0;      // bind time -> "time_stamp" in get_client_info
    time_t timeRecvEcho = 0;   // last echo time -> "time_receive_echo"
    uint32_t tickServer = 0;   // last tick we sent in an echo reply
    uint32_t tickLastRecvLastEcho = 0;  // client tick of the last echo (bad-echo check)
    time_t timeLastReportHeartBeat = 0; // 180s heartbeat-report throttle
    // vm_checkClientAccelerate (0x804ef50) accounting.  The original driver
    // runs this on every echo AND on every move packet: it counts the packet,
    // syncs the client tick against the driver tick, flags an abnormal rate
    // (v9=1 client tick went backwards, 2 client running FAST, 3 client running
    // SLOW) and - after a THIRD failure - calls cmd_close_client, i.e. this is
    // the only place in the driver that force-closes a live game client.
    uint64_t tickLocalStartSync = 0;      // driver tick at the first packet
    uint32_t tickClientStartSync = 0;     // client tick at the first packet
    uint32_t tickClientSendLastPacket = 0;  // client tick of the last accepted packet
    int totalReceive = 0;                 // packets the accelerate check has seen
    int failedCount = 0;                  // accumulated abnormalities (3 => close)
    // vm_checkClientMoveTo (0x804f0a8): the per-step move rate gate.  A step
    // costs >= 110 ms, and the same field is what vm_cmdAccelerateMoveTo
    // compares against (>= 87 ms per step).  The original updates it on every
    // accepted move, so a missing update also breaks the accelerate path.
    uint32_t tickLastRecvLastMoveTo = 0;  // client tick of the last accepted move
};

// Start the game client service on `port` (start_service_on).  Returns 0 on
// failure, 1 on success.  Spawns a listener thread; each accepted socket gets
// its own GameClient slot and packet state machine.
int startGameService(int port);

// Feed raw socket bytes into the client state machine and dispatch commands.
void gameServiceFeed(int clientNo, const char* data, size_t len);

// Disconnect a game client (socket closed).
void gameServiceDisconnect(int clientNo);

// set_client_attrib(client_no, mapping) support (vm_f_set_client_attrib
// 0x804d0ac): OR attribOrMask onto the game client's attrib and update its
// maxBodySize/peekMsgNum.  do_auth_new_connection sets "internal" here so
// cmd_cfg_* (INTERNAL_COMM) packets are no longer denied.
void gameServiceSetAttrib(int clientNo, uint32_t attribOrMask, int maxBodySize, int peekMsgNum);

// Dispatch one parsed packet to the command table (vm_clientSocketCallback's
// cmdIndex lookup + attrib check + cmdFunc call).
void gameServiceDispatchPacket(GameClient& cl);

// get_client_info(client_no) support (vm_f_get_client_info 0x804d478):
// returns a mapping describing the client slot, or an empty mapping when the
// client does not exist.  When a socket is attached, local/remote address and
// port fields are included.
Value gameGetClientInfo(int clientNo);

// get_all_client_no() support (vm_f_get_all_client_no 0x804d3f8): all live
// game client numbers (the driver's unified clientInfo[] table).
std::vector<int> gameGetAllClientNo();

// Send raw wire bytes to a game client's socket (vm_sendFullPacket path).
// The caller has already built the complete frame.  Returns false when the
// client slot is unknown.
bool gameServiceSend(int clientNo, const std::string& data);

// ---- combat anti-cheat answer ("c_end_animate", 0x4104) -------------------
//
// combatd.c::wait_commands stores, per player,
//     answer = ((question * 7) ^ 140017600) & 0x7FFFFFFF
// and sends `question` to the client in the 0x5203 prompt.  The real client
// keeps that number in its own LPC variable `_answer`
// (GFightMgr::OnMsg_C_WaitCommand -> set_answer_by_question(question):
// asktao_dump.exe 0x5bfe4a..0x5bfe85) and echoes it in 0x4104.  When that
// variable is not ready within one second the client sends a stale value
// instead (sub_572470 waits 0x3E8 ms, then sends *(_DWORD*)(sym+8)), so
// c_end_animate.c rejects the animate-done and the round only advances on the
// 25 s wait_command_time timer.
//
// noteCombatQuestion remembers the question of the last prompt sent to a
// client; substituteCombatAnswer maps a client echo that does not match it to
// the formula value (exactly what the original client computes).
// LPCVM_COMBAT_AC=0 disables the substitution.
void noteCombatQuestion(int clientNo, uint32_t question);
uint32_t combatAnswerFor(uint32_t question);
bool substituteCombatAnswer(int clientNo, uint32_t clientAnswer, uint32_t& out);

// LPCVM_FILEDDBG support: register an outbound filed REQUEST so the
// outstanding-request watchdog can name the request that never got its reply
// (see the watchdog block in game_service.cpp).  No-op unless the runtime probe
// _probe_on\LPCVM_FILEDDBG is present.  NOTE: the length parameter is
// `unsigned int`, not size_t - this target is Win32, where the two are DIFFERENT
// symbols (size_t is 64-bit), so the definition must match exactly.
void filedWatchdogMarkSent(uint16_t cmdNo, const uint8_t* body, unsigned int len);

// bind_to_client(fd, [type]) (vm_f_bind_to_client 0x804ee68): attach a
// connected socket (from socket()/connect()) to a game client slot and start
// its receive/dispatch loop.  Returns the client number, or -1 on failure.
int gameServiceBindClient(int fd, uint32_t clientType);

} // namespace lpc
