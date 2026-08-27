/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: scheduled.o
 * Source: /daemons/scheduled.c
 * Inherits: (none)
 * Functions: 14 names, 14 code segments found
 * Strings: 14, Variables: 2
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   array execute_list;
   mapping scheme_events;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

array query_execute_list();
private void init();
private void execute_event(string event_name);
private void destruct();
int update_scheme(string event_name, function f, int execute_time, mixed time_setting = 0, int repeat = 0);
mixed query_scheme_event(string event_name);
private void calc_event_execute_time(string event_name);
private void raise_scheme_event();
private void create();
mapping query_all_scheme_events();
int delete_scheme(string event_name);
private void daemon();
private void insert_execute_list(string event_name);
int create_scheme(string event_name, function f, int execute_time, mixed time_setting = 0, int repeat = 0);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   scheme_events = ([ ]);  // /daemons/scheduled.c:427
   execute_list = ({ });

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
array query_execute_list() {
    /* ---- Enhanced Pseudocode ---- */
    return execute_list;

}

// --------------------------------------------------------------------
// @code  nc=53, nargs=0, insts=53
// --------------------------------------------------------------------
private void init() {
    /* ---- Enhanced Pseudocode ---- */
    mixed m;
    mixed mks;

    m = get_daemon_data("/daemons/scheduled.c");
    if ((!is_mapping(m))) {
        return;
    }
    loop (mks in m) {
        if (is_function(mks[1][1])) {
            create_scheme(mks[0], mks[1][1], mks[1][3], mks[1][2], mks[1][0]);
        }
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=61, nargs=1, insts=61
// --------------------------------------------------------------------
private void execute_event(string event_name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed event;

    event = scheme_events[event_name];
    if (is_function(event[1])) {
        "/daemons/threadd.c"->dispatch_service(event[1]);
    } else {
        delete_scheme(event_name);
        execute_list -= ({ event_name });
        return;
    }
    execute_list -= ({ event_name });
    if ((!event[0])) {
        map_delete(scheme_events, event_name);
        unregister_daemon_data("/daemons/scheduled.c", event_name);
    } else {
        calc_event_execute_time(event_name);
        insert_execute_list(event_name);
    }

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=10, nargs=0, insts=10
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    debug_delete_thread("scheduled");
    delete_timer("scheme_timer");

}

// --------------------------------------------------------------------
// @code  nc=60, nargs=5, insts=60
// --------------------------------------------------------------------
int update_scheme(string event_name, function f, int execute_time, mixed time_setting, int repeat) {
    /* ---- Enhanced Pseudocode ---- */
    mixed event;

    if ((!is_array(event = scheme_events[event_name]))) {
        return 0;
    }
    event[0] = repeat;
    event[1] = f;
    event[2] = time_setting;
    event[3] = execute_time;
    unregister_daemon_data("/daemons/scheduled.c", event_name);
    register_daemon_data("/daemons/scheduled.c", event_name, event);
    execute_list -= ({ event_name });
    insert_execute_list(event_name);
    return 1;

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
mixed query_scheme_event(string event_name) {
    /* ---- Enhanced Pseudocode ---- */
    return scheme_events[event_name];

}

// --------------------------------------------------------------------
// @code  nc=134, nargs=1, insts=134
// --------------------------------------------------------------------
private void calc_event_execute_time(string event_name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed next_ti;
    mixed event;
    mapping lt;

    event = scheme_events[event_name];
    if (is_int(event[2])) {
        event[3] = (time() + event[2]);
    } else if (is_function(event[2])) {
        event[3] = (*event[2])();
    } else if (is_string(event[2])) {
        switch (event[2]) {
            case "per_year":
            lt = local_time();
            ++lt["year"];
            next_ti = mktime(lt);
            break;
            case "per_month":
            lt = local_time();
            ++lt["mon"];
            next_ti = mktime(lt);
            break;
            case "per_week":
            next_ti = (time() + 604800);
            break;
            case "per_day":
            next_ti = (time() + 86400);
            break;
            default:
            next_ti = time();
            break;
        }  // end switch

        // --- label L0116 ---
        event[3] = next_ti;
    }
    unregister_daemon_data("/daemons/scheduled.c", event_name);
    register_daemon_data("/daemons/scheduled.c", event_name, event);

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
private void raise_scheme_event() {
    /* ---- Enhanced Pseudocode ---- */
    raise_event("scheme_event", 1);

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=14, nargs=0, insts=14
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    create_thread("scheduled", "daemon");
    create_event("scheme_event");
    register_post_init((: init :));

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
mapping query_all_scheme_events() {
    /* ---- Enhanced Pseudocode ---- */
    return scheme_events;

}

// --------------------------------------------------------------------
// @code  nc=26, nargs=1, insts=26
// --------------------------------------------------------------------
int delete_scheme(string event_name) {
    /* ---- Enhanced Pseudocode ---- */
    if (is_undefined(scheme_events[event_name])) {
        return 0;
    }
    map_delete(scheme_events, event_name);
    execute_list -= ({ event_name });
    unregister_daemon_data("/daemons/scheduled.c", event_name);
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=100, nargs=0, insts=100
// --------------------------------------------------------------------
private void daemon() {
    /* ---- Enhanced Pseudocode ---- */
    mixed event;
    int next_time;

    set_as_daemon();
    next_time = 2147483647;

    // --- label L0008 ---
    while (1) {
        if ((sizeof(execute_list) != 0)) {
            event = scheme_events[execute_list[0]];

            // --- label L0023 ---
            while ((event[3] <= time())) {
                try {
                    execute_event(execute_list[0]);
                } catch { /* error handled */ }

                // --- label L0039 ---
                if ((sizeof(execute_list) != 0)) {
                    event = scheme_events[execute_list[0]];
                } else {
                    event = ({});
                    break;
                }
            }  // end loop (verified)
            if ((event != ({}))) {
                next_time = (event[3] - time());
                delete_timer("scheme_timer");
                set_timer(next_time, "scheme_timer", (: raise_scheme_event :));
            }
        }
        wait_event("scheme_event", 1, 0, 0);
        wait_event("scheme_event", 1, 0, next_time);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @code  nc=283, nargs=1, insts=283
// --------------------------------------------------------------------
private void insert_execute_list(string event_name) {
    /* ---- Enhanced Pseudocode ---- */
    int execute_time;
    mixed len;
    int pos;
    int org_pos;
    int flag;
    mixed event;
    mixed pre_event;

    execute_time = scheme_events[event_name][3];
    flag = 0;
    len = sizeof(execute_list);
    if (len) {
        do {
            pos = ((int)len / 2);
            org_pos = 0;
            event = scheme_events[execute_list[pos]];
            if ((1 == len)) {
                if ((event[3] <= execute_time)) {
                    execute_list += ({ event_name });
                } else {
                    execute_list = (({ event_name }) + execute_list);
                }
                break;
            }
            pre_event = scheme_events[execute_list[(pos - 1)]];

            // --- label L0075 ---
            while (1) {
                if (((event[3] >= execute_time) && (pre_event[3] <= execute_time))) {
                    execute_list = ((execute_list[0..(pos - 1)] + ({ event_name })) + execute_list[pos..<1]);
                    break;
                }
                if ((event[3] < execute_time)) {
                    org_pos = pos;
                    pos = ((int)(pos + len) / 2);
                    event = scheme_events[execute_list[pos]];
                    pre_event = scheme_events[execute_list[(pos - 1)]];
                    if (((pos == (len - 1)) && flag)) {
                        flag = 0;
                        execute_list += ({ event_name });
                        break;
                    }
                    if (((pos == (len - 1)) && (!flag))) {
                        flag = 1;
                    }
                } else if ((event[3] > execute_time)) {
                    pos = ((int)(pos + org_pos) / 2);
                    if ((!pos)) {
                        execute_list = (({ event_name }) + execute_list);
                        break;
                    }
                    event = scheme_events[execute_list[pos]];
                    pre_event = scheme_events[execute_list[(pos - 1)]];
                }
            }  // end loop (verified)
        } while (0);

        // --- label L0236 ---
    } else {
        execute_list += ({ event_name });
    }
    execute_time -= time();
    if ((execute_time <= 0)) {
        raise_event("scheme_event", 1);
        return;
    }
    if ((((!is_mapping(get_timer("scheme_timer"))) ? 2147483647 : get_timer("scheme_timer")["timeout"]) > execute_time)) {
        delete_timer("scheme_timer");
        set_timer(execute_time, "scheme_timer", (: raise_scheme_event :));
    }

}

// --------------------------------------------------------------------
// @code  nc=77, nargs=5, insts=77
// --------------------------------------------------------------------
int create_scheme(string event_name, function f, int execute_time, mixed time_setting, int repeat) {
    /* ---- Enhanced Pseudocode ---- */
    mixed event;

    if (is_array(scheme_events[event_name])) {
        if (is_function(scheme_events[event_name][1])) {
            return 0;
        }
        delete_scheme(event_name);

        // --- label L0023 ---
    }
    event = allocate_array(4);
    event[0] = repeat;
    event[1] = f;
    event[2] = time_setting;
    event[3] = execute_time;
    scheme_events[event_name] = event;
    insert_execute_list(event_name);
    register_daemon_data("/daemons/scheduled.c", event_name, event);
    scheme_events[event_name] = event;
    return 1;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        14
   Code segments found:         14
   Successfully decoded:        14
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */