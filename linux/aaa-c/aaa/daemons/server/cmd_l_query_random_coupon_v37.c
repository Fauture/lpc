/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_query_random_coupon.o
 * Source: /aaa/daemons/server/cmd_l_query_random_coupon.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 22, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void main(mixed agent, string dist, string account, string gid, string type, string para, int start_time, int end_time, int once, int coin_type);
private void query_coupon(mixed agent, string dist, string account, string gid, string type, string para, int start_time, int end_time, int once, int coin_type);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=27, nargs=10, insts=27
// --------------------------------------------------------------------
void main(mixed agent, string dist, string account, string gid, string type, string para, int start_time, int end_time, int once, int coin_type) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: query_coupon, agent, dist, account, gid, type, para, start_time, end_time, once, coin_type :));

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=215, nargs=10, insts=215
// --------------------------------------------------------------------
private void query_coupon(mixed agent, string dist, string account, string gid, string type, string para, int start_time, int end_time, int once, int coin_type) {
    /* ---- Enhanced Pseudocode ---- */
    int num;
    mixed cno;
    int result;
    mixed arr;
    mixed sql_cmd;
    mixed st;
    mixed et;
    mixed checksum;
    mapping encourage;

    num = 0;
    if ((!is_object(agent))) {
        return;
    }
    result = 3;
    do {
        cno = "/daemons/dbd.c"->assure_connection("adb");
        if ((-1 == cno)) {
            break;
        }
        st = ctime(start_time, 3);
        et = ctime(end_time, 3);
        if (coin_type) {
            if ((strlen(trim(coupon_charge_type)) < 1)) {
                catch(error("Not config coupon_charge_type.\n"));

                // --- label L0056 ---
                break;
            }
            sql_cmd = mysql_bind_parameters(sprintf("select * from credit_log where account = ? and source in (%s) and time between ? and ? and gold_coin >= ? limit 10000", coupon_charge_type), account, st, et, once);
        } else {
            sql_cmd = mysql_bind_parameters("select * from credit_log where account = ?  and time between ? and ? and  silver_coin >= ? limit 10000", account, st, et, once);
        }
        arr = mysql_select(cno, sql_cmd, 10000);
        if ((!is_array(arr))) {
            break;
        }
        if ((sizeof(arr) < 1)) {
            result = 2;
            break;
        }
        loop (encourage in arr) {
            checksum = save_string(md5(iconv(sprintf("%s$%s$%s$%s$%s$%s$%s$%s$%s", encourage["transaction_id"], encourage["account"], encourage["source"], encourage["gold_coin"], encourage["silver_coin"], encourage["coupon_gold_used"], encourage["coupon_silver_used"], encourage["time"], "201004"), "utf-8", "GBK")))[1..<2];
            if (stricmp(checksum, encourage["checksum"])) {
                continue;
            }
            if (coin_type) {
                num += (((int)encourage["gold_coin"] - (int)encourage["coupon_gold_used"]) / once);
            } else {
                num += (((int)encourage["silver_coin"] - (int)encourage["coupon_silver_used"]) / once);
            }
        }  // end loop (verified)
        result = 0;
    } while (0);

    // --- label L0198 ---
    if ((!is_object(agent))) {
        return;
    }
    agent->send_message(13081, account, gid, result, para, num);

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