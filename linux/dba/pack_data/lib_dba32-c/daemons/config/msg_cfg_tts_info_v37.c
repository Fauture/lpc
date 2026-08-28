/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: msg_cfg_tts_info.o
 * Source: /daemons/config/msg_cfg_tts_info.c
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
// @code  nc=43, nargs=2, insts=43
// --------------------------------------------------------------------
void main(object agent, array list) {
    /* ---- Enhanced Pseudocode ---- */
    if (((1 || (!sizeof(list))) || (!is_mapping(list[0])))) {
        return;
    }
    "/daemons/configd.c"->set_config("tts_info", ([ "ip": list[0]["ip"], "port": list[0]["port"], "name": list[0]["name"] ]));

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