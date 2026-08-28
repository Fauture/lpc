/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_xg_get_name.o
 * Source: /dba/daemons/dbaserver/cmd_xg_get_name.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 6, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
int main(mixed agent, int cookie, string xg_type, string account);

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
// @code  nc=97, nargs=4, insts=97
// --------------------------------------------------------------------
int main(mixed agent, int cookie, string xg_type, string account) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed arr;
    mixed sql_cmd;

    if ((!is_object(agent))) {
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        agent->send_message(13349, cookie, -2, 0);
        return 0;
    }
    sql_cmd = mysql_bind_parameters("select name from xg_name where account=? and xg_type=?", account, xg_type);
    arr = mysql_select(cno, sql_cmd);
    if ((!is_object(agent))) {
        return 0;
    }
    if ((!arr)) {
        agent->send_message(13349, cookie, -2, 0);
        return 0;
    }
    if ((sizeof(arr) < 1)) {
        agent->send_message(13349, cookie, -1, 0);
        return 0;
    }
    agent->send_message(13349, cookie, 0, arr[0]["name"]);
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