/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_log_item_transfer.o
 * Source: /dba/daemons/dbaserver/cmd_log_item_transfer.c
 * Inherits: (none)
 * Inherited programs: /feature/common/batch_log.c(), /inherit/misc/handler.c()
 * Functions: 4 names, 4 code segments found
 * Strings: 26, Variables: 10
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
int main(object agent, int line, int type, string action, string iid, string item_name, int item_amount, int transfer_type, string transfer_id, string para1, string para2, int para3, string memo, string para);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   part_static_sql = "insert into item_transfer_log (line, type, action, account_from, account_to, gid_from, gid_to, lv_from, lv_to, gold_coin_from, gold_coin_to, mac_from, mac_to, ip_from, ip_to, locality_from, locality_to, item_iid, item_name, item_amount, transfer_type, transfer_id, para1, para2, para3, memo, update_time) values ";  // /dba/daemons/dbaserver/cmd_log_item_transfer.c:107
   part_dynamic_sql = "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

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
    set_table_name("item_transfer_log");
    set_static_sql(part_static_sql);
    register_post_init((: do_init :));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=205, nargs=14, insts=205
// --------------------------------------------------------------------
int main(object agent, int line, int type, string action, string iid, string item_name, int item_amount, int transfer_type, string transfer_id, string para1, string para2, int para3, string memo, string para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;
    mapping m;

    if ((!is_object(agent))) {
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ldb");
    if ((cno == -1)) {
        return 0;
    }
    if ((!is_mapping(m = restore_value(para)))) {
        return 0;
    }
    if (is_batch_enabled()) {
        sql_cmd = mysql_bind_parameters(part_dynamic_sql, line, type, action, m["account_from"], m["account_to"], m["gid_from"], m["gid_to"], m["lv_from"], m["lv_to"], m["gold_coin_from"], m["gold_coin_to"], m["mac_from"], m["mac_to"], m["ip_from"], m["ip_to"], m["locality_from"], m["locality_to"], iid, item_name, item_amount, transfer_type, transfer_id, para1, para2, para3, memo, ctime(time(), 3));
        insert_log(sql_cmd);
    } else {
        sql_cmd = mysql_bind_parameters((part_static_sql + part_dynamic_sql), line, type, action, m["account_from"], m["account_to"], m["gid_from"], m["gid_to"], m["lv_from"], m["lv_to"], m["gold_coin_from"], m["gold_coin_to"], m["mac_from"], m["mac_to"], m["ip_from"], m["ip_to"], m["locality_from"], m["locality_to"], iid, item_name, item_amount, transfer_type, transfer_id, para1, para2, para3, memo, ctime(time(), 3));
        if (((*f_execute)(cno, sql_cmd) < 1)) {
            add_failed_times();
            "/daemons/logd.c"->log_local(1, "log_item_transfer", "log_failed", (string)m["account_from"], (string)m["gid_from"], (string)m["gid_to"], save_string(($<)[1..<1]));
        }
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