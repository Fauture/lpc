/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_set_local_encrypt_info.o
 * Source: /aaa/daemons/server/cmd_l_set_local_encrypt_info.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 27, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void set_local_encrypt_info(mixed agent, string account, string text);
void main(object agent, string account, string text);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=260, nargs=3, insts=260
// --------------------------------------------------------------------
private void set_local_encrypt_info(mixed agent, string account, string text) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed arr;
    mixed sql_cmd;
    mixed file_name;
    mixed str;
    mixed log_sql_cmd;
    mapping m;
    mixed local_encrypt_info;

    try {
        m = restore_value(text);
    } catch { /* error handled */ }

    // --- label L0012 ---
    if ((0 || (!is_mapping(m)))) {
        return;
    }
    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 1)) {
        return;
    }
    sql_cmd = mysql_bind_parameters("select * from local_encrypt where account = ?", account);
    if ((!is_array(arr = mysql_select(cno, sql_cmd)))) {
        return;
    }
    if ((!is_object(agent))) {
        return;
    }
    if (m["unlock"]) {
        if ((!sizeof(arr))) {
            return;
        }
        sql_cmd = mysql_bind_parameters("delete from local_encrypt where account = ?", account);
        if (((*f_execute)(cno, sql_cmd) < 1)) {
            return;
        }
        log_sql_cmd = mysql_bind_parameters("insert into acc_log(ip, time,account, dist, type, msg, memo) values(?, ?, ?, ?, ?, ?, ?)", agent->get_peer_ip(), ctime(time(), 3), account, "", "pwd_protect", "205", arr[0]["time"]);
        (*f_execute)(cno, log_sql_cmd);
        if ((!is_object(agent))) {
            return;
        }
        agent->send_message(5015, account, 2);
        return;
    }
    local_encrypt_info = ([ "encrypt_type": m["encrypt_type"], "seed1": m["seed1"], "seed2": m["seed2"], "replace_str": m["replace_str"][0..9], "file_length": m["file_length"], "auth_key": m["auth_key"] ]);
    file_name = save_string(md5(account))[1..<2];
    str = save_string(local_encrypt_info);
    if ((sizeof(arr) < 1)) {
        sql_cmd = mysql_bind_parameters("insert into local_encrypt(account, file_name, local_encrypt_info, state, time, checksum) values(?, ?, ?, ?, ?, ?)", account, file_name, str, (int)m["state"], (string)m["time"], "/aaa/daemons/accountd.c"->calc_local_encrypt_checksum(m, str));
        log_sql_cmd = mysql_bind_parameters("insert into acc_log(ip, time,account, dist, type, msg, memo) values(?, ?, ?, ?, ?, ?, ?)", agent->get_peer_ip(), ctime(time(), 3), account, "", "pwd_protect", "105", "");
    } else {
        sql_cmd = mysql_bind_parameters("update local_encrypt set file_name = ?, local_encrypt_info = ?, state = ?, time = ?, checksum = ? where account = ? ", file_name, str, (int)m["state"], (string)m["time"], "/aaa/daemons/accountd.c"->calc_local_encrypt_checksum(m, str), account);
    }
    if (((*f_execute)(cno, sql_cmd) < 1)) {
        return;
    }
    if ((is_string(log_sql_cmd) && (cno > 0))) {
        (*f_execute)(cno, log_sql_cmd);
    }
    if ((!is_object(agent))) {
        return;
    }
    agent->send_message(5015, account, 1);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=20, nargs=3, insts=20
// --------------------------------------------------------------------
void main(object agent, string account, string text) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: set_local_encrypt_info, agent, account, text :));

}

/* ================================================================
   DECLARED FUNCTIONS (no bytecode - likely inherited or forward decl)
   ================================================================ */
/*
   private mixed calc_local_encrypt_checksum(mapping m, mapping le_info);  // inherited from parent or forward declaration
*/

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        2
   Code segments found:         2
   Successfully decoded:        2
   Unresolved/minimal:          0
   Functions without code:      1
   Inherited programs:          1
      - /inherit/misc/handler.c()

   Note: Functions without code are usually inherited from the
   parent object (), forward declarations, or
   simple property accessor methods that the compiler optimizes away.
   ================================================================ */