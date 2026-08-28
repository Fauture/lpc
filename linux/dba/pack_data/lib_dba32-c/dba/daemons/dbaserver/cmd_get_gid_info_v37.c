/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_get_gid_info.o
 * Source: /dba/daemons/dbaserver/cmd_get_gid_info.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 8, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
int main(mixed agent, int cookie, string gid);

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
// @code  nc=131, nargs=3, insts=131
// --------------------------------------------------------------------
int main(mixed agent, int cookie, string gid) {
    /* ---- Enhanced Pseudocode ---- */
    mixed sql_cmd;
    mixed arr;
    int val64;
    mixed cno;
    int first;
    int second;

    if ((!is_object(agent))) {
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        agent->send_message(17431, cookie, -2, 0);
        return 0;
    }
    first = 0;
    second = 0;
    sscanf(gid[0..7], "%x", &first);
    sscanf(gid[8..15], "%x", &second);
    val64 = ({ first, second });
    sql_cmd = mysql_bind_parameters("select type, name from gid_info where gid=?", itoa64(val64));
    arr = mysql_select(cno, sql_cmd);
    if (((!is_array(arr)) || ((sizeof(arr) >= 1) && (!is_mapping(arr[0]))))) {
        return 0;
    }
    if ((sizeof(arr) < 1)) {
        agent->send_message(17431, cookie, -1, 0);
        return 0;
    }
    agent->send_message(17431, cookie, 0, arr[0]["type"], arr[0]["name"]);
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