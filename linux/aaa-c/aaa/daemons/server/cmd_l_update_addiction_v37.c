/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_update_addiction.o
 * Source: /aaa/daemons/server/cmd_l_update_addiction.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 12, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void update_addiction(mixed agent, string account, string text);
void main(object agent, string account, string text);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=119, nargs=3, insts=119
// --------------------------------------------------------------------
private void update_addiction(mixed agent, string account, string text) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed arr;
    mixed sql_cmd;
    mapping m;

    try {
        m = restore_value(text);
    } catch { /* error handled */ }

    // --- label L0012 ---
    if ((0 || (!is_mapping(m)))) {
        return;
    }
    if ((!is_object(agent))) {
        return;
    }
    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 1)) {
        return;
    }
    sql_cmd = mysql_bind_parameters("select * from antiaddiction where account = ?", account);
    arr = mysql_select(cno, sql_cmd);
    if ((!is_array(arr))) {
        return;
    }
    if ((sizeof(arr) < 1)) {
        sql_cmd = mysql_bind_parameters("insert into antiaddiction(account, last_offline, total_offline, total_online) values(?, ?, ?, ?)", account, (int)m["last_offline"], (int)m["total_offline"], (int)m["total_online"]);
    } else {
        sql_cmd = mysql_bind_parameters("update antiaddiction set last_offline = ?, total_offline = ?, total_online = ? where account = ?", (int)m["last_offline"], (int)m["total_offline"], (int)m["total_online"], account);
    }
    if (((*f_execute)(cno, sql_cmd) < 1)) {
        return;
    }
    if ((!is_object(agent))) {
        return;
    }
    agent->send_message(9103, account, text);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=20, nargs=3, insts=20
// --------------------------------------------------------------------
void main(object agent, string account, string text) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: update_addiction, agent, account, text :));

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