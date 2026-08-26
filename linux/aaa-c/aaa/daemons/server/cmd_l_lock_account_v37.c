/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_lock_account.o
 * Source: /aaa/daemons/server/cmd_l_lock_account.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 21, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void lock_account(mixed agent, string account);
void main(object agent, string account);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=132, nargs=2, insts=132
// --------------------------------------------------------------------
private void lock_account(mixed agent, string account) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    string state;
    string msg;
    mixed sql_cmd;
    mapping account_info;

    if ((!is_object(agent))) {
        return;
    }
    state = "0";
    msg = "锁定账号失败。";
    do {
        account_info = "/aaa/daemons/accountd.c"->query_account_info(account);
        if ((!is_mapping(account_info))) {
            break;
        }
        if ((account_info["checksum"] != "/aaa/daemons/accountd.c"->calc_account_checksum(account_info))) {
            msg = "锁定账号失败（原因：1009）。";
            break;
        }
        if ((account_info["locked"] == "1")) {
            state = "1";
            msg = "该账号已经处于锁定状态，不需要再次锁定。";
            break;
        }
        account_info["locked"] = "1";
        if ((!"/aaa/daemons/accountd.c"->update_account_info(account_info))) {
            break;
        }
        state = "1";
        msg = "账号锁定成功，下次登入前请先登入逍遥会员中心解除锁定。";
    } while (0);

    // --- label L0082 ---
    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno >= 0)) {
        sql_cmd = mysql_bind_parameters("insert into acc_log(ip, time,account, dist, type, msg, memo) values(?, ?, ?, ?, ?, ?, ?)", "", ctime(time(), 3)[0..13], account, "", "lock_account", state, msg);
        (*f_execute)(cno, sql_cmd);
    }
    if ((!is_object(agent))) {
        return;
    }
    agent->send_message(13193, account, state, msg);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=19, nargs=2, insts=19
// --------------------------------------------------------------------
void main(object agent, string account) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: lock_account, agent, account :));

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