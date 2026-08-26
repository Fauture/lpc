/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: module_maintain.o
 * Source: /feature/common/module_maintain.c
 * Inherits: (none)
 * Functions: 9 names, 9 code segments found
 * Strings: 6, Variables: 1
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   mapping message_map;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void set_mm_path(string new_path);
private void do_init_after_loaded();
mixed query_message_routine(string msg);
int delete_message_routine(string msg);
int add_message_routine(string msg, function f);
mapping get_message_map();
private void create();
mixed process_msg(string msg, mixed para);
mixed get_mm_path();

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   message_map = ([ ]);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=10, nargs=1, insts=10
// --------------------------------------------------------------------
void set_mm_path(string new_path) {
    /* ---- Enhanced Pseudocode ---- */
    this_object()->set("mm_path", new_path);

}

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=28, nargs=0, insts=28
// --------------------------------------------------------------------
private void do_init_after_loaded() {
    /* ---- Enhanced Pseudocode ---- */
    mixed mm_path;

    mm_path = this_object()->get_mm_path();
    if (((!is_string(mm_path)) || (!strlen(mm_path)))) {
        return;
    }
    "/daemons/mmd.c"->register_entry(mm_path, object_name(this_object(), 1));

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
mixed query_message_routine(string msg) {
    /* ---- Enhanced Pseudocode ---- */
    return message_map[msg];

}

// --------------------------------------------------------------------
// @code  nc=10, nargs=1, insts=10
// --------------------------------------------------------------------
int delete_message_routine(string msg) {
    /* ---- Enhanced Pseudocode ---- */
    map_delete(message_map, msg);
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=12, nargs=2, insts=12
// --------------------------------------------------------------------
int add_message_routine(string msg, function f) {
    /* ---- Enhanced Pseudocode ---- */
    message_map[msg] = f;
    return 1;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
mapping get_message_map() {
    /* ---- Enhanced Pseudocode ---- */
    return message_map;

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=10, nargs=0, insts=10
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    if ((!is_clone())) {
        register_post_init((: do_init_after_loaded :));
    }

}

// --------------------------------------------------------------------
// @code  nc=25, nargs=2, insts=25
// --------------------------------------------------------------------
mixed process_msg(string msg, mixed para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed f;

    f = query_message_routine(msg);
    if ((!is_function(f))) {
        return 0;
    }
    if (is_mapping(para)) {
        return (*f)(para);
    }
    return (*f)();

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=9, nargs=0, insts=9
// --------------------------------------------------------------------
mixed get_mm_path() {
    /* ---- Enhanced Pseudocode ---- */
    return this_object()->query("mm_path");

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        9
   Code segments found:         9
   Successfully decoded:        9
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */