/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_block_account.o
 * Source: /aaa/daemons/server/cmd_l_block_account.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 22, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void block_account(mixed agent, string server_name, string account, int interval, string reason, string ma_account, int to_gy);
void main(object agent, string server_name, string account, int interval, string reason, string ma_account, int to_gy);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=119, nargs=7, insts=119
// --------------------------------------------------------------------
private void block_account(mixed agent, string server_name, string account, int interval, string reason, string ma_account, int to_gy) {
    /* ---- Enhanced Pseudocode ---- */
    string dist;
    mixed cno;
    mixed ret;
    mixed msg;
    mixed sql_cmd;

    if ((!is_object(agent))) {
        return;
    }
    dist = (string)agent->query("dist");
    cno = "/daemons/dbd.c"->assure_connection("adb");
    ret = "/aaa/daemons/accountd.c"->block_account(dist, account, interval, reason, 1);
    }
    msg = sprintf("GS游戏服务器%s%s帐号%s，原因:%s =>", agent->get_name(), ((interval == 0) ? "解封" : "封闭"), ((interval < 0) ? "永久" : (("" + interval) + "秒")), reason);
    if ((to_gy && (-2 != interval))) {
        "/aaa/daemons/admintoold.c"->add_notice_gy(account, ma_account, interval, reason);
    }
    if ((cno == -1)) {
        return;
    }
    sql_cmd = mysql_bind_parameters("insert into acc_log(ip, time,account, dist, type, msg, memo) values(?, ?, ?, ?, ?, ?, ?)", agent->get_peer_ip(), ctime(time(), 3)[0..13], account, "", "block_account", (msg + (ret ? "成功" : "失败")), "");
    (*f_execute)(cno, sql_cmd);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=24, nargs=7, insts=24
// --------------------------------------------------------------------
void main(object agent, string server_name, string account, int interval, string reason, string ma_account, int to_gy) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: block_account, agent, server_name, account, interval, reason, ma_account, to_gy :));

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