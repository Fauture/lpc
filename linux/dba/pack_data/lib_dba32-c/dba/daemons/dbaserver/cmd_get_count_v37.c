/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_get_count.o
 * Source: /dba/daemons/dbaserver/cmd_get_count.c
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
int main(mixed agent, int cookie, string path, string name, string branch);

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
// @code  nc=93, nargs=5, insts=93
// --------------------------------------------------------------------
int main(mixed agent, int cookie, string path, string name, string branch) {
    /* ---- Enhanced Pseudocode ---- */
    mixed sql_cmd;
    mixed arr;
    mixed cno;

    if ((!is_object(agent))) {
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        agent->send_message(17410, cookie, -2, 0);
        return 0;
    }
    sql_cmd = sprintf("select count(*) from data where path='%s'", path);
    if ((name != "*")) {
        sql_cmd += sprintf(" and name='%s'", name);
    }
    if ((branch != "*")) {
        sql_cmd += sprintf(" and branch='%s'", branch);
    }
    arr = mysql_select(cno, sql_cmd);
    if ((!arr)) {
        agent->send_message(17410, cookie, -2, 0);
        return 0;
    }
    agent->send_message(17410, cookie, 0, arr[0]["count(*)"]);
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