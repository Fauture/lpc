/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_delete_content.o
 * Source: /dba/daemons/dbaserver/cmd_delete_content.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 4 names, 4 code segments found
 * Strings: 8, Variables: 2
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
int main(mixed agent, int cookie, string path, string name, string branch);
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
// @code  nc=81, nargs=5, insts=81
// --------------------------------------------------------------------
int main(mixed agent, int cookie, string path, string name, string branch) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    int ret;
    mixed sql_cmd;

    if ((!is_object(agent))) {
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        agent->send_message(9221, cookie, -2, 0);
        return 0;
    }
    sql_cmd = mysql_bind_parameters("delete from data where path=? and name=? and branch=?", path, name, branch);
    ret = (((*f_execute)(cno, sql_cmd) >= 1) ? 0 : -1);
    agent->send_message(9221, cookie, ret, 0);
    if ((((path == "runtime") && in_peel_data) && (ret == 0))) {
        "/dba/daemons/peel_datad.c"->remove_from_online_list(name);
    }
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