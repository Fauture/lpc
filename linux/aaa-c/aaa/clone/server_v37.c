/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: server.o
 * Source: /aaa/clone/server.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/agent.c(), /feature/common/ob_source.c()
 * Functions: 4 names, 4 code segments found
 * Strings: 2, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/agent.c";
inherit "/feature/common/ob_source.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

int is_server();
private void destruct();
int is_test_agent();
private void create(mapping m);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int is_server() {
    /* ---- Enhanced Pseudocode ---- */
    return 1;

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
// @brief  predicate: boolean check
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
int is_test_agent() {
    /* ---- Enhanced Pseudocode ---- */
    return is_test_dist(query("dist"));

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=14, nargs=1, insts=14
// --------------------------------------------------------------------
private void create(mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!is_clone())) {
        return;
    }
    replace_entire_dbase(m);
    set_cmd_module("/aaa/daemons/serverd.c");

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        4
   Code segments found:         4
   Successfully decoded:        4
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          2
      - /inherit/misc/agent.c()
      - /feature/common/ob_source.c()
   ================================================================ */