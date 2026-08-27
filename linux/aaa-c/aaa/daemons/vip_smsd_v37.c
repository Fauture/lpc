/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: vip_smsd.o
 * Source: /aaa/daemons/vip_smsd.c
 * Inherits: (none)
 * Functions: 14 names, 14 code segments found
 * Strings: 84, Variables: 2
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   int g_module_status;
   string sms_auth_url;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void do_when_batch_block_account(string dist, array arr_account, string btime);
private void do_notice_block(string dist, string account, string btime);
string get_account_desc(string account);
mixed query_vip_sms_info(string account);
int update_vip_sms_info(string account, mapping vip_sms_info);
void do_when_block_account(string dist, string account, string btime);
void manage_vip_sms(mixed agent, string dist, string gid, string account, int oper, string para);
void send_vip_sms(mixed agent, string dist, string gid, string account, string auth_type, string sms_type, string sms_code, string ti, string cookie, string para);
private void do_init();
int handle_result(mixed ret);
private void create();
mixed get_vip_sms_url(string dist, string account, string auth_type, string sms_type, string sms_code, string ti, string cookie, mapping param);
mixed test_send_sms(string dist, string account, string auth_type, string sms_type, string sms_code, string ti, string cookie, mapping param);
int delete_vip_sms_info(string account);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   g_module_status = 1;  // /aaa/daemons/vip_smsd.c:737
   sms_auth_url = "";

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=74, nargs=3, insts=74
// --------------------------------------------------------------------
void do_when_batch_block_account(string dist, array arr_account, string btime) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed count;
    mixed arr;
    mixed sql_cmd;
    mixed account;

    if (((!g_module_status) || (!strlen(sms_auth_url)))) {
        return;
    }
    count = sizeof(arr_account);
    if (((cno = "/daemons/dbd.c"->assure_connection("adb")) < 0)) {
        return;
    }
    sql_cmd = sprintf("select account from vip_sms_protect where account in (%s) and p_block_account=1", implode(allocate_array(count, "?"), ","));
    sql_cmd = mysql_bind_parameters_arr(sql_cmd, arr_account);
    arr = mysql_select_column(cno, sql_cmd, 0, count);
    if (((!is_array(arr)) || (sizeof(arr) < 1))) {
        return;
    }
    loop (account in arr) {
        do_notice_block(dist, account, btime);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=93, nargs=3, insts=93
// --------------------------------------------------------------------
private void do_notice_block(string dist, string account, string btime) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ti;
    mixed http_ret;
    mixed ret;
    mixed url;
    string sms_type;

    if ((!strlen(btime))) {
        sms_type = "AccountUnBlocked";
    } else {
        sms_type = "AccountBlocked";
    }
    ti = time();
    if ((!is_string(url = get_vip_sms_url(dist, account, "vip_sms", sms_type, sprintf("%04d", (ti % 10000)), ctime(ti, 3), sprintf("%06d", random(1000000)), ([ ]))))) {
        return;
    }
    http_ret = "/daemons/http_clientd.c"->http_post_web(url, 15, 65535, "");
    ret = handle_result(http_ret);
    if (((1 == ret) || (4 == ret))) {
        switch (when_return_user_not_found) {
            case 2:
            catch(error("User not found for vip_sms(account=%s)\n", account));

            // --- label L0080 ---
            break;
            case 3:
            delete_vip_sms_info(account);
            break;
            default:
            break;
        }  // end switch

        // --- label L0090 ---
    }

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=56, nargs=1, insts=56
// --------------------------------------------------------------------
string get_account_desc(string account) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    mixed account_desc;

    if ((sizeof(account) < 1)) {
        return account;
    }
    if ((sizeof(account) < 6)) {
        return (account[0..<2] + "*");
    }
    account_desc = account[0..1];
    for (i = 2; i <= (sizeof(account) - 3); i++) {
        account_desc += "*";
    }  // end loop (verified)
    account_desc += account[<2..<1];
    return account_desc;

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=60, nargs=1, insts=60
// --------------------------------------------------------------------
mixed query_vip_sms_info(string account) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed arr;
    mixed sql_cmd;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 0)) {
        return -1;
    }
    sql_cmd = mysql_bind_parameters("select * from vip_sms_protect where account=?", account);
    arr = mysql_select(cno, sql_cmd, 1);
    if ((!is_array(arr))) {
        return -1;
    }
    if ((!sizeof(arr))) {
        return ([ ]);
    }
    map_delete((mapping)arr[0], "account");
    map_delete((mapping)arr[0], "update_time");
    return arr[0];

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=44, nargs=2, insts=44
// --------------------------------------------------------------------
int update_vip_sms_info(string account, mapping vip_sms_info) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed sql_cmd;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 0)) {
        return -1;
    }
    vip_sms_info["account"] = account;
    vip_sms_info["update_time"] = ctime(time(), 3);
    sql_cmd = convert_mapping_to_sql_cmd("update", "vip_sms_protect", vip_sms_info, "account");
    return (*f_execute)(cno, sql_cmd);

}

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=39, nargs=3, insts=39
// --------------------------------------------------------------------
void do_when_block_account(string dist, string account, string btime) {
    /* ---- Enhanced Pseudocode ---- */
    mapping vip_sms;

    if (((!g_module_status) || (!strlen(sms_auth_url)))) {
        return;
    }
    if ((((!is_mapping(vip_sms = query_vip_sms_info(account))) || (sizeof(vip_sms) < 1)) || (!atoi(vip_sms["p_block_account"])))) {
        return;
    }
    do_notice_block(dist, account, btime);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=259, nargs=6, insts=259
// --------------------------------------------------------------------
void manage_vip_sms(mixed agent, string dist, string gid, string account, int oper, string para) {
    /* ---- Enhanced Pseudocode ---- */
    int ret;
    mixed params;
    mapping vip_sms_info;

    if ((!is_object(agent))) {
        return;
    }
    try {
        params = restore_value(para);
    } catch { /* error handled */ }

    // --- label L0017 ---
    if ((0 || (!is_array(params)))) {
        agent->send_message(25467, gid, oper, 2, save_string(([ ])));
        return;
    }
    vip_sms_info = query_vip_sms_info(account);
    if ((!is_object(agent))) {
        return;
    }
    if ((!is_mapping(vip_sms_info))) {
        agent->send_message(25467, gid, oper, 99, save_string(([ ])));
        return;
    }
    if ((!sizeof(vip_sms_info))) {
        agent->send_message(25467, gid, oper, 1, save_string(([ ])));
        return;
    }
    switch (oper) {
        case 1:
        case 2:
        if (((sizeof(params) < 1) || (!is_int(params[0])))) {
            agent->send_message(25467, gid, oper, 2, save_string(([ ])));
            return;
        }
        vip_sms_info["p_coin_trade"] = (string)params[0];
        break;
        case 3:
        case 4:
        if (((sizeof(params) < 1) || (!is_int(params[0])))) {
            agent->send_message(25467, gid, oper, 2, save_string(([ ])));
            return;
        }
        vip_sms_info["p_coin_consum"] = (string)params[0];
        break;
        case 5:
        vip_sms_info["p_block_account"] = "1";
        break;
        case 6:
        vip_sms_info["p_coin_trade"] = "0";
        break;
        case 7:
        vip_sms_info["p_coin_consum"] = "0";
        break;
        case 8:
        vip_sms_info["p_block_account"] = "0";
        break;
        case 9:
        vip_sms_info["p_oper_role"] = "1";
        break;
        case 10:
        vip_sms_info["p_oper_role"] = "0";
        break;
        default:
        return;
    }  // end switch

    // --- label L0211 ---
    ret = update_vip_sms_info(account, vip_sms_info);
    map_delete((mapping)vip_sms_info, "account");
    map_delete((mapping)vip_sms_info, "update_time");
    if ((!is_object(agent))) {
        return;
    }
    if ((ret >= 0)) {
        agent->send_message(25467, gid, oper, 0, save_string(vip_sms_info));
    } else {
        agent->send_message(25467, gid, oper, 99, save_string(vip_sms_info));
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=156, nargs=10, insts=156
// --------------------------------------------------------------------
void send_vip_sms(mixed agent, string dist, string gid, string account, string auth_type, string sms_type, string sms_code, string ti, string cookie, string para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ret;
    mixed http_ret;
    mixed url;
    mixed param;

    if ((!is_object(agent))) {
        return;
    }
    if ((((!g_module_status) || (!is_string(sms_auth_url))) || (!strlen(sms_auth_url)))) {
        agent->send_message(25469, gid, auth_type, sms_type, sms_code, 0);
        return;
    }
    try {
        param = restore_value(para);
    } catch { /* error handled */ }

    // --- label L0042 ---
    if ((0 || (!is_mapping(param)))) {
        agent->send_message(25469, gid, auth_type, sms_type, sms_code, 2);
        return;
    }
    if ((!is_string(url = get_vip_sms_url(dist, account, auth_type, sms_type, sms_code, ti, cookie, param)))) {
        agent->send_message(25469, gid, auth_type, sms_type, sms_code, 2);
        return;
    }
    http_ret = "/daemons/http_clientd.c"->http_post_web(url, 15, 65535, "");
    ret = handle_result(http_ret);
    if ((("vip_sms" == auth_type) && ((1 == ret) || (4 == ret)))) {
        switch (when_return_user_not_found) {
            case 2:
            catch(error("User not found for vip_sms(account=%s)\n", account));

            // --- label L0128 ---
            break;
            case 3:
            delete_vip_sms_info(account);
            break;
            default:
            break;
        }  // end switch

        // --- label L0138 ---
    }
    if ((!is_object(agent))) {
        return;
    }
    agent->send_message(25469, gid, auth_type, sms_type, sms_code, ret);

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=36, nargs=0, insts=36
// --------------------------------------------------------------------
private void do_init() {
    /* ---- Enhanced Pseudocode ---- */
    if (((!is_object(find_object("/aaa/daemons/configd.c"))) || (!is_mapping("/aaa/daemons/configd.c"->get_my_info())))) {
        register_post_init((: do_init :));
        return;
    }
    if ((!is_string(sms_auth_url = "/aaa/daemons/configd.c"->get_my_info()["sms_auth_url"]))) {
        sms_auth_url = "";
    }

}

// --------------------------------------------------------------------
// @brief  event handler: respond to event
// @code  nc=103, nargs=1, insts=103
// --------------------------------------------------------------------
int handle_result(mixed ret) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ret_status;
    mixed error_code;

    if ((is_string(ret) && (sscanf(ret, "%*s<Status>%s</Status>%*s", &error_code) >= 3))) {
        ret_status = error_code;
    } else {
        ret_status = ret;
    }
    switch (ret_status) {
        case "success":
        case "partial_send_success":
        return 0;
        case "user_not_found":
        return 1;
        case "ERROR_phone_no_pass":
        return 4;
        case "suspect-day":
        return 7;
        case "suspect-sourcetype":
        return 8;
        case "suspect-divide":
        return 9;
        case "invalid_sourcetype":
        return 6;
        case "message-param-format":
        return 2;
        case "message-invalid-param":
        return 2;
        case "invalid-param-count":
        return 2;
        case "invalid-sourceType":
        return 3;
        case "invalid-channel":
        return 3;
        case "verifycode-error":
        return 3;
        case "message-content-error":
        return 3;
        case "send-failure":
        return 3;
        case "server-error":
        return 3;
        case -2:
        case -1:
        case -3:
        return 99;
        default:
        return 3;
    }  // end switch

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=7, nargs=0, insts=7
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    register_post_init((: do_init :));

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=249, nargs=8, insts=249
// --------------------------------------------------------------------
mixed get_vip_sms_url(string dist, string account, string auth_type, string sms_type, string sms_code, string ti, string cookie, mapping param) {
    /* ---- Enhanced Pseudocode ---- */
    mixed prop_name;
    mixed str_game;
    mixed str_serv;
    string str_para;
    mixed account_desc;

    str_para = "";
    switch (sms_type) {
        case "CoinsTrade":
        if (((!param["cur_trade_coin"]) || (!param["coin_trade_quota"]))) {
            return;
        }
        str_para = sprintf(",%d,%s,%d", (int)param["cur_trade_coin"], cookie, (int)param["coin_trade_quota"]);
        break;
        case "ChangeCoinsTrade":
        if ((!param["coin_trade_quota"])) {
            return;
        }
        str_para = sprintf(",%d,%s", (int)param["coin_trade_quota"], cookie);
        break;
        case "CancelCoinsTrade":
        if ((!param["coin_trade_quota"])) {
            return;
        }
        str_para = sprintf(",%s,%d", cookie, (int)param["coin_trade_quota"]);
        break;
        case "OverConsumption":
        if ((!param["coin_consum_quota"])) {
            return;
        }
        str_para = sprintf(",%d,%s", (int)param["coin_consum_quota"], cookie);
        break;
        case "ChangeConsumption":
        if ((!param["coin_consum_quota"])) {
            return;
        }
        str_para = sprintf(",%d,%s", (int)param["coin_consum_quota"], cookie);
        break;
        case "CancelConsumption":
        if ((!param["coin_consum_quota"])) {
            return;
        }
        str_para = sprintf(",%s,%d", cookie, (int)param["coin_consum_quota"]);
        break;
        case "PropTrade":
        case "PropFpTrade":
        case "CancelPropTrade":
        if ((!is_string(prop_name = param["prop_name"]))) {
            return;
        }
        str_para = sprintf(",%s,%s", convert_to_url_encode(prop_name), cookie);
        break;
        case "CancelAllPropTrade":
        str_para = sprintf(",%s", cookie);
        break;
        case "AccountBlocked":
        case "AccountUnBlocked":
        break;
        case "CancelBlockMessage":
        str_para = sprintf(",%s", cookie);
        break;
        case "RoleDelete":
        case "RoleTrade":
        case "RoleQbTrade":
        case "CancelRoleOperate":
        str_para = sprintf(",%s", cookie);
        break;
        case "CleanLockPwd":
        str_para = sprintf(",%s", cookie);
        break;
        case "RoleAuth":
        str_para = sprintf(",%s", cookie);
        break;
        default:
        return;
    }  // end switch

    // --- label L0214 ---
    str_serv = convert_to_url_encode(dist);
    str_game = convert_to_url_encode("ÎÊµÀ");
    account_desc = get_account_desc(account);
    str_para = (sprintf("messageParam=%s,%s,%s,%s", sms_code, account_desc, str_game, str_serv) + str_para);
    return sprintf("%s?account=%s&sourceType=%s&%s&sign=Gyyx.Core.WebApi", sms_auth_url, account, sms_type, str_para);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=69, nargs=8, insts=69
// --------------------------------------------------------------------
mixed test_send_sms(string dist, string account, string auth_type, string sms_type, string sms_code, string ti, string cookie, mapping param) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ret;
    mixed http_ret;
    mixed url;

    if (((!is_string(sms_auth_url)) || (!strlen(sms_auth_url)))) {
        printf("sms_auth_url no config.");
        return;
    }
    if ((!is_string(url = get_vip_sms_url(dist, account, auth_type, sms_type, sms_code, ti, cookie, param)))) {
        return 2;
    }
    printf("URL£º%s\n", url);
    http_ret = "/daemons/http_clientd.c"->http_post_web(url, 15, 65535, "");
    if (is_string(http_ret)) {
        http_ret = iconv(http_ret, "gbk", "utf-8");
    }
    printf("http_ret£º%O\n", http_ret);
    ret = handle_result(http_ret);
    return ret;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=28, nargs=1, insts=28
// --------------------------------------------------------------------
int delete_vip_sms_info(string account) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed sql_cmd;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 0)) {
        return -1;
    }
    sql_cmd = mysql_bind_parameters("delete from vip_sms_protect where account=?", account);
    return (*f_execute)(cno, sql_cmd);

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        14
   Code segments found:         14
   Successfully decoded:        14
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */