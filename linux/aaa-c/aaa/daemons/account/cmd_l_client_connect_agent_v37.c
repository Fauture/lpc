/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_client_connect_agent.o
 * Source: /aaa/daemons/account/cmd_l_client_connect_agent.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 1 names, 1 code segments found
 * Strings: 89, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void main(mixed account_ob, string account, int auth_key, string server);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=959, nargs=4, insts=959
// --------------------------------------------------------------------
void main(mixed account_ob, string account, int auth_key, string server) {
    /* ---- Enhanced Pseudocode ---- */
    mixed service;
    int privilege;
    mixed state;
    mixed btime;
    int status_state;
    mixed all_protects;
    mixed real_reason;
    string breason;
    mixed dist;
    mixed default_service;
    string protects;
    mixed agent;
    mapping special_service;
    mapping dist_info;
    mapping matrix_info;
    mapping account_info;
    mixed vip_sms;
    mixed temp_block_info;
    mixed phone_auth;
    mapping alipay_info;
    mixed paras;
    int check_block_day;
    int left_deny_time;
    int left_deny_day;
    mixed need_prompt;
    int protect_type;
    mixed is_binded;
    mixed spec_protect;

    protects = "";
    if ((!is_object(account_ob))) {
        return;
    }
    account_info = "/aaa/daemons/accountd.c"->query_account_info(account);
    if ((!is_object(account_ob))) {
        return;
    }
    if (((!is_mapping(account_info)) || (account_info["password"] != account_ob->query_password()))) {
        account_ob->send_message(8165, "连接服务器错误，无法登入。（原因：1011）", 1);
        account_ob->to_be_disconnect(1);
        return;
    }
    check_block_day = 0;
    btime = account_info["blocked_time"];
    if ("/aaa/daemons/accountd.c"->is_apex_enabled()) {
        btime = trim((string)btime);
    }
    real_reason = account_info["blocked_reason"];
    breason = "运营中被封闭的账号";
    if (is_undefined(btime = time_desc_to_int(btime, 3))) {
        breason = "运营中被封闭的账号。（原因：1010）";
        "/aaa/daemons/accountd.c"->get_extra_block_reason_msg(check_block_day, real_reason, &breason);
        account_ob->send_message(8165, breason, 1);
        account_ob->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", sprintf("该账号被封，禁止登入，原因：%O\n", "block_time字段格式错误"));
        }
        return;
    }
    if ((btime > time())) {
        left_deny_time = (btime - time());
        if ((left_deny_time < 15552000)) {
            left_deny_day = (left_deny_time / 86400);
            left_deny_day = ((left_deny_time % 86400) ? (left_deny_day + 1) : left_deny_day);
            check_block_day = left_deny_day;
            breason = sprintf("营运中被封闭的账号，将于#R%d#n天后解除封闭。", left_deny_day);
        }
        if ((sscanf(real_reason, "need_prompt:%s", &need_prompt) == 1)) {
            breason = need_prompt;
        }
        "/aaa/daemons/accountd.c"->get_extra_block_reason_msg(check_block_day, real_reason, &breason);
        account_ob->send_message(8165, breason, 1);
        account_ob->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", breason);
        }
        return;
    }
    if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
        "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", sprintf("请求登入GS服务器:%s\n", server));
    }
    if (((account_ob->query_auth_key() != auth_key) || (stricmp(account_ob->query_account(), trim(account)) != 0))) {
        account_ob->send_message(8165, "连接服务器错误，无法登入。（原因：1002）", 1);
        account_ob->to_be_disconnect(1);
        if ((is_string(account_ob->query_account()) || is_string((account_ob->query_account())->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account_ob->query_account()), "login", sprintf("用户端发送的账号(%s)或者AUTH_KEY(%d)不正确", account, auth_key));
        }
        return;
    }
    if ((!dist = account_ob->query("login_dist"))) {
        account_ob->send_message(8165, "连接服务器错误，无法登入。（原因：1003）", 1);
        account_ob->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "用户端还没有获取服务器列表就直接要求连接服务器\n");
        }
        return;
    }
    dist_info = "/aaa/daemons/configd.c"->get_dist_info(dist);
    if ((!is_mapping(dist_info))) {
        account_ob->send_message(8165, "连接服务器错误，无法登入。（原因：1004）", 1);
        account_ob->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "AAA 服务器取得玩家要求登入的区组资讯，从而无法获取玩家的服务资讯\n");
        }
        return;
    }
    default_service = dist_info["service"];
    special_service = "/aaa/daemons/configd.c"->query_account_service_in_dist(account, dist);
    if ((!is_object(account_ob))) {
        return;
    }
    service = service_desc_to_value(default_service);
    if (is_mapping(special_service)) {
        service |= service_desc_to_value(special_service["permit"]);
        service %= (~service_desc_to_value(special_service["forbid"]));
    }
    if (((!is_string(account_ob->query("protect"))) || (account_ob->query("protect") == ""))) {
        service |= 1;
    }
    if ((!(service & 16))) {
        privilege = 0;
    } else {
        privilege = account_ob->query_privilege();
    }
    if (atoi(account_info["auto_lock"])) {
        service |= 512;
    }
    if ((sizeof(account_info["org_password"]) > 0)) {
        service |= 384;
    }
    if (atoi(account_info["adult"])) {
        service |= 1024;
    }
    if (((256 != (service & 256)) && (account_info["locked"] == "1"))) {
        account_ob->send_message(8165, "该账号已被锁定，请先登入逍遥会员中心解除锁定！（原因：1012）", 1);
        account_ob->to_be_disconnect(1);
        return;
    }
    state = "/aaa/daemons/accountd.c"->get_account_controlled_safe_state(account);
    if ((!is_object(account_ob))) {
        return;
    }
    if ((state & -2147483648)) {
        service |= 4096;
    }
    if ((state & 8)) {
        service |= 32768;
    }
    if ("/aaa/daemons/accountd.c"->is_enable_matrix_info_for_gs(account)) {
        service |= 131072;
    }
    if ((!is_object(account_ob))) {
        return;
    }
    do {
        if ((!(state & -2147483648))) {
            break;
        }
    } while (0);

    // --- label L0570 ---
    if (account_ob->query("local_encrypt")) {
        service |= 262144;
    }
    agent = "/aaa/daemons/accountd.c"->assure_server_agent(server);
    if ((!is_object(account_ob))) {
        return;
    }
    if ((!is_object(agent))) {
        account_ob->send_message(8165, "无法连接服务器", 1);
        account_ob->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "AAA 无法连接GS服务器，登入失败\n");
        }
        return;
    }
    account_info["last_login_ip"] = account_ob->get_peer_ip();
    account_info["last_login_time"] = time_to_string();
    account_info["last_login_id"] = account_ob->query_temp("mac_addr");
    "/aaa/daemons/accountd.c"->add_coins(account_info, dist, server, account, 10000, 0);
    protect_type = (int)account_ob->query("protect_range");
    is_binded = "/aaa/daemons/accountd.c"->is_protect_actived(account, get_auth_protect_bits(), account_ob);
    if ((!is_object(account_ob))) {
        return;
    }
    spec_protect = get_auth_spec_game_bits();
    all_protects = is_binded;
    if (is_mapping(account_ob->query_temp("protects"))) {
        protects = implode(keys(account_ob->query_temp("protects")), "");
    }
    if ((protect_type == 2)) {
        is_binded %= spec_protect;
    }
    status_state = (int)account_ob->query("safe_status");
    do {
        if ((!is_mapping(matrix_info = "/aaa/daemons/accountd.c"->query_matrix_info(account)))) {
            break;
        }
        if ((sizeof(matrix_info["used_matrix"]) >= 50)) {
            status_state = 2;
        }
    } while (0);

    // --- label L0746 ---
    if ((!is_object(account_ob))) {
        return;
    }
    vip_sms = "/aaa/daemons/vip_smsd.c"->query_vip_sms_info(account);
    if ((!is_object(account_ob))) {
        return;
    }
    if ((-1 == vip_sms)) {
        account_ob->send_message(8165, "连接服务器错误，无法登入。（原因：1004）", 1);
        account_ob->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "无法取得玩家的VIP短信功能设置资讯。");
        }
        return;
    }
    phone_auth = "/aaa/daemons/accountd.c"->query_phone_auth_info(account, 1);
    if ((!is_object(account_ob))) {
        return;
    }
    alipay_info = "/aaa/daemons/accountd.c"->query_alipay_info(account);
    if ((!is_object(account_ob))) {
        return;
    }
    temp_block_info = ([ "end_ti": time_desc_to_int(account_info["temp_blocked_time"], 3), "reason": account_info["temp_blocked_reason"] ]);
    paras = ([ "privilege": privilege, "account_id": account_ob->query("account_id"), "service": service, "safe_status": status_state, "mac": (string)account_ob->query_temp("mac_addr"), "protects": protects, "protect_range": (int)protect_type, "is_bind": (int)is_binded, "all_protects": (int)all_protects, "passpod_type": (int)account_ob->query("passpod_type"), 0: phone_auth, alipay_info["alipay_account"]: "phah", "alipay_account": "", "login_mode": ((account_ob->query_temp("qr_code_info/login") == 2) ? 1 : 0) ]);
    agent->send_message(9048, account, auth_key, save_string(vip_sms), save_string(temp_block_info), save_string(paras));
    "/aaa/daemons/accountd.c"->update_account_info(account_info, ({ "last_login_ip", "last_login_time", "last_login_id" }));
    if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
        "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "发送用户端登入请求给GS游戏服务器\n");
    }
    return "time_left";

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        1
   Code segments found:         1
   Successfully decoded:        1
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          1
      - /inherit/misc/handler.c()
   ================================================================ */