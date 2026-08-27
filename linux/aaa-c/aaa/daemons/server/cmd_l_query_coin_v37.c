/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_query_coin.o
 * Source: /aaa/daemons/server/cmd_l_query_coin.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 11, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void main(object agent, string dist, string server, string gid, string account, string para1, string para2);
private void query_coin(mixed agent, string dist, string server, string gid, string account, string para1, string para2);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=24, nargs=7, insts=24
// --------------------------------------------------------------------
void main(object agent, string dist, string server, string gid, string account, string para1, string para2) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: query_coin, agent, dist, server, gid, account, para1, para2 :));

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=95, nargs=7, insts=95
// --------------------------------------------------------------------
private void query_coin(mixed agent, string dist, string server, string gid, string account, string para1, string para2) {
    /* ---- Enhanced Pseudocode ---- */
    mixed checksum;
    mapping account_info;
    mapping coin_info;

    if ((!is_object(agent))) {
        return;
    }
    account_info = "/aaa/daemons/accountd.c"->query_account_info(account);
    if ((!is_object(agent))) {
        return;
    }
    if ((!is_mapping(account_info))) {
        agent->send_message(25459, gid, account, para1, para2, 0, 0, 0, save_string(([ ])));
        return;
    }
    checksum = "/aaa/daemons/accountd.c"->calc_account_checksum(account_info);
    if (((account_info["checksum"] != "") && (stricmp(checksum, account_info["checksum"]) != 0))) {
        return;
    }
    coin_info = "/aaa/daemons/accountd.c"->get_coin_info(account_info);
    if ((!is_object(agent))) {
        return;
    }
    agent->send_message(25459, gid, account, para1, para2, coin_info["have_pwd"], coin_info["ul_ti"], 604800, save_string(coin_info));

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        2
   Code segments found:         2
   Successfully decoded:        2
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          1
      - /inherit/misc/handler.c()
   ================================================================ */