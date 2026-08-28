/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_log_debug.o
 * Source: /dba/daemons/dbaserver/cmd_log_debug.c
 * Inherits: (none)
 * Inherited programs: /feature/common/batch_log.c(), /inherit/misc/handler.c()
 * Functions: 5 names, 5 code segments found
 * Strings: 13, Variables: 10
 *********************************************************************/

inherit "/feature/common/batch_log.c";
inherit "/inherit/misc/handler.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   /* mixed global_0; */
   /* mixed global_1; */
   /* mixed global_2; */
   /* mixed global_3; */
   /* mixed global_4; */
   /* mixed global_5; */
   /* mixed global_6; */
   /* mixed global_7; */
   string part_dynamic_sql;
   string part_static_sql;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
private void do_init();
int _main(string server, string memo, string para);
private void create();
int main(mixed agent, string server, string memo, string para);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   part_static_sql = "insert into debug_log (update_time, server, type, action, para1, para2, para3, memo) values ";  // /dba/daemons/dbaserver/cmd_log_debug.c:92
   part_dynamic_sql = "(?, ?, ?, ?, ?, ?, ?, ?)";

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
// @brief  constructor: initialize object state
// @code  nc=10, nargs=0, insts=10
// --------------------------------------------------------------------
private void do_init() {
    /* ---- Enhanced Pseudocode ---- */
    "/dba/daemons/batch_logd.c"->register_entry(get_table_name(), this_object());

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=99, nargs=3, insts=99
// --------------------------------------------------------------------
int _main(string server, string memo, string para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;
    mapping m;

    cno = "/daemons/dbd.c"->assure_connection("ldb");
    if ((cno == -1)) {
        return 0;
    }
    if ((!is_mapping(m = restore_value(para)))) {
        return 0;
    }
    if (is_batch_enabled()) {
        sql_cmd = mysql_bind_parameters(part_dynamic_sql, ctime(time(), 3), server, m["type"], m["action"], m["para1"], m["para2"], m["para3"], memo);
        insert_log(sql_cmd);
    } else {
        sql_cmd = mysql_bind_parameters((part_static_sql + part_dynamic_sql), ctime(time(), 3), server, m["type"], m["action"], m["para1"], m["para2"], m["para3"], memo);
        if (((*f_execute)(cno, sql_cmd) < 1)) {
            add_failed_times();
        }
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=13, nargs=0, insts=13
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    set_table_name("debug_log");
    set_static_sql(part_static_sql);
    register_post_init((: do_init :));

}

// --------------------------------------------------------------------
// @code  nc=16, nargs=4, insts=16
// --------------------------------------------------------------------
int main(mixed agent, string server, string memo, string para) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!is_object(agent))) {
        return 0;
    }
    return _main(server, memo, para);

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        5
   Code segments found:         5
   Successfully decoded:        5
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          2
      - /feature/common/batch_log.c()
      - /inherit/misc/handler.c()
   ================================================================ */