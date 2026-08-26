/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_log_cost_coin.o
 * Source: /aaa/daemons/server/cmd_l_log_cost_coin.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 26, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _main(object agent, string dist, string server, string gid, string account, int cost_platform, int cost_coin, int item_price, string buy_item, int buy_item_amount, string para, string memo);
void main(object agent, string dist, string server, string gid, string account, int cost_platform, int cost_coin, int item_price, string buy_item, int buy_item_amount, string para, string memo);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=155, nargs=12, insts=155
// --------------------------------------------------------------------
private void _main(object agent, string dist, string server, string gid, string account, int cost_platform, int cost_coin, int item_price, string buy_item, int buy_item_amount, string para, string memo) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed log_memo;
    mixed sql_cmd;
    mapping m;

    log_memo = sprintf("cost_coin=%d, item_price=%d, buy_item=%s, buy_item_amount=%d", cost_coin, item_price, buy_item, buy_item_amount);
    if ((!is_object(agent))) {
        "/daemons/logd.c"->log_local(1, "log_cost_coin", "agent_disconnected", account, gid, server, log_memo);
        return;
    }
    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno == -1)) {
        "/daemons/logd.c"->log_local(1, "log_cost_coin", "db_disconnected", account, gid, server, log_memo);
        return;
    }
    try {
        m = restore_value(para);
    } catch { /* error handled */ }

    // --- label L0062 ---
    if ((0 || (!is_mapping(m)))) {
        "/daemons/logd.c"->log_local(1, "log_cost_coin", "bad_para", account, gid, server, ((log_memo + ", para=") + para));
        return;
    }
    sql_cmd = mysql_bind_parameters("insert into cost_log(gid, account, server, dist, cost_time, cost_platform, cost_coin, item_price, buy_item, buy_item_amount, silver_coin_cost, silver_coin_left, gold_coin_cost, gold_coin_left, cur_state, uid, para1, para2, para3, gs_memo) VALUES(?, ?, ?, ?, date_format(CURRENT_TIMESTAMP, '%Y%m%d%H%i%s'), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", gid, account, server, dist, cost_platform, cost_coin, item_price, buy_item, buy_item_amount, m["silver_coin_cost"], m["silver_coin_left"], m["gold_coin_cost"], m["gold_coin_left"], m["cur_state"], m["uid"], m["para1"], m["para2"], m["para3"], m["gs_memo"]);
    if (((*f_execute)(cno, sql_cmd) < 1)) {
        "/daemons/logd.c"->log_local(1, "log_cost_coin", "log_cost_failed", account, gid, server, (log_memo + sprintf(", sql_err=%s", (*f_get_error)(cno))));
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=29, nargs=12, insts=29
// --------------------------------------------------------------------
void main(object agent, string dist, string server, string gid, string account, int cost_platform, int cost_coin, int item_price, string buy_item, int buy_item_amount, string para, string memo) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _main, agent, dist, server, gid, account, cost_platform, cost_coin, item_price, buy_item, buy_item_amount, para, memo :));

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