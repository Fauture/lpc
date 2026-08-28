/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: logd.o
 * Source: /daemons/logd.c
 * Inherits: (none)
 * Functions: 9 names, 9 code segments found
 * Strings: 41, Variables: 2
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   int _ldb;
   mapping severity_desc;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void log_login_info(string log_file_name, string action, string memo);
int get_ldb();
void log_message(string log_file_name, string memo);
void browse_log(string sql_cond, int max_count = 128);
private void init();
private void destruct();
void assure_log_file(string log_file_name);
void log_local(int severity, string type, string action, string para1, string para2, string para3, string memo);
private void create();

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   _ldb = 65535;  // /daemons/logd.c:13
   severity_desc = ([ 1 : "informative", 2 : "minor", 3 : "major", 4 : "critical" ]);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=14, nargs=3, insts=14
// --------------------------------------------------------------------
void log_login_info(string log_file_name, string action, string memo) {
    /* ---- Enhanced Pseudocode ---- */
    to_log_file(log_file_name, ((ctime(time(), 1) + ":") + memo));

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int get_ldb() {
    /* ---- Enhanced Pseudocode ---- */
    return _ldb;

}

// --------------------------------------------------------------------
// @code  nc=16, nargs=2, insts=16
// --------------------------------------------------------------------
void log_message(string log_file_name, string memo) {
    /* ---- Enhanced Pseudocode ---- */
    to_log_file(log_file_name, ((ctime()[4..18] + ":") + memo));

}

// --------------------------------------------------------------------
// @code  nc=105, nargs=2, insts=105
// --------------------------------------------------------------------
void browse_log(string sql_cond, int max_count) {
    /* ---- Enhanced Pseudocode ---- */
    mapping ret;
    mixed query_index;
    string color;

    query_index = sql_query(_ldb, ("select * from log where " + sql_cond));
    if ((query_index == -1)) {
        printf("[1;31mEmpty result.\n[2;37;0m");
        return;
    }
    try {
        printf("[1;36m%-20s %-11s %-10s %-16s\n[2;37;0m", "time", "severity", "type", "action");
        color = "[37m";

        // --- label L0033 ---
        while ((is_mapping(ret = sql_step(query_index)) && max_count--)) {
            color = ((color == "[37m") ? "[36m" : "[37m");
            printf("%s%-20s %-11s %-10s %-16s Para: %s/%s/%s Memo: %s\n[2;37;0m", color, ret["time"], ret["severity"], ret["type"], ret["action"], ret["para1"][0..11], ret["para2"][0..11], ret["para3"][0..11], ret["memo"]);
        }  // end loop (verified)
    } catch { /* error handled */ }

    // --- label L0099 ---
    sql_finalize(query_index);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=101, nargs=0, insts=101
// --------------------------------------------------------------------
private void init() {
    /* ---- Enhanced Pseudocode ---- */
    if ((0 && ("/daemons/dbd.c"->assure_connection("mdb") == -1))) {
        broadcast("[1;32mFailed to connect to MDB.\n[2;37;0m");
    }
    if ((1 && ("/daemons/dbd.c"->assure_connection("ldb") == -1))) {
        broadcast("[1;32mFailed to connect to log database.\n[2;37;0m");
    }
    _ldb = sql_open_db((("/dba/log/log_" + _server_name) + ".sql"));
    if ((_ldb == -1)) {
        error((("Failed to open database: /dba/log/log_" + _server_name) + ".sql\n"));
    }
    printf("Pack database of sqlite: %s.\n", (("/dba/log/log_" + _server_name) + ".sql"));
    sql_execute(_ldb, "vacuum");
    if ((!sql_get_table(_ldb, "log"))) {
        sql_execute(_ldb, "create table log (time varchar(32), severity varchar(16), type varchar(24), action varchar(24), para1 varchar(24), para2 varchar(24), para3 varchar(24), memo text);");
        sql_execute(_ldb, "create index log_time_idx on log (time);");
        sql_execute(_ldb, "create index log_sev_type_idx on log (severity, type);");
        sql_execute(_ldb, "create index log_action_idx on log (action);");
        sql_execute(_ldb, "create index log_para1_idx on log (para1);");
    }
    sql_execute(_ldb, "pragma synchronous = off");
    sql_execute(_ldb, sprintf("delete from log where time < '%s'", time_desc((time() - 604800))));

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=16, nargs=0, insts=16
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    if ((_ldb != -1)) {
        sql_close_db(_ldb);
        _ldb = -1;
    }

}

// --------------------------------------------------------------------
// @brief  verification: confirm/guarantee state
// @code  nc=7, nargs=1, insts=7
// --------------------------------------------------------------------
void assure_log_file(string log_file_name) {
    /* ---- Enhanced Pseudocode ---- */
    assure_file(log_file_name);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=31, nargs=7, insts=31
// --------------------------------------------------------------------
void log_local(int severity, string type, string action, string para1, string para2, string para3, string memo) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_service((: sql_insert, _ldb, "log", ([ "time": time_desc(), "severity": (string)severity_desc[severity], "type": type, "action": action, "para1": para1, "para2": para2, "para3": para3, "memo": memo ]) :));

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=7, nargs=0, insts=7
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    register_post_init((: init :));

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        9
   Code segments found:         9
   Successfully decoded:        9
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */