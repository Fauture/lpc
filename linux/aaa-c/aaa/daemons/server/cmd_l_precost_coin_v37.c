/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_precost_coin.o
 * Source: /aaa/daemons/server/cmd_l_precost_coin.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 4 names, 4 code segments found
 * Strings: 39, Variables: 3
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   /* mixed global_0; */
   int log_timeout;
   int log_timeout_status;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

int precost_coin(int cno, string gid, string account, string server, string dist, int cost, string para1, string para2, string coin_password, string coin_type, string type, mixed &result, mapping account_info, mixed fields);
private mixed _main(mixed agent, string dist, string server, string gid, string account, int cost, string para1, string para2, string coin_password, string coin_type, string type, int send_time, int prc_cookie);
private void _main_thread(mixed agent, string dist, string server, string gid, string account, int cost, string para1, string para2, string coin_password, string coin_type, string type, int send_time, int prc_cookie);
void main(object agent, string dist, string server, string gid, string account, int cost, string para1, string para2, string coin_password, string coin_type, string type, int send_time, int prc_cookie);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   log_timeout = 15;  // /aaa/daemons/server/cmd_l_precost_coin.c:271
   log_timeout_status = 1;

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=229, nargs=14, insts=229
// --------------------------------------------------------------------
int precost_coin(int cno, string gid, string account, string server, string dist, int cost, string para1, string para2, string coin_password, string coin_type, string type, mixed &result, mapping account_info, mixed fields) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ti;
    int ret;
    mixed gold_coin;
    mixed silver_coin;
    mixed total_coin;
    mixed limit_trade_coin;

    ret = 0;
    do {
        if ((stricmp(account_info["coin_password"], coin_password) != 0)) {
            if (strlen(coin_password)) {
                ret = 3;
            } else {
                ret = 7;
            }
            break;
        }
        if ((((strlen(coin_type) > 0) && stricmp(coin_type, "gold_coin")) && stricmp(coin_type, "silver_coin"))) {
            ret = 6;
            break;
        }
        gold_coin = atoi(account_info["gold_coin"]);
        silver_coin = atoi(account_info["silver_coin"]);
        total_coin = (gold_coin + silver_coin);
        if (((type == "coin_trade") || (type == "hongbhxxt"))) {
            limit_trade_coin = atoi(account_info["limit_trade_coin"]);
            gold_coin -= limit_trade_coin;
            silver_coin -= limit_trade_coin;
            total_coin -= limit_trade_coin;
        }
        if (((type == "tanxss") || (type == "wendcbg"))) {
            limit_trade_coin = atoi(account_info["limit_trade_coin"]);
            silver_coin -= limit_trade_coin;
            total_coin -= limit_trade_coin;
        }
        if (((strlen(coin_type) < 1) && (total_coin < cost))) {
            break;
        }
        if ((cost < 0)) {
            break;
        }
        if (((!stricmp(coin_type, "gold_coin")) && (gold_coin < cost))) {
            ret = 4;
            break;
        }
        if (((!stricmp(coin_type, "silver_coin")) && (silver_coin < cost))) {
            ret = 5;
            break;
        }
        ti = time();
        if (((ti - atoi(account_info["trade_lock_time"])) < 60)) {
            ret = 2;
            break;
        }
        account_info["trade_lock_time"] = (string)ti;
        if ((!"/aaa/daemons/account_transactiond.c"->update_account_info(cno, account_info))) {
            break;
        }
        ret = 1;
    } while (0);

    // --- label L0217 ---
    result = ([ "result": ret ]);
    return (ret == 1);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=184, nargs=13, insts=184
// --------------------------------------------------------------------
private mixed _main(mixed agent, string dist, string server, string gid, string account, int cost, string para1, string para2, string coin_password, string coin_type, string type, int send_time, int prc_cookie) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    int result;
    mixed start_time;
    mixed end_time;
    mapping m;
    mapping log_m;
    mixed func;

    if ((!is_object(agent))) {
        return;
    }
    start_time = time();
    log_m = ([ "cost": cost, "para1": para1, "para2": para2, "coin_type": coin_type, "type": type, "send_ti": send_time, "exec_st": start_time ]);
    log_m["recv_timeout"] = (start_time - send_time);
    if ((log_timeout_status && (log_m["recv_timeout"] >= log_timeout))) {
        "/daemons/logd.c"->log_local(1, "precost_coin", "recv_timeout", account, gid, server, save_string(log_m));
    }
    if (((cno = "/daemons/dbd.c"->assure_connection("adb")) == -1)) {
        agent->send_message(25463, gid, account, "", 0, para1, para2, coin_password, send_time, prc_cookie);
        return 0;
    }
    func = (: call_other, "/aaa/daemons/server/cmd_l_precost_coin.c", "precost_coin", cno, gid, account, server, dist, cost, para1, para2, coin_password, coin_type, type :);
    m = "/aaa/daemons/account_transactiond.c"->do_account_transaction(cno, account, "account", func);
    end_time = time();
    log_m["exec_et"] = end_time;
    log_m["exec_timeout"] = (end_time - start_time);
    if ((log_timeout_status && (log_m["exec_timeout"] >= log_timeout))) {
        "/daemons/logd.c"->log_local(1, "precost_coin", "exec_timeout", account, gid, server, save_string(log_m));
    }
    if ((!is_mapping(m))) {
        result = 0;
    } else {
        result = (int)m["result"];
    }
    agent->send_message(25463, gid, account, "", result, para1, para2, coin_password, send_time, prc_cookie);
    return result;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=43, nargs=13, insts=43
// --------------------------------------------------------------------
private void _main_thread(mixed agent, string dist, string server, string gid, string account, int cost, string para1, string para2, string coin_password, string coin_type, string type, int send_time, int prc_cookie) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ret;

    try {
        ret = _main(agent, dist, server, gid, account, cost, para1, para2, coin_password, coin_type, type, send_time, prc_cookie);
    } catch { /* error handled */ }

    // --- label L0024 ---
    if ((1 == ret)) {
        "/aaa/daemons/account_transactiond.c"->set_precost_lock(account, time());
    } else {
        "/aaa/daemons/account_transactiond.c"->delete_precost_lock(account);
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=63, nargs=13, insts=63
// --------------------------------------------------------------------
void main(object agent, string dist, string server, string gid, string account, int cost, string para1, string para2, string coin_password, string coin_type, string type, int send_time, int prc_cookie) {
    /* ---- Enhanced Pseudocode ---- */
    if (("/aaa/daemons/account_transactiond.c"->is_precost_locked(account) || "/aaa/daemons/account_transactiond.c"->is_cost_fail_times_full(account))) {
        agent->send_message(25463, gid, account, "", 2, para1, para2, coin_password, send_time, prc_cookie);
        return;
    }
    "/aaa/daemons/account_transactiond.c"->set_precost_lock(account, -1);
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _main_thread, agent, dist, server, gid, account, cost, para1, para2, coin_password, coin_type, type, send_time, prc_cookie :));

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        4
   Code segments found:         4
   Successfully decoded:        4
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          1
      - /inherit/misc/handler.c()
   ================================================================ */