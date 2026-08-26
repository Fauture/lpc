/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_convert_silver_coin.o
 * Source: /aaa/daemons/server/cmd_l_convert_silver_coin.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 3 names, 3 code segments found
 * Strings: 21, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _main(mixed agent, string dist, string server, string gid, string account, string cookie, string para1, string para2, string type);
void main(object agent, string dist, string server, string gid, string account, string cookie, string para1, string para2, string type);
int convert_coin(int cno, string gid, string account, string server, string dist, string cookie, string para1, string para2, string type, mixed &result, mapping account_info, mixed fields);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=138, nargs=9, insts=138
// --------------------------------------------------------------------
private void _main(mixed agent, string dist, string server, string gid, string account, string cookie, string para1, string para2, string type) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    int convert_silver_coin;
    int cur_silver_coin;
    int cur_gold_coin;
    int result;
    int cur_limit_trade_coin;
    mapping m;
    mixed func;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno == -1)) {
        agent->send_message(8505, gid, account, cookie, para1, para2, 2, 0, 0, 0);
        return;
    }
    func = (: call_other, "/aaa/daemons/server/cmd_l_convert_silver_coin.c", "convert_coin", cno, gid, account, server, dist, cookie, para1, para2, type :);
    m = "/aaa/daemons/account_transactiond.c"->do_account_transaction(cno, account, "account", func);
    if ((!is_mapping(m))) {
        result = 2;
        convert_silver_coin = 0;
        cur_silver_coin = 0;
        cur_gold_coin = 0;
        cur_limit_trade_coin = 0;
    } else {
        result = (int)m["result"];
        convert_silver_coin = (int)m["convert_silver_coin"];
        cur_silver_coin = (int)m["cur_silver_coin"];
        cur_gold_coin = (int)m["cur_gold_coin"];
        cur_limit_trade_coin = (int)m["limit_trade_coin"];
    }
    if ((!is_object(agent))) {
        return;
    }
    agent->send_message(8505, gid, account, cookie, para1, para2, result, convert_silver_coin, cur_silver_coin, cur_gold_coin, cur_limit_trade_coin);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=26, nargs=9, insts=26
// --------------------------------------------------------------------
void main(object agent, string dist, string server, string gid, string account, string cookie, string para1, string para2, string type) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _main, agent, dist, server, gid, account, cookie, para1, para2, type :));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=118, nargs=12, insts=118
// --------------------------------------------------------------------
int convert_coin(int cno, string gid, string account, string server, string dist, string cookie, string para1, string para2, string type, mixed &result, mapping account_info, mixed fields) {
    /* ---- Enhanced Pseudocode ---- */
    mixed gold_coin_left;
    int convert_silver_coin;
    int cur_gold_coin;
    int ret;

    ret = 2;
    convert_silver_coin = atoi(account_info["silver_coin"]);
    if ((convert_silver_coin <= 0)) {
        result = ([ "result": 3, "convert_silver_coin": 0, "cur_silver_coin": atoi(account_info["silver_coin"]), "cur_gold_coin": atoi(account_info["gold_coin"]), "cur_limit_trade_coin": atoi(account_info["limit_trade_coin"]) ]);
        return 0;
    }
    gold_coin_left = atoi(account_info["gold_coin"]);
    if (((2147483647 - convert_silver_coin) >= gold_coin_left)) {
        cur_gold_coin = (gold_coin_left + convert_silver_coin);
    } else {
        cur_gold_coin = 2147483647;
    }
    account_info["silver_coin"] = "0";
    account_info["gold_coin"] = (string)cur_gold_coin;
    if ((!"/aaa/daemons/account_transactiond.c"->update_account_info(cno, account_info, fields))) {
        result = ([ "result": ret ]);
        return 0;
    }
    result = ([ "result": 0, "convert_silver_coin": convert_silver_coin, "cur_silver_coin": 0, "cur_gold_coin": cur_gold_coin, "cur_limit_trade_coin": atoi(account_info["limit_trade_coin"]) ]);
    return 1;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        3
   Code segments found:         3
   Successfully decoded:        3
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          1
      - /inherit/misc/handler.c()
   ================================================================ */