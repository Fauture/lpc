/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_cfg_get_ddb_list.o
 * Source: /aaa/daemons/server/cmd_cfg_get_ddb_list.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 1 names, 1 code segments found
 * Strings: 6, Variables: 0
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
// @code  nc=62, nargs=3, insts=62
// --------------------------------------------------------------------
void main(object agent, int type, string name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ddb_list;
    mixed ddb_info;

    if (((type != 4) && (type != 9))) {
        return;
    }
    if ((type == 4)) {
        ddb_info = "/aaa/daemons/configd.c"->get_ddb_info_by_dist(agent->query("dist"));
        if ((!is_mapping(ddb_info))) {
            return;
        }
        ddb_list = ({ ddb_info });
    } else {
        ddb_list = "/aaa/daemons/configd.c"->get_ddb_list_by_spa(name);
        if ((!is_array(ddb_list))) {
            return;
        }
    }
    agent->send_message(9335, ddb_list);

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