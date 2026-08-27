/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_cfg_get_spa_info.o
 * Source: /aaa/daemons/admintool/cmd_cfg_get_spa_info.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 1 names, 1 code segments found
 * Strings: 4, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void main(object agent, int type, string name, string wanna_dist);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=44, nargs=4, insts=44
// --------------------------------------------------------------------
void main(object agent, int type, string name, string wanna_dist) {
    /* ---- Enhanced Pseudocode ---- */
    mixed spa_list;
    mixed specified_spa_list;
    mapping m;

    if ((!is_mapping(spa_list = "/aaa/daemons/configd.c"->get_spa_list()))) {
        return;
    }
    specified_spa_list = ({});
    loop (m in values(spa_list)) {
        if ((m["dist"] != wanna_dist)) {
            continue;
        }
        specified_spa_list += ({ m });
    }  // end loop (verified)
    agent->send_message(16629, specified_spa_list);

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