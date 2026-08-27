/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_get_server_list.o
 * Source: /aaa/daemons/account/cmd_l_get_server_list.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 29, Variables: 2
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   int enable_select_optimal_server;
   /* mixed global_0; */

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private int sort_by_server_id(mapping data);
void main(mixed account_ob, string account, int auth_key, string dist, int network_type);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   enable_select_optimal_server = 1;

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
private int sort_by_server_id(mapping data) {
    /* ---- Enhanced Pseudocode ---- */
    return data["id"];

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=364, nargs=5, insts=364
// --------------------------------------------------------------------
void main(mixed account_ob, string account, int auth_key, string dist, int network_type) {
    /* ---- Enhanced Pseudocode ---- */
    mixed available;
    mixed online;
    mixed max_user;
    mixed status;
    int min_online;
    mixed gs_user_over;
    mixed gs_user_full;
    mixed gs_user_busy;
    mixed gs_cpu_full;
    mixed gs_cpu_busy;
    mixed cpu_cost;
    mixed buf;
    mixed tmp;
    int server;
    mixed ip;
    int min_online_server;
    mapping server_list;
    mapping data;

    if ((!is_object(account_ob))) {
        return;
    }
    if ((is_string(account_ob->query_account()) || is_string((account_ob->query_account())->query_temp("login_info/account")))) {
        "/daemons/logd.c"->log_login_info(get_login_info_file(account_ob->query_account()), "login", "客户端请求服务器列表\n");
    }
    if (((account_ob->query_auth_key() != auth_key) || (stricmp(account_ob->query_account(), trim(account)) != 0))) {
        if ((is_string(account_ob->query_account()) || is_string((account_ob->query_account())->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account_ob->query_account()), "login", sprintf("客户端发送的帐号(%s)或者AUTH_KEY(%d)不正确", account, auth_key));
        }
        return;
    }
    server_list = (mapping)"/aaa/daemons/configd.c"->get_server_list_by_dist(dist);
    account_ob->lite_set("login_dist", dist);
    gs_user_over = "/aaa/daemons/configd.c"->get_global_config("gs_user_over");
    gs_user_full = "/aaa/daemons/configd.c"->get_global_config("gs_user_full");
    gs_user_busy = "/aaa/daemons/configd.c"->get_global_config("gs_user_busy");
    gs_cpu_full = "/aaa/daemons/configd.c"->get_global_config("gs_cpu_full");
    gs_cpu_busy = "/aaa/daemons/configd.c"->get_global_config("gs_cpu_busy");
    gs_user_over = atof((gs_user_over ? gs_user_over : 0));
    gs_user_full = atof((gs_user_full ? gs_user_full : 0));
    gs_user_busy = atof((gs_user_busy ? gs_user_busy : 0));
    gs_cpu_full = atof((gs_cpu_full ? gs_cpu_full : 0));
    gs_cpu_busy = atof((gs_cpu_busy ? gs_cpu_busy : 0));
    min_online = 2147483647;
    buf = pkt_pad_buffer(sizeof(server_list));
    tmp = allocate_buffer(0);
    loop (data in sort(values(server_list), (: sort_by_server_id :))) {
        server = data["name"];
        ip = select_server_ip(network_type, data);
        available = atoi((data["available"] ? data["available"] : 0));
        online = atoi((data["online"] ? data["online"] : 0));
        max_user = atoi((data["max_user"] ? data["max_user"] : 0));
        cpu_cost = (atof((data["cpu_cost"] ? data["cpu_cost"] : 0)) / 100);
        status = calc_server_status_as_aaa(available, online, max_user, cpu_cost, gs_user_over, gs_user_full, gs_user_busy, gs_cpu_full, gs_cpu_busy);
        buf += pkt_pad_buffer(server, ip);
        tmp += pkt_pad_buffer(status);
        if ((((enable_select_optimal_server && online && (online < min_online)) && available) && (!is_mapping(limit_login_server[server])))) {
            min_online = online;
            min_online_server = server;
        }
    }  // end loop (verified)
    account_ob->lite_set_temp("network_type", network_type);
    if ((enable_select_optimal_server && is_string(min_online_server))) {
        optimal_login_server = min_online_server;
        optimal_login_server_time = time();
    }
    if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
        "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "发送服务器列表给客户端\n");
    }
    send_to_client(account_ob->get_client_no(), 17237, (buf + tmp));

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