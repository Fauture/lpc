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
};

// Start the game client service on `port` (start_service_on).  Returns 0 on
// failure, 1 on success.  Spawns a listener thread; each accepted socket gets
// its own GameClient slot and packet state machine.
int startGameService(int port);

// Feed raw socket bytes into the client state machine and dispatch commands.
void gameServiceFeed(int clientNo, const char* data, size_t len);

// Disconnect a game client (socket closed).
void gameServiceDisconnect(int clientNo);

// Dispatch one parsed packet to the command table (vm_clientSocketCallback's
// cmdIndex lookup + attrib check + cmdFunc call).
void gameServiceDispatchPacket(GameClient& cl);

} // namespace lpc
