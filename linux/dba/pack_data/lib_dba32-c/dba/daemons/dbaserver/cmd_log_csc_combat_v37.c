/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_log_csc_combat.o
 * Source: /dba/daemons/dbaserver/cmd_log_csc_combat.c
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
int main(object agent, string server, string account, string gid, string name, int para1, int para2, string para3, string para4, string para5, string memo, string para);

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
// @code  nc=89, nargs=12, insts=89
// --------------------------------------------------------------------
int main(object agent, string server, string account, string gid, string name, int para1, int para2, string para3, string para4, string para5, string memo, string para) {
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
    sql_cmd = mysql_bind_parameters("insert into csc_combat_log(server, account, gid, name, mac, ip, insider, lv, tao, g_coin, s_coin, score, result, para1, para2, para3, para4, para5, memo, update_time) values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", server, account, gid, name, m["mac"], m["ip"], m["insider"], m["level"], m["tao"], m["g_coin"], m["s_coin"], m["score"], m["result"], para1, para2, para3, para4, para5, memo, ctime(time(), 3));
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