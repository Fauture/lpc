/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: batch_logd.o
 * Source: /dba/daemons/batch_logd.c
 * Inherits: (none)
 * Functions: 26 names, 26 code segments found
 * Strings: 30, Variables: 7
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   mapping all_entries;
   int batch_enabled;
   mapping batch_enabled_table;
   mapping custom_max_sql_size;
   mapping custom_max_write_interval;
   mapping stat;
   int writeQ;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void check_force_write_log();
mapping query_all_entries();
int is_batch_enabled_by_table(mixed table_name);
int get_max_write_interval(mixed table_name);
private void do_stat(string table, string field, int value);
void set_max_write_interval(string table_name, int write_interval);
void disable_batch_by_table(string table_name);
void insert_sql_cmd(string table, string sql_cmd, int log_count, int wait_time);
int is_batch_enabled();
void register_entry(mixed index, mixed ob);
private void destruct();
void set_max_sql_size(string table_name, int sql_size);
mixed query_entry(mixed index);
void save();
void enable_batch();
void disable_batch();
void remove_entry(mixed index);
private void start_write_thread();
void write_log(mapping msg);
private void create();
int get_max_sql_size(mixed table_name);
private void daemon();
private void save_daemon_data();
void enable_batch_by_table(string table_name);
private void recovery_daemon_data();
private void end_write_thread();

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   batch_enabled = 1;  // /dba/daemons/batch_logd.c:51
   batch_enabled_table = ([ ]);  // /dba/daemons/batch_logd.c:441
   custom_max_sql_size = ([ ]);
   custom_max_write_interval = ([ ]);
   all_entries = ([ ]);
   stat = ([ ]);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  validator: verify data
