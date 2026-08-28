/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_update_log.o
 * Source: /dba/daemons/dbaserver/cmd_update_log.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 3, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
int main(object agent, int cookie, string db, string sql_cmd);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=4, nargs=0, insts=4
// --------------------------------------------------------------------
mixed get_db_type() {
    /* ---- Enhanced Pseudocode ---- */
    return;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=41, nargs=4, insts=41
// --------------------------------------------------------------------
int main(object agent, int cookie, string db, string sql_cmd) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed total;

    if ((!is_object(agent))) {
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection(db);
    if ((cno == -1)) {
        return 0;
    }
    total = (*f_execute)(cno, sql_cmd);
    agent->send_message(62751, cookie, total);
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