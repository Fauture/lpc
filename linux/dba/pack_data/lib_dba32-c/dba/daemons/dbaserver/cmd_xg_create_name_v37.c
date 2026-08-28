/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_xg_create_name.o
 * Source: /dba/daemons/dbaserver/cmd_xg_create_name.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 5, Variables: 2
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
int main(mixed agent, int cookie, string account, string xg_type, string name);

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
// @code  nc=110, nargs=5, insts=110
// --------------------------------------------------------------------
int main(mixed agent, int cookie, string account, string xg_type, string name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    int ret;
    mixed sql_cmd;

    if ((!is_object(agent))) {
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        agent->send_message(13345, cookie, -2);
        return 0;
    }
    if (operating_map[(xg_type + name)]) {
        agent->send_message(13345, cookie, -1);
        return 0;
    }
    operating_map[(xg_type + name)] = 1;
    sql_cmd = mysql_bind_parameters("insert into xg_name (account, xg_type, name, time) values (?, ?, ?, ?)", account, xg_type, name, ctime(time(), 3));
    ret = (*f_execute)(cno, sql_cmd);
    map_delete(operating_map, (xg_type + name));
    if ((!is_object(agent))) {
        return 0;
    }
    if ((ret < 1)) {
        agent->send_message(13345, cookie, -1);
        return 0;
    }
    agent->send_message(13345, cookie, 0);
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