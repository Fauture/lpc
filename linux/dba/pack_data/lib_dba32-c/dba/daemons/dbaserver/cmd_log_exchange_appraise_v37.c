/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_log_exchange_appraise.o
 * Source: /dba/daemons/dbaserver/cmd_log_exchange_appraise.c
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
int main(object agent, string server, string exchange_ti, int report, int star, string content, mixed phone, string para1, string para2, string para3, string memo, string appraiser_para, string bargainer_para);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   part_static_sql = "insert into exchange_appraise_log( update_time, server, exchange_time, account1, name1, gid1, lv1, tao1, gold_coin1, account2, name2, gid2, lv2, tao2, gold_coin2, report, star, content, phone, para1, para2, para3, memo) values ";  // /dba/daemons/dbaserver/cmd_log_exchange_appraise.c:99
   part_dynamic_sql = "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

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
    set_table_name("exchange_appraise_log");
    set_static_sql(part_static_sql);
    register_post_init((: do_init :));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=185, nargs=13, insts=185
// --------------------------------------------------------------------
int main(object agent, string server, string exchange_ti, int report, int star, string content, mixed phone, string para1, string para2, string para3, string memo, string appraiser_para, string bargainer_para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;
    mapping appraiser_info;
    mapping bargainer_info;

    if ((!is_object(agent))) {
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ldb");
    if ((cno == -1)) {
        return 0;
    }
    try {
        appraiser_info = restore_value(appraiser_para);
    } catch { /* error handled */ }

    // --- label L0033 ---
    try {
        bargainer_info = restore_value(bargainer_para);
    } catch { /* error handled */ }

    // --- label L0044 ---
    if (((!is_mapping(appraiser_info)) || (!is_mapping(bargainer_info)))) {
        return 0;
    }
    if (is_batch_enabled()) {
        sql_cmd = mysql_bind_parameters(part_dynamic_sql, ctime(time(), 3), server, exchange_ti, appraiser_info["account"], appraiser_info["name"], appraiser_info["gid"], appraiser_info["level"], appraiser_info["tao"], appraiser_info["gold_coin"], bargainer_info["account"], bargainer_info["name"], bargainer_info["gid"], bargainer_info["level"], bargainer_info["tao"], bargainer_info["gold_coin"], report, star, content, phone, para1, para2, para3, memo);
        insert_log(sql_cmd);
    } else {
        sql_cmd = mysql_bind_parameters((part_static_sql + part_dynamic_sql), ctime(time(), 3), server, exchange_ti, appraiser_info["account"], appraiser_info["name"], appraiser_info["gid"], appraiser_info["level"], appraiser_info["tao"], appraiser_info["gold_coin"], bargainer_info["account"], bargainer_info["name"], bargainer_info["gid"], bargainer_info["level"], bargainer_info["tao"], bargainer_info["gold_coin"], report, star, content, phone, para1, para2, para3, memo);
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