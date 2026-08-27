/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_cfg_get_tts_info.o
 * Source: /aaa/daemons/server/cmd_cfg_get_tts_info.c
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
// @code  nc=71, nargs=3, insts=71
// --------------------------------------------------------------------
void main(object agent, int type, string name) {
    /* ---- Enhanced Pseudocode ---- */
    mapping tts_info;

    if ((((type != 5) && (type != 18)) && (type != 9))) {
        return;
    }
    tts_info = "/aaa/daemons/configd.c"->get_tts_info_by_dist(agent->query("dist"));
    if ((((((!is_mapping(tts_info)) || (!is_string(tts_info["ip"]))) || (tts_info["ip"] == "")) || (!is_int(tts_info["port"]))) || (tts_info["port"] == 0))) {
        return;
    }
    agent->send_message(9343, ({ tts_info }));

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