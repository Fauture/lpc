/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_get_gid.o
 * Source: /dba/daemons/dbaserver/cmd_get_gid.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 7, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
int main(mixed agent, int cookie, string type, string name);

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
// @code  nc=98, nargs=4, insts=98
// --------------------------------------------------------------------
int main(mixed agent, int cookie, string type, string name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed sql_cmd;
    mixed arr;
    mixed val64;
    mixed gid;
    mixed cno;

    if ((!is_object(agent))) {
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        agent->send_message(13333, cookie, -2, 0);
        return 0;
    }
    sql_cmd = mysql_bind_parameters("select gid from gid_info where type=? and name=?", type, name);
    arr = mysql_select(cno, sql_cmd);
    if ((!arr)) {
        return 0;
    }
    if ((sizeof(arr) != 1)) {
        agent->send_message(13333, cookie, -1, 0);
        return 0;
    }
    val64 = atoi64(arr[0]["gid"]);
    gid = sprintf("%08X%08X", val64[0], val64[1]);
    agent->send_message(13333, cookie, 0, gid);
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