/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_clone_account.o
 * Source: /aaa/daemons/server/cmd_l_clone_account.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 3 names, 3 code segments found
 * Strings: 21, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private int insert_account_info(mapping info, string table);
private void clone_account(object agent, string account, string password, int n);
int main(object agent, string account, string password, int n);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=94, nargs=2, insts=94
// --------------------------------------------------------------------
private int insert_account_info(mapping info, string table) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;
    string sql_cmd1;
    mixed arr;
    mixed mks;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno == -1)) {
        return 0;
    }
    sql_cmd = sprintf("insert into %s(", table);
    sql_cmd1 = "values(";
    arr = ({});
    loop (mks in info) {
        sql_cmd += sprintf("%s, ", mks[0]);
        sql_cmd1 += "?, ";
        arr += ({ mks[1] });
    }  // end loop (verified)
    sql_cmd1 = (sql_cmd1[0..<3] + ")");
    sql_cmd = ((sql_cmd[0..<3] + ") ") + sql_cmd1);
    sql_cmd = mysql_bind_parameters_arr(sql_cmd, arr);
    if (((*f_execute)(cno, sql_cmd) < 0)) {
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=166, nargs=4, insts=166
// --------------------------------------------------------------------
private void clone_account(object agent, string account, string password, int n) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    int i;
    mapping account_info;
    mixed md5_pwd;
    mixed arr;
    mixed inserted_name;
    mixed new_account;
    mixed sql_cmd;
    mixed send_str;
    mixed checksum;

    inserted_name = ({});
    account_info = "/aaa/daemons/accountd.c"->query_account_info(account);
    i = 1;

    // --- label L0017 ---
    while (i <= n) {
        new_account = sprintf("%s%d", account, i);
        account_info["account"] = new_account;
        cno = "/daemons/dbd.c"->assure_connection("adb");
        if ((cno == -1)) {
            return;
        }
        sql_cmd = mysql_bind_parameters("select * from account where account = ?", new_account);
        arr = mysql_select(cno, sql_cmd);
        if ((!arr)) {
            return;
        }
        if ((sizeof(arr) >= 1)) {
        } else {
            md5_pwd = save_string(md5(password))[1..<2];
            account_info["password"] = "/aaa/daemons/accountd.c"->calc_password_new_md5(new_account, md5_pwd);
            checksum = "/aaa/daemons/accountd.c"->calc_account_checksum(account_info);
            account_info["checksum"] = checksum;
            if ((insert_account_info(account_info, "account") == 0)) {
                send_str = save_string(inserted_name);
                agent->send_message(13190, account, send_str, n, 0);
                return;
            }
            inserted_name += ({ new_account });
        }
        ++i;
    }  // end loop (verified)
    send_str = save_string(inserted_name);
    if ((sizeof(inserted_name) >= 1)) {
        agent->send_message(13190, account, send_str, n, 1);
    } else {
        agent->send_message(13190, account, send_str, n, 0);
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=29, nargs=4, insts=29
// --------------------------------------------------------------------
int main(object agent, string account, string password, int n) {
    /* ---- Enhanced Pseudocode ---- */
    if ((n <= 0)) {
        return 0;
    }
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: clone_account, agent, account, password, n :));
    return 1;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        3
   Code segments found:         3
   Successfully decoded:        3
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          1
      - /inherit/misc/handler.c()
   ================================================================ */