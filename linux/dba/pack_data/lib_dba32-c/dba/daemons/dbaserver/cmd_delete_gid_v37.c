/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_delete_gid.o
 * Source: /dba/daemons/dbaserver/cmd_delete_gid.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 9, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
int main(mixed agent, int cookie, string gid);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
mixed get_db_type() {
    /* ---- Enhanced Pseudocode ---- */
    return "ddb";

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=163, nargs=3, insts=163
// --------------------------------------------------------------------
int main(mixed agent, int cookie, string gid) {
    /* ---- Enhanced Pseudocode ---- */
    mixed sql_cmd;
    mixed arr;
    mixed val64;
    mixed cno;

    if ((!is_object(agent))) {
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        agent->send_message(9233, cookie, -2);
        return 0;
    }
    val64 = atoi64(("0x" + gid));
    if ((val64[0] > 0)) {
        gid = sprintf("%d%d", val64[0], val64[1]);
    } else {
        gid = sprintf("%d", val64[1]);
    }
    sql_cmd = mysql_bind_parameters("select gid from gid_info where gid=?", gid);
    arr = mysql_select(cno, sql_cmd);
    if ((!arr)) {
        return 0;
    }
    if ((sizeof(arr) <= 0)) {
        agent->send_message(9233, cookie, -1);
        return 0;
    }
    sql_cmd = mysql_bind_parameters("delete from gid_info where gid = ?", gid);
    if (((*f_execute)(cno, sql_cmd) < 1)) {
        agent->send_message(9233, cookie, -1);
        return 0;
    }
    sql_cmd = mysql_bind_parameters("select gid from gid_info where gid=?", gid);
    arr = mysql_select(cno, sql_cmd);
    if ((!arr)) {
        return 0;
    }
    if ((sizeof(arr) == 1)) {
        agent->send_message(9233, cookie, -1);
        return 0;
    }
    agent->send_message(9233, cookie, 0);
    return 1;

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