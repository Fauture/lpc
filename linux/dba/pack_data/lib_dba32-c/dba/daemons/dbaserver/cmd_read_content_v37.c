/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_read_content.o
 * Source: /dba/daemons/dbaserver/cmd_read_content.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 4 names, 4 code segments found
 * Strings: 14, Variables: 3
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   int faild_times;
   /* mixed global_0; */
   int in_peel_data;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
void stop_peel_data();
int main(mixed agent, int cookie, string path, string name, string branch, string from_table = "data");
void start_peel_data();

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   in_peel_data = 0;  // /dba/daemons/dbaserver/cmd_read_content.c:103
   faild_times = 0;

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
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
void stop_peel_data() {
    /* ---- Enhanced Pseudocode ---- */
    in_peel_data = 0;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=175, nargs=6, insts=175
// --------------------------------------------------------------------
int main(mixed agent, int cookie, string path, string name, string branch, string from_table) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed arr;
    mixed sql_cmd;
    buffer content;
    mapping m;

    if ((!is_object(agent))) {
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        agent->send_message(17409, cookie, -2, 0);
        return 0;
    }
    if ((from_table != "data1")) {
        from_table = "data";
    }
    sql_cmd = ("select path, name, branch, content, checksum from " + from_table);
    sql_cmd += mysql_bind_parameters(" where path=? and name=? and branch=?", path, name, branch);
    arr = mysql_select(cno, sql_cmd);
    if ((!arr)) {
        ++faild_times;
        "/daemons/dbd.c"->notice_db_failed("ddb");
        return 0;
    }
    if ((sizeof(arr) != 1)) {
        agent->send_message(17409, cookie, -1, 0);
        return 1;
    }
    if ((!is_mapping(m = arr[0]))) {
        agent->send_message(17409, cookie, -2, 0);
        return 1;
    }
    content = (buffer)m["content"];
    path = m["path"];
    name = m["name"];
    branch = m["branch"];
    if ((int32((create_check_sum(1, path, name, branch, content) ^ -1418120497)) != atoi(m["checksum"]))) {
        agent->send_message(17409, cookie, -3, 0);
        return 1;
    }
    agent->send_message(17409, cookie, 0, content);
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
void start_peel_data() {
    /* ---- Enhanced Pseudocode ---- */
    in_peel_data = 1;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        4
   Code segments found:         4
   Successfully decoded:        4
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          1
      - /inherit/misc/handler.c()
   ================================================================ */