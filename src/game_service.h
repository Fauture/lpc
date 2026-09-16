#pragma once
// Game client service (port 8101) - faithful port of the original driver's
// vm_efun_magic.c client-socket protocol:
//   - Vm_Packet_Header: MZ sync code, big-endian len/cmd, checksum
//   - clientInfo[2104] connection table with per-client state machine
//   - cmdList[924] / serverCmdList[3] command dispatch via cmdIndex
//   - cmdFunc -> vm_apply -> LPC command handlers
//   - attrib permission check (CLIENT_USER=1, INTERNAL_COMM=2, FROM_SERVER=4)

#include <cstdint>
#include <string>
#include <vector>

#include "value.h"

namespace lpc {

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

// bind_to_client(fd, [type]) (vm_f_bind_to_client 0x804ee68): attach a
// connected socket (from socket()/connect()) to a game client slot and start
// its receive/dispatch loop.  Returns the client number, or -1 on failure.
int gameServiceBindClient(int fd, uint32_t clientType);

} // namespace lpc
