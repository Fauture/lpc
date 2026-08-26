/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_add_coin.o
 * Source: /aaa/daemons/server/cmd_l_add_coin.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 11 names, 11 code segments found
 * Strings: 64, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void do_when_coin_trade_add_ok(string account, int add, string para2);
private void success_notice(string account, int add, string order_id, string dist);
private void _main(mixed agent, string dist, string server, string gid, string account, int add_silver_coin, int add_gold_coin, int add_limit_coin, string cookie, string para1, string para2, string type);
private void _do_when_coin_trade_add_ok(string account, int add, int coin_trade_type);
private int log_add_gold_coin(int cno, string server, string dist, string account, string gid, int add_gold_coin, int cur_gold_coin, int cur_silver_coin, string para, string type);
int add_coin(int cno, string gid, string account, string server, string dist, string cookie, string para2, string type, int add_silver_coin, int add_gold_coin, int add_limit_coin, mixed &result, mapping account_info, mixed fields);
private int log_coin_common(int cno, string gid, string account, string server, string dist, int gold_coin_left, int silver_coin_left, int add_gold_coin, int add_silver_coin, int cur_gold_coin, int cur_silver_coin, string para);
private int log_gbuy_coin(int cno, string server, string dist, string account, string gid, int gold_coin_before, int silver_coin_before, int add_gold_coin, int add_silver_coin, int gold_coin_after, int silver_coin_after, string para);
void main(object agent, string dist, string server, string gid, string account, int add_silver_coin, int add_gold_coin, int add_limit_coin, string cookie, string para1, string para2, string type);
private int prepare_add_salary(int cno, string account, string gid, string cookie, int &result);
private int log_coin_trade(int cno, string server, string dist, string account, string gid, int add, int gold_coin_before, int gold_coin_after, string para);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=54, nargs=3, insts=54
// --------------------------------------------------------------------
private void do_when_coin_trade_add_ok(string account, int add, string para2) {
    /* ---- Enhanced Pseudocode ---- */
    mixed dst_account;
    mapping m;

    try {
        m = restore_value(para2);
    } catch { /* error handled */ }

    // --- label L0012 ---
    switch (m["log_type"]) {
        case 0:
        dst_account = account;
        break;
        case 1:
        dst_account = m["owner_account"];
        break;
        default:
    }  // end switch

    // --- label L0034 ---
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(dst_account) ? dst_account[0] : 0), (: _do_when_coin_trade_add_ok, dst_account, add, m["log_type"] :));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=33, nargs=4, insts=33
// --------------------------------------------------------------------
private void success_notice(string account, int add, string order_id, string dist) {
    /* ---- Enhanced Pseudocode ---- */
    mapping ret;

    ret = "/aaa/daemons/coin_traded.c"->cointrad_success_notice(account, ctime(time(), 3), add, order_id, dist);
    if (ret["result"]) {
        "/daemons/logd.c"->log_local(1, "coin_trade_success", "sell_coin_success", account, "", order_id, save_string(ret));
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=228, nargs=12, insts=228
// --------------------------------------------------------------------
private void _main(mixed agent, string dist, string server, string gid, string account, int add_silver_coin, int add_gold_coin, int add_limit_coin, string cookie, string para1, string para2, string type) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed add;
    int cur_silver_coin;
    int cur_gold_coin;
    int result;
    int cur_limit_trade_coin;
    mapping m;
    mixed memo;
    string failed_reason;
    mixed func;

    memo = sprintf("add_silver_coin=%d, add_gold_coin=%d, cookie=%s, para1=%s, para2=%s, type=%s", add_silver_coin, add_gold_coin, cookie, para1, para2, type);
    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno == -1)) {
        agent->send_message(25465, gid, account, cookie, para1, para2, 2, add_silver_coin, add_gold_coin, 0, 0, 0);
        "/daemons/logd.c"->log_local(1, "add_coin", "db_disconnected", account, gid, server, memo);
        return;
    }
    add = maximum(add_silver_coin, add_gold_coin);
    func = (: call_other, "/aaa/daemons/server/cmd_l_add_coin.c", "add_coin", cno, gid, account, server, dist, cookie, para2, type, add_silver_coin, add_gold_coin, add_limit_coin :);
    m = "/aaa/daemons/account_transactiond.c"->do_account_transaction(cno, account, "account", func);
    if ((!is_mapping(m))) {
        result = 2;
        cur_silver_coin = 0;
        cur_gold_coin = 0;
        cur_limit_trade_coin = 0;
        failed_reason = (is_string(m) ? m : "do_transaction_failed");
    } else {
        result = (int)m["result"];
        add_silver_coin = (int)m["add_silver_coin"];
        add_gold_coin = (int)m["add_gold_coin"];
        cur_silver_coin = (int)m["cur_silver_coin"];
        cur_gold_coin = (int)m["cur_gold_coin"];
        cur_limit_trade_coin = (int)m["limit_trade_coin"];
        failed_reason = (string)m["reason"];
    }
    if ((0 != result)) {
        "/daemons/logd.c"->log_local(1, "add_coin", "add_coin_failed", account, gid, server, (memo + sprintf(", result=%O, reason=%O, sql_err=%s", result, failed_reason, (*f_get_error)(cno))));
    }
    if ((!is_object(agent))) {
        return;
    }
    agent->send_message(25465, gid, account, cookie, para1, para2, result, add_silver_coin, add_gold_coin, cur_silver_coin, cur_gold_coin, cur_limit_trade_coin);
    if (((0 == result) && ("coin_trade" == type))) {
        do_when_coin_trade_add_ok(account, add, para2);
    }

}

// --------------------------------------------------------------------
// @brief  internal handler: sub-action processor
// @code  nc=106, nargs=3, insts=106
// --------------------------------------------------------------------
private void _do_when_coin_trade_add_ok(string account, int add, int coin_trade_type) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mapping coin_trade_info;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno == -1)) {
        catch(error("update_coin_trade error."));

        // --- label L0021 ---
        return;
    }
    coin_trade_info = "/aaa/daemons/account_transactiond.c"->query_account_coin_trade_info(cno, account);
    if ((((!is_mapping(coin_trade_info)) || ((int)coin_trade_info["gold_coin"] < add)) || (coin_trade_info["checksum"] != "/aaa/daemons/account_transactiond.c"->calc_account_coin_trade_checksum(coin_trade_info)))) {
        catch(error("update_coin_trade error."));

        // --- label L0062 ---
        return;
    }
    coin_trade_info["gold_coin"] = (string)((int)coin_trade_info["gold_coin"] - add);
    coin_trade_info["checksum"] = "/aaa/daemons/account_transactiond.c"->calc_account_coin_trade_checksum(coin_trade_info);
    if ((!"/aaa/daemons/account_transactiond.c"->update_account_coin_trade_info(cno, coin_trade_info))) {
        catch(error("update_coin_trade error."));

        // --- label L0101 ---
        return;
    }

}

// --------------------------------------------------------------------
// @code  nc=52, nargs=10, insts=52
// --------------------------------------------------------------------
private int log_add_gold_coin(int cno, string server, string dist, string account, string gid, int add_gold_coin, int cur_gold_coin, int cur_silver_coin, string para, string type) {
    /* ---- Enhanced Pseudocode ---- */
    mixed sql_cmd;
    string name;

    switch (type) {
        case "fishing":
        name = "Â–“£online≤∂”„ªÓ∂Ø";
        break;
        default:
        return 0;
    }  // end switch

    // --- label L0014 ---
    sql_cmd = mysql_bind_parameters("insert into add_gold_coin_log(gid, account, server, dist, cost_time, cost_platform, cost_coin, item_price, buy_item, buy_item_amount, silver_coin_cost, silver_coin_left, gold_coin_cost, gold_coin_left, cur_state, uid, para1, para2, para3, gs_memo) VALUES(?, ?, ?, ?, date_format(CURRENT_TIMESTAMP, '%Y%m%d%H%i%s'), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", gid, account, server, dist, 0, 0, 0, name, 0, 0, cur_silver_coin, add_gold_coin, cur_gold_coin, 0, "", 0, "", "", para);
    if (((*f_execute)(cno, sql_cmd) < 1)) {
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=309, nargs=14, insts=309
// --------------------------------------------------------------------
int add_coin(int cno, string gid, string account, string server, string dist, string cookie, string para2, string type, int add_silver_coin, int add_gold_coin, int add_limit_coin, mixed &result, mapping account_info, mixed fields) {
    /* ---- Enhanced Pseudocode ---- */
    int ret;
    int cur_silver_coin;
    int cur_gold_coin;
    mixed silver_coin_left;
    mixed gold_coin_left;
    mixed limit_trade_coin_left;
    int cur_limit_trade_coin;
    mixed log_ret;

    cur_silver_coin = 0;
    cur_gold_coin = 0;
    ret = 2;
    if ((add_silver_coin < 0)) {
        add_silver_coin = 0;
    }
    if ((add_gold_coin < 0)) {
        add_gold_coin = 0;
    }
    if ((("salary" == type) && (!prepare_add_salary(cno, account, gid, cookie, &ret)))) {
        result = ([ "result": ret, "reason": "add_salary_failed" ]);
        return 0;
    }
    silver_coin_left = atoi(account_info["silver_coin"]);
    if (((2147483647 - add_silver_coin) >= silver_coin_left)) {
        cur_silver_coin = (silver_coin_left + add_silver_coin);
    } else {
        cur_silver_coin = 2147483647;
    }
    gold_coin_left = atoi(account_info["gold_coin"]);
    if (((2147483647 - add_gold_coin) >= gold_coin_left)) {
        cur_gold_coin = (gold_coin_left + add_gold_coin);
    } else {
        cur_gold_coin = 2147483647;
    }
    if ((add_limit_coin < 0)) {
        add_limit_coin = 0;
    }
    limit_trade_coin_left = atoi(account_info["limit_trade_coin"]);
    if (((2147483647 - add_limit_coin) >= limit_trade_coin_left)) {
        cur_limit_trade_coin = (limit_trade_coin_left + add_limit_coin);
    } else {
        cur_limit_trade_coin = 2147483647;
    }
    account_info["silver_coin"] = (string)cur_silver_coin;
    account_info["gold_coin"] = (string)cur_gold_coin;
    account_info["limit_trade_coin"] = (string)cur_limit_trade_coin;
    if ((!"/aaa/daemons/account_transactiond.c"->update_account_info(cno, account_info, fields))) {
        result = ([ "result": ret, "reason": "update_account_failed" ]);
        return 0;
    }
    switch (type) {
        case "coin_trade":
        if ((add_silver_coin > 0)) {
            log_ret = log_coin_trade(cno, server, dist, account, gid, add_silver_coin, silver_coin_left, cur_silver_coin, para2);
        } else {
            log_ret = log_coin_trade(cno, server, dist, account, gid, add_gold_coin, gold_coin_left, cur_gold_coin, para2);
        }
        break;
        case "group_buying":
        log_ret = log_gbuy_coin(cno, server, dist, account, gid, gold_coin_left, silver_coin_left, add_gold_coin, add_silver_coin, cur_gold_coin, cur_silver_coin, para2);
        break;
        default:
        if (((add_gold_coin > 0) && (type == "fishing"))) {
            log_ret = log_add_gold_coin(cno, server, dist, account, gid, add_gold_coin, cur_gold_coin, cur_silver_coin, para2, type);
            if ((!log_ret)) {
                break;
            }
        }
        log_ret = log_coin_common(cno, gid, account, server, dist, gold_coin_left, silver_coin_left, add_gold_coin, add_silver_coin, cur_gold_coin, cur_silver_coin, para2);
        break;
    }  // end switch

    // --- label L0275 ---
    if ((!log_ret)) {
        result = ([ "result": ret, "reason": "write_log_failed" ]);
        return 0;
    }
    result = ([ "result": 0, "add_silver_coin": add_silver_coin, "add_gold_coin": add_gold_coin, "cur_silver_coin": cur_silver_coin, "cur_gold_coin": cur_gold_coin, "limit_trade_coin": cur_limit_trade_coin ]);
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=35, nargs=12, insts=35
// --------------------------------------------------------------------
private int log_coin_common(int cno, string gid, string account, string server, string dist, int gold_coin_left, int silver_coin_left, int add_gold_coin, int add_silver_coin, int cur_gold_coin, int cur_silver_coin, string para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed sql_cmd;

    sql_cmd = mysql_bind_parameters("insert into add_coin_log(gid, account, server, dist, add_time, silver_coin_left, gold_coin_left, add_silver_coin, add_gold_coin, cur_silver_coin, cur_gold_coin, gs_memo) values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", gid, account, server, dist, ctime(time(), 3), silver_coin_left, gold_coin_left, add_silver_coin, add_gold_coin, cur_silver_coin, cur_gold_coin, para);
    if (((*f_execute)(cno, sql_cmd) < 1)) {
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=69, nargs=12, insts=69
// --------------------------------------------------------------------
private int log_gbuy_coin(int cno, string server, string dist, string account, string gid, int gold_coin_before, int silver_coin_before, int add_gold_coin, int add_silver_coin, int gold_coin_after, int silver_coin_after, string para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed sql_cmd;
    mapping m;

    try {
        m = restore_value(para);
    } catch { /* error handled */ }

    // --- label L0012 ---
    if ((!is_mapping(m))) {
        return 0;
    }
    sql_cmd = mysql_bind_parameters("insert into gbuy_coin_log(update_time, server, account, gid, name, type, gold_coin, silver_coin, gcoin_before, gcoin_after, scoin_before, scoin_after, gbuy_gcoin_before, gbuy_gcoin_after, gbuy_scoin_before, gbuy_scoin_after) values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) ", ctime(time(), 3), server, account, gid, m["char_name"], m["log_type"], add_gold_coin, add_silver_coin, gold_coin_before, gold_coin_after, silver_coin_before, silver_coin_after, m["gbuy_gcoin_before"], m["gbuy_gcoin_after"], m["gbuy_scoin_before"], m["gbuy_scoin_after"]);
    if (((*f_execute)(cno, sql_cmd) < 1)) {
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=29, nargs=12, insts=29
// --------------------------------------------------------------------
void main(object agent, string dist, string server, string gid, string account, int add_silver_coin, int add_gold_coin, int add_limit_coin, string cookie, string para1, string para2, string type) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _main, agent, dist, server, gid, account, add_silver_coin, add_gold_coin, add_limit_coin, cookie, para1, para2, type :));

}

// --------------------------------------------------------------------
// @code  nc=149, nargs=5, insts=149
// --------------------------------------------------------------------
private int prepare_add_salary(int cno, string account, string gid, string cookie, int &result) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cookie_existed;
    int last_add_coin_time;
    mixed cur_add_coin_time;
    mixed last_add_coin_info;
    mixed sql_cmd;
    mixed last_add_coin_cookie;
    mixed new_add_coin_cookie;
    mapping m;

    sql_cmd = mysql_bind_parameters("select last_add_coin_cookie from relative_acc where account = ?", account);
    m = mysql_select_single_result(cno, sql_cmd);
    cookie_existed = (is_mapping(m) ? 1 : 0);
    if (((!is_mapping(m)) || (strlen(m["last_add_coin_cookie"]) < 1))) {
        last_add_coin_time = 0;
    } else {
        last_add_coin_cookie = m["last_add_coin_cookie"];
        last_add_coin_info = explode(last_add_coin_cookie, "/");
        last_add_coin_time = atoi(last_add_coin_info[0]);
        last_add_coin_info = last_add_coin_info[1..<1];
    }
    cur_add_coin_time = atoi(cookie);
    if (((cur_add_coin_time < last_add_coin_time) || ((cur_add_coin_time == last_add_coin_time) && (member_array(gid, last_add_coin_info) >= 0)))) {
        result = 3;
        return 0;
    }
    if ((last_add_coin_time == cur_add_coin_time)) {
        new_add_coin_cookie = ((last_add_coin_cookie + "/") + gid);
    } else {
        new_add_coin_cookie = ((cookie + "/") + gid);
    }
    if (cookie_existed) {
        sql_cmd = mysql_bind_parameters("update relative_acc set last_add_coin_cookie = ? where account = ?", new_add_coin_cookie, account);
    } else {
        sql_cmd = mysql_bind_parameters("insert into relative_acc(account, last_add_coin_cookie) values(?, ?)", account, new_add_coin_cookie);
    }
    if (((*f_execute)(cno, sql_cmd) < 1)) {
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=63, nargs=9, insts=63
// --------------------------------------------------------------------
private int log_coin_trade(int cno, string server, string dist, string account, string gid, int add, int gold_coin_before, int gold_coin_after, string para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed sql_cmd;
    mapping m;

    try {
        m = restore_value(para);
    } catch { /* error handled */ }

    // --- label L0012 ---
    if ((!is_mapping(m))) {
        return 0;
    }
    sql_cmd = mysql_bind_parameters("insert into coin_trade_log(update_time, server, account, gid, name, order_id, type, amount, gold_coin_before, gold_coin_after) values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?) ", ctime(time(), 3), server, account, gid, m["name"], (m["order_id"] ? m["order_id"] : 0), m["log_type"], add, gold_coin_before, gold_coin_after);
    if (((*f_execute)(cno, sql_cmd) < 1)) {
        return 0;
    }
    return 1;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        11
   Code segments found:         11
   Successfully decoded:        11
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          1
      - /inherit/misc/handler.c()
   ================================================================ */