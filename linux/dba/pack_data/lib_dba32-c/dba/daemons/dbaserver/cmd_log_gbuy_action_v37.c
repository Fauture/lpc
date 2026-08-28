/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_log_gbuy_action.o
 * Source: /dba/daemons/dbaserver/cmd_log_gbuy_action.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 13, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
int main(object agent, string server, int gbuy_gcoin, int gbuy_scoin, int action, int coin_type, int coin, string gbg_id, string gbg_name, int gbg_amount, int para1, string para2, string para3, string memo, string para);

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
// @code  nc=91, nargs=15, insts=91
// --------------------------------------------------------------------
int main(object agent, string server, int gbuy_gcoin, int gbuy_scoin, int action, int coin_type, int coin, string gbg_id, string gbg_name, int gbg_amount, int para1, string para2, string para3, string memo, string para) {
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
    sql_cmd = mysql_bind_parameters("insert into gbuy_action_log (update_time, server, account, gid, name, mac, ip, insider, lv, gold_coin, silver_coin, gbuy_gcoin, gbuy_scoin, action, coin_type, coin, gbg_id, gbg_name, gbg_amount, para1, para2, para3, memo) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", ctime(time(), 3), server, m["account"], m["gid"], m["name"], m["mac"], m["ip"], m["insider"], m["level"], m["gold_coin"], m["silver_coin"], gbuy_gcoin, gbuy_scoin, action, coin_type, coin, gbg_id, gbg_name, gbg_amount, para1, para2, para3, memo);
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