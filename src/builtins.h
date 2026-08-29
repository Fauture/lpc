#pragma once
#include <stdexcept>
#include <string>
#include <vector>

#include "value.h"

namespace lpc {

class Interpreter;

// Thrown by input_string() when the serving telnet client has disconnected.
// The original driver kills the pended process when the session closes; the
// VM thread runner catches this quietly and ends the daemon thread instead of
// letting the LPC while(1) shell loop spin on repeated EOF.
struct TelnetClosedError : std::runtime_error {
    explicit TelnetClosedError(const std::string& m) : std::runtime_error(m) {}
};

Value call_builtin(Interpreter* interp, const std::string& name, const std::vector<Value>& args);

// Select the per-project path profile used by vfsRoot()/dataRoot()/etcRoot()
// and the virtual-path resolution inside read_file/read_ini/load_object.
// Called by runPakOriginal() from the pak path before running the master.
// Empty fields keep the current value.
void setVfsProfile(const std::string& vfsRoot, const std::string& dataRoot,
                   const std::string& etcDir, const std::string& pakPrefix,
                   const std::string& objPrefix);

// The active project's object prefix ("aaa" or "dba").  Used by the object
// loader to try "<objPrefix>/x" and "x" variants of a virtual path.
std::string vfsObjPrefix();

// True while the boot/broadcast phase is loading objects.  sleep() returns
// almost immediately during this phase so a daemon's create()-time
// init-retry loop (e.g. share_datad) does not block the whole broadcast and
// starve the remaining daemons / post_init.
void setBootLoading(bool loading);

// vm_interSscanf (0x8109908): C-style sscanf.  Parses input against fmt,
// appends one output Value per conversion (in order) to `out`, and returns the
// number of successful conversions.  Supports %s %d %u %x %c %f %* (suppress)
// and literal matching.  maxOutputs caps the trailing whole-rest match the
// original performs when the format is exhausted with input remaining (the
// number of reference arguments still available); -1 = unlimited.
int lpcSscanf(const std::string& input, const std::string& fmt, std::vector<Value>& out,
              int maxOutputs = -1);

// Telnet client plumbing shared with the VM thread runner (original_vm.cpp).
// The daemon thread that serves one telnet connection registers itself with
// its client number so input_string()/printf() route to the right socket.
void setThreadTelnetClient(int clientNo);
int threadTelnetClient();

// Driver-level telnet shell configured by the -w/-p command line options
// (mirroring the original driver: only when a port is configured does the
// driver listen for telnet).  startDriverTelnet(port) binds and accepts
// connections that go through the driver shell (password check +
// execute_data).  Returns 1 on success.
int startDriverTelnet(int port);
void setDriverTelnetPassword(const std::string& pwd);

// ---- driver message queues (vm_f_create_queue/send_to_queue/receive_from_queue) ----
// Queue ids are the object ids returned by create_queue() (1-based, shared
// with events).  A queue may also be addressed by the name string passed to
// create_queue() (vm_lookupPendObject).  These are real cross-thread queues:
// send_to_queue from any OS thread enqueues and wakes a blocked receiver.
int64_t queueCreate(int64_t qid, const std::string& name);
bool queueSend(int64_t qid, const Value& v);     // false -> queue not found
void queueDelete(int64_t qid);
// Resolve a queue reference (int qid or registered name string) to a qid,
// or -1 if unknown.  Thread-safe.
int64_t queueResolveId(const Value& v);
// Block until an element is available or timeoutMs elapses (timeoutMs < 0 =
// forever).  Returns the element; `ok` is false on timeout / missing queue.
Value queueReceive(int64_t qid, int timeoutMs, bool& ok);
// True when no function closure is currently in flight on a driver queue (all
// dispatched services have been received AND invoked to completion).  The game
// accept thread polls this before feeding a packet so a dispatched
// _check_client_data finishes its lite_set_temp("antibot_answer") before the
// client's next packet is dispatched (preserves the original cooperative
// scheduler's reply-then-cleanup ordering).
bool serviceQueueIdle();

} // namespace lpc
