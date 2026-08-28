/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_move_content.o
 * Source: /dba/daemons/dbaserver/cmd_move_content.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 8, Variables: 0
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
// @code  nc=142, nargs=5, insts=142
// --------------------------------------------------------------------
int main(mixed agent, int cookie, string path, string name, string branch) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed ret;
    mixed sql_cmd;

    if ((!is_object(agent))) {
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        agent->send_message(21665, cookie, -2);
        return 0;
    }
    sql_cmd = "/dba/daemons/peel_datad.c"->build_move_to_data_sql(name);
    ret = (((*f_execute)(cno, sql_cmd) >= 1) ? 0 : -1);
    if ((ret == -1)) {
        agent->send_message(21665, cookie, ret);
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno != -1)) {
        sql_cmd = "/dba/daemons/peel_datad.c"->build_move_to_log_sql(name);
        ret = (*f_execute)(cno, sql_cmd);
    }
    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        agent->send_message(21665, cookie, -2);
        return 0;
    }
    sql_cmd = "/dba/daemons/peel_datad.c"->build_delete_from_data1_sql(name);
    ret = (((*f_execute)(cno, sql_cmd) >= 1) ? 0 : -1);
    agent->send_message(21665, cookie, ret);
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