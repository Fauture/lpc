/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_log_csp_combat.o
 * Source: /dba/daemons/dbaserver/cmd_log_csp_combat.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 16, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
int main(object agent, string server, string dist, string account, string gid, string name, int para1, string para2, string para3, string memo, string para);

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
// @code  nc=105, nargs=11, insts=105
// --------------------------------------------------------------------
int main(object agent, string server, string dist, string account, string gid, string name, int para1, string para2, string para3, string memo, string para) {
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
    try {
        m = restore_value(para);
    } catch { /* error handled */ }

    // --- label L0033 ---
    if ((!is_mapping(m))) {
        return 0;
    }
    sql_cmd = mysql_bind_parameters("insert into csp_combat_log(update_time, server, dist, account, gid, name, mac, ip, lv, tao, league_id, season_id, round_id, matchday_id, match_id, party_id, result, party_contrib, para1, para2, para3, memo) values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", ctime(time(), 3), server, dist, account, gid, name, m["mac"], m["ip"], m["lv"], m["tao"], m["league"], m["season"], m["round"], m["matchday"], m["match_id"], m["party_id"], m["result"], m["party_contrib"], para1, para2, para3, memo);
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