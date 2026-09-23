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

// Caller context (object + LPC function) for [efun-stub] log lines.  The VM
// sets this right before falling through to call_builtin() so every log of an
// unimplemented efun names the exact object::function that invoked it - that
// makes the runtime stub list attributable to the boot stage that needs it.
struct StubCallCtx {
    std::string obj;
    std::string fn;
    // Registry key of the CALLING instance (e.g.
    // "clone:gs/clone/char/user.o#16137").  this_object() evaluates to an
    // ObjectValue whose programName is the placeholder "<this>", so
    // destruct_object(this_object()) can only be resolved through this key.
    std::string key;
};
StubCallCtx& stubCallCtx();

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
// Diagnostic: "name=qid" for every registered queue (empty when none).
std::string queueNameDump();
// Number of elements currently queued on qid (0 when the queue is missing /
// destroyed).  Used by the cooperative scheduler's lost-wake guard: a receiver
// that registered its park AFTER the sender pushed (and woke nobody) re-checks
// this and reruns instead of sleeping forever on a non-empty queue.
size_t queuePendingItems(int64_t qid);
// Number of elements currently queued on qid (0 when the queue is missing /
// destroyed).  Used by the cooperative scheduler's lost-wake guard: a receiver
// that registered its park AFTER the sender pushed (and woke nobody) re-checks
// this and reruns instead of sleeping forever on a non-empty queue.
size_t queuePendingItems(int64_t qid);
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
// Non-consuming peek of an event's currently-raised bits (by NAME; used by the
// cooperative scheduler's lost-wake guard: raise_event may arrive between a
// waiter's poll and its parkCurrent registration, leaving bits raised that no
// registered waiter will consume).
uint32_t eventPeekRaised(const std::string& name);
// Clear (consume) raised bits of an event (by NAME).  The scheduler uses this
// when it wakes a waiter for a raise, mirroring the original driver's
// vm_f_raise_event (0x80f07b8): raise ORs the mask into the event, then walks
// the pend list and for the FIRST intersecting waiter clears that waiter's
// consumed bits (events &= ~v5) and resumes it.  Bits are cleared only when a
// waiter actually consumes them, so a raise with no waiter leaves the bits set
// and a later wait_event's immediate probe still succeeds.
void eventClearBits(const std::string& name, uint32_t mask);
// TEMP cfg-round-trip ledger arm: set when the gs sends a 17520 (read_etc)
// config request; scheduler event ledger then prints ev_readetcd raises.
void cfgLedgerArm();
bool cfgLedgerArmed();
// Current LPC thread's unique, stable id (the original's PCB->tid).  Used to
// build per-thread event names ("ev_filed" + tid/32) and bits (1 << tid%32).
int64_t lpcThreadId();
// The tid of the LPC PROCESS currently executing on the cooperative scheduler
// (the original's PCB->tid).  get_thread_id() must return a DIFFERENT value
// per process - configd's read_etc builds its event name from it, and two
// processes sharing an id would wake each other's wait_event.  On the single
// scheduler thread lpcThreadId() is identical for every process, so the efun
// prefers this when a process is running.
int64_t schedulerCurrentTid();
// True when no function closure is currently in flight on a driver queue (all
// dispatched services have been received AND invoked to completion).  The game
// accept thread polls this before feeding a packet so a dispatched
// _check_client_data finishes its lite_set_temp("antibot_answer") before the
// client's next packet is dispatched (preserves the original cooperative
// scheduler's reply-then-cleanup ordering).
bool serviceQueueIdle();

// The connection number of the client command currently being dispatched by
// THIS thread (-1 = none).  game_service sets it around feedClient(); the VM
// dispatch (Scheduler::execOnMain) reads it to attribute the command chain to
// a connection, so the chain-timeline probe (original_vm.h) can prove whether
// two commands of one connection ever overlap - the LPC re-entrancy guard
// answers 9101 「正在处理中」 exactly when they do.
void setDispatchCno(int64_t cno);
int64_t dispatchCno();

// Per-connection flow gate: ms since the last packet was sent to that client
// (used by pkt_protocol.cpp to keep a connection busy until its command's
// client-visible result has been delivered and the flow went quiet).
void noteClientSend(int64_t cno);
int64_t clientSendIdleMs(int64_t cno);
// Event-driven version of the per-packet flow gate: blocks until the connection
// has been quiet for `idleMs` or `timeoutMs` elapsed (see its definition).
bool waitClientSendIdle(int64_t cno, int64_t idleMs, int timeoutMs);

} // namespace lpc
