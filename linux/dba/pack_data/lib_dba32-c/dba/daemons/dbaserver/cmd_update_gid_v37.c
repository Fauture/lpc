/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_update_gid.o
 * Source: /dba/daemons/dbaserver/cmd_update_gid.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 9, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
int main(mixed agent, int cookie, string gid, string type, string name);

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
// @code  nc=166, nargs=5, insts=166
// --------------------------------------------------------------------
int main(mixed agent, int cookie, string gid, string type, string name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    int ret;
    mixed arr;
    int val64;
    mixed sql_cmd;
    int first;
    int second;

    if ((!is_object(agent))) {
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        agent->send_message(9235, cookie, -2, 0);
        return 0;
    }
    sql_cmd = mysql_bind_parameters("select gid from gid_info where type=? and name=?", type, name);
    arr = mysql_select(cno, sql_cmd);
    if ((!arr)) {
        return 0;
    }
    if ((sizeof(arr) >= 1)) {
        val64 = atoi64(arr[0]["gid"]);
        if ((gid == sprintf("%08X%08X", val64[0], val64[1]))) {
            ret = 0;
        } else {
            ret = -1;
        }
        agent->send_message(9235, cookie, ret, 0);
        return 0;
    }
    first = 0;
    second = 0;
    sscanf(gid[0..7], "%x", &first);
    sscanf(gid[8..15], "%x", &second);
    val64 = ({ first, second });
    sql_cmd = mysql_bind_parameters("update gid_info set type=?, name=? where gid=?", type, name, itoa64(val64));
    if (((*f_execute)(cno, sql_cmd) < 1)) {
        agent->send_message(9235, cookie, -1, 0);
        return 0;
    }
    agent->send_message(9235, cookie, 0, 0);
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