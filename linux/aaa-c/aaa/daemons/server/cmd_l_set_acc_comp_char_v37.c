/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_set_acc_comp_char.o
 * Source: /aaa/daemons/server/cmd_l_set_acc_comp_char.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 6, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void set_acc_comp_char(mixed agent, string gid, string name, string dist, string account, string branch, string comp);
void main(object agent, string gid, string name, string dist, string account, string branch, string comp);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=42, nargs=7, insts=42
// --------------------------------------------------------------------
private void set_acc_comp_char(mixed agent, string gid, string name, string dist, string account, string branch, string comp) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed sql_cmd;

    cno = 0;
    if ((!is_object(agent))) {
        return;
    }
    if (((cno = "/daemons/dbd.c"->assure_connection("adb")) < 0)) {
        return;
    }
    sql_cmd = mysql_bind_parameters("insert into acc_comp(dist, account, branch, comp, time, gid, name) values(?, ?, ?, ?, ?, ?, ?)", dist, account, branch, comp, ctime(time(), 3), gid, name);
    (*f_execute)(cno, sql_cmd);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=24, nargs=7, insts=24
// --------------------------------------------------------------------
void main(object agent, string gid, string name, string dist, string account, string branch, string comp) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: set_acc_comp_char, agent, gid, name, dist, account, branch, comp :));

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