/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_update_recharge_return_credit.o
 * Source: /aaa/daemons/server/cmd_l_update_recharge_return_credit.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 3 names, 3 code segments found
 * Strings: 22, Variables: 2
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   mapping credit_index_map;
   /* mixed global_0; */

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private mixed get_recharge_return_credit(int start_time, int end_time, string dist, string account, int cur_bonus_index, int test_dist_flag, int &total_coin, int &max_bonus_index);
private void update_recharge_return_credit(mixed agent, string dist, string account, string gid, int start_time, int end_time, int cur_bonus_index, int test_dist_flag);
void main(mixed agent, string dist, string account, string gid, int start_time, int end_time, int cur_bonus_index, int test_dist_flag);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   credit_index_map = ([ "充值返利活动20150728充值返利十元礼包" : 0, "充值返利活动20150728充值返利五十元礼包" : 1, "充值返利活动20150728充值返利百元礼包" : 2, "充值返利活动20150728充值返利五百元礼包" : 3, "充值返利活动20150728充值返利千元礼包" : 4 ]);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=198, nargs=8, insts=198
// --------------------------------------------------------------------
private mixed get_recharge_return_credit(int start_time, int end_time, string dist, string account, int cur_bonus_index, int test_dist_flag, int &total_coin, int &max_bonus_index) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed arr;
    mixed bonus_types;
    mixed sql_cmd;
    string coin_type;
    mapping encourage;

    if (((cno = "/daemons/dbd.c"->assure_connection("adb")) == -1)) {
        return;
    }
    if (test_dist_flag) {
        coin_type = "silver_coin";
    } else {
        coin_type = "gold_coin";
    }
    sql_cmd = mysql_bind_parameters(sprintf("select sum(%s) as coins from credit_log where account = ? and source in (%s) and time >= ? and time <= ?", coin_type, coupon_charge_type), account, ctime(start_time, 3), ctime(end_time, 3));
    if ((!is_array(arr = mysql_select(cno, sql_cmd, 1)))) {
        return;
    }
    if ((sizeof(arr) < 1)) {
        return ({});
    }
    total_coin = atoi(arr[0]["coins"]);
    if ((total_coin >= 100000)) {
        bonus_types = ({ "充值返利活动20150728充值返利十元礼包", "充值返利活动20150728充值返利五十元礼包", "充值返利活动20150728充值返利百元礼包", "充值返利活动20150728充值返利五百元礼包", "充值返利活动20150728充值返利千元礼包" });
    } else if ((total_coin >= 50000)) {
        bonus_types = ({ "充值返利活动20150728充值返利十元礼包", "充值返利活动20150728充值返利五十元礼包", "充值返利活动20150728充值返利百元礼包", "充值返利活动20150728充值返利五百元礼包" });
    } else if ((total_coin >= 10000)) {
        bonus_types = ({ "充值返利活动20150728充值返利十元礼包", "充值返利活动20150728充值返利五十元礼包", "充值返利活动20150728充值返利百元礼包" });
    } else if ((total_coin >= 5000)) {
        bonus_types = ({ "充值返利活动20150728充值返利十元礼包", "充值返利活动20150728充值返利五十元礼包" });
    } else if ((total_coin >= 1000)) {
        bonus_types = ({ "充值返利活动20150728充值返利十元礼包" });
    } else {
        return ({});
    }
    if ((((1 << sizeof(bonus_types)) - 1) <= cur_bonus_index)) {
        return ({});
    }
    sql_cmd = mysql_bind_parameters(sprintf("select encourage_type, used from encourage where account = ? and encourage_type in ('%s') and insert_time >= ? and insert_time <= ?", implode(bonus_types, "', '")), account, ctime(start_time, 3), ctime((end_time + 86400), 3));
    if ((!is_array(arr = mysql_select(cno, sql_cmd, 10)))) {
        return;
    }
    loop (encourage in arr) {
        if ((int)encourage["used"]) {
            max_bonus_index |= (1 << credit_index_map[encourage["encourage_type"]]);
        }
        bonus_types -= ({ encourage["encourage_type"] });
    }  // end loop (verified)
    return bonus_types;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=86, nargs=8, insts=86
// --------------------------------------------------------------------
private void update_recharge_return_credit(mixed agent, string dist, string account, string gid, int start_time, int end_time, int cur_bonus_index, int test_dist_flag) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ti;
    int total_coin;
    mixed max_bonus_index;
    mixed bonus_types;
    mixed type;

    if ((!is_object(agent))) {
        return;
    }
    ti = time();
    if (((ti <= start_time) || (ti >= (end_time + 86400)))) {
        return;
    }
    total_coin = 0;
    max_bonus_index = cur_bonus_index;
    bonus_types = get_recharge_return_credit(start_time, end_time, dist, account, cur_bonus_index, test_dist_flag, &total_coin, &max_bonus_index);
    if ((!is_array(bonus_types))) {
        return;
    }
    loop (type in bonus_types) {
        "/aaa/daemons/server/cmd_l_add_encourage_coupon.c"->add_coupon(agent, dist, account, gid, type, 0, "", ctime((end_time + 86400), 3));
    }  // end loop (verified)
    if ((!is_object(agent))) {
        return;
    }
    agent->send_message(64889, account, gid, total_coin, max_bonus_index);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=25, nargs=8, insts=25
// --------------------------------------------------------------------
void main(mixed agent, string dist, string account, string gid, int start_time, int end_time, int cur_bonus_index, int test_dist_flag) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: update_recharge_return_credit, agent, dist, account, gid, start_time, end_time, cur_bonus_index, test_dist_flag :));

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