/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_cfg_get_ahserver_info.o
 * Source: /aaa/daemons/server/cmd_cfg_get_ahserver_info.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 1 names, 1 code segments found
 * Strings: 8, Variables: 0
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
// @code  nc=68, nargs=3, insts=68
// --------------------------------------------------------------------
void main(object agent, int type, string name) {
    /* ---- Enhanced Pseudocode ---- */
    mapping ahserver_info;

    if ((type != 5)) {
        return;
    }
    ahserver_info = "/aaa/daemons/configd.c"->get_ahserver_info(agent->query("ahserver"));
    if ((((((!is_mapping(ahserver_info)) || (!is_string(ahserver_info["ip"]))) || (!is_int(ahserver_info["port"]))) || (ahserver_info["ip"] == "")) || (ahserver_info["port"] == 0))) {
        return;
    }
    agent->send_message(9333, ahserver_info["ip"], ahserver_info["port"], ahserver_info["ahserver"]);

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