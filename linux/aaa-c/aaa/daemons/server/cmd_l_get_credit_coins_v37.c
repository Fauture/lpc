/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_get_credit_coins.o
 * Source: /aaa/daemons/server/cmd_l_get_credit_coins.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 8, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _main(mixed agent, string account, string gid, string ref_time);
int main(mixed agent, string account, string gid, string ref_time);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=61, nargs=4, insts=61
// --------------------------------------------------------------------
private void _main(mixed agent, string account, string gid, string ref_time) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    int coins;
    mixed arr;
    mixed sql_cmd;

    if (((cno = "/daemons/dbd.c"->assure_connection("adb")) == -1)) {
        return;
    }
    sql_cmd = mysql_bind_parameters("SELECT sum(gold_coin) as coins FROM credit_log WHERE account = ? and time between ? and ?", account, ref_time, ctime(time(), 3));
    arr = mysql_select(cno, sql_cmd, 1);
    if ((!is_array(arr))) {
        return;
    }
    if (sizeof(arr)) {
        coins = atoi(arr[0]["coins"]);
    } else {
        coins = 0;
    }
    agent->send_message(25487, gid, (string)coins);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=23, nargs=4, insts=23
// --------------------------------------------------------------------
int main(mixed agent, string account, string gid, string ref_time) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _main, agent, account, gid, ref_time :));
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