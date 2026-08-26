/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: admintoold.o
 * Source: /aaa/daemons/admintoold.c
 * Inherits: (none)
 * Inherited programs: /feature/daemon/register.c(), /feature/daemon/handler_module.c()
 * Functions: 6 names, 6 code segments found
 * Strings: 40, Variables: 0
 *********************************************************************/

inherit "/feature/daemon/register.c";
inherit "/feature/daemon/handler_module.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

int auth_admintool(object agent, string account, string password, string mac_addr, mixed ret, mixed peer_ip);
private void destruct();
private void send_login_result(object agent, int result, string privilege, string msg, int cookie, string account, string spa_ip, int spa_port);
private void do_auth_admintool(object agent, string account, string password, string mac_addr, int cookie);
private void create();
private void _do_auth_admintool(mixed agent, string account, string password, string mac_addr, int cookie);

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
        auth_info["prompt"] = "ÕËºÅµÇÈëÊ§°Ü¡££¨Ô­Òò£º2001£©";
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
        auth_info["prompt"] = "ÕËºÅ²»´æÔÚ";
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
        auth_info["prompt"] = "IPµØÖ·²»ÔÚËø¶¨·¶Î§ÄÚ";
        auth_info["privilege"] = "";
        return 0;
    }
    if (((account_info["permit_id"] != "") && (!check_for_permit(account_info["permit_id"], mac_addr, ",;/")))) {
        auth_info["result"] = 0;
        auth_info["prompt"] = "»úÆ÷IDµØÖ·²»ÔÚËø¶¨·¶Î§ÄÚ";
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
// @code  nc=4, nargs=0, insts=4
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=16, nargs=8, insts=16
// --------------------------------------------------------------------
private void send_login_result(object agent, int result, string privilege, string msg, int cookie, string account, string spa_ip, int spa_port) {
    /* ---- Enhanced Pseudocode ---- */
    agent->send_message(9986, result, privilege, msg, cookie, account, spa_ip, spa_port);

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
// @brief  constructor: initialize object state
// @code  nc=13, nargs=0, insts=13
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
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
   Total function names:        6
   Code segments found:         6
   Successfully decoded:        6
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          2
      - /feature/daemon/register.c()
      - /feature/daemon/handler_module.c()
   ================================================================ */