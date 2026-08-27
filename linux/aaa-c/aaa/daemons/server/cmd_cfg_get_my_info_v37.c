/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_cfg_get_my_info.o
 * Source: /aaa/daemons/server/cmd_cfg_get_my_info.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 1 names, 1 code segments found
 * Strings: 20, Variables: 0
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
// @code  nc=353, nargs=3, insts=353
// --------------------------------------------------------------------
void main(object agent, int type, string name) {
    /* ---- Enhanced Pseudocode ---- */
    int port;
    int id;
    mixed dist_id;
    int bad;
    mixed mks;
    mixed settings;
    mixed got_settings;
    string dist;
    mapping info;
    mixed dist_info;

    info = "/aaa/daemons/configd.c"->get_info_by_type(type, name);
    if ((!is_mapping(info))) {
        return;
    }
    if (((!is_int(info["port"])) || (!is_int(info["id"])))) {
        bad = 1;
    }
    if ((((!bad) && (((((type == 5) || (type == 18)) || (type == 7)) || (type == 4)) || (type == 19))) && ((!is_string(info["dist"])) && (info["dist"] == "")))) {
        bad = 1;
    }
    if (bad) {
        printf("[1;31m[config] ·þÎñÆ÷ %s È±ÉÙ»ù±¾ÅäÖÃÐÅÏ¢\n[2;37;0m", name);
        return;
    }
    dist = (string)info["dist"];
    port = (int)info["port"];
    id = (int)info["id"];
    if (((type == 7) && (id != 0))) {
        id = 0;
    }
    settings = ({});
    got_settings = ({});
    loop (mks in info) {
        if ((((((((((((mks[0] == "memo") || (mks[0] == "name")) || (mks[0] == "ip")) || (mks[0] == "alias")) || (mks[0] == "port")) || (mks[0] == "dist")) || (mks[0] == "server")) || (mks[0] == "ccs")) || (mks[0] == "dba")) || (mks[0] == "ldba")) || (mks[0] == "spa"))) {
            continue;
        }
        got_settings += ({ mks[0] });
        settings += ({ ({ mks[0], mks[1] }) });
    }  // end loop (verified)
    if (is_mapping(dist_info = "/aaa/daemons/configd.c"->get_dist_info(info["dist"]))) {
        loop (mks in dist_info) {
            if (((((((mks[0] == "memo") || (mks[0] == "name")) || (mks[0] == "dist")) || (mks[0] == "alias")) || (mks[0] == "service")) || (mks[0] == "spa"))) {
                continue;
            }
            if ((mks[0] == "id")) {
                dist_id = mks[1];
                settings += ({ ({ "dist_id", mks[1] }) });
                continue;
            }
            if ((member_array(mks[0], got_settings) >= 0)) {
                continue;
            }
            settings += ({ ({ mks[0], mks[1] }) });
        }  // end loop (verified)
    }
    if ((!is_int(dist_id))) {
        return;
    }
    agent->send_message(13157, name, port, dist, id, dist_id, settings);

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