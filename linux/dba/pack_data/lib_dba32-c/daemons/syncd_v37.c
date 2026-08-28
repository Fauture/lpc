/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: syncd.o
 * Source: /daemons/syncd.c
 * Inherits: (none)
 * Functions: 10 names, 10 code segments found
 * Strings: 6, Variables: 0
 *********************************************************************/

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void delete_mutex_by_remote(string name);
mixed take_mutex_by_remote(string name, string by, int timeout, int hold_time);
int give_mutex_by_remote(string name, string by);
private int try_take(mapping m, string by, int hold_time);
private void post_request_to_thread(string name, string by, int timeout, int hold_time, int cookie, int method, string from, mapping stat);
private void take_mutex_by_remote_in_thread(array req);
private void destruct();
int create_mutex_by_remote(string name);
private mixed refresh_mutex(string name);
private void create();

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=23, nargs=1, insts=23
// --------------------------------------------------------------------
void delete_mutex_by_remote(string name) {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;

    if ((!is_mapping(m = all_mutex[name]))) {
        return;
    }
    map_delete(all_mutex, name);
    delete_event(m["event"]);

}

// --------------------------------------------------------------------
// @code  nc=37, nargs=4, insts=37
// --------------------------------------------------------------------
mixed take_mutex_by_remote(string name, string by, int timeout, int hold_time) {
    /* ---- Enhanced Pseudocode ---- */
    mixed m;

    m = refresh_mutex(name);
    if ((!is_mapping(m))) {
        return 0;
    }
    if (try_take(m, by, hold_time)) {
        return 1;
    }
    if ((timeout <= 0)) {
        return 0;
    }
    return (: post_request_to_thread, name, by, timeout, hold_time :);

}

// --------------------------------------------------------------------
// @code  nc=72, nargs=2, insts=72
// --------------------------------------------------------------------
int give_mutex_by_remote(string name, string by) {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;

    m = refresh_mutex(name);
    if ((!is_mapping(m))) {
        return 0;
    }
    if ((m["locked"] == 0)) {
        return 0;
    }
    if ((m["owner"] != by)) {
        return 0;
    }
    --m["locked"];
    if ((m["locked"] <= 0)) {
        m["locked"] = 0;
        m["owner"] = 0;
        m["release"] = 0;
        raise_event(m["event"], 1);
    }
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=59, nargs=3, insts=59
// --------------------------------------------------------------------
private int try_take(mapping m, string by, int hold_time) {
    /* ---- Enhanced Pseudocode ---- */
    if ((m["locked"] == 0)) {
        m["owner"] = by;
        m["locked"] = 1;
        m["release"] = (time() + hold_time);
        return 1;
    }
    if ((m["owner"] == by)) {
        ++m["locked"];
        m["release"] += hold_time;
        return 1;
    }
    return 0;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=36, nargs=8, insts=36
// --------------------------------------------------------------------
private void post_request_to_thread(string name, string by, int timeout, int hold_time, int cookie, int method, string from, mapping stat) {
    /* ---- Enhanced Pseudocode ---- */
    mixed tid;

    tid = create_thread(0, (: take_mutex_by_remote_in_thread :), ({ name, by, (time() + timeout), hold_time, cookie, method, from, stat }));
    if ((tid == -1)) {
        "/daemons/rpcd.c"->send_result(cookie, method, from, 0, stat);
        return;
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=121, nargs=1, insts=121
// --------------------------------------------------------------------
private void take_mutex_by_remote_in_thread(array req) {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;
    mixed timeout;
    mixed release;

    while (1) {
        m = refresh_mutex(req[0]);
        if ((!is_mapping(m))) {
            "/daemons/rpcd.c"->send_result(req[4], req[5], req[6], 0, req[7]);
            return;
        }
        if ((req[2] <= time())) {
            "/daemons/rpcd.c"->send_result(req[4], req[5], req[6], 0, req[7]);
            return;
        }
        if (try_take(m, req[1], req[3])) {
            "/daemons/rpcd.c"->send_result(req[4], req[5], req[6], 1, req[7]);
            return;
        }
        timeout = (req[2] - time());
        release = (m["release"] - time());
        if ((timeout > release)) {
            timeout = release;
        }
        wait_event(m["event"], 1, 0, timeout);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=4, nargs=0, insts=4
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @code  nc=29, nargs=1, insts=29
// --------------------------------------------------------------------
int create_mutex_by_remote(string name) {
    /* ---- Enhanced Pseudocode ---- */
    if (is_mapping(all_mutex[name])) {
        return 0;
    }
    all_mutex[name] = ([ "owner": 0, "release": 0, "locked": 0, "event": create_event(name) ]);
    return 0;

}

// --------------------------------------------------------------------
// @code  nc=41, nargs=1, insts=41
// --------------------------------------------------------------------
private mixed refresh_mutex(string name) {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;

    if ((!is_mapping(m = all_mutex[name]))) {
        return 0;
    }
    if ((m["release"] <= time())) {
        m["release"] = 0;
        m["locked"] = 0;
        m["owner"] = 0;
    }
    return m;

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=4, nargs=0, insts=4
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        10
   Code segments found:         10
   Successfully decoded:        10
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */