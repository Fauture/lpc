/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_cost_coin.o
 * Source: /aaa/daemons/server/cmd_l_cost_coin.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 10 names, 10 code segments found
 * Strings: 95, Variables: 4
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   mapping forbid_limit_coin;
   /* mixed global_0; */
   int log_timeout;
   int log_timeout_status;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void do_when_coin_trade_add_ok(string account, int cost, string para2);
private void success_notice(string account, int cost, string order_id, string dist, string gid);
private void _main(mixed agent, string dist, string server, string gid, string account, int cost, string cookie, string para1, string para2, string coin_type, int cur_state, string uid, int send_time);
int cost_coin(int cno, string gid, string account, string server, string dist, int cost, string cookie, string para2, string coin_type, int cur_state, string uid, int test_flag, mixed &result, mapping account_info, mixed fields);
private void _do_when_coin_trade_add_ok(string account, int cost, int coin_trade_type);
private int log_coin_common(int cno, string gid, string account, string server, string dist, int cost, int silver_coin_cost, int silver_coin_left, int gold_coin_cost, int gold_coin_left, int cur_state, string uid, string para);
private int log_gbuy_coin(int cno, string server, string dist, string account, string gid, int gold_coin_before, int silver_coin_before, int add_gold_coin, int add_silver_coin, int gold_coin_after, int silver_coin_after, string para);
void main(object agent, string dist, string server, string gid, string account, int cost, string cookie, string para1, string para2, string coin_type, int cur_state, string uid, int send_time);
private int log_coin_red_packets(int cno, string dist, string server, string gid, string account, int cost, int gold_coin_before, int gold_coin_after, string para, string coin_type);
private int log_coin_trade(int cno, string dist, string server, string gid, string account, int cost, int gold_coin_before, int gold_coin_after, string para);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   log_timeout = 15;  // /aaa/daemons/server/cmd_l_cost_coin.c:20
   log_timeout_status = 1;  // /aaa/daemons/server/cmd_l_cost_coin.c:24
   forbid_limit_coin = ([ "coin_trade" : 1, "hongbhxxt" : 1, "tanxss" : 2, "wendcbg" : 2 ]);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=54, nargs=3, insts=54
// --------------------------------------------------------------------
private void do_when_coin_trade_add_ok(string account, int cost, string para2) {
    /* ---- Enhanced Pseudocode ---- */
    mixed dst_account;
    mapping m;

    try {
        m = restore_value(para2);
    } catch { /* error handled */ }

    // --- label L0012 ---
    switch (m["log_type"]) {
        case 2:
        dst_account = account;
        break;
        case 3:
        dst_account = m["owner_account"];
        break;
        default:
    }  // end switch

    // --- label L0034 ---
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(dst_account) ? dst_account[0] : 0), (: _do_when_coin_trade_add_ok, dst_account, cost, m["log_type"] :));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=33, nargs=5, insts=33
// --------------------------------------------------------------------
private void success_notice(string account, int cost, string order_id, string dist, string gid) {
    /* ---- Enhanced Pseudocode ---- */
    mapping ret;

    ret = "/aaa/daemons/coin_traded.c"->cointrad_success_notice(account, ctime(time(), 3), cost, order_id, dist);
    if (ret["result"]) {
        "/daemons/logd.c"->log_local(1, "coin_trade_success", "sell_coin_to_buyer", account, gid, order_id, save_string(ret));
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=319, nargs=13, insts=319
// --------------------------------------------------------------------
private void _main(mixed agent, string dist, string server, string gid, string account, int cost, string cookie, string para1, string para2, string coin_type, int cur_state, string uid, int send_time) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    int result;
    mixed start_time;
    mixed end_time;
    mapping m;
    mixed memo;
    string failed_reason;
    mapping log_m;
    mixed func;

    memo = sprintf("cost=%d, cookie=%s, para1=%s, para2=%s, coin_type=%s, cur_state=%d, uid=%s", cost, cookie, para1, para2, coin_type, cur_state, uid);
    if ((!is_object(agent))) {
        "/daemons/logd.c"->log_local(1, "cost_coin", "agent_disconnected", account, gid, server, memo);
        return;
    }
    start_time = time();
    log_m = ([ "cost": cost, "cookie": cookie, "para1": para1, "para2": para2, "coin_type": coin_type, "cur_state": cur_state, "uid": uid, "send_ti": send_time, "exec_st": start_time ]);
    log_m["recv_timeout"] = (start_time - send_time);
    if ((log_timeout_status && (log_m["recv_timeout"] >= log_timeout))) {
        "/daemons/logd.c"->log_local(1, "cost_coin", "recv_timeout", account, gid, server, save_string(log_m));
    }
    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno == -1)) {
        agent->send_message(25461, gid, account, 0, 0, 0, 0, 0, para1, para2);
        "/daemons/logd.c"->log_local(1, "cost_coin", "db_disconnected", account, gid, server, memo);
        return;
    }
    func = (: call_other, "/aaa/daemons/server/cmd_l_cost_coin.c", "cost_coin", cno, gid, account, server, dist, cost, cookie, para2, coin_type, cur_state, uid, (int)agent->is_test_agent() :);
    m = "/aaa/daemons/account_transactiond.c"->do_account_transaction(cno, account, "account", func);
    end_time = time();
    log_m["exec_et"] = end_time;
    log_m["exec_timeout"] = (end_time - start_time);
    if ((log_timeout_status && (log_m["exec_timeout"] >= log_timeout))) {
        "/daemons/logd.c"->log_local(1, "cost_coin", "exec_timeout", account, gid, server, save_string(log_m));
    }
    if ((!is_mapping(m))) {
        result = 0;
        m = ([ "silver_coin": 0, "gold_coin": 0, "silver_coin_cost": 0, "gold_coin_cost": 0, "trade_coin": 0, "last_trade_coin": "", "consum_coin": 0, "last_consum_coin": "", "cookie": cookie, "limit_trade_coin": 0 ]);
        failed_reason = (is_string(m) ? m : "do_transaction_failed");
    } else {
        result = (int)m["result"];
        failed_reason = (string)m["reason"];
    }
    if ((!result)) {
        "/daemons/logd.c"->log_local(1, "cost_coin", "cost_failed", account, gid, server, (memo + sprintf(", result=%O, reason=%O, sql_err=%s", result, failed_reason, (*f_get_error)(cno))));
        "/aaa/daemons/account_transactiond.c"->register_cost_fail(account);
    } else {
        "/aaa/daemons/account_transactiond.c"->unregister_cost_fail(account);
    }
    if ((!is_object(agent))) {
        return;
    }
    agent->send_message(25461, gid, account, result, para1, para2, save_string(m));
    if (((1 == result) && ("coin_trade" == cookie))) {
        do_when_coin_trade_add_ok(account, cost, para2);
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=492, nargs=15, insts=492
// --------------------------------------------------------------------
int cost_coin(int cno, string gid, string account, string server, string dist, int cost, string cookie, string para2, string coin_type, int cur_state, string uid, int test_flag, mixed &result, mapping account_info, mixed fields) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ti;
    int silver_coin;
    int gold_coin;
    int gold_coin_cost;
    int silver_coin_cost;
    int limit_trade_coin_cost;
    int gold_coin_before;
    int silver_coin_before;
    int limit_trade_coin;
    mixed log_ret;

    gold_coin = atoi(account_info["gold_coin"]);
    silver_coin = atoi(account_info["silver_coin"]);
    silver_coin_cost = 0;
    gold_coin_cost = 0;
    gold_coin_before = gold_coin;
    silver_coin_before = silver_coin;
    if ((strlen(coin_type) < 1)) {
        if (((gold_coin + silver_coin) >= cost)) {
            if ((silver_coin >= cost)) {
                silver_coin_cost = cost;
            } else {
                silver_coin_cost = silver_coin;
                gold_coin_cost = (cost - silver_coin);
            }
        } else {
            gold_coin_cost = gold_coin;
            silver_coin_cost = (cost - gold_coin_cost);
        }
    } else if ((!stricmp(coin_type, "silver_coin"))) {
        silver_coin_cost = cost;
    } else if ((!stricmp(coin_type, "gold_coin"))) {
        gold_coin_cost = cost;
    }
    gold_coin -= gold_coin_cost;
    silver_coin -= silver_coin_cost;
    if ((((gold_coin_cost > 0) && (gold_coin < 0)) || ((silver_coin_cost > 0) && (silver_coin < 0)))) {
        catch(error("the number of left coin is negative!\n"));

        // --- label L0132 ---
    }
    switch (cookie) {
        case "coin_trade":
        ti = time();
        if (is_same_day(ti, time(account_info["last_trade_coin"]))) {
            account_info["trade_coin"] = (string)((atoi(account_info["trade_coin"]) + gold_coin_cost) + silver_coin_cost);
        } else {
            account_info["trade_coin"] = (string)(gold_coin_cost + silver_coin_cost);
        }
        account_info["last_trade_coin"] = ctime(ti, 3);
        break;
        case "group_buying":
        break;
        default:
        ti = time();
        if (is_same_day(ti, time(account_info["last_consum_coin"]))) {
            account_info["consum_coin"] = (string)((atoi(account_info["consum_coin"]) + gold_coin_cost) + silver_coin_cost);
        } else {
            account_info["consum_coin"] = (string)(gold_coin_cost + silver_coin_cost);
        }
        account_info["last_consum_coin"] = ctime(ti, 3);
        break;
    }  // end switch

    // --- label L0229 ---
    limit_trade_coin = atoi(account_info["limit_trade_coin"]);
    limit_trade_coin_cost = 0;
    if ((!forbid_limit_coin[cookie])) {
        if (test_flag) {
            limit_trade_coin -= silver_coin_cost;
        } else {
            limit_trade_coin -= gold_coin_cost;
        }
        if ((limit_trade_coin < 0)) {
            limit_trade_coin = 0;
        }
        limit_trade_coin_cost = (atoi(account_info["limit_trade_coin"]) - limit_trade_coin);
        account_info["limit_trade_coin"] = (string)limit_trade_coin;
    } else if ((forbid_limit_coin[cookie] == 2)) {
        if ((!test_flag)) {
            limit_trade_coin -= gold_coin_cost;
        }
        if ((limit_trade_coin < 0)) {
            limit_trade_coin = 0;
        }
        limit_trade_coin_cost = (atoi(account_info["limit_trade_coin"]) - limit_trade_coin);
        account_info["limit_trade_coin"] = (string)limit_trade_coin;
    }
    account_info["gold_coin"] = (string)gold_coin;
    account_info["silver_coin"] = (string)silver_coin;
    account_info["trade_lock_time"] = "";
    result = ([ "silver_coin": silver_coin, "gold_coin": gold_coin, "gold_coin_cost": gold_coin_cost, "silver_coin_cost": silver_coin_cost, "trade_coin": atoi(account_info["trade_coin"]), "last_trade_coin": account_info["last_trade_coin"], "consum_coin": atoi(account_info["consum_coin"]), "last_consum_coin": account_info["last_consum_coin"], "cookie": cookie, "limit_trade_coin": limit_trade_coin, "limit_trade_coin_cost": limit_trade_coin_cost ]);
    if ((!"/aaa/daemons/account_transactiond.c"->update_account_info(cno, account_info, fields))) {
        result << ([ "result": 0, "reason": "update_account_failed" ]);
        return 0;
    }
    switch (cookie) {
        case "coin_trade":
        log_ret = log_coin_trade(cno, dist, server, gid, account, cost, gold_coin_before, gold_coin, para2);
        break;
        case "group_buying":
        log_ret = log_gbuy_coin(cno, server, dist, account, gid, gold_coin_before, silver_coin_before, gold_coin_cost, silver_coin_cost, gold_coin, silver_coin, para2);
        break;
        case "hongbhxxt":
        log_ret = log_coin_red_packets(cno, dist, server, gid, account, cost, gold_coin_before, gold_coin, para2, coin_type);
        break;
        default:
        log_ret = log_coin_common(cno, gid, account, server, dist, cost, silver_coin_cost, silver_coin, gold_coin_cost, gold_coin, cur_state, uid, para2);
        break;
    }  // end switch

    // --- label L0468 ---
    if ((!log_ret)) {
        result << ([ "result": 0, "reason": "write_log_failed" ]);
        return 0;
    }
    result["result"] = 1;
    return 1;

}

