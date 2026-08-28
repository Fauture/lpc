/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_write_content.o
 * Source: /dba/daemons/dbaserver/cmd_write_content.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 4 names, 4 code segments found
 * Strings: 11, Variables: 2
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   /* mixed global_0; */
   int in_peel_data;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
void stop_peel_data();
int main(mixed agent, int cookie, string path, string name, string branch, buffer content);
void start_peel_data();

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   in_peel_data = 0;

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
// @code  nc=148, nargs=6, insts=148
// --------------------------------------------------------------------
int main(mixed agent, int cookie, string path, string name, string branch, buffer content) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    int ret;
    int sql_ret;
    mixed checksum;
    mixed sql_cmd;
    string content_text;

    if ((!is_object(agent))) {
        return 0;
    }
    content_text = (string)content;
    if ((((path == "runtime") && in_peel_data) && "/dba/daemons/peel_datad.c"->is_in_peeling(content_text))) {
        agent->send_message(9219, cookie, -2, 0);
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        agent->send_message(9219, cookie, -2, 0);
        return 0;
    }
    checksum = int32((create_check_sum(1, path, name, branch, content) ^ -1418120497));
    sql_cmd = mysql_bind_parameters("update data set path=?, name=?, branch = ?, content=?, time=?, checksum=? where path=? and name=? and branch=?", path, name, branch, content_text, ctime(time(), 3), checksum, path, name, branch);
    if (((sql_ret = (*f_execute)(cno, sql_cmd)) >= 1)) {
        ret = 0;
    } else if ((sql_ret == 0)) {
        ret = -1;
    } else {
        ret = -2;
    }
    agent->send_message(9219, cookie, ret, 0);
    if (((path == "runtime") && in_peel_data)) {
        "/dba/daemons/peel_datad.c"->add_to_online_list(name, content_text);
    }
    "/dba/daemons/dbaserverd.c"->update_data_length_stat_info(path, name, branch, content_text);
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