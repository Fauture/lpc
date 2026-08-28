/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_log_important_pet.o
 * Source: /dba/daemons/dbaserver/cmd_log_important_pet.c
 * Inherits: (none)
 * Inherited programs: /feature/common/batch_log.c(), /inherit/misc/handler.c()
 * Functions: 4 names, 4 code segments found
 * Strings: 18, Variables: 10
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
int main(object agent, int type, string action, string para1, string para2, int para3, string memo, string para);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   part_static_sql = "insert into important_pet_log(update_time, line, type, action, account_from, account_to, gid_from, gid_to, lv_from, lv_to, pet_iid, pet_name, pet_property, para1, para2, para3, memo) values ";  // /dba/daemons/dbaserver/cmd_log_important_pet.c:87
   part_dynamic_sql = "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

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
    set_table_name("important_pet_log");
    set_static_sql(part_static_sql);
    register_post_init((: do_init :));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=141, nargs=8, insts=141
// --------------------------------------------------------------------
int main(object agent, int type, string action, string para1, string para2, int para3, string memo, string para) {
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
        sql_cmd = mysql_bind_parameters(part_dynamic_sql, ctime(time(), 3), m["line"], type, action, m["account_from"], m["account_to"], m["gid_from"], m["gid_to"], m["lv_from"], m["lv_to"], m["pet_iid"], m["pet_name"], m["pet_property"], para1, para2, para3, memo);
        insert_log(sql_cmd);
    } else {
        sql_cmd = mysql_bind_parameters((part_static_sql + part_dynamic_sql), ctime(time(), 3), m["line"], type, action, m["account_from"], m["account_to"], m["gid_from"], m["gid_to"], m["lv_from"], m["lv_to"], m["pet_iid"], m["pet_name"], m["pet_property"], para1, para2, para3, memo);
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