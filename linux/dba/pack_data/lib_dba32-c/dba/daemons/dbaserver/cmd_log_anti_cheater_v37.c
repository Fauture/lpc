/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_log_anti_cheater.o
 * Source: /dba/daemons/dbaserver/cmd_log_anti_cheater.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c(), /feature/common/batch_log.c()
 * Functions: 4 names, 4 code segments found
 * Strings: 15, Variables: 10
 *********************************************************************/

inherit "/inherit/misc/handler.c";
inherit "/feature/common/batch_log.c";

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
int main(object agent, string type, string action, string para1, string para2, string para3, string memo, string para_str);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   part_static_sql = "insert into anti_cheater_log(update_time, type, action, account, gid, name, lv, gold_coin, ip, mac, para1, para2, para3, memo) values";  // /dba/daemons/dbaserver/cmd_log_anti_cheater.c:82
   part_dynamic_sql = "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

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
    set_table_name("anti_cheater_log");
    set_static_sql(part_static_sql);
    register_post_init((: do_init :));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=125, nargs=8, insts=125
// --------------------------------------------------------------------
int main(object agent, string type, string action, string para1, string para2, string para3, string memo, string para_str) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;
    mapping para;

    if ((!is_object(agent))) {
        return 0;
    }
    try {
        para = restore_value(para_str);
    } catch { /* error handled */ }

    // --- label L0019 ---
    cno = "/daemons/dbd.c"->assure_connection("ldb");
    if ((cno == -1)) {
        return 0;
    }
    if (is_batch_enabled()) {
        sql_cmd = mysql_bind_parameters(part_dynamic_sql, ctime(time(), 3), type, action, para["account"], para["gid"], para["name"], para["level"], para["gold_coin"], para["ip"], para["mac"], para1, para2, para3, memo);
        insert_log(sql_cmd);
    } else {
        sql_cmd = mysql_bind_parameters((part_static_sql + part_dynamic_sql), ctime(time(), 3), type, action, para["account"], para["gid"], para["name"], para["level"], para["gold_coin"], para["ip"], para["mac"], para1, para2, para3, memo);
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
      - /inherit/misc/handler.c()
      - /feature/common/batch_log.c()
   ================================================================ */