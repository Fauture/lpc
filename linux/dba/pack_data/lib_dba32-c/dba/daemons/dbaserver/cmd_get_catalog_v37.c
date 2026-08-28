/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_get_catalog.o
 * Source: /dba/daemons/dbaserver/cmd_get_catalog.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 13, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
int main(mixed agent, int cookie, string path, string name, string branch, string condition, int start_pos, int count);

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
// @code  nc=166, nargs=8, insts=166
// --------------------------------------------------------------------
int main(mixed agent, int cookie, string path, string name, string branch, string condition, int start_pos, int count) {
    /* ---- Enhanced Pseudocode ---- */
    mixed sql_cmd;
    mixed arr;
    mixed cno;
    mixed str;
    int i;

    if ((!is_object(agent))) {
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        agent->send_message(9225, cookie, -2, 0);
        return 0;
    }
    if ((name == "*")) {
        if (sizeof(branch)) {
            sql_cmd = mysql_bind_parameters("select name from data where path=? and branch=?", path, branch);
        } else {
            sql_cmd = mysql_bind_parameters("select name from data where path=? and branch=''", path);
        }
    } else {
        sql_cmd = mysql_bind_parameters("select branch from data where path=? and name=? and branch<>''", path, name);
    }
    if ((condition != "")) {
        sql_cmd += (" and " + condition);
    }
    if ((count > 1000000)) {
        count = 1000000;
    }
    if ((count != 0)) {
        sql_cmd += (((" limit " + maximum((start_pos - 1), 0)) + ",") + count);
    }
    arr = mysql_select_column(cno, sql_cmd, 0, count);
    if ((!arr)) {
        agent->send_message(9225, cookie, -2, 0);
        return 0;
    }
    str = implode(arr, "\");
    i = 0;

    // --- label L0130 ---
    while ((i < sizeof(str))) {
        agent->send_message(9227, cookie, str[i..((i + 49152) - 1)]);
        i += 49152;
    }  // end loop (verified)
    agent->send_message(9225, cookie, 0, 0);
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