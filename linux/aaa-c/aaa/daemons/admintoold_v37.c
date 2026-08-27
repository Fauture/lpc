/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: admintoold.o
 * Source: /aaa/daemons/admintoold.c
 * Inherits: (none)
 * Inherited programs: /feature/daemon/register.c(), /feature/daemon/handler_module.c()
 * Functions: 10 names, 10 code segments found
 * Strings: 56, Variables: 4
 *********************************************************************/

inherit "/feature/daemon/register.c";
inherit "/feature/daemon/handler_module.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   mapping block_account_list;
   /* mixed global_0; */
   string gy_block_url;
   string gy_unblock_url;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

int auth_admintool(object agent, string account, string password, string mac_addr, mixed ret, mixed peer_ip);
private void destruct();
private void send_login_result(object agent, int result, string privilege, string msg, int cookie, string account, string spa_ip, int spa_port);
void add_notice_gy(string account, string ma_account, int interval, string reason);
void save();
private void do_auth_admintool(object agent, string account, string password, string mac_addr, int cookie);
private void notice_gy_block(string key);
private void timer_save();
private void create();
private void _do_auth_admintool(mixed agent, string account, string password, string mac_addr, int cookie);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   gy_block_url = "http://interface.account.gyyx.cn/batchuser/suspend/?";  // /aaa/daemons/admintoold.c:31
   gy_unblock_url = "http://interface.account.gyyx.cn/batchuser/resume/?";
   block_account_list = ([ ]);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=244, nargs=6, insts=244
