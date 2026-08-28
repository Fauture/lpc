/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_create_content.o
 * Source: /dba/daemons/dbaserver/cmd_create_content.c
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
int main(mixed agent, int cookie, string path, string name, string branch, buffer content);

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
// @code  nc=90, nargs=6, insts=90
// --------------------------------------------------------------------
int main(mixed agent, int cookie, string path, string name, string branch, buffer content) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed ret;
    mixed sql_cmd;
    string content_text;

    if ((!is_object(agent))) {
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        agent->send_message(9223, cookie, -2, 0);
        return 0;
    }
    content_text = (string)content;
    sql_cmd = mysql_bind_parameters("insert into data (path, name, branch, content, time, checksum) values (?, ?, ?, ?, ?, ?)", path, name, branch, content_text, ctime(time(), 3), int32((create_check_sum(1, path, name, branch, content) ^ -1418120497)));
    ret = (((*f_execute)(cno, sql_cmd) >= 1) ? 0 : -1);
    agent->send_message(9223, cookie, ret, 0);
    "/dba/daemons/dbaserverd.c"->update_data_length_stat_info(path, name, branch, content_text);
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