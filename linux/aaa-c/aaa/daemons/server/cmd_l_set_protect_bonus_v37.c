/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_set_protect_bonus.o
 * Source: /aaa/daemons/server/cmd_l_set_protect_bonus.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 13, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void set_protect_bonus(mixed agent, string dist, string account, string bonus_info);
void main(object agent, string dist, string account, string bonus_info);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=134, nargs=4, insts=134
// --------------------------------------------------------------------
private void set_protect_bonus(mixed agent, string dist, string account, string bonus_info) {
    /* ---- Enhanced Pseudocode ---- */
    mixed year;
    int month;
    int cno;
    mixed arr;
    mixed sql_cmd;
    mixed branch;
    string protects;
    mapping m;
    mapping lt_m;

    cno = 0;
    if ((!is_object(agent))) {
        return;
    }
    if (((cno = "/daemons/dbd.c"->assure_connection("adb")) < 0)) {
        return;
    }
    lt_m = local_time();
    year = lt_m["year"];
    if ((!month = (lt_m["mon"] - 1))) {
        month = 12;
        --year;
    }
    branch = sprintf("%d-%02d", year, month);
    sql_cmd = mysql_bind_parameters("select * from protect_bonus where account = ? and branch = ?", account, branch);
    arr = mysql_select(cno, sql_cmd);
    if ((!is_array(arr))) {
        return;
    }
    m = restore_value(bonus_info);
    if ((!is_mapping(m))) {
        protects = "";
    } else {
        protects = m["protects"];
    }
    if ((sizeof(arr) < 1)) {
        sql_cmd = mysql_bind_parameters("insert into protect_bonus(account, branch, dist, bonus_time, bonus_type, bonus_info, insert_time) values(?, ?, ?, ?, ?, ?, ?)", account, branch, dist, time_to_string(), protects, bonus_info, ctime(time(), 3));
    } else {
        sql_cmd = mysql_bind_parameters("update protect_bonus set dist = ?, bonus_time = ?, bonus_type = ?, bonus_info = ? where account = ? and branch = ?", dist, time_to_string(), protects, bonus_info, account, branch);
    }
    (*f_execute)(cno, sql_cmd);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=21, nargs=4, insts=21
// --------------------------------------------------------------------
void main(object agent, string dist, string account, string bonus_info) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: set_protect_bonus, agent, dist, account, bonus_info :));

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