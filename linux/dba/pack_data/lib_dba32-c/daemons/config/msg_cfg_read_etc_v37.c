/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: msg_cfg_read_etc.o
 * Source: /daemons/config/msg_cfg_read_etc.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 1 names, 1 code segments found
 * Strings: 2, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void main(object agent, int cookie, int result);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=3, insts=10
// --------------------------------------------------------------------
void main(object agent, int cookie, int result) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/configd.c"->msg_cfg_read_etc(cookie, result);

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
      - /inherit/misc/handler.c()
   ================================================================ */