// --------------------------------------------------------------------
int auth_admintool(object agent, string account, string password, string mac_addr, mixed ret, mixed peer_ip) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed md5password;
    mixed sql_cmd;
    mapping account_info;
    mapping auth_info;
    mixed legal_ip;

    auth_info = ([ ]);
    ret = auth_info;
    md5password = save_string(md5(get_raw_password(("!*" + password))))[1..<2];
    if (((cno = "/daemons/dbd.c"->assure_connection("adb")) == -1)) {
        printf("[1;31mÁ¬½Ó ADB Êý¾Ý¿âÊ§°Ü\n[2;37;0m");
        auth_info["result"] = 0;
        auth_info["prompt"] = "ÕÊºÅµÇÂ¼Ê§°Ü¡££¨Ô­Òò£º2001£©";
        auth_info["privilege"] = "";
        return 0;
    }
    sql_cmd = mysql_bind_parameters("select * from admin_acc where account = ?", account);
    account_info = mysql_select_single_result(cno, sql_cmd);
    if ((!is_object(agent))) {
        return 0;
    }
    if ((!is_mapping(account_info))) {
        auth_info["result"] = 2;
        auth_info["prompt"] = "ÕÊºÅ²»´æÔÚ";
        auth_info["privilege"] = "";
        return 0;
    }
    if ((stricmp(account_info["password"], md5password) != 0)) {
        auth_info["result"] = 3;
        auth_info["prompt"] = "ÃÜÂë²»ÕýÈ·";
        auth_info["privilege"] = "";
        return 0;
    }
    if (is_string(peer_ip)) {
        legal_ip = peer_ip;
    } else {
        legal_ip = agent->get_peer_ip();
    }
    if (((account_info["permit_ip"] != "") && (!check_for_permit(account_info["permit_ip"], legal_ip, ",;/")))) {
        auth_info["result"] = 0;
        auth_info["prompt"] = "IPµØÖ·²»ÔÚ°ó¶¨·¶Î§ÄÚ";
        auth_info["privilege"] = "";
        return 0;
    }
    if (((account_info["permit_id"] != "") && (!check_for_permit(account_info["permit_id"], mac_addr, ",;/")))) {
        auth_info["result"] = 0;
        auth_info["prompt"] = "»úÆ÷IDµØÖ·²»ÔÚ°ó¶¨·¶Î§ÄÚ";
        auth_info["privilege"] = "";
        return 0;
    }
    auth_info["privilege"] = "/aaa/daemons/configd.c"->analyse_privilege("admintool", account_info["privilege"]);
    auth_info["result"] = 1;
    auth_info["prompt"] = "";
    return 1;

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=18, nargs=0, insts=18
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    if (sizeof(block_account_list)) {
        register_daemon_data("/aaa/daemons/admintoold.c", "block_account_list", block_account_list);
    }
    if (get_timer("tNoticeGyBlock")) {
        delete_timer("tNoticeGyBlock");
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=16, nargs=8, insts=16
// --------------------------------------------------------------------
private void send_login_result(object agent, int result, string privilege, string msg, int cookie, string account, string spa_ip, int spa_port) {
    /* ---- Enhanced Pseudocode ---- */
    agent->send_message(16631, result, privilege, msg, cookie, account, spa_ip, spa_port);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=80, nargs=4, insts=80
// --------------------------------------------------------------------
void add_notice_gy(string account, string ma_account, int interval, string reason) {
    /* ---- Enhanced Pseudocode ---- */
    int key;

    if ((interval == 0)) {
        key = sprintf("UNBLOCK@%s@%s", ma_account, reason);
    } else {
        key = sprintf("BLOCK@%s@%s@%d", ma_account, reason, ((interval < 0) ? 0 : maximum(1, (interval / 86400))));
    }
    if ((!is_mapping(block_account_list[key]))) {
        block_account_list[key] = ([ account: 1 ]);
    } else {
        block_account_list[key][account] = 1;
    }
    if ((sizeof(block_account_list[key]) >= 1000)) {
        "/daemons/threadd.c"->dispatch_service((: notice_gy_block, key :));
    } else if ((!get_timer("tNoticeGyBlock"))) {
        set_timer(60, "tNoticeGyBlock", (: timer_save :));
    }

}

// --------------------------------------------------------------------
// @code  nc=14, nargs=0, insts=14
// --------------------------------------------------------------------
void save() {
    /* ---- Enhanced Pseudocode ---- */
    mixed key;

    loop (key in keys(block_account_list)) {
        notice_gy_block(key);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=16, nargs=5, insts=16
// --------------------------------------------------------------------
private void do_auth_admintool(object agent, string account, string password, string mac_addr, int cookie) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch(random(10), (: _do_auth_admintool, agent, account, password, mac_addr, cookie :));

}

// --------------------------------------------------------------------
// @brief  notification: send message
// @code  nc=163, nargs=1, insts=163
// --------------------------------------------------------------------
private void notice_gy_block(string key) {
    /* ---- Enhanced Pseudocode ---- */
    mixed suspend_days;
    mixed arr_account;
    mixed ret;
    mixed ma_account;
    mixed reason;
    mixed body;
    mixed url;
    mixed str_account;
    mixed checksum;
    mixed m_account;

    m_account = block_account_list[key];
    if (((!is_mapping(m_account)) || (sizeof(m_account) < 1))) {
        return;
    }
    arr_account = keys(m_account);
    map_delete(block_account_list, key);
    if ((sscanf(key, "BLOCK@%s@%s@%d", &ma_account, &reason, &suspend_days) == 3)) {
        str_account = implode(arr_account, ",");
        body = sprintf("{AccountListStr: %O}", str_account);
        checksum = save_string(md5((body + "gyyx")))[1..<2];
        reason = ((strlen(reason) < 1) ? "unknown" : reason);
        url = sprintf((gy_block_url + "operator_account=%s&suspend_days=%d&operator_reason=%s&sign=%s"), ma_account, suspend_days, convert_to_url_encode(reason), checksum);
        ret = "/daemons/http_clientd.c"->http_post_web(url, 15, 65535, body);
    } else if ((sscanf(key, "UNBLOCK@%s@%s", &ma_account, &reason) == 2)) {
        str_account = implode(arr_account, ",");
        body = sprintf("{AccountListStr: %O}", str_account);
        checksum = save_string(md5((body + "gyyx")))[1..<2];
        reason = ((strlen(reason) < 1) ? "unknown" : reason);
        url = sprintf((gy_unblock_url + "operator_account=%s&operator_reason=%s&sign=%s"), ma_account, convert_to_url_encode(reason), checksum);
        ret = "/daemons/http_clientd.c"->http_post_web(url, 15, 65535, body);
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=9, nargs=0, insts=9
// --------------------------------------------------------------------
private void timer_save() {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_service((: save :));

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=31, nargs=0, insts=31
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;

    if (is_mapping(m = get_daemon_data("/aaa/daemons/admintoold.c"))) {
        if (is_mapping(m["block_account_list"])) {
            block_account_list = m["block_account_list"];
        }
    }
    "/daemons/connectiond.c"->register_auth("cmd_ma_l_account", (: do_auth_admintool :));
    load_all_handlers("/aaa/daemons/admintool/");

}

// --------------------------------------------------------------------
// @brief  internal handler: sub-action processor
// @code  nc=88, nargs=5, insts=88
// --------------------------------------------------------------------
private void _do_auth_admintool(mixed agent, string account, string password, string mac_addr, int cookie) {
    /* ---- Enhanced Pseudocode ---- */
    mixed auth_ret;
    mixed admintool;
    mapping auth_info;

    if ((!is_object(agent))) {
        return;
    }
    auth_ret = auth_admintool(agent, account, password, mac_addr, &auth_info);
    if ((!is_object(agent))) {
        return;
    }
    send_login_result(agent, auth_info["result"], auth_info["privilege"], auth_info["prompt"], cookie, account, "", 0);
    if ((!auth_ret)) {
        agent->to_be_disconnect(1);
        return;
    }
    admintool = clone_object("/aaa/clone/admintool.c");
    admintool->set_name(account);
    admintool->set("account", account);
    admintool->set("privilege", auth_info["privilege"]);
    agent->relay_agent(admintool);
    set_client_attrib(admintool->get_client_no(), ([ "type": "Internal", "peek": -1 ]));

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        10
   Code segments found:         10
   Successfully decoded:        10
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          2
      - /feature/daemon/register.c()
      - /feature/daemon/handler_module.c()
   ================================================================ */