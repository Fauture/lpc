/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_read_pet_deposit.o
 * Source: /dba/daemons/dbaserver/cmd_read_pet_deposit.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 6, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
int main(mixed agent, int cookie, string pet_iid, string status);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
mixed get_db_type() {
    /* ---- Enhanced Pseudocode ---- */
    return "ddb";

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=74, nargs=4, insts=74
// --------------------------------------------------------------------
int main(mixed agent, int cookie, string pet_iid, string status) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;
    mixed arr;
    buffer content;

    if ((!is_object(agent))) {
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        agent->send_message(21505, cookie, -2, 0);
        return 0;
    }
    sql_cmd = mysql_bind_parameters("select * from pet_deposit where pet_iid=? and status=? limit 1", pet_iid, atoi(status));
    arr = mysql_select(cno, sql_cmd);
    if ((!arr)) {
        "/daemons/dbd.c"->notice_db_failed("ddb");
        return 0;
    }
    content = (buffer)save_string(arr);
    agent->send_message(21505, cookie, 0, content);
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