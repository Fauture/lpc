/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: msg_log_cost_coin.o
 * Source: /dba/daemons/dbaserver/msg_log_cost_coin.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 7, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
int main(object agent, string type, string action, string cost_type, int cost, string account, string gid, string item_name, int amount, string uid, string memo);

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
// @code  nc=56, nargs=11, insts=56
// --------------------------------------------------------------------
int main(object agent, string type, string action, string cost_type, int cost, string account, string gid, string item_name, int amount, string uid, string memo) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;
    mixed server;

    if ((!is_object(agent))) {
        return 0;
    }
    server = agent->query("name");
    cno = "/daemons/dbd.c"->assure_connection("ldb");
    if ((cno == -1)) {
        return 0;
    }
    sql_cmd = mysql_bind_parameters("insert into cost_coin_log (update_time, server, type, action, cost_type, cost, account, gid, item_name, amount, uid,  para1, para2, para3, memo) values (date_format(CURRENT_TIMESTAMP, '%Y%m%d%H%i%s'), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", server, type, action, cost_type, cost, account, gid, item_name, amount, uid, "", "", "", memo);
    (*f_execute)(cno, sql_cmd);
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