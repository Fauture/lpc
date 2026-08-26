/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: msg_cfg_spa_info.o
 * Source: /daemons/config/msg_cfg_spa_info.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 1 names, 1 code segments found
 * Strings: 6, Variables: 0
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
// @code  nc=34, nargs=2, insts=34
// --------------------------------------------------------------------
void main(object agent, array list) {
    /* ---- Enhanced Pseudocode ---- */
    mapping spa;

    if ((sizeof(list) <= 0)) {
        return;
    }
    spa = list[0];
    "/aaa/daemons/configd.c"->set_config("spa_info", ([ "ip": spa["ip"], "port": spa["port"], "name": spa["name"] ]));

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