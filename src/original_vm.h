#pragma once
#include <atomic>
#include <functional>
#include <iosfwd>

#include "o_loader.h"
#include "value.h"

// LPCVM_GETENV("NAME"): std::getenv looked up ONCE per call site.  The
// process environment never changes after start-up, but MSVC's getenv takes
// the CRT environment lock and walks environ on EVERY call, and the
// interpreter tested dozens of LPCVM_* switches that way on per-opcode paths
// (op 23, the cross-object call_other, ...): an xperf profile of a quiet gs
// boot put ~15% of all gs CPU in ucrtbase called straight from runLoop.
// (Not a `#define getenv`: MSVC's <stdlib.h> already defines that macro.)
#include <cstdlib>
#define LPCVM_GETENV(name) \
    ([]() -> const char* { static const char* const lpcvmEnvV_ = std::getenv(name); return lpcvmEnvV_; }())

namespace lpc {

// ---- runtime-toggleable diagnostics -------------------------------------
// The LPCVM_* switches are read ONCE per process and cached, because the
// interpreter consults several of them on per-opcode / per-efun hot paths.
// That makes a probe impossible to arm on a server that is already running -
// and a reboot costs ~11 minutes (gs must import every zone/NPC).
//
// This adds a FILE switch next to the env var, re-read at most once a second:
//     mkdir _probe_on ; type nul > _probe_on\LPCVM_ECHO
// A probe is ON when the env var is set OR its marker file exists; deleting
// the file turns it back off.
//
// `dynProbeFlag` registers/looks the slot up (mutex + scan) - call it ONCE
// into a function-local static and then just load the returned atomic:
//     static bool echoOn() {
//         static const std::atomic<bool>& f = dynProbeFlag("LPCVM_ECHO");
//         return f.load(std::memory_order_relaxed);
//     }
const std::atomic<bool>& dynProbeFlag(const char* name);

// ---- liveness counters ([hb] heartbeat) ---------------------------------
// A service that stops making progress used to be invisible: no crash, the
// listening socket still ESTABLISHED, the process idle - only "the client
// cannot log in" (measured 2026-09-16: the dba executed its last SQL query at
// ~17:25, then silently stopped dispatching filed requests, so every login
// timed out with 9030 通讯故障 and left the account stunned with 9026).
//
// Every service prints ONE line per minute:
//   [hb] t=<ms> pktsIn= pktsOut= filedIn= filedOut= schedIters= clients= regs=
// so after the fact it is obvious WHICH counter stopped advancing.
struct LivenessCounters {
    std::atomic<uint64_t> schedIters{0};   // scheduler main-loop iterations
    std::atomic<uint64_t> pktsIn{0};       // frames received from any socket
    std::atomic<uint64_t> pktsOut{0};      // frames written to any socket
    std::atomic<uint64_t> filedIn{0};      // 0x3400/0x4401/0x2403/0x236F in
    std::atomic<uint64_t> filedOut{0};     // ... and out
    std::atomic<uint64_t> sendFail{0};     // gameServiceSend failures
    std::atomic<uint64_t> threads{0};      // detached worker threads spawned
    // ---- per-connection command-chain counters (see the chain probe below) --
    // The 9101 「正在处理中」 regression could not be attributed with the
    // counters we had (socket frames, scheduler iterations): what matters is how
    // many COMMAND CHAINS one connection has in flight at once.  These are
    // bumped by the probe's call sites, so they are always on - an atomic add is
    // far cheaper than the diagnosis of a missing END.
    std::atomic<uint64_t> chainBegin{0};   // commands handed to the VM
    std::atomic<uint64_t> chainEnd{0};     // command chains finished
    std::atomic<uint64_t> chainPark{0};    // chains parked on a blocking efun
    std::atomic<uint64_t> chainResume{0};  // parked chains woken
    std::atomic<uint64_t> chainOverlap{0}; // !!! command started while one was live
};
LivenessCounters& liveness();

// ---- per-connection command-chain timeline probe -------------------------
// The public drivers give ONE user command at a time to the interpreter:
// LDMud's backend loop calls get_message() only when the interactive has no
// running command_giver, and FluffOS buffers input the same way.  Our port
// reads each socket in its own thread and dispatches every parsed packet as
// soon as it arrives, so one connection can have several commands in flight -
// and the LPC re-entrancy guard (login.o `_process_command` ->
// lite_set_temp("operating",1)) answers 9101 「正在处理中」 exactly then.
//
// Four speculative fixes for that symptom failed because they were verified
// against a dialog COUNT scraped from the client, which cannot say WHICH chain
// overlapped WHICH.  This probe is the direct observation instead:
// vmchainBegin/vmchainEnd bracket every client command, the scheduler loop
// reports PARK/RESUME/END of the chains it runs, and OVERLAP is emitted
// exactly when a command starts while the same connection still has an
// unfinished chain (chainOverlap counts them).  Acceptance for the
// per-connection gate is therefore "OVERLAP == 0, chainOverlap == 0", not a
// dialog guess.
//
// Implemented at the end of original_vm.cpp (it needs the scheduler's process
// and VM types).  Enable with env LPCVM_CHAINTRACE=1 or the marker file
// _probe_on\LPCVM_CHAINTRACE (runtime-armed, see dynProbeFlag).
void vmchainBegin(int64_t clientNo, int64_t chainKey, const void* vm,
                  const std::string& entry);
void vmchainEnd(int64_t clientNo, int64_t chainKey, const void* vm,
                const std::string& entry, bool completed);
void vmchainPark(int64_t clientNo, int64_t chainKey, const void* vm,
                 const std::string& entry, int condKind, const std::string& condKey);
// Scheduler-loop hooks: vmchainRun() before a queued chain runs (closes the
// previous PARK with its RESUME), vmchainRunExit() when the run returned
// (records whether it parked/yielded instead of finishing).
void vmchainRun(int64_t chainKey, const void* vm);
void vmchainRunExit(int64_t chainKey, bool parked);
// A continuation chain (spawned by the command via thread_*/dispatch_service)
// joins the connection of the command it continues, so the gate sees one
// command per connection instead of a tagged chain plus an untagged one.
void vmchainTagLive(int64_t clientNo, int64_t chainKey);
void vmchainMailbox(int64_t clientNo, int64_t chainKey, const char* what);
// Live (unfinished) command chains of one connection.  >1 means overlap.
int vmchainLiveChains(int64_t clientNo);
// Frame-chain text for the probe's "[chain] ..." field (implementation sits at
// the end of original_vm.cpp, where class OriginalVm is complete).
std::string vmchainTextOf(const void* vm);
// ---- per-connection command gate ----------------------------------------
// Blocks while the connection still has an unfinished command chain, i.e. it
// makes the reader/dispatch thread obey the public drivers' rule: ONE command
// per interactive at a time (LDMud's backend loop, FluffOS's comm.c).  Call it
// BEFORE dispatching a packet of that connection.  Woken by the probe when a
// chain ends; bounded by timeoutMs so a stuck command cannot wedge the reader
// thread forever (returns false on timeout).
bool waitClientCommandIdle(int64_t clientNo, int timeoutMs);
// Wall clock in MICROseconds for the reply-delivery probes (LPCVM_FLOW).
int64_t usNow();
// The reply-delivery state machine (LPCVM_FLOW=1): one line per park, wake
// and inbound internal packet, microsecond stamped so "how long did the
// parked chain wait for THIS reply" is a number instead of a theory.
void flowProbePark(const char* key, const char* fn);
void flowProbeWake(const char* key, const char* fn);
void flowProbeReply(int64_t cno, int64_t cmd, int64_t len);
// Signalled by the probe whenever a command chain ends (used by the gate).
void chainWaitNotify();
// Active-command reference counting (see the comment on the implementation):
// a client command's LPC continuation may run on a chain the driver command has
// already returned from, so the gate counts COMMANDS, not driver chains.
// cmdActiveAttach/Detach bracket a process's run (scheduler loop) and an inline
// dispatch (execOnMain).
void cmdActiveAttach(int64_t clientNo);
void cmdActiveDetach(int64_t clientNo);
// The script's own re-entrancy marker.  login.o::_process_command keeps
// `lite_set_temp("operating",1)` set while a command is between phases, on chains
// the driver's process table does not associate with the connection; the gate
// therefore also reads that marker from the VM the connection's commands run on
// (registered here at every dispatch).
void registerClientScriptVm(int64_t cno, const void* vm);
bool clientLpcBusy(int64_t cno);

// Runs the original action-bytecode VM for a parsed .o object.
// Currently supports the common global-initializer opcodes and stubs efun/user
// calls so that .pak/.o files can be loaded and initialized.
int runOriginalObject(const OriginalObject& obj, std::ostream& out, const std::string& searchRoot = "");
int runOriginalObjectInit(const OriginalObject& obj, const std::string& searchRoot = "");
Value callOriginalObjectFunction(const std::string& oPath, const std::string& funName,
                                 const std::vector<Value>& args, const std::string& searchRoot = "");

// Call a function on the master program (the driver's global program, i.e.
// the object loaded with runOriginalObject).  Used by the telnet server and
// other driver-side entry points.
Value callMasterProgramFunction(const std::string& funName, const std::vector<Value>& args,
                                const std::string& searchRoot);

// [net-pump] 火忘派发 (真 pcb 进程, 可 park/恢复, 不阻塞不交付)。
// doneFlag (可选): 链真正结束时置 true。
void callMasterProgramFunctionSpawn(const std::string& funName,
                                    const std::vector<Value>& args, int clientNo,
                                    const std::shared_ptr<std::atomic<bool>>& doneFlag =
                                        std::shared_ptr<std::atomic<bool>>());

// [net-pump] 非阻塞探针: 该连接是否有活跃命令链 (泵/调度线程用)。
bool clientCommandBusy(int64_t clientNo);

// The search root under which the master program ("<main>") is registered.
std::string masterSearchRoot();

// Driver-side "post heart beat" pass (the original main loop calls
// start_gs.o's post_heart_beat(me) once per beat; the callbacks registered
// through register_post_heart_beat drive e.g. the anti-cheat challenge and the
// CCS keep-alive).  Returns the number of passes performed (0/1).  Set
// LPCVM_NO_HB to disable.
int postHeartBeatAll();

// Driver-side anti-cheat "check_server" challenge.  The client sends 0x5108
// every ~10-30 s; /gs/cmds/normal/check_server.c must answer with
// me->send(8625, x, y, answer) (0x21B1) after loading the script the client
// supplied.  Without that answer the client logs itself out after a few minutes
// (the reported 3-5 minute disconnect).  `cryptedBuf` is the 0x5108 payload
// (BE32 original length + LZSS + rolling xor); returns true when the challenge
// ran and fills x/y/answer.
bool antiCheatCheckServer(const std::string& cryptedBuf, int& x, int& y, int& answer);

// Preprocessor defines of the master (global) program: name -> expansion.
// The runtime queries these in expand_macro (the packet protocol analyser
// resolves command numbers such as MSG_L_AUTH -> 0x5351 from them).
bool masterDefineLookup(const std::string& name, std::string& expansion);

// Apply a function closure on its owning object's VM (the original driver's
// vm_applyFun executes the function in the current/owning program context).
// Used by the packet protocol analyser to invoke registered command handlers.
Value applyClosureOnOwner(const Value& closure, const std::vector<Value>& args);

// Instance-variable access by NAME (the original driver's vm_getObjVar):
// resolves "varName" (optionally "progName::varName") to the object's variable
// slot using the PROG_VARS records the loader now keeps.  Returns the value
// (undefined when unknown) and whether a slot was written.
Value objVarGet(const Value& obj, const std::string& name);
bool objVarSet(const Value& obj, const std::string& name, const Value& v);
// save_object(): the object dbase as "progName::varName=value\n" lines.
std::string objVarsToString(const Value& obj);

// Spawn threads queued at runtime by create_thread (e.g. the telnet daemon
// thread created when a client connects).  Startup threads are drained by
// runOriginalObject itself; this drains the runtime ones.

// Driver-side object loader used by the load_object / safe_load_object efuns:
// loads the .o for a virtual path (e.g. "/aaa/daemons/configd.c"), registers
// it in the object registry (so find_object / cross-object calls work) and
// runs its global initializer + create().  Returns 1 on success.
int loadObjectIntoRegistry(const std::string& virtualPath, const std::string& searchRoot,
                           const std::vector<Value>& createArgs = {});

// find_object: returns an object value if the object is loaded, otherwise 0.
// (The original driver does not auto-load here; load_object/safe_load_object
// do the loading.)
Value findObjectInRegistry(const std::string& virtualPath, const std::string& searchRoot);
// True when the value holds an object that is still alive.  Object values handed
// out by a registry VM carry that VM's liveness flag, which unregisterDestructed()
// clears when the object is REALLY freed (the driver's second destruct phase - the
// first, "marked" phase only sets beingDestroyed_ and keeps the object registered
// so its destructor chain can still call save()/get_name()).
//
// The original invalidates the REFERENCES on a real free: the slot holds 0 from
// then on.  Every observable of an object value therefore has to behave as if it
// held 0 - is_object()/is_type(10), the truth test (`if (temp("combat"))`),
// equality (a freed reference equals 0 and equals another freed one), mapping
// keys, object_name() and to_string().  This is what lets game code that stores an
// object and only clears it on the normal path recover: monsterd::touch_off_combat
// does me->set_temp("combat", cob) and can then destruct cob without ever building
// a monster team (idle_template, the "no encounter" marker city rooms carry in
// etc/room.list), and is_in_combat() (is_object(temp("combat")) &&
// !cob->query_temp("finished")) must go false by itself.
bool objectValueAlive(const Value& v);
// True when the value is an object value that was really freed (the "reference is
// 0 now" state).  False for every non-object value, so callers can use it to
// substitute the int 0.
bool objectValueFreed(const Value& v);

// clone_object(virtualPath, createArgs...): find the loaded template object,
// create a fresh instance (independent instance variables), run its create()
// with createArgs and register it in the object registry under a unique clone
// key.  Returns an object value, or 0 if the template is not loaded.
Value cloneObjectInRegistry(const std::string& virtualPath, const std::string& searchRoot,
                            const std::vector<Value>& createArgs = {});

// Cooperative scheduler main loop (stage B): drives daemon processes, wake
// timers and queue/event wakeups.  Replaces main()'s idle sleep loop.
void schedulerRun();

// The driver's main loop owns the LISTENING sockets too: the original scans
// them together with the connected ones and accepts on the driver thread,
// so a listener must be handed to it instead of parking a dedicated
// accept() thread (which the scheduler can neither wake nor account for).
// `cb` runs ON THE SCHEDULER THREAD, receives the accepted socket and must
// not call into the VM; returning false drops the listener.
void registerServiceSocket(int fd,
                           std::function<bool(unsigned int fd)> cb);


// Canonical registry key for an object path.  LPC addresses rooms and daemons
// by their program path ("/gs/zone/lanxianzhen/lanxianzhen.c") while the
// registry key of the loaded object is "pak:gs/zone/lanxianzhen/lanxianzhen.o";
// the driver treats both as the same object, so object identity and the
// container (env) key must be normalized through this.
std::string canonicalObjectPath(const std::string& key);
// Identity of an object name, i.e. the pointer the original driver compares
// (vm_compareValue 0x80bbcb4 compares the object pointer stored in the value,
// cases 6..0xA).  It must identify the INSTANCE: this engine gives every
// instance its own VM but SHARES the read-only program object between a clone
// and its template, so the identity returned is the instance's VM, not
// &vm->object().  Two spellings of one object ("/gs/zone/x/x", "/gs/zone/x/x.c",
// "pak:gs/zone/x/x.o") yield the SAME pointer; two different clones - and a
// clone vs its own basic object - yield different ones.  Null when the name
// resolves to no registered instance.
const void* objectIdentityOf(const std::string& name);

// lodger_object(container): the container's contents as an ARRAY of object values,
// in the driver's lodger-chain order (vm_f_lodger_object 0x80f41c8; vm_f_move_object
// 0x80f5348 links at the tail, which is the order envLodgerMove() keeps).  The
// argument is a registry key or object name of any spelling; an unknown or empty
// container yields an EMPTY array.  (The original raises "Not found the object for
// lodger_object." for a name it cannot resolve or a destructed object; we stay
// lenient because our registry can spell one object two ways and the 2026-09-27
// attempt showed that throwing here aborts live GM queries and destructor chains.)
Value makeLodgerArray(const std::string& containerKey);

// destruct_object(ob) support: unregister the object identified by `key`
// (any spelling: clone key, pak: path, .c/.o program path) so it can no longer
// be resolved.  Returns false when the key was not registered.
bool destructObjectByKey(const std::string& key);

// Registry key of a live OriginalObject instance ("" when unknown).  A
// this_object() value built on a detached VM carries the placeholder "<this>"
// as its programName, so the only reliable identity is its owner object
// pointer (threadd::daemon destroys the logged-out character with
// destruct_object(this_object())).
std::string regKeyOfObjectPtr(const void* objPtr);

// object_template()'s template key for a registered object ("" when unknown or
// when the object has no template).  Diagnostic accessor: a clone must be built
// with the BASIC OBJECT as its template - basic_object.c::create() takes
// my_import_name from it, and iid.c::create_iid()'s `query("combined")` guard
// resolves through basic_object() -> find_basic_object_by_import_name(name).
std::string objectTemplateKey(const std::string& key);

// Milliseconds since this process started, from ONE shared origin so that
// timestamps printed by different probes ([recv-accept] in game_service.cpp,
// [send] in builtins.cpp, [life]/[room] in original_vm.cpp) are directly
// comparable.  The client reports that the original reaches the character
// screen in ~1ms after choosing a line, so the login path must be measured on
// a single clock.
int64_t msSinceStart();

} // namespace lpc
