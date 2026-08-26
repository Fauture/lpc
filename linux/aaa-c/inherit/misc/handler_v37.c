/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: handler.o
 * Source: /inherit/misc/handler.c
 * Inherits: (none)
 * Functions: 3 names, 3 code segments found
 * Strings: 4, Variables: 1
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   int failed_times;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void add_failed_times(int times = 1);
int get_failed_times();
private void create(mapping m);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   failed_times = 0;

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=10, nargs=1, insts=10
// --------------------------------------------------------------------
void add_failed_times(int times) {
    /* ---- Enhanced Pseudocode ---- */
    failed_times += times;
    return 1;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int get_failed_times() {
    /* ---- Enhanced Pseudocode ---- */
    return failed_times;

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=35, nargs=1, insts=35
// --------------------------------------------------------------------
private void create(mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    mixed module;
    mixed arr;

    if ((this_object() == find_object("/inherit/misc/handler.c"))) {
        return ([ ]);
    }
    module = m["module"];
    arr = explode(object_name(), "/");
    module->register_entry(arr[<1][0..<3], this_object());

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