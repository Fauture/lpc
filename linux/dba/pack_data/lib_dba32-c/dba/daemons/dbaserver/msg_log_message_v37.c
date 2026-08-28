/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: msg_log_message.o
 * Source: /dba/daemons/dbaserver/msg_log_message.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 3 names, 3 code segments found
 * Strings: 25, Variables: 4
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   /* mixed global_0; */
   mapping log_path_table;
   mapping table_to_cmd;
   mapping user_log_filter;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
private void create();
int main(mixed agent, string path, int severity, string type, string action, string para1, string para2, string para3, string memo);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   log_path_table = ([ "db_log" : "common_log", "user_log" : "user_log", "gs_log" : "gs_log", "active_stat_log" : "gs_log", "important_log" : "important_log" ]);
   table_to_cmd = ([ "common_log" : "cmd_log_common", "important_log" : "cmd_log_important", "user_log" : "cmd_log_user" ]);
   user_log_filter = set_case_check(([ ]), 0);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
mixed get_db_type() {
    /* ---- Enhanced Pseudocode ---- */
    return "ldb";

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=44, nargs=0, insts=44
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    int filter;
    mixed count;
    int i;

    count = read_ini("MsgFilter", "count", INI_FILE);
    for (i = 1; i <= atoi(count); i++) {
        filter = trim(read_ini("MsgFilter", ("Filter" + i), INI_FILE));
        if ((!strlen(filter))) {
            continue;
        }
        user_log_filter[filter] = 1;
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=152, nargs=9, insts=152
// --------------------------------------------------------------------
int main(mixed agent, string path, int severity, string type, string action, string para1, string para2, string para3, string memo) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;
    int table;
    mixed server;

    if ((!is_object(agent))) {
        return 0;
    }
    server = agent->query("name");
    if ((!is_string(table = log_path_table[path]))) {
        return 0;
    }
    if ((path == "user_log")) {
        if (user_log_filter[((type + ":") + action)]) {
            return 1;
        }
        if (user_log_filter[(type + ":*")]) {
            return 1;
        }
        if (user_log_filter["*"]) {
            return 1;
        }
    }
    if (is_string(table_to_cmd[table])) {
        "/dba/daemons/dbaserverd.c"->process_command(agent, table_to_cmd[table], ({ severity, type, action, para1, para2, para3, memo }));
        return 1;
    }
    cno = "/daemons/dbd.c"->assure_connection("ldb");
    if ((cno == -1)) {
        return 0;
    }
    switch (table) {
        case "gs_log":
        sql_cmd = mysql_bind_parameters((("insert into " + table) + " (server, dist, type, time, ctime, count, memo) values (?, ?, ?, ?, ?, ?, ?)"), para1, para2, para3, type, action, severity, memo);
        break;
        default:
        sql_cmd = mysql_bind_parameters((("insert into " + table) + " (server, severity, type, action, para1, para2, para3, memo, update_time) values (?, ?, ?, ?, ?, ?, ?, ?, ?)"), server, severity, type, action, para1, para2, para3, memo, ctime(time(), 3));
        break;
    }  // end switch

    // --- label L0140 ---
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