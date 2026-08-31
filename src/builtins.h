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

// Driver predefines injected at boot (the original's /D CONFIG=... flags).
// expand_macro() consults this table first; entries are plain strings (no
// surrounding quotes).  Called by runPakOriginal() so start_<prefix>.o
// resolves INI_FILE via expand_macro("CONFIG") to the real config file
// (gs keeps gs1.ini, the others <prefix>.ini).
void injectDefine(const std::string& name, const std::string& value);

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

// ---- driver events (vm_f_create_event / wait_event / raise_event) ----
// Events are named pend objects (gameshell vmObject[] table); raise_event sets
// bits on the event, wait_event blocks until one of its bits is set (or the
// timeout expires).  Mirrors vm_f_wait_event (0x80f097c) / vm_f_raise_event
// (0x80f07b8): wait returns 0 when the event fired (bits cleared), -501 on
// timeout, -101 when the event does not exist.  These are real cross-thread
// waits: raise_event from any OS thread wakes a blocked waiter.
// createEvent(name, oid): registers the event under BOTH its name and its
// object id (the original vm_createPendObject tables the event in vmObject[]
// so vm_f_wait_event can look it up by int oid via vm_getPendObject(14, oid)
// OR by name via vm_lookupPendObject).
void eventCreate(const std::string& name, int64_t oid);
void eventDelete(const std::string& name);
// mask = the bits to raise; returns 0 on success, -101 if the event is unknown.
// ref: string event name or int oid (create_event's return value).
int64_t eventRaise(const Value& ref, uint32_t mask);
// Blocks until (events & mask) != 0 or timeoutSecs expires (seconds; -1 =
// forever, 0 = poll once).  Clears the matched bits on success.  Returns 0 on
// success, -501 on timeout, -101 if the event is unknown.  ref: string name or
// int oid.
int64_t eventWait(const Value& ref, uint32_t mask, double timeoutSecs);
// Current LPC thread's unique, stable id (the original's PCB->tid).  Used to
// build per-thread event names ("ev_filed" + tid/32) and bits (1 << tid%32).
int64_t lpcThreadId();
// Reserve an unused tid in the caller thread (create_thread needs the child's
// tid synchronously to return it); the spawned thread then calls lpcThreadBind.
int64_t lpcAllocTid();
// Force the current thread's tid to `tid` and register it in the process
// table under `name` (empty = unnamed).  Mirrors vm_spawnProcess registering
// the PCB with a name.
void lpcThreadBind(int64_t tid, const std::string& name);
// Mark the process as terminated (PCB freed); get_thread_id queries go -1.
void lpcThreadMarkDead(int64_t tid);
// vm_lookupProcess(name): tid of a live process with that name, else -1.
int64_t lpcThreadIdByName(const std::string& name);
// vm_getPCB(tid): tid if a live process has it, else -1.
int64_t lpcThreadIdByTid(int64_t tid);
// True when no function closure is currently in flight on a driver queue (all
// dispatched services have been received AND invoked to completion).  The game
// accept thread polls this before feeding a packet so a dispatched
// _check_client_data finishes its lite_set_temp("antibot_answer") before the
// client's next packet is dispatched (preserves the original cooperative
// scheduler's reply-then-cleanup ordering).
bool serviceQueueIdle();

} // namespace lpc
