/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: mmd.o
 * Source: /daemons/mmd.c
 * Inherits: (none)
 * Inherited programs: /feature/daemon/register.c(), /feature/common/dbase.c(), /feature/common/module_maintain.c()
 * Functions: 12 names, 12 code segments found
 * Strings: 23, Variables: 6
 *********************************************************************/

inherit "/feature/daemon/register.c";
inherit "/feature/common/dbase.c";
inherit "/feature/common/module_maintain.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   array common_message;
   /* mixed global_0; */
   /* mixed global_1; */
   /* mixed global_2; */
   /* mixed global_3; */
   int mmdQ;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private int translate_request(mapping msg);
buffer build_module_msg_info();
private void destruct();
mixed post_request(mixed msg);
private void unload_module(mapping path_map, mixed para);
private void load_module(mapping path_map, mixed para);
mapping query_by_path(string path);
private void process_request(mapping msg);
private void update_module(mapping path_map, mixed para);
private void create();
private void daemon();
private void post_request_done(array result, int tid, mixed result_map);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   mmdQ = 65535;  // /daemons/mmd.c:471
   common_message = ({ "load", "unload", "update" });

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=55, nargs=1, insts=55
// --------------------------------------------------------------------
private int translate_request(mapping msg) {
    /* ---- Enhanced Pseudocode ---- */
    mixed oper;
    mixed path;
    mixed m;
    mixed f;

    oper = msg["msg"];
    if ((((oper != "load") && (oper != "unload")) && (oper != "update"))) {
        return 0;
    }
    f = query_message_routine(oper);
    if (is_function(f)) {
        path = msg["path"];
        m = query_by_path(path);
        (*f)(m, msg["para"]);
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=63, nargs=0, insts=63
// --------------------------------------------------------------------
buffer build_module_msg_info() {
    /* ---- Enhanced Pseudocode ---- */
    mixed buf;
    mixed m;
    mixed message_map;
    int path;
    mixed ob;

    buf = pad_buffer(sizeof(common_message), common_message);
    m = query_all_entries();
    buf += pad_buffer(sizeof(m));
    loop (path in keys(m)) {
        if ((!is_object(ob = find_object(m[path])))) {
            buf += pad_buffer(path, 0);
        } else {
            message_map = ob->get_message_map();
            buf += pad_buffer(path, sizeof(message_map), keys(message_map));
        }
    }  // end loop (verified)
    return buf;

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=52, nargs=0, insts=52
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    int i;

    printf("Stop MMD.all services.\n");
    send_to_queue(mmdQ, -1);

    // --- label L0009 ---
    while ((get_thread_id("tmodulemaintaind") != -1)) {
        sleep(0.1);
    }  // end loop (verified)
    delete_queue(mmdQ);
    for (i = 0; i <= ((((get_system_info()["max_threads"] + 32) - 1) / 32) - 1); i++) {
        delete_event(("mmd_block" + i));
    }  // end loop (verified)
    printf("MMD.all services done.\n");

}

// --------------------------------------------------------------------
// @code  nc=53, nargs=1, insts=53
// --------------------------------------------------------------------
mixed post_request(mixed msg) {
    /* ---- Enhanced Pseudocode ---- */
    mixed result;

    wait_event(("mmd_block" + (get_thread_id() / 32)), (1 << (get_thread_id() % 32)), 0, 0);
    result = ({ 0 });
    msg["callback"] = (: post_request_done, result, get_thread_id() :);
    send_to_queue(mmdQ, msg);
    wait_event(("mmd_block" + (get_thread_id() / 32)), (1 << (get_thread_id() % 32)), 0, -1);
    return result[0];

}

// --------------------------------------------------------------------
// @code  nc=32, nargs=2, insts=32
// --------------------------------------------------------------------
private void unload_module(mapping path_map, mixed para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed str;
    int path;

    loop (path in keys(path_map)) {
        if ((path == get_mm_path())) {
            continue;
        }
        str = path_map[path];
        if ((!is_object(find_object(str)))) {
            continue;
        }
        destruct_object(str);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @code  nc=31, nargs=2, insts=31
// --------------------------------------------------------------------
private void load_module(mapping path_map, mixed para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed str;
    int path;

    loop (path in keys(path_map)) {
        if ((path == get_mm_path())) {
            continue;
        }
        str = path_map[path];
        if (is_object(find_object(str))) {
            continue;
        }
        update(str);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=100, nargs=1, insts=100
// --------------------------------------------------------------------
mapping query_by_path(string path) {
    /* ---- Enhanced Pseudocode ---- */
    mixed m;
    mixed ret;
    int len;
    int str;

    if ((path == "*")) {
        return query_all_entries();
    }
    if ((ret = query_entry(path))) {
        return ([ path: ret ]);
    }
    len = strlen(path);
    if ((path[(len - 2)..(len - 1)] != "/*")) {
        return ([ ]);
    }
    path = path[0..(len - 3)];
    len = strlen(path);
    m = query_all_entries();
    ret = ([ ]);
    loop (str in keys(m)) {
        if ((strlen(str) <= len)) {
            continue;
        }
        if (((str[len] != 47) || (str[0..(len - 1)] != path))) {
            continue;
        }
        ret[str] = m[str];
    }  // end loop (verified)
    return ret;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=83, nargs=1, insts=83
// --------------------------------------------------------------------
private void process_request(mapping msg) {
    /* ---- Enhanced Pseudocode ---- */
    mixed oper;
    mixed path;
    int str;
    mixed path_map;
    mixed result_map;
    mixed ob;
    mixed f;

    do {
        if ((!is_string(oper = msg["msg"]))) {
            break;
        }
        if ((!is_string(path = msg["path"]))) {
            break;
        }
        if (translate_request(msg)) {
            break;
        }
        if ((!sizeof((path_map = query_by_path(path))))) {
            break;
        }
        result_map = ([ ]);
        loop (str in values(path_map)) {
            ob = find_object(str);
            if ((!is_object(ob))) {
                continue;
            }
            result_map[str] = ob->process_msg(oper, dup(msg["para"]));
        }  // end loop (verified)
    } while (0);

    // --- label L0070 ---
    if (is_function(f = msg["callback"])) {
        (*f)(result_map);
    }

}

// --------------------------------------------------------------------
// @code  nc=26, nargs=2, insts=26
// --------------------------------------------------------------------
private void update_module(mapping path_map, mixed para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed str;
    int path;

    loop (path in keys(path_map)) {
        if ((path == get_mm_path())) {
            continue;
        }
        str = path_map[path];
        update(str);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=57, nargs=0, insts=57
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    int i;

    set_mm_path("MM_D");
    mmdQ = create_queue("qmodulemaintaind");
    create_thread("tmodulemaintaind", "daemon");
    add_message_routine("load", (: load_module :));
    add_message_routine("unload", (: unload_module :));
    add_message_routine("update", (: update_module :));
    for (i = 0; i <= ((((get_system_info()["max_threads"] + 32) - 1) / 32) - 1); i++) {
        create_event(("mmd_block" + i));
    }  // end loop (verified)
    printf("Start MMD.all services.\n");

}

// --------------------------------------------------------------------
// @code  nc=45, nargs=0, insts=45
// --------------------------------------------------------------------
private void daemon() {
    /* ---- Enhanced Pseudocode ---- */
    mixed msg;

    set_as_daemon();
    printf("MMD.service daemon start.\n");

    // --- label L0007 ---
    while ((receive_from_queue(mmdQ, &msg) == 0)) {
        if ((msg == -1)) {
            printf("MMD.service daemon stop.\n");
            return;
        }
        if ((!is_mapping(msg))) {
            printf("Bad operation(%O) in MMD queue, ingored.\n", msg);
            continue;
        }
        try {
            process_request(msg);
        } catch { /* error handled */ }

        // --- label L0040 ---
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @code  nc=25, nargs=3, insts=25
// --------------------------------------------------------------------
private void post_request_done(array result, int tid, mixed result_map) {
    /* ---- Enhanced Pseudocode ---- */
    if (is_mapping(result_map)) {
        result[0] = result_map;
    }
    raise_event(("mmd_block" + (tid / 32)), (1 << (tid % 32)));

}

/* ================================================================
   DECLARED FUNCTIONS (no bytecode - likely inherited or forward decl)
   ================================================================ */
/*
   void post_request_async(mixed msg, function callback);  // inherited from parent or forward declaration
*/

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        12
   Code segments found:         12
   Successfully decoded:        12
   Unresolved/minimal:          0
   Functions without code:      1
   Inherited programs:          3
      - /feature/daemon/register.c()
      - /feature/common/dbase.c()
      - /feature/common/module_maintain.c()

   Note: Functions without code are usually inherited from the
   parent object (), forward declarations, or
   simple property accessor methods that the compiler optimizes away.
   ================================================================ */