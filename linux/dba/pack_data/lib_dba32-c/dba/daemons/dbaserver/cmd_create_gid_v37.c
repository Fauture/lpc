/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_create_gid.o
 * Source: /dba/daemons/dbaserver/cmd_create_gid.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 3 names, 3 code segments found
 * Strings: 8, Variables: 2
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   /* mixed global_0; */
   mapping operating_map;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
private void create_gid(object agent, int cookie, string type, string name);
int main(mixed agent, int cookie, string type, string name);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   operating_map = ([ ]);

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
// @code  nc=153, nargs=4, insts=153
// --------------------------------------------------------------------
private void create_gid(object agent, int cookie, string type, string name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    int ret;
    mixed arr;
    mixed val64;
    mixed sql_cmd;
    mixed gid;

    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        agent->send_message(13329, cookie, -2, 0);
        return;
    }
    sql_cmd = mysql_bind_parameters("select gid from gid_info where type=? and name=?", type, name);
    arr = mysql_select(cno, sql_cmd, 10);
    if ((!arr)) {
        return;
    }
    if ((sizeof(arr) >= 1)) {
        agent->send_message(13329, cookie, -1, 0);
        return;
    }
    sql_cmd = mysql_bind_parameters("insert into gid_info (type, name, time) values (?, ?, ?)", type, name, ctime(time(), 3));
    ret = (*f_execute)(cno, sql_cmd);
    if ((ret < 1)) {
        agent->send_message(13329, cookie, -1, 0);
        return;
    }
    sql_cmd = mysql_bind_parameters("select gid from gid_info where type=? and name=?", type, name);
    arr = mysql_select(cno, sql_cmd, 10);
    if ((!is_array(arr))) {
        return;
    }
    if ((sizeof(arr) != 1)) {
        agent->send_message(13329, cookie, -1, 0);
        return;
    }
    val64 = atoi64(arr[0]["gid"]);
    gid = sprintf("%08X%08X", val64[0], val64[1]);
    agent->send_message(13329, cookie, 0, gid);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=56, nargs=4, insts=56
// --------------------------------------------------------------------
int main(mixed agent, int cookie, string type, string name) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!is_object(agent))) {
        return 0;
    }
    if (operating_map[(type + name)]) {
        agent->send_message(13329, cookie, -1, 0);
        return 0;
    }
    operating_map[(type + name)] = 1;
    try {
        create_gid(agent, cookie, type, name);
    } catch { /* error handled */ }

    // --- label L0045 ---
    map_delete(operating_map, (type + name));
    return 1;

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