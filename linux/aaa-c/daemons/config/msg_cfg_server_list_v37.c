/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: msg_cfg_server_list.o
 * Source: /daemons/config/msg_cfg_server_list.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 1 names, 1 code segments found
 * Strings: 3, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void main(object agent, array list);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=2, insts=10
// --------------------------------------------------------------------
void main(object agent, array list) {
    /* ---- Enhanced Pseudocode ---- */
    "/aaa/daemons/configd.c"->set_config("game_server_list", list);

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