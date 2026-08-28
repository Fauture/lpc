/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_get_gid_arr.o
 * Source: /dba/daemons/dbaserver/cmd_get_gid_arr.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 12, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
int main(mixed agent, int cookie, string type, string content);

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
// @code  nc=190, nargs=4, insts=190
// --------------------------------------------------------------------
int main(mixed agent, int cookie, string type, string content) {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr;
    mixed q_arr;
    int cno;
    mixed str;
    mixed sql_cmd;
    mixed results;
    mapping m;
    mixed val64;
    mixed gid;
    mixed gid_info_list;
    int i;
    mixed buf;
    mixed text;

    arr = (explode(content, ",") - ({ "" }));
    if ((sizeof(arr) < 1)) {
        agent->send_message(17433, cookie, -1, 0);
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno < 0)) {
        return 0;
    }
    q_arr = allocate_array(sizeof(arr), "?");
    str = implode(q_arr, ",");
    sql_cmd = mysql_bind_parameters_arr(sprintf("select * from gid_info where type = '%s' and name in (%s)", type, str), arr);
    results = mysql_select(cno, sql_cmd, sizeof(arr));
    if ((!is_object(agent))) {
        return 0;
    }
    if ((!results)) {
        agent->send_message(17433, cookie, -2, 0);
        return 0;
    }
    if ((sizeof(results) < 1)) {
        agent->send_message(17433, cookie, -1, 0);
        return 0;
    }
    gid_info_list = allocate_array(sizeof(results));
    i = 0;
    for (i = 0; i <= (sizeof(results) - 1); i++) {
        m = results[i];
        val64 = atoi64(m["gid"]);
        gid = sprintf("%08X%08X", val64[0], val64[1]);
        gid_info_list[i] = ({ m["name"], gid, m["type"] });
    }  // end loop (verified)
    buf = lzss_compress(save_string(gid_info_list));
    text = save_string(buf);
    agent->send_message(17433, cookie, 0, text);
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