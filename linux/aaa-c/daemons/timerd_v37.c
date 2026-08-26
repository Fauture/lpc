/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: timerd.o
 * Source: /daemons/timerd.c
 * Inherits: (none)
 * Functions: 6 names, 6 code segments found
 * Strings: 6, Variables: 1
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   mapping timer_map;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void register_timer(string name, int start_time, int end_time, int interval, function f);
private void timer_handler(string name);
private void destruct();
private int cal_timer_period(int start_time, int end_time, int interval);
private void create();
void remove_timer(string name);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   timer_map = ([ ]);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=64, nargs=5, insts=64
// --------------------------------------------------------------------
void register_timer(string name, int start_time, int end_time, int interval, function f) {
    /* ---- Enhanced Pseudocode ---- */
    mixed period;
    mapping temp;

    if ((!is_undefined(timer_map[name]))) {
        remove_timer(name);
    }
    period = cal_timer_period(start_time, end_time, interval);
    if ((period == -1)) {
        return;
    }
    temp = (timer_map[name] = ([ ]));
    temp["start_time"] = start_time;
    temp["end_time"] = end_time;
    temp["interval"] = interval;
    temp["callback"] = f;
    set_timer(period, name, (: timer_handler, name :));

}

// --------------------------------------------------------------------
// @code  nc=63, nargs=1, insts=63
// --------------------------------------------------------------------
private void timer_handler(string name) {
    /* ---- Enhanced Pseudocode ---- */
    mapping timer_info;
    mixed period;

    delete_timer(name);
    timer_info = timer_map[name];
    if ((!is_mapping(timer_info))) {
        return;
    }
    try {
        (*timer_info["callback"])(name);
    } catch { /* error handled */ }

    // --- label L0027 ---
    period = cal_timer_period((int)timer_info["start_time"], (int)timer_info["end_time"], (int)timer_info["interval"]);
    if ((period == -1)) {
        map_delete(timer_map, name);
        return;
    }
    set_timer(period, name, (: timer_handler, name :));

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=9, nargs=0, insts=9
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    register_daemon_data("/daemons/timerd.c", "timer_map", timer_map);

}

// --------------------------------------------------------------------
// @code  nc=40, nargs=3, insts=40
// --------------------------------------------------------------------
private int cal_timer_period(int start_time, int end_time, int interval) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cur_time;
    mixed next_time;

    cur_time = time();
    if ((cur_time >= end_time)) {
        return -1;
    }
    if ((start_time > cur_time)) {
        return (start_time - cur_time);
    }
    next_time = (cur_time + interval);
    if ((next_time > end_time)) {
        return (interval - (next_time - end_time));
    }
    return interval;

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    unregister_daemon_data("/daemons/timerd.c", "timer_map");

}

// --------------------------------------------------------------------
// @code  nc=17, nargs=1, insts=17
// --------------------------------------------------------------------
void remove_timer(string name) {
    /* ---- Enhanced Pseudocode ---- */
    if (is_undefined(timer_map[name])) {
        return;
    }
    map_delete(timer_map, name);
    delete_timer(name);

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        6
   Code segments found:         6
   Successfully decoded:        6
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */