/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: msg_log_login.o
 * Source: /dba/daemons/dbaserver/msg_log_login.c
 * Inherits: (none)
 * Inherited programs: /feature/common/batch_log.c(), /inherit/misc/handler.c()
 * Functions: 4 names, 4 code segments found
 * Strings: 27, Variables: 10
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
int main(object agent, string account, string char_name, int privilege, int login_mode, int time_used, string login_ip, string para);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   part_static_sql = "insert into login_log (ip, account, char_name, dist, server, privilege, login_mode, time_used, login_ip, last_login_ip, gold_coin, silver_coin, insider, level, tao, party, family, create_time, gid, protect_type, upgrade_state, update_time, memo) values ";  // /dba/daemons/dbaserver/msg_log_login.c:104
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
    set_table_name("login_log");
    set_static_sql(part_static_sql);
    register_post_init((: do_init :));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=189, nargs=8, insts=189
// --------------------------------------------------------------------
int main(object agent, string account, string char_name, int privilege, int login_mode, int time_used, string login_ip, string para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;
    mixed server;
    mapping m;

    if ((!is_object(agent))) {
        return 0;
    }
    server = agent->query("name");
    cno = "/daemons/dbd.c"->assure_connection("ldb");
    if ((cno == -1)) {
        return 0;
    }
    if ((!is_mapping(m = restore_value(para)))) {
        return 0;
    }
    if (is_batch_enabled()) {
        sql_cmd = mysql_bind_parameters(part_dynamic_sql, server, account, char_name, (string)"/daemons/configd.c"->get_config("my_dist"), m["mac"], privilege, login_mode, time_used, login_ip, m["last_login_ip"], m["gold_coin"], m["silver_coin"], m["insider"], m["level"], m["tao"], m["party"], m["family"], m["create_time"], m["gid"], m["protect_type"], m["upgrade_state"], ctime(time(), 3), (string)m["memo"]);
        insert_log(sql_cmd);
    } else {
        sql_cmd = mysql_bind_parameters((part_static_sql + part_dynamic_sql), server, account, char_name, (string)"/daemons/configd.c"->get_config("my_dist"), m["mac"], privilege, login_mode, time_used, login_ip, m["last_login_ip"], m["gold_coin"], m["silver_coin"], m["insider"], m["level"], m["tao"], m["party"], m["family"], m["create_time"], m["gid"], m["protect_type"], m["upgrade_state"], ctime(time(), 3), (string)m["memo"]);
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