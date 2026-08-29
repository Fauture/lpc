#pragma once
#include <string>
#include <vector>

#include "value.h"

namespace lpc {

// ---------------------------------------------------------------------------
// XDBC support layer (recovered from the original driver):
//   - rdl efun            -> AES-128 ECB (rijndael-alg-fst), vm_f_rdl 0x80e0314
//   - save_buffer format  -> vm_saveValueAscii 0x8107ac4 (text form)
//   - the XDBC client in start_aaa.o talks to a server on <db host>:9527
//     with length-prefixed, optionally AES-encrypted frames carrying
//     serialized mappings.  This module provides that server in-process:
//     connections to port 9527 are handled by an emulator backed by
//     MysqlClient, so the ORIGINAL LPC xdbc code runs unmodified.
// ---------------------------------------------------------------------------

// vm_f_rdl: action 16 = encrypt, 17 = decrypt.  Key is used as raw bytes
// (zero padded to 16); data is processed in 16-byte blocks with zero padding,
// the result is rounded up to a multiple of 16.
std::string rdlAes(const std::string& data, int action, const std::string& key);

// vm_saveValueAscii: int -> decimal, float -> "%#g", string -> quoted with
// '"' '\\' '\r' escaped, buffer -> ":HEX:", array -> ({e,}), mapping ->
// ([k:v,]) in vm_mappingIndices order, undefined -> 'V', nil -> 'U'.
std::string saveValueAscii(const Value& v);

// Parse one value in the save format from the front of s (leading whitespace
// skipped); trailing bytes are ignored (the original restores exactly one
// value and reports the consumed offset).
bool restoreValueAscii(const std::string& s, Value& out);

// ---- virtual socket layer (vm_f_socket / vm_f_connect / vm_f_send /
//      vm_f_recv / vm_f_select / vm_f_close_socket) ----

// socket(type): allocate a socket, returns the fd (1-based, like the driver).
int xdbcSocket();

// connect(fd, host, port): port 9527 routes to the in-process XDBC emulator,
// anything else performs a real blocking TCP connect.  0 = ok, -1 = failure.
int xdbcConnect(int fd, const std::string& host, int port);

// send(fd, data): bytes accepted (emulator processes frames synchronously).
int xdbcSend(int fd, const std::string& data);

// recv(fd, len): up to len bytes (empty string when nothing is buffered).
std::string xdbcRecv(int fd, int len);

// select(fds): drops non-ready fds from the vector, returns the ready count.
int xdbcSelect(std::vector<int>& fds, int timeoutMs);

void xdbcCloseSocket(int fd);

// True when the fd exists (used by the recv/select efun wrappers).
bool xdbcSocketExists(int fd);

// Detach a connected socket fd from the virtual socket table, returning its
// real OS socket handle (0 when absent or not a real TCP socket).  Used by
// bind_to_client to hand the socket over to the game client layer.
uintptr_t xdbcDetachSocket(int fd);

// ---- listening socket layer (vm_f_bind / vm_f_listen / vm_f_accept /
//      vm_f_get_sock_info) ----
// bind(fd, 0, port): make fd a listener on the given port.  0 = ok, -1 = fail.
int xdbcBind(int fd, int port);
// listen(fd, backlog): start listening.  0 = ok, -1 = fail.
int xdbcListen(int fd, int backlog);
// accept(fd, &addr): accept one pending connection, return the NEW fd
// (registered in the socket table) or -1 when nothing is pending.
int xdbcAccept(int fd);
// get_sock_info(fd): mapping of socket metadata the driver exposes
// (local_port, remote_port, ...).  Empty when the fd is unknown.
Value xdbcGetSockInfo(int fd);
// True when the fd is a listening socket (used by select to report readiness).
bool xdbcIsListener(int fd);

} // namespace lpc
