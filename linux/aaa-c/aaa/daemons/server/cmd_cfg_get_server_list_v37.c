/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_cfg_get_server_list.o
 * Source: /aaa/daemons/server/cmd_cfg_get_server_list.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 1 names, 1 code segments found
 * Strings: 15, Variables: 0
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
// @code  nc=154, nargs=3, insts=154
// --------------------------------------------------------------------
void main(object agent, int type, string name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ip;
    mixed server_list;
    mixed buf;
    mixed tmp1;
    mixed tmp2;
    mixed mks;

    if (((((type == 5) || (type == 7)) || (type == 4)) || (type == 16))) {
        server_list = "/aaa/daemons/configd.c"->get_server_list_by_dist(agent->query("dist"));
    } else {
        server_list = "/aaa/daemons/configd.c"->get_server_list();
    }
    if ((!is_mapping(server_list))) {
        return;
    }
    buf = pad_buffer(17247, sizeof(server_list));
    tmp1 = allocate_buffer(0);
    tmp2 = allocate_buffer(0);
    loop (mks in server_list) {
        ip = sprintf("%s,%s,%s,%d", mks[1]["ip"], mks[1]["ip_wt"], mks[1]["ip_dx"], (int)mks[1]["enable_multi_ip"]);
        buf += pad_buffer(mks[0], ip, mks[1]["port"]);
        tmp1 += pad_buffer(mks[1]["id"]);
        tmp2 += pad_buffer(mks[1]["enable"]);
    }  // end loop (verified)
    if (((type == 5) || (type == 7))) {
        send_to_client(agent->get_client_no(), ((buf + tmp1) + tmp2));
    } else {
        agent->send_message(17247, values(server_list));
    }

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