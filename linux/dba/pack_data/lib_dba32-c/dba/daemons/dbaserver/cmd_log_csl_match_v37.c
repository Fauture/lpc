/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_log_csl_match.o
 * Source: /dba/daemons/dbaserver/cmd_log_csl_match.c
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
int main(object agent, string server, string home_dist, string away_dist, int para1, string para2, string para3, string memo, string para);

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
// @code  nc=103, nargs=9, insts=103
// --------------------------------------------------------------------
int main(object agent, string server, string home_dist, string away_dist, int para1, string para2, string para3, string memo, string para) {
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
    sql_cmd = mysql_bind_parameters("insert into csl_match_log(update_time, server, home_dist, away_dist, league_id, season_id, stage_id, round_id, matchday_id, result, home_score, away_score, home_gid_num, away_gid_num, home_avg_lv, away_avg_lv, para1, para2, para3, memo) values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", ctime(time(), 3), server, home_dist, away_dist, m["league"], m["season"], m["stage"], m["round"], m["matchday"], m["result"], m["home_score"], m["away_score"], m["home_gid_num"], m["away_gid_num"], m["home_avg_lv"], m["away_avg_lv"], para1, para2, para3, memo);
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