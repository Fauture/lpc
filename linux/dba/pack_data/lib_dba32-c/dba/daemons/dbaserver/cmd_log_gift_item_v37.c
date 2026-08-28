/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_log_gift_item.o
 * Source: /dba/daemons/dbaserver/cmd_log_gift_item.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 12, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
int main(object agent, string server, int type, string action, string item_name, string iid, string para1, string para2, string para3, string memo, string para);

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
// @code  nc=84, nargs=11, insts=84
// --------------------------------------------------------------------
int main(object agent, string server, int type, string action, string item_name, string iid, string para1, string para2, string para3, string memo, string para) {
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
    sql_cmd = mysql_bind_parameters("insert into gift_log (server, type, action, account, gid, lv, tao,  login_ip, mac, item_name, iid, gold_coin,  insider, para1, para2, para3, memo,  update_time) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", server, type, action, m["account"], m["gid"], m["level"], m["tao"], m["login_ip"], m["mac"], item_name, iid, m["gold_coin"], m["insider"], para1, para2, para3, memo, ctime(time(), 3));
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