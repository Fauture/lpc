/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: msg_l_server_result.o
 * Source: /aaa/daemons/account/msg_l_server_result.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 1 names, 1 code segments found
 * Strings: 20, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void main(mixed agent, int result, int auth_key, int seed, string msg);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=195, nargs=5, insts=195
// --------------------------------------------------------------------
void main(mixed agent, int result, int auth_key, int seed, string msg) {
    /* ---- Enhanced Pseudocode ---- */
    mixed privilege;
    int port;
    mixed account_ob;
    string ip;
    mapping server_info;

    if ((!is_object(agent))) {
        return;
    }
    account_ob = "/aaa/daemons/accountd.c"->get_account_ob_by_auth_key(auth_key);
    if ((!is_object(account_ob))) {
        return;
    }
    privilege = account_ob->query_privilege();
    ip = "";
    port = 0;
    if ((!result)) {
        account_ob->send_message(13143, result, privilege, ip, port, seed, msg);
        if ((is_string(account_ob->query_account()) || is_string((account_ob->query_account())->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account_ob->query_account()), "login", "GS服务器禁止玩家登入\n");
        }
        return;
    }
    server_info = "/aaa/daemons/configd.c"->get_server_info(agent->get_name());
    if ((((!is_mapping(server_info)) || (!is_int(server_info["port"]))) || (server_info["port"] == ""))) {
        msg = "连接服务器错误，无法登入。（原因：1005）";
        account_ob->send_message(13143, 0, privilege, ip, port, seed, msg);
        if ((is_string(account_ob->query_account()) || is_string((account_ob->query_account())->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account_ob->query_account()), "login", "获取服务器埠资讯失败，玩家无法登入\n");
        }
        return;
    }
    ip = select_server_ip(account_ob->query_temp("network_type"), server_info);
    port = server_info["port"];
    account_ob->send_message(13143, result, privilege, ip, port, seed, msg);
    if ((is_string(account_ob->query_account()) || is_string((account_ob->query_account())->query_temp("login_info/account")))) {
        "/daemons/logd.c"->log_login_info(get_login_info_file(account_ob->query_account()), "login", sprintf("GS服务器允许玩家登入，发送 SEED:%d\n", seed));
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