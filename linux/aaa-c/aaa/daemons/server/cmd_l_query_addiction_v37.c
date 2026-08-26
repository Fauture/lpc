/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_query_addiction.o
 * Source: /aaa/daemons/server/cmd_l_query_addiction.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 8, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void query_addiction(mixed agent, string account);
void main(object agent, string account);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=73, nargs=2, insts=73
// --------------------------------------------------------------------
private void query_addiction(mixed agent, string account) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed arr;
    string result_text;
    mixed sql_cmd;

    if ((!is_object(agent))) {
        return;
    }
    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 1)) {
        return;
    }
    sql_cmd = mysql_bind_parameters("select * from antiaddiction where account = ?", account);
    arr = mysql_select(cno, sql_cmd);
    if ((!is_object(agent))) {
        return;
    }
    if ((!is_array(arr))) {
        result_text = "";
    } else if ((sizeof(arr) < 1)) {
        result_text = save_string(([ ]));
    } else {
        result_text = save_string(arr[0]);
    }
    agent->send_message(9103, account, result_text);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=19, nargs=2, insts=19
// --------------------------------------------------------------------
void main(object agent, string account) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: query_addiction, agent, account :));

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