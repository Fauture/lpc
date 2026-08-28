/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: updatehook.o
 * Source: /feature/common/updatehook.c
 * Inherits: (none)
 * Functions: 3 names, 3 code segments found
 * Strings: 1, Variables: 1
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   array hook_lists;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void destruct();
void register_update_hook(function func);
private void create();

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   hook_lists = ({ });

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=14, nargs=0, insts=14
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    if (is_clone()) {
        return;
    }
    register_daemon_data(object_name(this_object(), 1), "hook_lists", hook_lists);

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=1, insts=9
// --------------------------------------------------------------------
void register_update_hook(function func) {
    /* ---- Enhanced Pseudocode ---- */
    hook_lists += ({ func });

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=43, nargs=0, insts=43
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;
    mixed func;

    if (is_clone()) {
        return;
    }
    m = get_daemon_data(object_name(this_object(), 1));
    if ((is_mapping(m) && is_mapping(m["hook_lists"]))) {
        hook_lists = m["hook_lists"];
    }
    clean_array(hook_lists, 9);
    loop (func in hook_lists) {
        (*func)(this_object());
    }  // end loop (verified)

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        3
   Code segments found:         3
   Successfully decoded:        3
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */