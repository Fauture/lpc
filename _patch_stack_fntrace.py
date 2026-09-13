# -*- coding: utf-8 -*-
"""1) /STACK:0x2000000 for lpcvm (the scheduler runs on the MAIN thread: the
   1 MB reserve overflowed on a deep newbie-gift -> build_item chain and killed
   the GS with 0xC00000FD).
2) LPCVM_FNTRACE=<substr> probe: log ENTER/RETURN(+value) of matching LPC
   functions, to see whether taskd's closure calls (*task_tip)(me, info) run.
"""
import io

# ---- CMakeLists: stack reserve -------------------------------------------
CM = 'CMakeLists.txt'
d = io.open(CM, 'rb').read()
OLD = ("""    # 32-bit driver process: the original gameshell binary is X86 with
    # LARGEADDRESSAWARE, giving the process a 4GB user address space on 64-bit
    # Windows.  Without it the 2GB cap exhausts during GS boot (bad allocation
    # at ~1.9GB, dropping the rest of post_init -> tasks/zone data never
    # loads).  /STACK:0x100000 keeps the main thread's reserved stack small so
    # the address space stays available for LPC heap data; deep LPC call
    # chains run on explicitly-reserved big-stack threads (create_thread).
    target_link_options(lpcvm PRIVATE /machine:X86 /LARGEADDRESSAWARE /STACK:0x100000)
""").replace('\n', '\r\n').encode('ascii')
NEW = ("""    # 32-bit driver process: the original gameshell binary is X86 with
    # LARGEADDRESSAWARE, giving the process a 4GB user address space on 64-bit
    # Windows.  Without it the 2GB cap exhausts during GS boot (bad allocation
    # at ~1.9GB, dropping the rest of post_init -> tasks/zone data never
    # loads).
    # /STACK:0x2000000 (32MB) - the cooperative scheduler runs on the MAIN
    # thread (main.cpp -> lpc::schedulerRun()), so every LPC call nests on the
    # main thread's stack.  With the old 1MB reserve, opening the newbie gift
    # (open_newgift_item -> rebuild_newbie_gift -> new_item -> move_to_bag ->
    # build_item_ex -> get_item_basic_info -> ... ~100 nested LPC frames) blew
    # the stack: [crash] code=0xc00000fd frames=4, killing the whole GS.  The
    # original driver ran on Linux's 8MB default.  "deep chains run on
    # create_thread threads" is no longer true - create_thread goes through
    # scheduler().spawn() (single-threaded) too.
    target_link_options(lpcvm PRIVATE /machine:X86 /LARGEADDRESSAWARE /STACK:0x2000000)
""").replace('\n', '\r\n').encode('ascii')
assert d.count(OLD) == 1, 'CMakeLists block not found (%d)' % d.count(OLD)
io.open(CM, 'wb').write(d.replace(OLD, NEW))
print('CMakeLists.txt patched')

# ---- original_vm.cpp: FNTRACE probe --------------------------------------
P = 'src/original_vm.cpp'
d = io.open(P, 'rb').read()


def B(s):
    return s.replace('\n', '\r\n').encode('ascii')


HELPER_OLD = B("""// Loop control block (value type 20 in the original VM).
struct Frame {""")
HELPER_NEW = B("""// LPCVM_FNTRACE=<substr>: diagnostic ENTER/RETURN trace of LPC functions
// whose name contains <substr>, with the return value.  Env-gated and cached
// once (a per-call getenv would slow the whole VM down).
static bool fnTraceMatch(const std::string& name) {
    static const char* const flt = std::getenv("LPCVM_FNTRACE");
    if (!flt || !*flt || name.empty()) return false;
    return name.find(flt) != std::string::npos;
}

// Loop control block (value type 20 in the original VM).
struct Frame {""")
assert d.count(HELPER_OLD) == 1, 'helper anchor not found'
d = d.replace(HELPER_OLD, HELPER_NEW)

ENT_OLD = B("""        lpcRingPush(fn.name.c_str());
        mallLog("ENTER-inline", fn.name, bp, (passedArgs >= 0) ? passedArgs : fn.argCount, nullptr);""")
ENT_NEW = B("""        lpcRingPush(fn.name.c_str());
        if (fnTraceMatch(fn.name))
            std::cerr << "[fn] ENTER " << fn.name << " obj=" << objName_
                      << " args=" << ((passedArgs >= 0) ? passedArgs : fn.argCount) << "\\n";
        mallLog("ENTER-inline", fn.name, bp, (passedArgs >= 0) ? passedArgs : fn.argCount, nullptr);""")
assert d.count(ENT_OLD) == 1, 'entry-inline anchor not found'
d = d.replace(ENT_OLD, ENT_NEW)

ENT2_OLD = B("""        lpcRingPush(fn.name.c_str());
        mallLog("ENTER", fn.name, bp, (passedArgs >= 0) ? passedArgs : fn.argCount, nullptr);""")
ENT2_NEW = B("""        lpcRingPush(fn.name.c_str());
        if (fnTraceMatch(fn.name))
            std::cerr << "[fn] ENTER " << fn.name << " obj=" << objName_
                      << " args=" << ((passedArgs >= 0) ? passedArgs : fn.argCount) << "\\n";
        mallLog("ENTER", fn.name, bp, (passedArgs >= 0) ? passedArgs : fn.argCount, nullptr);""")
assert d.count(ENT2_OLD) == 1, 'entry anchor not found'
d = d.replace(ENT2_OLD, ENT2_NEW)

RET_OLD = B("""                // ---- explicit stack machine: return to the caller ----
                // Clean the callee's argument/local slots off the stack""")
RET_NEW = B("""                if (fnTraceMatch(f.fn->name))
                    std::cerr << "[fn] RETURN " << f.fn->name
                              << " ret=" << to_string(returnValue_, false).substr(0, 160) << "\\n";
                // ---- explicit stack machine: return to the caller ----
                // Clean the callee's argument/local slots off the stack""")
assert d.count(RET_OLD) == 1, 'return anchor not found'
d = d.replace(RET_OLD, RET_NEW)

io.open(P, 'wb').write(d)
after = d.count(b'\r\n')
print('original_vm.cpp patched; CRLF', after, 'bareLF', d.count(b'\n') - after)
