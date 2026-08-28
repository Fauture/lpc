/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_get_vote_result.o
 * Source: /dba/daemons/dbaserver/cmd_l_get_vote_result.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 3 names, 3 code segments found
 * Strings: 12, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
int main(mixed agent, string me, string start_time, string end_time);
private int get_vote_num(int cno, string para3, string start_time, string end_time);

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
// @code  nc=96, nargs=4, insts=96
// --------------------------------------------------------------------
int main(mixed agent, string me, string start_time, string end_time) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    int count_agree;
    int count_disagree;
    int count_neutral;

    if ((!is_object(agent))) {
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ldb");
    if ((cno == -1)) {
        agent->send_message(62724, me, 0, 0, 0, 0);
        return 0;
    }
    count_agree = get_vote_num(cno, "agree", start_time, end_time);
    count_disagree = get_vote_num(cno, "disagree", start_time, end_time);
    count_neutral = get_vote_num(cno, "neutral", start_time, end_time);
    if ((((count_agree < 0) || (count_disagree < 0)) || (count_neutral < 0))) {
        agent->send_message(62724, me, 0, 0, 0, 0);
    } else {
        agent->send_message(62724, me, 1, count_agree, count_disagree, count_neutral);
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=42, nargs=4, insts=42
// --------------------------------------------------------------------
private int get_vote_num(int cno, string para3, string start_time, string end_time) {
    /* ---- Enhanced Pseudocode ---- */
    mixed sql_cmd;
    mixed count;
    mixed result;

    sql_cmd = sprintf("select count(*) from %s where type = ? and action = ? and para3 = ? and (update_time between ? and ?)", "common_log");
    sql_cmd = mysql_bind_parameters(sql_cmd, "merge_dist", "vote", para3, start_time, end_time);
    result = mysql_select(cno, sql_cmd);
    if ((!result)) {
        return -1;
    }
    count = atoi(result[0]["count(*)"]);
    return count;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        3
   Code segments found:         3
   Successfully decoded:        3
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          1
      - /inherit/misc/handler.c()
   ================================================================ */