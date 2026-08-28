/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_log_ip_task.o
 * Source: /dba/daemons/dbaserver/cmd_log_ip_task.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 4, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
int main(object agent, int line, string ip, string task, int num, string memo);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
mixed get_db_type() {
    /* ---- Enhanced Pseudocode ---- */
    return "ldb";

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=48, nargs=6, insts=48
// --------------------------------------------------------------------
int main(object agent, int line, string ip, string task, int num, string memo) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;

    if ((!is_object(agent))) {
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ldb");
    if ((cno == -1)) {
        return 0;
    }
    sql_cmd = mysql_bind_parameters("insert into ip_task_log(time, dist, line, ip, task, num, memo) values(?, ?, ?, ?, ?, ?, ?)", ctime(time(), 3), _dist_name, line, ip, task, num, memo);
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