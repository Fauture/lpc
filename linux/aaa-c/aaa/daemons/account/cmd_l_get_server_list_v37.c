/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_get_server_list.o
 * Source: /aaa/daemons/account/cmd_l_get_server_list.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 32, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private int sort_by_server_id(mapping data);
void main(mixed account_ob, string account, int auth_key, string dist, int network_type);

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
// @code  nc=303, nargs=5, insts=303
// --------------------------------------------------------------------
void main(mixed account_ob, string account, int auth_key, string dist, int network_type) {
    /* ---- Enhanced Pseudocode ---- */
    mixed status;
    mixed server_info;
    mixed buf;
    mixed tmp;
    mixed server;
    mixed ip;
    mapping server_list;
    mapping data;
    //mixed <loop_1>;
    mixed available;
    mixed online;
    mixed max_user;
    mixed cpu_cost;
    mixed gs_user_over;
    mixed gs_user_full;
    mixed gs_user_busy;
    mixed gs_cpu_full;
    mixed gs_cpu_busy;

    if ((!is_object(account_ob))) {
        return;
    }
    if ((is_string(account_ob->query_account()) || is_string((account_ob->query_account())->query_temp("login_info/account")))) {
        "/daemons/logd.c"->log_login_info(get_login_info_file(account_ob->query_account()), "login", "用户端请求服务器列表\n");
    }
    if (((account_ob->query_auth_key() != auth_key) || (stricmp(account_ob->query_account(), trim(account)) != 0))) {
        if ((is_string(account_ob->query_account()) || is_string((account_ob->query_account())->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account_ob->query_account()), "login", sprintf("用户端发送的账号(%s)或者AUTH_KEY(%d)不正确", account, auth_key));
        }
        return;
    }
    server_list = (mapping)"/aaa/daemons/configd.c"->get_server_list_by_dist(dist);
    account_ob->set("login_dist", dist);
    server_info = ({});
    buf = pkt_pad_buffer(sizeof(server_list));
    tmp = allocate_buffer(0);
    loop (data in sort(values(server_list), (: sort_by_server_id :))) {
        server = data["name"];
        ip = select_server_ip(network_type, data);
        available = data["available"];
        online = data["online"];
        max_user = data["max_user"];
        cpu_cost = data["cpu_cost"];
        gs_user_over = "/aaa/daemons/configd.c"->get_global_config("gs_user_over");
        gs_user_full = "/aaa/daemons/configd.c"->get_global_config("gs_user_full");
        gs_user_busy = "/aaa/daemons/configd.c"->get_global_config("gs_user_busy");
        gs_cpu_full = "/aaa/daemons/configd.c"->get_global_config("gs_cpu_full");
        gs_cpu_busy = "/aaa/daemons/configd.c"->get_global_config("gs_cpu_busy");
        status = calc_server_status_as_aaa(atoi((available ? available : 0)), atoi((online ? online : 0)), atoi((max_user ? max_user : 0)), (atof((cpu_cost ? cpu_cost : 0)) / 100), atof((gs_user_over ? gs_user_over : 0)), atof((gs_user_full ? gs_user_full : 0)), atof((gs_user_busy ? gs_user_busy : 0)), atof((gs_cpu_full ? gs_cpu_full : 0)), atof((gs_cpu_busy ? gs_cpu_busy : 0)));
        server_info += ({ ([ "server": server, "ip": ip, "status": status ]) });
        buf += pkt_pad_buffer(server, ip);
        tmp += pkt_pad_buffer(status);
    }  // end loop (verified)
    account_ob->set_temp("network_type", network_type);
    if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
        "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "发送服务器列表给用户端\n");
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