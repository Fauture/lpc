/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_auth_refresh_pic.o
 * Source: /aaa/daemons/account/cmd_l_auth_refresh_pic.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 1 names, 1 code segments found
 * Strings: 2, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void main(mixed agent, int auth_index);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=2, insts=10
// --------------------------------------------------------------------
void main(mixed agent, int auth_index) {
    /* ---- Enhanced Pseudocode ---- */
    "/aaa/daemons/accountd.c"->refresh_matrix_pic(agent, auth_index);

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