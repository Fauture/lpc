cmake_minimum_required(VERSION 3.20)
project(lpcvm C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    # All sources are UTF-8 without BOM.  Without /utf-8, cl decodes them as
    # GBK (codepage 936) and certain multi-byte comment sequences are read as
    # invalid GBK pairs that swallow newlines / restructure the token stream -
    # original_vm.cpp then fails with phantom "namespace" errors near
    # Scheduler::spawn that do not exist under /utf-8 (verified: /Zs passes
    # clean with /utf-8, fails without).
    add_compile_options(/W3 /EHsc /utf-8)
    # The Debug CRT enables per-container STL checks (_ITERATOR_DEBUG_LEVEL=2
    # under /MDd); every stack push/pop / vector op in the interpreter then
    # pays range/orphan bookkeeping measured at ~1-2us per VM opcode in Debug.
    # The boot/import regressions run Debug builds, so disable the container
    # debug checks for Debug configs too (pure codegen switch, no semantics
    # change; _DEBUG CRT heap checks stay on).
    add_compile_definitions($<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>)
else()
    add_compile_options(-Wall -Wextra)
endif()

# The d3des source is a classic K&R-style .c file; keep it compiled as C.
set_source_files_properties(src/d3des.c PROPERTIES LANGUAGE C)
# SQLite amalgamation (in-memory DB backing the game's sql_* efuns).
set_source_files_properties(third_party/sqlite/sqlite3.c PROPERTIES LANGUAGE C)

add_executable(lpcvm
    src/main.cpp
    src/value.cpp
    src/lexer.cpp
    src/ast.cpp
    src/parser.cpp
    src/interpreter.cpp
    src/builtins.cpp
    src/o_loader.cpp
    src/lzss.cpp
    src/pak_loader.cpp
    src/original_vm.cpp
    # ---- vmcore: the new execution core (docs/vm_core_refactor.md v2) ----
    src/vmcore/pcb.cpp
    src/vmcore/object.cpp
    src/vmcore/error.cpp
    src/vmcore/call.cpp
    src/vmcore/values.cpp
    src/vmcore/interp.cpp
    src/vmcore/sched.cpp
    src/vmcore/efun_adapt.cpp
    src/vmcore/driver_model.cpp
    src/map_module.cpp
    src/game_service.cpp
    src/pkt_protocol.cpp
    src/mysql_client.cpp
    src/xdbc.cpp
    src/d3des.c
    third_party/sqlite/sqlite3.c
)

target_include_directories(lpcvm PRIVATE src third_party/sqlite)
if(WIN32)
    target_link_libraries(lpcvm PRIVATE ws2_32)
    # 32-bit driver process: the original gameshell binary is X86 with
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
    if(CMAKE_GENERATOR_PLATFORM MATCHES "x64" OR CMAKE_SIZEOF_VOID_P EQUAL 8)
        target_link_options(lpcvm PRIVATE /STACK:0x2000000)
    else()
        target_link_options(lpcvm PRIVATE /machine:X86 /LARGEADDRESSAWARE /STACK:0x2000000)
    endif()
    # Release keeps /O2 but also emits a PDB, so crash dumps and ETW/xperf CPU
    # samples resolve to function names (boot profiling).  /OPT:REF,ICF restore
    # the linker defaults that /DEBUG would otherwise turn off.  /GL+/LTCG:
    # 全程序优化, 解释器类代码跨 TU 内联 (interp loop ↔ op 实现 ↔ Value
    # 构造析构), 2026-11-07 羽化执行 300ms/轮 专项.
    target_compile_options(lpcvm PRIVATE $<$<CONFIG:Release>:/Zi /GL>)
    target_link_options(lpcvm PRIVATE $<$<CONFIG:Release>:/DEBUG /OPT:REF /OPT:ICF /LTCG>)
endif()
