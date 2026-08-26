/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_get_acc_comp_char.o
 * Source: /aaa/daemons/server/cmd_l_get_acc_comp_char.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 8, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void get_acc_comp_char(mixed agent, string gid, string dist, string account, string branch, string comp);
void main(object agent, string gid, string dist, string account, string branch, string comp);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=74, nargs=6, insts=74
// --------------------------------------------------------------------
private void get_acc_comp_char(mixed agent, string gid, string dist, string account, string branch, string comp) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed sql_cmd;
    mapping m;

    cno = 0;
    if ((!is_object(agent))) {
        return;
    }
    if (((cno = "/daemons/dbd.c"->assure_connection("adb")) < 0)) {
        return;
    }
    sql_cmd = mysql_bind_parameters("select gid, name from acc_comp where dist = ? and account = ? and branch = ? and comp = ?", dist, account, branch, comp);
    m = mysql_select_single_result(cno, sql_cmd);
    if ((!is_mapping(m))) {
        m = ([ ]);
        m["ret"] = 0;
    } else {
        m["ret"] = 1;
    }
    if ((!is_object(agent))) {
        return;
    }
    agent->send_message(5027, gid, comp, save_string(m));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=23, nargs=6, insts=23
// --------------------------------------------------------------------
void main(object agent, string gid, string dist, string account, string branch, string comp) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: get_acc_comp_char, agent, gid, dist, account, branch, comp :));

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