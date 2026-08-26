/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_cfg_get_cta_list.o
 * Source: /aaa/daemons/server/cmd_cfg_get_cta_list.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 1 names, 1 code segments found
 * Strings: 3, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void main(object agent, int type, string name);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=27, nargs=3, insts=27
// --------------------------------------------------------------------
void main(object agent, int type, string name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cta_list;

    cta_list = "/aaa/daemons/configd.c"->get_cta_list();
    if ((!is_mapping(cta_list))) {
        return;
    }
    cta_list = values(cta_list);
    agent->send_message(9071, cta_list);

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