/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_client_connect_agent.o
 * Source: /aaa/daemons/account/cmd_l_client_connect_agent.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 1 names, 1 code segments found
 * Strings: 111, Variables: 0
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
// @code  nc=1320, nargs=4, insts=1320
// --------------------------------------------------------------------
void main(mixed account_ob, string account, int auth_key, string server) {
    /* ---- Enhanced Pseudocode ---- */
    mixed service;
    int privilege;
    mixed state;
    mixed btime;
    int status_state;
    mixed all_protects;
    mixed is_qr_login;
    mixed real_reason;
    string breason;
    mixed change_server;
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
    mapping phone_auth;
    mapping alipay_info;
    mixed mp_device;
    mixed coin_info;
    mapping phone_bind;
    mixed app_bind;
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
        account_ob->send_message(8165, "连接服务器错误，无法登录。（原因：1011）", 1);
        account_ob->to_be_disconnect(1);
        return;
    }
    agent = "/aaa/daemons/accountd.c"->assure_server_agent(server);
    if ((!is_object(agent))) {
        account_ob->send_message(8165, "无法连接服务器", 1);
        account_ob->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "AAA 无法连接GS服务器，登录失败\n");
        }
        return;
    }
    if (account_ob->lite_query_temp("select_server")) {
        return;
    }
    if ((!dist = account_ob->lite_query("login_dist"))) {
        account_ob->send_message(8165, "连接服务器错误，无法登录。（原因：1003）", 1);
        account_ob->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "客户端还没有获取服务器列表就直接要求连接服务器\n");
        }
        return;
    }
    if ((is_string(change_server = "/aaa/daemons/gameserverd.c"->select_optimal_server(dist, server)) && (change_server != server))) {
        account_ob->send_message(8165, sprintf("由于您选择的#R%s#n过于繁忙，系统自动帮您分配到#R%s#n，如有需要请自行切换到所需的线路，祝您游戏愉快。", server, change_server), 1);
        server = change_server;
        agent = "/aaa/daemons/accountd.c"->assure_server_agent(server);
        if ((!is_object(agent))) {
            account_ob->send_message(8165, "无法连接服务器", 1);
            account_ob->to_be_disconnect(1);
            if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
                "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "AAA 无法连接GS服务器，登录失败\n");
            }
            return;
        }
    }
    check_block_day = 0;
    btime = account_info["blocked_time"];
    if ("/aaa/daemons/accountd.c"->is_apex_enabled()) {
        btime = trim((string)btime);
    }
    real_reason = account_info["blocked_reason"];
    breason = "运营中被封闭的帐号";
    if (is_undefined(btime = time_desc_to_int(btime, 3))) {
        breason = "运营中被封闭的帐号。（原因：1010）";
        "/aaa/daemons/accountd.c"->get_extra_block_reason_msg(check_block_day, real_reason, &breason);
        account_ob->send_message(8165, breason, 1);
        account_ob->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", sprintf("该帐号被封，禁止登录，原因：%O\n", "block_time字段格式错误"));
        }
        return;
    }
    if ((btime > time())) {
        left_deny_time = (btime - time());
        if ((left_deny_time < 15552000)) {
            left_deny_day = (left_deny_time / 86400);
            left_deny_day = ((left_deny_time % 86400) ? (left_deny_day + 1) : left_deny_day);
            check_block_day = left_deny_day;
            breason = sprintf("运营中被封闭的帐号，将于#R%d#n天后解除封闭。", left_deny_day);
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
        "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", sprintf("请求登录GS服务器:%s\n", server));
    }
    if (((account_ob->query_auth_key() != auth_key) || (stricmp(account_ob->query_account(), trim(account)) != 0))) {
        account_ob->send_message(8165, "连接服务器错误，无法登录。（原因：1002）", 1);
        account_ob->to_be_disconnect(1);
        if ((is_string(account_ob->query_account()) || is_string((account_ob->query_account())->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account_ob->query_account()), "login", sprintf("客户端发送的帐号(%s)或者AUTH_KEY(%d)不正确", account, auth_key));
        }
        return;
    }
    dist_info = "/aaa/daemons/configd.c"->get_dist_info(dist);
    if ((!is_mapping(dist_info))) {
        account_ob->send_message(8165, "连接服务器错误，无法登录。（原因：1004）", 1);
        account_ob->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "AAA 服务器取得玩家要求登录的区组信息，从而无法获取玩家的服务信息\n");
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
        account_ob->send_message(8165, "该帐号已被锁定，请先登录光宇游戏社区解除锁定！（原因：1012）", 1);
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

    // --- label L0691 ---
    if (account_ob->query("local_encrypt")) {
        service |= 262144;
    }
    if ((!is_object(account_ob))) {
        return;
    }
    if ((!is_object(agent))) {
        account_ob->send_message(8165, "无法连接服务器", 1);
        account_ob->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "AAA 无法连接GS服务器，登录失败\n");
        }
        return;
    }
    account_info["last_login_ip"] = account_ob->get_peer_ip();
    account_info["last_login_time"] = time_to_string();
    account_info["last_login_id"] = account_ob->query_temp("mac_addr");
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
    if ((is_mapping(matrix_info = "/aaa/daemons/accountd.c"->query_matrix_info(account)) && (sizeof(matrix_info["used_matrix"]) >= 50))) {
        status_state = 2;
    }
    if ((!is_object(account_ob))) {
        return;
    }
    coin_info = "/aaa/daemons/accountd.c"->get_coin_info(account_info);
    if ((!is_object(account_ob))) {
        return;
    }
    vip_sms = "/aaa/daemons/vip_smsd.c"->query_vip_sms_info(account);
    if ((!is_object(account_ob))) {
        return;
    }
    if ((-1 == vip_sms)) {
        account_ob->send_message(8165, "连接服务器错误，无法登录。（原因：1004）", 1);
        account_ob->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "无法取得玩家的VIP短信功能设置信息。");
        }
        return;
    }
    phone_auth = "/aaa/daemons/accountd.c"->query_phone_auth_info(account, 1);
    if ((!is_object(account_ob))) {
        return;
    }
    if ((-1 == phone_auth)) {
        account_ob->send_message(8165, "连接服务器错误，无法登录。（原因：1004）", 1);
        account_ob->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "无法取得玩家的手机认证信息。\n");
        }
        return;
    }
    phone_bind = "/aaa/daemons/accountd.c"->query_phone_bind_info(account, 1);
    if ((!is_object(account_ob))) {
        return;
    }
    if ((-1 == phone_bind)) {
        account_ob->send_message(8165, "连接服务器错误，无法登录。（原因：1004）", 1);
        account_ob->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "无法取得玩家的手机绑定信息。\n");
        }
        return;
    }
    app_bind = "/aaa/daemons/accountd.c"->query_app_bind_info(account, 1);
    if ((!is_object(account_ob))) {
        return;
    }
    if ((-1 == app_bind)) {
        account_ob->send_message(8165, "连接服务器错误，无法登录。（原因：1004）", 1);
        account_ob->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "无法取得玩家的光宇游戏APP绑定信息。\n");
        }
        return;
    }
    alipay_info = "/aaa/daemons/accountd.c"->query_alipay_info(account);
    if ((!is_object(account_ob))) {
        return;
    }
    if ((-1 == alipay_info)) {
        account_ob->send_message(8165, "连接服务器错误，无法登录。（原因：1004）", 1);
        account_ob->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "无法取得玩家的支付宝信息。\n");
        }
        return;
    }
    mp_device = "/aaa/daemons/accountd.c"->query_mp_device_info(account);
    if ((!is_object(account_ob))) {
        return;
    }
    if ((-1 == mp_device)) {
        account_ob->send_message(8165, "连接服务器错误，无法登录。（原因：1004）", 1);
        account_ob->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "无法取得玩家的小秘书消息推送设备信息。\n");
        }
        return;
    }
    temp_block_info = ([ "end_ti": time_desc_to_int(account_info["temp_blocked_time"], 3), "reason": account_info["temp_blocked_reason"] ]);
    is_qr_login = ((account_ob->query_temp("qr_code/login") == 2) ? 1 : 0);
    paras = ([ "time_left": 0, "privilege": privilege, "account_id": account_ob->query("account_id"), "service": service, "safe_status": status_state, "mac": (string)account_ob->query_temp("mac_addr"), "protects": protects, "protect_range": (int)protect_type, "is_bind": (int)is_binded, "all_protects": (int)all_protects, "passpod_type": (is_qr_login ? 1 : (int)account_ob->query("passpod_type")), "phah": ((int)phone_auth["state"] ? phone_auth : 0), "phbd": ((int)phone_bind["state"] ? phone_bind : 0), "appbd": (is_mapping(app_bind) ? app_bind : 0), "alipay_account": ((int)alipay_info["state"] ? alipay_info["alipay_account"] : ""), "qr_login": is_qr_login, "mp_device": mp_device, "active_path": (int)account_info["active_path"], "vip_sms_info": vip_sms, "temp_block_info": temp_block_info, "coin_info": coin_info ]);
    account_ob->lite_set_temp("select_server", server);
    agent->send_message(9048, account, auth_key, save_string(paras));
    "/aaa/daemons/accountd.c"->update_account_info(account_info, ({ "last_login_ip", "last_login_time", "last_login_id" }));
    if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
        "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "发送客户端登录请求给GS游戏服务器\n");
    }

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