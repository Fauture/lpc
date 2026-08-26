/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_borrow_local_encrypt.o
 * Source: /aaa/daemons/server/cmd_l_borrow_local_encrypt.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 19, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void borrow_local_encrypt(mixed agent, string account, string file_name, string auth_key);
void main(object agent, string account, string file_name, string auth_key);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=155, nargs=4, insts=155
// --------------------------------------------------------------------
private void borrow_local_encrypt(mixed agent, string account, string file_name, string auth_key) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed sql_cmd;
    mixed log_sql_cmd;
    mapping m;
    mapping le_info;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 1)) {
        return;
    }
    sql_cmd = mysql_bind_parameters("select * from local_encrypt where file_name = ?", file_name);
    m = mysql_select_single_result(cno, sql_cmd);
    if ((!is_mapping(m))) {
        if (is_object(agent)) {
            agent->send_message(5041, account, 0);
        }
        return;
    }
    m["account"] = account;
    m["time"] = ctime(time(), 3);
    le_info = restore_value(m["local_encrypt_info"]);
    if ((le_info["auth_key"] != auth_key)) {
        agent->send_message(5041, account, 0);
        return;
    }
    sql_cmd = mysql_bind_parameters("insert into local_encrypt(account, file_name, local_encrypt_info, state, time, checksum) values(?, ?, ?, ?, ?, ?)", account, file_name, m["local_encrypt_info"], 1, m["time"], "/aaa/daemons/accountd.c"->calc_local_encrypt_checksum(m, m["local_encrypt_info"]));
    if (((*f_execute)(cno, sql_cmd) < 1)) {
        agent->send_message(5041, account, 0);
        return;
    }
    if ((cno > 0)) {
        log_sql_cmd = mysql_bind_parameters("insert into acc_log(ip, time,account, dist, type, msg, memo) values(?, ?, ?, ?, ?, ?, ?)", agent->get_peer_ip(), ctime(time(), 3), account, "", "pwd_protect", "105", "");
        (*f_execute)(cno, log_sql_cmd);
    }
    if ((!is_object(agent))) {
        return;
    }
    agent->send_message(5041, account, 1);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=21, nargs=4, insts=21
// --------------------------------------------------------------------
void main(object agent, string account, string file_name, string auth_key) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: borrow_local_encrypt, agent, account, file_name, auth_key :));

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