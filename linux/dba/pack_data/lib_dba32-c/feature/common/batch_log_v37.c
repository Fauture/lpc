/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: batch_log.o
 * Source: /feature/common/batch_log.c
 * Inherits: (none)
 * Functions: 17 names, 17 code segments found
 * Strings: 8, Variables: 7
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   array dynamic_sql_arr;
   int dynamic_sql_length;
   int last_write_log;
   int log_count;
   string static_sql;
   int static_sql_length;
   string table_name;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void set_table_name(string name);
void check_force_write_log();
string get_dynamic_sql();
void update_last_write_time();
void force_write_log();
int get_log_count();
string get_table_name();
int get_last_write_log();
int is_batch_enabled();
void insert_log(string value_desc);
private void destruct();
void save();
void write_log();
string get_static_sql();
string get_whole_sql();
array get_dynamic_sql_arr();
void set_static_sql(string sql);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   table_name = "";  // /feature/common/batch_log.c:23
   static_sql = "";  // /feature/common/batch_log.c:35
   static_sql_length = 0;
   log_count = 0;
   dynamic_sql_length = 0;
   dynamic_sql_arr = ({ });
   last_write_log = time();

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
void set_table_name(string name) {
    /* ---- Enhanced Pseudocode ---- */
    table_name = name;

}

// --------------------------------------------------------------------
// @brief  validator: verify data
// @code  nc=20, nargs=0, insts=20
// --------------------------------------------------------------------
void check_force_write_log() {
    /* ---- Enhanced Pseudocode ---- */
    if ((!is_batch_enabled())) {
        return;
    }
    if (((time() - last_write_log) < "/dba/daemons/batch_logd.c"->get_max_write_interval(table_name))) {
        return;
    }
    force_write_log();

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=22, nargs=0, insts=22
// --------------------------------------------------------------------
string get_dynamic_sql() {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr;

    if ((sizeof((arr = dynamic_sql_arr[0..(log_count - 1)])) < 1)) {
        return "";
    }
    return implode(arr, ",");

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
void update_last_write_time() {
    /* ---- Enhanced Pseudocode ---- */
    last_write_log = time();

}

// --------------------------------------------------------------------
// @code  nc=10, nargs=0, insts=10
// --------------------------------------------------------------------
void force_write_log() {
    /* ---- Enhanced Pseudocode ---- */
    if ((!is_batch_enabled())) {
        return;
    }
    write_log();

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int get_log_count() {
    /* ---- Enhanced Pseudocode ---- */
    return log_count;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
string get_table_name() {
    /* ---- Enhanced Pseudocode ---- */
    return table_name;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int get_last_write_log() {
    /* ---- Enhanced Pseudocode ---- */
    return last_write_log;

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=22, nargs=0, insts=22
// --------------------------------------------------------------------
int is_batch_enabled() {
    /* ---- Enhanced Pseudocode ---- */
    if (((!strlen(table_name)) || (!strlen(static_sql)))) {
        return 0;
    }
    return (!(!"/dba/daemons/batch_logd.c"->is_batch_enabled_by_table(table_name)));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=100, nargs=1, insts=100
// --------------------------------------------------------------------
void insert_log(string value_desc) {
    /* ---- Enhanced Pseudocode ---- */
    mixed new_length;

    if ((!is_batch_enabled())) {
        return;
    }
    if ((log_count < 1)) {
        dynamic_sql_arr = allocate_array(10000);
        log_count = 1;
        dynamic_sql_arr[(log_count - 1)] = value_desc;
        dynamic_sql_length = strlen(value_desc);
        return;
    }
    new_length = strlen(value_desc);
    if (((dynamic_sql_length < (("/dba/daemons/batch_logd.c"->get_max_sql_size(table_name) - static_sql_length) - new_length)) && (log_count < 10000))) {
        ++log_count;
        dynamic_sql_arr[(log_count - 1)] = value_desc;
        dynamic_sql_length += new_length;
        return;
    }
    write_log();
    dynamic_sql_arr = allocate_array(10000);
    log_count = 1;
    dynamic_sql_arr[(log_count - 1)] = value_desc;
    dynamic_sql_length = strlen(value_desc);

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    force_write_log();

}

// --------------------------------------------------------------------
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
void save() {
    /* ---- Enhanced Pseudocode ---- */
    force_write_log();

}

// --------------------------------------------------------------------
// @brief  data writer: persist information
// @code  nc=41, nargs=0, insts=41
// --------------------------------------------------------------------
void write_log() {
    /* ---- Enhanced Pseudocode ---- */
    mixed wait_time;
    mixed sql_cmd;

    if ((log_count < 1)) {
        return;
    }
    sql_cmd = get_whole_sql();
    wait_time = (time() - last_write_log);
    "/dba/daemons/batch_logd.c"->insert_sql_cmd(table_name, sql_cmd, log_count, wait_time);
    update_last_write_time();
    dynamic_sql_arr = ({});
    log_count = 0;
    dynamic_sql_length = 0;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
string get_static_sql() {
    /* ---- Enhanced Pseudocode ---- */
    return static_sql;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=10, nargs=0, insts=10
// --------------------------------------------------------------------
string get_whole_sql() {
    /* ---- Enhanced Pseudocode ---- */
    return ((static_sql + get_dynamic_sql()) + ";");

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=11, nargs=0, insts=11
// --------------------------------------------------------------------
array get_dynamic_sql_arr() {
    /* ---- Enhanced Pseudocode ---- */
    return dynamic_sql_arr[0..(log_count - 1)];

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=13, nargs=1, insts=13
// --------------------------------------------------------------------
void set_static_sql(string sql) {
    /* ---- Enhanced Pseudocode ---- */
    static_sql = sql;
    static_sql_length = strlen(sql);

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        17
   Code segments found:         17
   Successfully decoded:        17
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */