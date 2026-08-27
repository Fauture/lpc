/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_change_coupon.o
 * Source: /aaa/daemons/server/cmd_l_change_coupon.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 3 names, 3 code segments found
 * Strings: 30, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void do_encourage_log(string dist, string account, string gid, string type, string id, string cookie);
void main(object agent, string dist, string account, string gid, string type, string para, int start_time, int end_time, int once, int coin_type);
private void exchange_coupon(mixed agent, string dist, string account, string gid, string type, string para, int start_time, int end_time, int once, int coin_type);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=43, nargs=6, insts=43
// --------------------------------------------------------------------
private void do_encourage_log(string dist, string account, string gid, string type, string id, string cookie) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;
    mixed request_time;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((-1 == cno)) {
        printf("[1;31mÁ¬½Ó ADB Êý¾Ý¿âÊ§°Ü\n[2;37;0m");
        return;
    }
    request_time = ctime(time(), 3);
    sql_cmd = mysql_bind_parameters("insert into encourage_log(gid, account, encourage_type, request_time, dist, encourage_id, memo) values(?, ?, ?, ?, ?, ?, ?)", gid, account, type, request_time, dist, id, cookie);
    (*f_execute)(cno, sql_cmd);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=27, nargs=10, insts=27
// --------------------------------------------------------------------
void main(object agent, string dist, string account, string gid, string type, string para, int start_time, int end_time, int once, int coin_type) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: exchange_coupon, agent, dist, account, gid, type, para, start_time, end_time, once, coin_type :));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=361, nargs=10, insts=361
// --------------------------------------------------------------------
private void exchange_coupon(mixed agent, string dist, string account, string gid, string type, string para, int start_time, int end_time, int once, int coin_type) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    int result;
    mixed arr;
    mixed sql_cmd;
    mixed st;
    mixed et;
    string cookie;
    mixed checksum;
    mapping encourage;
    mixed transaction_id;
    mixed encourage_account;
    mixed source;
    mixed gold_coin;
    mixed silver_coin;
    mixed coupon_gold_used;
    mixed coupon_silver_used;
    mixed encourage_time;

    encourage = ([ ]);
    if ((!is_object(agent))) {
        return;
    }
    result = 3;
    cookie = "";
    do {
        cno = "/daemons/dbd.c"->assure_connection("adb");
        if ((-1 == cno)) {
            break;
        }
        if ((!sizeof(type))) {
            break;
        }
        st = ctime(start_time, 3);
        et = ctime(end_time, 3);
        if (coin_type) {
            if ((strlen(trim(coupon_charge_type)) < 1)) {
                catch(error("Not config coupon_charge_type.\n"));

                // --- label L0066 ---
                break;
            }
            sql_cmd = mysql_bind_parameters(sprintf("select * from credit_log where account = ? and source in (%s) and time between ? and ? and gold_coin >= coupon_gold_used + ? limit 1", coupon_charge_type), account, st, et, once);
        } else {
            sql_cmd = mysql_bind_parameters("select * from credit_log where account = ?  and time between ? and ? and  silver_coin >= coupon_silver_used + ?  limit 1", account, st, et, once);
        }
        arr = mysql_select(cno, sql_cmd);
        if ((!is_array(arr))) {
            break;
        }
        if ((sizeof(arr) < 1)) {
            result = 2;
            break;
        }
        loop (encourage in arr) {
            transaction_id = encourage["transaction_id"];
            encourage_account = encourage["account"];
            source = encourage["source"];
            gold_coin = encourage["gold_coin"];
            silver_coin = encourage["silver_coin"];
            coupon_gold_used = encourage["coupon_gold_used"];
            coupon_silver_used = encourage["coupon_silver_used"];
            encourage_time = encourage["time"];
            checksum = save_string(md5(iconv(sprintf("%s$%s$%s$%s$%s$%s$%s$%s$%s", transaction_id, encourage_account, source, gold_coin, silver_coin, coupon_gold_used, coupon_silver_used, encourage_time, "201004"), "utf-8", "GBK")))[1..<2];
            if (stricmp(checksum, encourage["checksum"])) {
                continue;
            }
            if (coin_type) {
                checksum = save_string(md5(iconv(sprintf("%s$%s$%s$%s$%s$%s$%s$%s$%s", transaction_id, encourage_account, source, gold_coin, silver_coin, (string)((int)coupon_gold_used + once), coupon_silver_used, encourage_time, "201004"), "utf-8", "GBK")))[1..<2];
                sql_cmd = mysql_bind_parameters("update credit_log set coupon_gold_used = ?,  para3 = ?, checksum = ? where id = ? ", ((int)coupon_gold_used + once), type, checksum, encourage["id"]);
            } else {
                checksum = save_string(md5(iconv(sprintf("%s$%s$%s$%s$%s$%s$%s$%s$%s", transaction_id, encourage_account, source, gold_coin, silver_coin, coupon_gold_used, (string)((int)coupon_silver_used + once), encourage_time, "201004"), "utf-8", "GBK")))[1..<2];
                sql_cmd = mysql_bind_parameters("update credit_log set coupon_silver_used = ?,  para3 = ?, checksum = ? where id = ? ", ((int)coupon_silver_used + once), type, checksum, encourage["id"]);
            }
            if (((*f_execute)(cno, sql_cmd) < 1)) {
                break;
            }
            cookie = save_string(([ "id": encourage["id"], "transaction_id": transaction_id, "account": encourage_account, "gold_coin": gold_coin, "silver_coin": silver_coin, "coupon_gold_used": coupon_gold_used, "coupon_silver_used": coupon_silver_used, "time": encourage_time, "type": type ]));
            result = 0;
            break;
        }  // end loop (verified)
    } while (0);

    // --- label L0329 ---
    if ((!is_object(agent))) {
        do_encourage_log(dist, account, gid, sprintf("%s/changenoagent", type), (string)encourage["id"], cookie);
        return;
    }
    agent->send_message(13079, account, gid, result, once, para, cookie);

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