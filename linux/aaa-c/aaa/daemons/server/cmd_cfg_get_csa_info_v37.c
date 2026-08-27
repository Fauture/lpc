/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_cfg_get_csa_info.o
 * Source: /aaa/daemons/server/cmd_cfg_get_csa_info.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 1 names, 1 code segments found
 * Strings: 8, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void main(object agent, int type, string name, string dummy_dist);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=66, nargs=4, insts=66
// --------------------------------------------------------------------
void main(object agent, int type, string name, string dummy_dist) {
    /* ---- Enhanced Pseudocode ---- */
    mapping csa_info;

    if (((type != 7) && (type != 16))) {
        return;
    }
    csa_info = "/aaa/daemons/configd.c"->get_csa_info_by_dist(agent->query("dist"));
    if ((((((!is_mapping(csa_info)) || (!is_string(csa_info["ip"]))) || (csa_info["ip"] == "")) || (!is_int(csa_info["port"]))) || (csa_info["port"] == 0))) {
        return;
    }
    agent->send_message(9341, ({ csa_info }));

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