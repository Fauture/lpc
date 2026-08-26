/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_update_first_credit_bonus.o
 * Source: /aaa/daemons/server/cmd_l_update_first_credit_bonus.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 4 names, 4 code segments found
 * Strings: 27, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private mixed get_bonus_type(string encourage_type, int start_time, int first_create_time, mapping credit_info);
private void update_first_credit_bonus(mixed agent, string dist, string account, string gid, int start_time, int end_time, int first_create_time, string encourage_type);
void main(mixed agent, string dist, string account, string gid, int start_time, int end_time, int first_create_time, string encourage_type);
private mixed get_first_credit_info(int start_time, int end_time, string dist, string account, string encourage_type, mapping result);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=97, nargs=4, insts=97
// --------------------------------------------------------------------
private mixed get_bonus_type(string encourage_type, int start_time, int first_create_time, mapping credit_info) {
    /* ---- Enhanced Pseudocode ---- */
    int coin_num;
    mixed first_credit_time;
    string item_name;

    coin_num = maximum(atoi(credit_info["gold_coin"]), atoi(credit_info["silver_coin"]));
    if ((coin_num < 1000)) {
        return;
    }
    first_credit_time = time_desc_to_int(credit_info["time"], 3);
    if ((first_credit_time < start_time)) {
        return;
    }
    switch (((maximum(0, (first_credit_time - first_create_time)) / 86400) + 1)) {
        case 1:
        if ((coin_num < 3000)) {
            item_name = "豪华首充10元礼包";
        } else {
            item_name = "豪华首充30元礼包";
        }
        break;
        case 2:
        case 3:
        if ((coin_num < 3000)) {
            item_name = "精致首充10元礼包";
        } else {
            item_name = "精致首充30元礼包";
        }
        break;
        default:
        if ((coin_num < 3000)) {
            item_name = "普通首充10元礼包";
        } else {
            item_name = "普通首充30元礼包";
        }
        break;
    }  // end switch

    // --- label L0091 ---
    return (encourage_type + item_name);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=87, nargs=8, insts=87
// --------------------------------------------------------------------
private void update_first_credit_bonus(mixed agent, string dist, string account, string gid, int start_time, int end_time, int first_create_time, string encourage_type) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ret;
    mixed type;
    mapping result;

    if ((!is_object(agent))) {
        return;
    }
    result = ([ ]);
    get_first_credit_info(start_time, end_time, dist, account, encourage_type, result);
    if (((!sizeof(result)) || (!is_object(agent)))) {
        return;
    }
    if (("has_credit" == result["desc"])) {
        if (is_string(type = get_bonus_type(encourage_type, start_time, first_create_time, result["credit_info"]))) {
            ret = "/aaa/daemons/server/cmd_l_add_encourage_coupon.c"->add_coupon(agent, dist, account, gid, type, "");
            if (ret) {
                result["desc"] = "has_bonus";
            }
        } else {
            result["desc"] = "not_enough_coin";
        }
    }
    agent->send_message(64921, account, gid, result["desc"]);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=25, nargs=8, insts=25
// --------------------------------------------------------------------
void main(mixed agent, string dist, string account, string gid, int start_time, int end_time, int first_create_time, string encourage_type) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: update_first_credit_bonus, agent, dist, account, gid, start_time, end_time, first_create_time, encourage_type :));

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=87, nargs=6, insts=87
// --------------------------------------------------------------------
private mixed get_first_credit_info(int start_time, int end_time, string dist, string account, string encourage_type, mapping result) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;
    mixed map;

    if (((cno = "/daemons/dbd.c"->assure_connection("adb")) == -1)) {
        return result;
    }
    sql_cmd = mysql_bind_parameters("select id from encourage where account = ? and dist = ? and encourage_type like ? limit 1", account, dist, sprintf("%s%%", encourage_type));
    if (is_mapping(map = mysql_select_single_result(cno, sql_cmd))) {
        result["desc"] = "has_bonus";
        return;
    }
    if ((0 != map)) {
        return;
    }
    sql_cmd = mysql_bind_parameters(sprintf("select id, gold_coin, silver_coin, time from credit_log where account = ? and source in (%s) and time < ? order by id limit 1", coupon_charge_type), account, ctime(end_time, 3));
    if (is_mapping(map = mysql_select_single_result(cno, sql_cmd))) {
        result["desc"] = "has_credit";
        result["credit_info"] = map;
        return;
    }
    if ((0 == map)) {
        result["desc"] = "no_credit";
    }
    return;

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