/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_log_report.o
 * Source: /dba/daemons/dbaserver/cmd_log_report.c
 * Inherits: (none)
 * Inherited programs: /feature/common/batch_log.c(), /inherit/misc/handler.c()
 * Functions: 4 names, 4 code segments found
 * Strings: 14, Variables: 10
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
private void create();
int main(object agent, string server, string para1, string para2, string para3, string para4, string para5, string memo, string reporter_para, string bereport_para);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   part_static_sql = "insert into report_log(update_time, server, account1, name1, gid1, lv1, tao1, gold_coin1, account2, name2, gid2, lv2, tao2, gold_coin2, para1, para2, para3, para4, para5, memo) values ";  // /dba/daemons/dbaserver/cmd_log_report.c:95
   part_dynamic_sql = "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

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
// @brief  constructor: initialize object state
// @code  nc=13, nargs=0, insts=13
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    set_table_name("report_log");
    set_static_sql(part_static_sql);
    register_post_init((: do_init :));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=179, nargs=10, insts=179
// --------------------------------------------------------------------
int main(object agent, string server, string para1, string para2, string para3, string para4, string para5, string memo, string reporter_para, string bereport_para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;
    mapping reporter_info;
    mapping bereport_info;

    if ((!is_object(agent))) {
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ldb");
    if ((cno == -1)) {
        return 0;
    }
    try {
        reporter_info = restore_value(reporter_para);
    } catch { /* error handled */ }

    // --- label L0033 ---
    try {
        bereport_info = restore_value(bereport_para);
    } catch { /* error handled */ }

    // --- label L0044 ---
    if (((!is_mapping(reporter_info)) || (!is_mapping(bereport_info)))) {
        return 0;
    }
    if (is_batch_enabled()) {
        sql_cmd = mysql_bind_parameters(part_dynamic_sql, ctime(time(), 3), server, reporter_info["account"], reporter_info["name"], reporter_info["gid"], reporter_info["lv"], reporter_info["tao"], reporter_info["gold_coin"], bereport_info["account"], bereport_info["name"], bereport_info["gid"], bereport_info["lv"], bereport_info["tao"], bereport_info["gold_coin"], para1, para2, para3, para4, para5, memo);
        insert_log(sql_cmd);
    } else {
        sql_cmd = mysql_bind_parameters((part_static_sql + part_dynamic_sql), ctime(time(), 3), server, reporter_info["account"], reporter_info["name"], reporter_info["gid"], reporter_info["lv"], reporter_info["tao"], reporter_info["gold_coin"], bereport_info["account"], bereport_info["name"], bereport_info["gid"], bereport_info["lv"], bereport_info["tao"], bereport_info["gold_coin"], para1, para2, para3, para4, para5, memo);
    }
    return 1;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        4
   Code segments found:         4
   Successfully decoded:        4
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          2
      - /feature/common/batch_log.c()
      - /inherit/misc/handler.c()
   ================================================================ */