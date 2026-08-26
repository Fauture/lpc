/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_cfg_get_ccs_info.o
 * Source: /aaa/daemons/server/cmd_cfg_get_ccs_info.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 1 names, 1 code segments found
 * Strings: 9, Variables: 0
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
// @code  nc=78, nargs=3, insts=78
// --------------------------------------------------------------------
void main(object agent, int type, string name) {
    /* ---- Enhanced Pseudocode ---- */
    mapping ccs_info;

    if ((((type != 5) && (type != 4)) && (type != 9))) {
        return;
    }
    ccs_info = "/aaa/daemons/configd.c"->get_ccs_info_by_dist(agent->query("dist"));
    if ((((((!is_mapping(ccs_info)) || (!is_string(ccs_info["ip"]))) || (!is_int(ccs_info["port"]))) || (ccs_info["ip"] == "")) || (ccs_info["port"] == 0))) {
        return;
    }
    agent->send_message(9063, ccs_info["ip"], ccs_info["port"], ccs_info["ccs"]);

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