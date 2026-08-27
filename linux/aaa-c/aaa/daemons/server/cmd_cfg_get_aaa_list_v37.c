/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_cfg_get_aaa_list.o
 * Source: /aaa/daemons/server/cmd_cfg_get_aaa_list.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 1 names, 1 code segments found
 * Strings: 7, Variables: 0
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
// @code  nc=69, nargs=3, insts=69
// --------------------------------------------------------------------
void main(object agent, int type, string name) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    mixed aaa_list;
    mapping aaa_info;
    mixed aaa_map;

    if (((type != 5) && (type != 18))) {
        return;
    }
    aaa_map = "/aaa/daemons/configd.c"->get_aaa_list();
    if ((!is_mapping(aaa_map))) {
        return;
    }
    aaa_list = allocate_array(sizeof(aaa_map));
    i = 0;
    loop (aaa_info in values(aaa_map)) {
        aaa_list[i++] = ({ aaa_info["name"], aaa_info["ip"], aaa_info["port"], aaa_info["attrib"] });
    }  // end loop (verified)
    agent->send_message(9337, aaa_list);

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