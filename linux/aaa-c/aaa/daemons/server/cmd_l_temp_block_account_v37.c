/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_temp_block_account.o
 * Source: /aaa/daemons/server/cmd_l_temp_block_account.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 19, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void temp_block_account(mixed agent, string server_name, string account, int interval, string reason);
void main(object agent, string server_name, string account, int interval, string reason);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=94, nargs=5, insts=94
// --------------------------------------------------------------------
private void temp_block_account(mixed agent, string server_name, string account, int interval, string reason) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed ret;
    mixed prompt;
    string dist;
    mixed msg;
    mixed sql_cmd;

    if ((!is_object(agent))) {
        return;
    }
    dist = (string)agent->query("dist");
    ret = "/aaa/daemons/accountd.c"->temp_block_account(dist, account, interval, reason, &prompt);
    if (((cno = "/daemons/dbd.c"->assure_connection("adb")) == -1)) {
        return;
    }
    msg = sprintf("GS游戏服务器%s%s账号%s，原因:%s =>", agent->get_name(), ((interval == 0) ? "解除封停" : "暂时封停"), ((interval == 0) ? "" : sprintf("%d秒", interval)), reason);
    sql_cmd = mysql_bind_parameters("insert into acc_log(ip, time,account, dist, type, msg, memo) values(?, ?, ?, ?, ?, ?, ?)", agent->get_peer_ip(), ctime(time(), 3)[0..13], account, "", "temp_block_account", (msg + (ret ? "成功" : "失败")), "");
    (*f_execute)(cno, sql_cmd);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=22, nargs=5, insts=22
// --------------------------------------------------------------------
void main(object agent, string server_name, string account, int interval, string reason) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: temp_block_account, agent, server_name, account, interval, reason :));

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