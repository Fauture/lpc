/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_query_coin.o
 * Source: /aaa/daemons/server/cmd_l_query_coin.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 18, Variables: 0
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
// @code  nc=225, nargs=7, insts=225
// --------------------------------------------------------------------
private void query_coin(mixed agent, string dist, string server, string gid, string account, string para1, string para2) {
    /* ---- Enhanced Pseudocode ---- */
    int unlock_time;
    int unlock_duration;
    int have_coin_password;
    mixed checksum;
    mapping account_info;
    mapping coin_info;

    if ((!is_object(agent))) {
        return;
    }
    coin_info = ([ ]);
    have_coin_password = (unlock_time = 0);
    unlock_duration = 604800;
    account_info = "/aaa/daemons/accountd.c"->query_account_info(account);
    if ((!is_object(agent))) {
        return;
    }
    if ((!is_mapping(account_info))) {
        agent->send_message(25459, gid, account, para1, para2, have_coin_password, unlock_time, unlock_duration, save_string(coin_info));
        return;
    }
    coin_info["gold_coin"] = atoi(account_info["gold_coin"]);
    coin_info["silver_coin"] = atoi(account_info["silver_coin"]);
    coin_info["trade_coin"] = atoi(account_info["trade_coin"]);
    coin_info["consum_coin"] = atoi(account_info["consum_coin"]);
    coin_info["last_trade_coin"] = account_info["last_trade_coin"];
    coin_info["last_consum_coin"] = account_info["last_consum_coin"];
    coin_info["limit_trade_coin"] = atoi(account_info["limit_trade_coin"]);
    unlock_time = account_info["unlock_coin_password_time"];
    if ((unlock_time != "")) {
        unlock_time = time_desc_to_int(unlock_time, 3);
    } else {
        unlock_time = 0;
    }
    have_coin_password = (is_string(account_info["coin_password"]) && (account_info["coin_password"] != ""));
    checksum = "/aaa/daemons/accountd.c"->calc_account_checksum(account_info);
    if (((account_info["checksum"] != "") && (stricmp(checksum, account_info["checksum"]) != 0))) {
        return;
    }
    if (((have_coin_password && (unlock_time > 0)) && ((unlock_time + 604800) < time()))) {
        account_info["unlock_coin_password_time"] = "";
        account_info["coin_password"] = "";
        if ("/aaa/daemons/accountd.c"->update_account_info(account_info)) {
            have_coin_password = 0;
        }
    }
    agent->send_message(25459, gid, account, para1, para2, have_coin_password, unlock_time, unlock_duration, save_string(coin_info));

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