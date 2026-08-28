/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_log_purchase.o
 * Source: /dba/daemons/dbaserver/cmd_log_purchase.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 14, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
int main(object agent, string gid, int order_time, string iid, int amount, int total_value, int tax, string para1, string para2, string para3, string para4, string memo, string para);

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
// @code  nc=145, nargs=13, insts=145
// --------------------------------------------------------------------
int main(object agent, string gid, int order_time, string iid, int amount, int total_value, int tax, string para1, string para2, string para3, string para4, string memo, string para) {
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
    if (order_time) {
        sql_cmd = mysql_bind_parameters("insert into purchase_platform_log(update_time, server, account, gid,  insider, lv, type, order_id, order_time, commodity_type, commodity_name,  commodity_iid, commodity_amount, unit_price, turnover, tax,  para1, para2, para3, para4, memo) values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", ctime(time(), 3), (string)m["server"], (string)m["account"], gid, m["insider"], m["lv"], m["action_type"], m["order_id"], order_time, m["type"], m["name"], iid, amount, (string)m["price"], total_value, tax, para1, para2, para3, para4, memo);
    } else {
        sql_cmd = mysql_bind_parameters("insert into purchase_platform_log(update_time, server, account, gid,  insider, lv, type, order_id, commodity_type, commodity_name,  commodity_iid, commodity_amount, unit_price, turnover, tax,  para1, para2, para3, para4, memo) values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", ctime(time(), 3), (string)m["server"], (string)m["account"], gid, m["insider"], m["lv"], m["action_type"], m["order_id"], m["type"], m["name"], iid, amount, (string)m["price"], total_value, tax, para1, para2, para3, para4, memo);
    }
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