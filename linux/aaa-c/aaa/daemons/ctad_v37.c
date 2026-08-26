/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: ctad.o
 * Source: /aaa/daemons/ctad.c
 * Inherits: (none)
 * Inherited programs: /feature/daemon/handler_module.c()
 * Functions: 1 names, 1 code segments found
 * Strings: 1, Variables: 0
 *********************************************************************/

inherit "/feature/daemon/handler_module.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void create();

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=7, nargs=0, insts=7
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    load_all_handlers("/aaa/daemons/cta/");

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        1
   Code segments found:         1
   Successfully decoded:        1
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          1
      - /feature/daemon/handler_module.c()
   ================================================================ */