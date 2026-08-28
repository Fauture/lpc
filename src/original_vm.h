#pragma once
#include <iosfwd>

#include "o_loader.h"
#include "value.h"

namespace lpc {

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

} // namespace lpc
