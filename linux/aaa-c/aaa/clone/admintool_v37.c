/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: admintool.o
 * Source: /aaa/clone/admintool.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/agent.c(), /feature/common/ob_source.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 1, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/agent.c";
inherit "/feature/common/ob_source.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void create();
int is_admintool();

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=11, nargs=0, insts=11
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    if ((!is_clone())) {
        return;
    }
    set_cmd_module("/aaa/daemons/admintoold.c");

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int is_admintool() {
    /* ---- Enhanced Pseudocode ---- */
    return 1;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        2
   Code segments found:         2
   Successfully decoded:        2
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          2
      - /inherit/misc/agent.c()
      - /feature/common/ob_source.c()
   ================================================================ */