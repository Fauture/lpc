#pragma once
#include <atomic>
#include <iosfwd>

#include "o_loader.h"
#include "value.h"

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
};
LivenessCounters& liveness();

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

// The search root under which the master program ("<main>") is registered.
std::string masterSearchRoot();

// Driver-side "post heart beat" pass (the original main loop calls
// start_gs.o's post_heart_beat(me) once per beat; the callbacks registered
// through register_post_heart_beat drive e.g. the anti-cheat challenge and the
// CCS keep-alive).  Returns the number of passes performed (0/1).  Set
// LPCVM_NO_HB to disable.
int postHeartBeatAll();

// Preprocessor defines of the master (global) program: name -> expansion.
// The runtime queries these in expand_macro (the packet protocol analyser
// resolves command numbers such as MSG_L_AUTH -> 0x5351 from them).
bool masterDefineLookup(const std::string& name, std::string& expansion);

// Apply a function closure on its owning object's VM (the original driver's
// vm_applyFun executes the function in the current/owning program context).
// Used by the packet protocol analyser to invoke registered command handlers.
Value applyClosureOnOwner(const Value& closure, const std::vector<Value>& args);

// Spawn threads queued at runtime by create_thread (e.g. the telnet daemon
// thread created when a client connects).  Startup threads are drained by
// runOriginalObject itself; this drains the runtime ones.
void drainPendingOriginalThreads(const std::string& searchRoot);

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

// clone_object(virtualPath, createArgs...): find the loaded template object,
// create a fresh instance (independent instance variables), run its create()
// with createArgs and register it in the object registry under a unique clone
// key.  Returns an object value, or 0 if the template is not loaded.
Value cloneObjectInRegistry(const std::string& virtualPath, const std::string& searchRoot,
                            const std::vector<Value>& createArgs = {});

// Cooperative scheduler main loop (stage B): drives daemon processes, wake
// timers and queue/event wakeups.  Replaces main()'s idle sleep loop.
void schedulerRun();

// Canonical registry key for an object path.  LPC addresses rooms and daemons
// by their program path ("/gs/zone/lanxianzhen/lanxianzhen.c") while the
// registry key of the loaded object is "pak:gs/zone/lanxianzhen/lanxianzhen.o";
// the driver treats both as the same object, so object identity and the
// container (env) key must be normalized through this.
std::string canonicalObjectPath(const std::string& key);

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

// Milliseconds since this process started, from ONE shared origin so that
// timestamps printed by different probes ([recv-accept] in game_service.cpp,
// [send] in builtins.cpp, [life]/[room] in original_vm.cpp) are directly
// comparable.  The client reports that the original reaches the character
// screen in ~1ms after choosing a line, so the login path must be measured on
// a single clock.
int64_t msSinceStart();

} // namespace lpc
