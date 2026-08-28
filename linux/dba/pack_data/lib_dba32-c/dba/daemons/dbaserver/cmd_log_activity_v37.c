/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_log_activity.o
 * Source: /dba/daemons/dbaserver/cmd_log_activity.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 1 names, 1 code segments found
 * Strings: 6, Variables: 3
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   /* mixed global_0; */
   string part_dynamic_sql;
   string part_static_sql;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

int main(object agent, string server, string activity, string start_time, string end_time, string para1, string para2, string para3, string memo);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   part_static_sql = "insert into activity_log(update_time, server, activity, start_time, end_time, para1, para2, para3, memo)  values ";  // /dba/daemons/dbaserver/cmd_log_activity.c:49
   part_dynamic_sql = "(?, ?, ?, ?, ?, ?, ?, ?, ?)";

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=50, nargs=9, insts=50
// --------------------------------------------------------------------
int main(object agent, string server, string activity, string start_time, string end_time, string para1, string para2, string para3, string memo) {
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
    sql_cmd = mysql_bind_parameters("insert into activity_log(update_time, server, activity, start_time, end_time, para1, para2, para3, memo)  values (?, ?, ?, ?, ?, ?, ?, ?, ?)", ctime(time(), 3), server, activity, start_time, end_time, para1, para2, para3, memo);
    return 1;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        1
   Code segments found:         1
   Successfully decoded:        1
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          1
      - /inherit/misc/handler.c()
   ================================================================ */