// @code  nc=24, nargs=0, insts=24
// --------------------------------------------------------------------
void check_force_write_log() {
    /* ---- Enhanced Pseudocode ---- */
    mixed entry;

    loop (entry in values(all_entries)) {
        if (is_object(entry)) {
            try {
                entry->check_force_write_log();
            } catch { /* error handled */ }

            // --- label L0020 ---
        }
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
mapping query_all_entries() {
    /* ---- Enhanced Pseudocode ---- */
    return all_entries;

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=15, nargs=1, insts=15
// --------------------------------------------------------------------
int is_batch_enabled_by_table(mixed table_name) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!is_batch_enabled())) {
        return 0;
    }
    return (!(!batch_enabled_table[table_name]));

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=16, nargs=1, insts=16
// --------------------------------------------------------------------
int get_max_write_interval(mixed table_name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed write_interval;

    if ((!is_int(write_interval = custom_max_write_interval[table_name]))) {
        return 300;
    }
    return write_interval;

}

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=45, nargs=3, insts=45
// --------------------------------------------------------------------
private void do_stat(string table, string field, int value) {
    /* ---- Enhanced Pseudocode ---- */
    mixed m;

    if (("total_size" == field)) {
        value = ((1 * value) / 1024);
    }
    if ((!is_mapping(m = stat[table]))) {
        stat[table] = ([ field: value ]);
        return;
    }
    m[field] = (value + (int)m[field]);

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=10, nargs=2, insts=10
// --------------------------------------------------------------------
void set_max_write_interval(string table_name, int write_interval) {
    /* ---- Enhanced Pseudocode ---- */
    custom_max_write_interval[table_name] = write_interval;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=32, nargs=1, insts=32
// --------------------------------------------------------------------
void disable_batch_by_table(string table_name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed entry;

    if (is_object(entry = query_entry(table_name))) {
        try {
            entry->force_write_log();
        } catch { /* error handled */ }

        // --- label L0017 ---
    }
    if ((!batch_enabled_table[table_name])) {
        return;
    }
    batch_enabled_table[table_name] = 0;

}

// --------------------------------------------------------------------
// @code  nc=68, nargs=4, insts=68
// --------------------------------------------------------------------
void insert_sql_cmd(string table, string sql_cmd, int log_count, int wait_time) {
    /* ---- Enhanced Pseudocode ---- */
    mixed msg;

    do_stat(table, "total_size", strlen(sql_cmd));
    do_stat(table, "total_count", log_count);
    do_stat(table, "total_wait", wait_time);
    do_stat(table, "total_stat", 1);
    if (((!writeQ) || (get_thread_id("tBatchLogWrite") == -1))) {
        write_file(("/dba/log/" + sprintf("%s_%s_%03d.sql", table, ctime(time(), 3), (tick() % 1000))), sql_cmd);
        return;
    }
    msg = ([ "oper": "execute", "sql": sql_cmd, "table": table, "log_count": log_count, "wait_time": wait_time ]);
    send_to_queue(writeQ, msg);

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
int is_batch_enabled() {
    /* ---- Enhanced Pseudocode ---- */
    return (!(!batch_enabled));

}

// --------------------------------------------------------------------
// @code  nc=13, nargs=2, insts=13
// --------------------------------------------------------------------
void register_entry(mixed index, mixed ob) {
    /* ---- Enhanced Pseudocode ---- */
    all_entries[index] = ob;
    enable_batch_by_table(index);

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=13, nargs=0, insts=13
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    save_daemon_data();
    "/daemons/scheduled.c"->delete_scheme("sBatchLogWriteLog");
    end_write_thread();

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=18, nargs=2, insts=18
// --------------------------------------------------------------------
void set_max_sql_size(string table_name, int sql_size) {
    /* ---- Enhanced Pseudocode ---- */
    if ((sql_size > 524288)) {
        sql_size = 524288;
    }
    custom_max_sql_size[table_name] = sql_size;

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
mixed query_entry(mixed index) {
    /* ---- Enhanced Pseudocode ---- */
    return all_entries[index];

}

// --------------------------------------------------------------------
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
void save() {
    /* ---- Enhanced Pseudocode ---- */
    end_write_thread();

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=25, nargs=0, insts=25
// --------------------------------------------------------------------
void enable_batch() {
    /* ---- Enhanced Pseudocode ---- */
    mixed entry;

    if (batch_enabled) {
        return;
    }
    batch_enabled = 1;
    loop (entry in values(query_all_entries())) {
        if (is_object(entry)) {
            entry->update_last_write_log();
        }
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=28, nargs=0, insts=28
// --------------------------------------------------------------------
void disable_batch() {
    /* ---- Enhanced Pseudocode ---- */
    mixed entry;

    loop (entry in values(query_all_entries())) {
        if (is_object(entry)) {
            try {
                entry->force_write_log();
            } catch { /* error handled */ }

            // --- label L0020 ---
        }
    }  // end loop (verified)
    batch_enabled = 0;

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
void remove_entry(mixed index) {
    /* ---- Enhanced Pseudocode ---- */
    map_delete(all_entries, index);

}

// --------------------------------------------------------------------
// @code  nc=18, nargs=0, insts=18
// --------------------------------------------------------------------
private void start_write_thread() {
    /* ---- Enhanced Pseudocode ---- */
    writeQ = create_queue("qBatchLogWrite");
    if ((get_thread_id("tBatchLogWrite") == -1)) {
        create_thread("tBatchLogWrite", "daemon");
    }

}

// --------------------------------------------------------------------
// @brief  data writer: persist information
// @code  nc=58, nargs=1, insts=58
// --------------------------------------------------------------------
void write_log(mapping msg) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed table;
    mixed sql_cmd;

    table = msg["table"];
    sql_cmd = msg["sql"];
    do {
        if (((cno = "/daemons/dbd.c"->assure_connection("ldb")) < 0)) {
            break;
        }
        if (((*f_execute)(cno, sql_cmd) < 0)) {
            break;
        }
        return;
    } while (0);

    // --- label L0037 ---
    write_file(("/dba/log/" + sprintf("%s_%s_%03d.sql", table, ctime(time(), 3), (tick() % 1000))), sql_cmd);
    do_stat(table, "total_fail", 1);

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=22, nargs=0, insts=22
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    require_uid();
    set_euid(0);
    recovery_daemon_data();
    "/daemons/scheduled.c"->create_scheme("sBatchLogWriteLog", (: check_force_write_log :), time(), 60, 1);
    start_write_thread();

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=21, nargs=1, insts=21
// --------------------------------------------------------------------
int get_max_sql_size(mixed table_name) {
    /* ---- Enhanced Pseudocode ---- */
    int sql_size;

    if (((!is_int(sql_size = custom_max_sql_size[table_name])) || (sql_size > 524288))) {
        return 524288;
    }
    return sql_size;

}

// --------------------------------------------------------------------
// @code  nc=46, nargs=0, insts=46
// --------------------------------------------------------------------
private void daemon() {
    /* ---- Enhanced Pseudocode ---- */
    mapping msg;

    set_as_daemon();

    // --- label L0004 ---
    while ((receive_from_queue(writeQ, &msg) == 0)) {
        if ((!is_mapping(msg))) {
            continue;
        }
        if (("quit" == msg["oper"])) {
            printf("BATCH_LOG_D.write_thread daemon done.\n");
            return;
        }
        if (("execute" != msg["oper"])) {
            continue;
        }
        try {
            write_log(msg);
        } catch { /* error handled */ }

        // --- label L0041 ---
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @code  nc=12, nargs=0, insts=12
// --------------------------------------------------------------------
private void save_daemon_data() {
    /* ---- Enhanced Pseudocode ---- */
    if (sizeof(all_entries)) {
        register_daemon_data("/dba/daemons/batch_logd.c", "all_entries", all_entries);
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=25, nargs=1, insts=25
// --------------------------------------------------------------------
void enable_batch_by_table(string table_name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed entry;

    if (batch_enabled_table[table_name]) {
        return;
    }
    batch_enabled_table[table_name] = 1;
    if (is_object(entry = query_entry(table_name))) {
        entry->update_last_write_log();
    }

}

// --------------------------------------------------------------------
// @code  nc=23, nargs=0, insts=23
// --------------------------------------------------------------------
private void recovery_daemon_data() {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;

    if ((!is_mapping(m = get_daemon_data("/dba/daemons/batch_logd.c")))) {
        return;
    }
    if (is_mapping(m["all_entries"])) {
        all_entries = m["all_entries"];
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=42, nargs=0, insts=42
// --------------------------------------------------------------------
private void end_write_thread() {
    /* ---- Enhanced Pseudocode ---- */
    mixed entry;

    loop (entry in values(query_all_entries())) {
        if (is_object(entry)) {
            try {
                entry->force_write_log();
            } catch { /* error handled */ }

            // --- label L0020 ---
        }
    }  // end loop (verified)
    send_to_queue(writeQ, ([ "oper": "quit" ]));

    // --- label L0028 ---
    while ((get_thread_id("tBatchLogWrite") != -1)) {
        sleep(0.1);
    }  // end loop (verified)
    delete_queue(writeQ);

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        26
   Code segments found:         26
   Successfully decoded:        26
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */