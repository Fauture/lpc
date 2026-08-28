/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: msg_cfg_my_info.o
 * Source: /daemons/config/msg_cfg_my_info.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 1 names, 1 code segments found
 * Strings: 8, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void main(object agent, string name, int port, string dist, int id, int dist_id, array list);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=60, nargs=7, insts=60
// --------------------------------------------------------------------
void main(object agent, string name, int port, string dist, int id, int dist_id, array list) {
    /* ---- Enhanced Pseudocode ---- */
    mixed setting;
    mixed pair;

    setting = ([ ]);
    "/daemons/configd.c"->set_config("my_dist", dist);
    "/daemons/configd.c"->set_config("my_name", name);
    "/daemons/configd.c"->set_config("my_port", port);
    "/daemons/configd.c"->set_config("my_id", id);
    "/daemons/configd.c"->set_config("dist_id", dist_id);
    loop (pair in list) {
        setting[pair[0]] = pair[1];
    }  // end loop (verified)
    "/daemons/configd.c"->set_config("my_setting", setting);

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