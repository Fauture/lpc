/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_log_behavior.o
 * Source: /dba/daemons/dbaserver/cmd_log_behavior.c
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
int main(mixed agent, string client_str, string memo, string para);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   part_static_sql = "insert into behavior_log (update_time, account, gid, time_used, time_used_fd, lv, tao, upgrade_type, addup_exp, addup_tao, addup_play, addup_round, addup_talk_times, addup_friend_times, addup_click_npc, addup_exchange, addup_click_mouse, addup_move_order, time_used_stall, memo) values ";  // /dba/daemons/dbaserver/cmd_log_behavior.c:107
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
    set_table_name("behavior_log");
    set_static_sql(part_static_sql);
    register_post_init((: do_init :));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=227, nargs=4, insts=227
// --------------------------------------------------------------------
int main(mixed agent, string client_str, string memo, string para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;
    mapping m;
    mapping client_msg;

    if ((!is_object(agent))) {
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ldb");
    if ((cno == -1)) {
        return 0;
    }
    if ((!is_mapping(client_msg = restore_value(client_str)))) {
        return 0;
    }
    if ((!is_mapping(m = restore_value(para)))) {
        return 0;
    }
    if (is_batch_enabled()) {
        sql_cmd = mysql_bind_parameters(part_dynamic_sql, ctime(time(), 3), (string)m["account"], (string)m["gid"], (int)client_msg["time_used"], (int)client_msg["time_used_fd"], (int)client_msg["level"], (int)client_msg["tao"], (int)client_msg["upgrade_type"], (int)client_msg["addup_exp"], (int)client_msg["addup_tao"], (int)client_msg["addup_play"], (int)client_msg["addup_round"], (int)client_msg["addup_talk_times"], (int)client_msg["addup_friend_times"], (int)client_msg["addup_click_npc"], (int)client_msg["addup_exchange"], (int)client_msg["addup_click_mouse"], (int)client_msg["addup_move_order"], (int)client_msg["time_used_stall"], memo);
        insert_log(sql_cmd);
    } else {
        sql_cmd = mysql_bind_parameters((part_static_sql + part_dynamic_sql), ctime(time(), 3), (string)m["account"], (string)m["gid"], (int)client_msg["time_used"], (int)client_msg["time_used_fd"], (int)client_msg["level"], (int)client_msg["tao"], (int)client_msg["upgrade_type"], (int)client_msg["addup_exp"], (int)client_msg["addup_tao"], (int)client_msg["addup_play"], (int)client_msg["addup_round"], (int)client_msg["addup_talk_times"], (int)client_msg["addup_friend_times"], (int)client_msg["addup_click_npc"], (int)client_msg["addup_exchange"], (int)client_msg["addup_click_mouse"], (int)client_msg["addup_move_order"], (int)client_msg["time_used_stall"], memo);
        if (((*f_execute)(cno, sql_cmd) < 1)) {
            add_failed_times();
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