// --------------------------------------------------------------------
// @brief  internal handler: sub-action processor
// @code  nc=134, nargs=3, insts=134
// --------------------------------------------------------------------
private void _do_when_coin_trade_add_ok(string account, int cost, int coin_trade_type) {
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
    if ((!is_mapping(coin_trade_info))) {
        coin_trade_info = ([ "account": account, "gold_coin": cost ]);
        coin_trade_info["checksum"] = "/aaa/daemons/account_transactiond.c"->calc_account_coin_trade_checksum(coin_trade_info);
        if ((!"/aaa/daemons/account_transactiond.c"->insert_account_coin_trade_info(cno, coin_trade_info))) {
            catch(error("update_coin_trade error."));

            // --- label L0068 ---
            return;
        }
        return;
    }
    if ((coin_trade_info["checksum"] != "/aaa/daemons/account_transactiond.c"->calc_account_coin_trade_checksum(coin_trade_info))) {
        catch(error("update_coin_trade error."));

        // --- label L0090 ---
        return;
    }
    coin_trade_info["gold_coin"] = (string)((int)coin_trade_info["gold_coin"] + cost);
    coin_trade_info["checksum"] = "/aaa/daemons/account_transactiond.c"->calc_account_coin_trade_checksum(coin_trade_info);
    if ((!"/aaa/daemons/account_transactiond.c"->update_account_coin_trade_info(cno, coin_trade_info))) {
        catch(error("update_coin_trade error."));

        // --- label L0129 ---
        return;
    }

}

// --------------------------------------------------------------------
// @code  nc=95, nargs=13, insts=95
// --------------------------------------------------------------------
private int log_coin_common(int cno, string gid, string account, string server, string dist, int cost, int silver_coin_cost, int silver_coin_left, int gold_coin_cost, int gold_coin_left, int cur_state, string uid, string para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed sql_cmd;
    mixed str;
    int key;
    mixed value;
    mixed gs_memo;
    mapping m;

    m = ([ ]);
    loop (str in explode(para, "/")) {
        if ((sscanf(str, "%s=%s", &key, &value) < 2)) {
            continue;
        }
        m[key] = value;
    }  // end loop (verified)
    if ((!is_string(gs_memo = m["gs_memo"]))) {
        gs_memo = para;
    }
    sql_cmd = mysql_bind_parameters("insert into cost_log(gid, account, server, dist, cost_time, cost_platform, cost_coin, item_price, buy_item, buy_item_amount, silver_coin_cost, silver_coin_left, gold_coin_cost, gold_coin_left, cur_state, uid, para1, para2, para3, gs_memo) VALUES(?, ?, ?, ?, date_format(CURRENT_TIMESTAMP, '%Y%m%d%H%i%s'), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", gid, account, server, dist, 0, cost, m["price"], m["name"], m["amount"], silver_coin_cost, silver_coin_left, gold_coin_cost, gold_coin_left, cur_state, uid, (string)m["level"], "", "", gs_memo);
    if (((*f_execute)(cno, sql_cmd) < 1)) {
        printf("log_coin_common mysql_execute failed.%s\n", (*f_get_error)(cno));
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=78, nargs=12, insts=78
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
        printf("log_gbuy_coin restore_value failed.\n");
        return 0;
    }
    sql_cmd = mysql_bind_parameters("insert into gbuy_coin_log(update_time, server, account, gid, name, type, gold_coin, silver_coin, gcoin_before, gcoin_after, scoin_before, scoin_after, gbuy_gcoin_before, gbuy_gcoin_after, gbuy_scoin_before, gbuy_scoin_after) values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) ", ctime(time(), 3), server, account, gid, m["char_name"], m["log_type"], add_gold_coin, add_silver_coin, gold_coin_before, gold_coin_after, silver_coin_before, silver_coin_after, m["gbuy_gcoin_before"], m["gbuy_gcoin_after"], m["gbuy_scoin_before"], m["gbuy_scoin_after"]);
    if (((*f_execute)(cno, sql_cmd) < 1)) {
        printf("log_gbuy_coin mysql_execute failed.%s", (*f_get_error)(cno));
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=35, nargs=13, insts=35
// --------------------------------------------------------------------
void main(object agent, string dist, string server, string gid, string account, int cost, string cookie, string para1, string para2, string coin_type, int cur_state, string uid, int send_time) {
    /* ---- Enhanced Pseudocode ---- */
    "/aaa/daemons/account_transactiond.c"->delete_precost_lock(account);
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _main, agent, dist, server, gid, account, cost, cookie, para1, para2, coin_type, cur_state, uid, send_time :));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=76, nargs=10, insts=76
// --------------------------------------------------------------------
private int log_coin_red_packets(int cno, string dist, string server, string gid, string account, int cost, int gold_coin_before, int gold_coin_after, string para, string coin_type) {
    /* ---- Enhanced Pseudocode ---- */
    mixed sql_cmd;
    mapping m;

    try {
        m = restore_value(para);
    } catch { /* error handled */ }

    // --- label L0012 ---
    if ((!is_mapping(m))) {
        printf("log_coin_red_packets restore_value failed.\n");
        return 0;
    }
    sql_cmd = mysql_bind_parameters("insert into coin_red_packets_log(update_time, server, account, gid, name, type, amount, gold_coin_before, gold_coin_after, para2) values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?) ", ctime(time(), 3), server, account, gid, m["name"], m["log_type"], cost, gold_coin_before, gold_coin_after, coin_type);
    if (((*f_execute)(cno, sql_cmd) < 1)) {
        printf("log_coin_red_packets mysql_execute failed.%s\n", (*f_get_error)(cno));
        return 0;
    }
    "/daemons/threadd.c"->dispatch(random(10), (: success_notice, account, cost, "hongbhxxt", dist, gid :));
    return 1;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=97, nargs=9, insts=97
// --------------------------------------------------------------------
private int log_coin_trade(int cno, string dist, string server, string gid, string account, int cost, int gold_coin_before, int gold_coin_after, string para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed sql_cmd;
    mapping m;

    try {
        m = restore_value(para);
    } catch { /* error handled */ }

    // --- label L0012 ---
    if ((!is_mapping(m))) {
        printf("log_coin_trade restore_value failed.\n");
        return 0;
    }
    sql_cmd = mysql_bind_parameters("insert into coin_trade_log(update_time, server, account, gid, name, order_id, type, amount, gold_coin_before, gold_coin_after) values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?) ", ctime(time(), 3), server, account, gid, m["name"], m["order_id"], m["log_type"], cost, gold_coin_before, gold_coin_after);
    if (((*f_execute)(cno, sql_cmd) < 1)) {
        printf("log_coin_trade mysql_execute failed.%s\n", (*f_get_error)(cno));
        return 0;
    }
    do {
        if ((3 != m["log_type"])) {
            break;
        }
        if (("silver_coin" == m["coin_type"])) {
            break;
        }
        "/daemons/threadd.c"->dispatch(random(10), (: success_notice, account, cost, m["order_id"], dist, gid :));
    } while (0);

    // --- label L0093 ---
    return 1;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        10
   Code segments found:         10
   Successfully decoded:        10
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          1
      - /inherit/misc/handler.c()
   ================================================================ */