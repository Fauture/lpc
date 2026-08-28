/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: peel_datad.o
 * Source: /dba/daemons/peel_datad.c
 * Inherits: (none)
 * Functions: 18 names, 18 code segments found
 * Strings: 68, Variables: 14
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   array auxiliary_module_list;
   string cur_peel_gid;
   int enable_delete_data;
   int enable_peel;
   int limit_peel_num;
   int max_peel_interval;
   int min_peel_interval;
   mapping online_account_list;
   mapping online_gid_list;
   mapping peeling_data_list;
   float sleep_interval;
   string sql_cmd_user_data;
   int start_time;
   int total_peel_num;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

string build_move_to_log_sql(string gid);
array get_peel_list(array gid_list, int peel_interval, int count);
void remove_from_online_list(string account);
int is_in_peeling(string data);
string build_delete_from_data1_sql(string gid);
private void log_cur_progress(string msg);
private void destruct();
private void notice_auxiliary_module(int type);
string build_move_to_data_sql(string gid);
int load_online_list();
void add_to_online_list(string account, string data);
private void create();
void stop_peel_data();
private void move_data(array peel_list);
private void daemon(string max_gid, int peel_interval);
void do_peel_data(array peel_list);
mixed read_gid_list(string start_gid, int count);
void start_peel_data(string start_gid, string max_gid, int peel_interval);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   enable_peel = 1;  // /dba/daemons/peel_datad.c:51
   enable_delete_data = 1;  // /dba/daemons/peel_datad.c:56
   cur_peel_gid = "";  // /dba/daemons/peel_datad.c:67
   sql_cmd_user_data = mysql_bind_parameters("path in (?,?,?,?,?,?,?,?,?,?)", "store", "user", "house", "stall", "garden", "leave_msg", "login_notify", "purchase_order_store", "friend_image", "collect_card");
   peeling_data_list = ([ ]);
   auxiliary_module_list = ({ "cmd_read_content", "cmd_delete_content", "cmd_write_content" });
   online_gid_list = ([ ]);
   online_account_list = ([ ]);
   start_time = 0;
   total_peel_num = 0;
   sleep_interval = 0.5;
   max_peel_interval = 155520000;
   min_peel_interval = 7776000;
   limit_peel_num = 1000;

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=13, nargs=1, insts=13
// --------------------------------------------------------------------
string build_move_to_log_sql(string gid) {
    /* ---- Enhanced Pseudocode ---- */
    return (mysql_bind_parameters("insert into activate_user_data select *, ? as update_time from data1 where name = ? and ", ctime(time(), 3), gid) + sql_cmd_user_data);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=60, nargs=3, insts=60
// --------------------------------------------------------------------
array get_peel_list(array gid_list, int peel_interval, int count) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed arr;
    mixed sql_cmd;

    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        printf("get peel list connect to ddb failed\n");
        return ({});
    }
    sql_cmd = sprintf("select name from data force index(name_path) where path = 'user' and name in('%s') and branch = ''", implode(gid_list, "','"));
    sql_cmd = mysql_bind_parameters((sql_cmd + " and time < ?"), ctime((time() - peel_interval), 3));
    if (((!is_array(arr = mysql_select_column(cno, sql_cmd, 0, count))) || (!sizeof(arr)))) {
        return ({});
    }
    return arr;

}

// --------------------------------------------------------------------
// @code  nc=18, nargs=1, insts=18
// --------------------------------------------------------------------
void remove_from_online_list(string account) {
    /* ---- Enhanced Pseudocode ---- */
    mixed gid;

    gid = online_account_list[account];
    map_delete(online_account_list, account);
    map_delete(online_gid_list, gid);

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=31, nargs=1, insts=31
// --------------------------------------------------------------------
int is_in_peeling(string data) {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;

    if (((!is_mapping(m = restore_value(data))) || (!is_string(m["locked_gid"])))) {
        return 0;
    }
    if ((!peeling_data_list[m["locked_gid"]])) {
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=10, nargs=1, insts=10
// --------------------------------------------------------------------
string build_delete_from_data1_sql(string gid) {
    /* ---- Enhanced Pseudocode ---- */
    return (mysql_bind_parameters("delete from data1 where name = ? and ", gid) + sql_cmd_user_data);

}

// --------------------------------------------------------------------
// @code  nc=11, nargs=1, insts=11
// --------------------------------------------------------------------
private void log_cur_progress(string msg) {
    /* ---- Enhanced Pseudocode ---- */
    printf(msg);
    log_file("/dba/log/peel_data_log.txt", msg);

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=10, nargs=0, insts=10
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    if (get_thread_info("peel_data_thread")) {
        delete_thread("peel_data_thread");
    }

}

// --------------------------------------------------------------------
// @brief  notification: send message
// @code  nc=40, nargs=1, insts=40
// --------------------------------------------------------------------
private void notice_auxiliary_module(int type) {
    /* ---- Enhanced Pseudocode ---- */
    mixed file;

    switch (type) {
        case 1:
        loop (file in auxiliary_module_list) {
            "/dba/daemons/dbaserverd.c"->query_entry(file)->start_peel_data();
        }  // end loop (verified)
        return;
        case 2:
        loop (file in auxiliary_module_list) {
            "/dba/daemons/dbaserverd.c"->query_entry(file)->stop_peel_data();
        }  // end loop (verified)
        return;
        default:
        return;
    }  // end switch

}

// --------------------------------------------------------------------
// @code  nc=10, nargs=1, insts=10
// --------------------------------------------------------------------
string build_move_to_data_sql(string gid) {
    /* ---- Enhanced Pseudocode ---- */
    return (mysql_bind_parameters("insert ignore into data select * from data1 where name = ? and ", gid) + sql_cmd_user_data);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=127, nargs=0, insts=127
// --------------------------------------------------------------------
int load_online_list() {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed arr;
    mixed sql_cmd;
    int gid;
    mapping data;
    mapping m;

    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        printf("connect to ddb failed\n");
        return 0;
    }
    sql_cmd = mysql_bind_parameters("select name, content from data where path = ?", "runtime");
    if ((!is_array(arr = mysql_select(cno, sql_cmd, 100000)))) {
        printf("select online list failed\n");
        return 0;
    }
    online_gid_list = ([ ]);
    online_account_list = ([ ]);
    loop (m in arr) {
        if ((!is_mapping(m))) {
            continue;
        }
        if (((((!is_string(m["name"])) || (!strlen(m["name"]))) || (!is_string(m["content"]))) || (!strlen(m["content"])))) {
            continue;
        }
        if (((!is_mapping(data = restore_value(m["content"]))) || (!is_string(gid = data["locked_gid"])))) {
            continue;
        }
        online_account_list[m["name"]] = gid;
        online_gid_list[gid] = m["name"];
    }  // end loop (verified)
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=33, nargs=2, insts=33
// --------------------------------------------------------------------
void add_to_online_list(string account, string data) {
    /* ---- Enhanced Pseudocode ---- */
    int gid;
    mapping m;

    if (((!is_mapping(m = restore_value(data))) || (!is_string(gid = m["locked_gid"])))) {
        return;
    }
    online_gid_list[gid] = account;
    online_account_list[account] = gid;

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=4, nargs=0, insts=4
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @code  nc=27, nargs=0, insts=27
// --------------------------------------------------------------------
void stop_peel_data() {
    /* ---- Enhanced Pseudocode ---- */
    if (get_thread_info("peel_data_thread")) {
        delete_thread("peel_data_thread");
    }
    notice_auxiliary_module(2);
    online_gid_list = ([ ]);
    online_account_list = ([ ]);
    log_cur_progress(sprintf("\nStop peel data:\ncur time : %O\ncur peel gid : %O\n", time_desc(), cur_peel_gid));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=118, nargs=1, insts=118
// --------------------------------------------------------------------
private void move_data(array peel_list) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    int move_num;
    int delete_num;
    mixed sql_cmd;

    sql_cmd = (sprintf("replace into data1 select * from data where name in ('%s') and ", implode(peel_list, "','")) + sql_cmd_user_data);
    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        printf("move data connect to ddb failed\n");
        return;
    }
    if (((!is_int(move_num = (*f_execute)(cno, sql_cmd))) || (move_num < 1))) {
        log_cur_progress(sprintf("Error move data failed : %O, %O\n", sql_cmd, move_num));
        return;
    }
    total_peel_num += move_num;
    if ((!enable_delete_data)) {
        return;
    }
    sql_cmd = (sprintf("delete from data where name in ('%s') and ", implode(peel_list, "','")) + sql_cmd_user_data);
    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        log_cur_progress("Error delete data connect to ddb failed\n");
        return;
    }
    if (((!is_int(delete_num = (*f_execute)(cno, sql_cmd))) || (delete_num < 1))) {
        log_cur_progress(sprintf("Error delete %O failed\n", sql_cmd));
        return;
    }
    if ((move_num != delete_num)) {
        log_cur_progress(sprintf("Error not same count for %O, move : %O, delete : %O\n", sql_cmd, move_num, delete_num));
    }

}

// --------------------------------------------------------------------
// @code  nc=112, nargs=2, insts=112
// --------------------------------------------------------------------
private void daemon(string max_gid, int peel_interval) {
    /* ---- Enhanced Pseudocode ---- */
    mixed gid_list;
    mixed peel_list;
    mixed msg;

    set_as_daemon();
    log_cur_progress("\nPeel progress:\n");
    do {
        if ((!enable_peel)) {
            break;
        }
        if ((strcmp(cur_peel_gid, max_gid) >= 0)) {
            break;
        }
        gid_list = read_gid_list(cur_peel_gid, limit_peel_num);
        if ((!is_array(gid_list))) {
            printf("read gid list error\n");
            sleep(10);
        } else if ((sizeof(gid_list) < 1)) {
            break;
        }
        cur_peel_gid = gid_list[<1];
        peel_list = get_peel_list(gid_list, peel_interval, limit_peel_num);
        if ((sizeof(peel_list) < 1)) {
            sleep(sleep_interval);
        } else {
            try {
                do_peel_data(peel_list);
            } catch { /* error handled */ }

            // --- label L0073 ---
            log_cur_progress(sprintf("%O : %O\n", time_desc(), cur_peel_gid));
            sleep(sleep_interval);
        }
    } while (1);

    // --- label L0086 ---
    notice_auxiliary_module(2);
    online_gid_list = ([ ]);
    online_account_list = ([ ]);
    msg = sprintf("\nFinish peel data:\ncur time : %O\ntotal cost time : %O\npeel database records num : %O\n", time_desc(), (time() - start_time), total_peel_num);
    log_cur_progress(msg);

}

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=40, nargs=1, insts=40
// --------------------------------------------------------------------
void do_peel_data(array peel_list) {
    /* ---- Enhanced Pseudocode ---- */
    int gid;

    peeling_data_list = ([ ]);
    loop (gid in peel_list) {
        if (online_gid_list[gid]) {
            continue;
        }
        peeling_data_list[gid] = 1;
    }  // end loop (verified)
    try {
        move_data(keys(peeling_data_list));
    } catch { /* error handled */ }

    // --- label L0033 ---
    peeling_data_list = ([ ]);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=131, nargs=2, insts=131
// --------------------------------------------------------------------
mixed read_gid_list(string start_gid, int count) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    int i;
    int first;
    int second;
    mixed arr;
    int val64;
    mixed sql_cmd;
    mixed gid;

    first = 0;
    second = 0;
    sscanf(start_gid[0..7], "%x", &first);
    sscanf(start_gid[8..15], "%x", &second);
    val64 = ({ first, second });
    sql_cmd = mysql_bind_parameters("select gid from gid_info where type=? and gid > ? order by gid limit ?", "user", itoa64(val64), count);
    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        printf("read gid list connect to ddb failed\n");
        return 0;
    }
    if ((!is_array(arr = mysql_select_column(cno, sql_cmd, 0, count)))) {
        return 0;
    }
    for (i = 0; i <= (sizeof(arr) - 1); i++) {
        if (((!is_string(gid = arr[i])) || (!strlen(gid)))) {
            arr[i] = 0;
            continue;
        }
        val64 = atoi64(gid);
        arr[i] = sprintf("%08X%08X", val64[0], val64[1]);
    }  // end loop (verified)
    clean_array(arr, 4);
    return arr;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=172, nargs=3, insts=172
// --------------------------------------------------------------------
void start_peel_data(string start_gid, string max_gid, int peel_interval) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed arr;
    mixed val64;
    mixed sql_cmd;
    mixed cur_max_gid;

    if ((!enable_peel)) {
        printf("disabled peel\n");
        return;
    }
    if (get_thread_info("peel_data_thread")) {
        printf("in peel data already\n");
        return;
    }
    if ((peel_interval < min_peel_interval)) {
        printf(sprintf("only peel time > %d\n", min_peel_interval));
        return;
    }
    if ((peel_interval > max_peel_interval)) {
        printf("only peel time < %d\n", max_peel_interval);
        return;
    }
    start_time = time();
    total_peel_num = 0;
    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        printf("select max gid connect to ddb failed\n");
        return;
    }
    sql_cmd = mysql_bind_parameters("select max(gid) from gid_info");
    if (((!is_array(arr = mysql_select_column(cno, sql_cmd, 0, 1))) || (!sizeof(arr)))) {
        printf("select max gid failed\n");
        return;
    }
    val64 = atoi64(arr[0]);
    cur_max_gid = sprintf("%08X%08X", val64[0], val64[1]);
    if (((max_gid == "*") || (cur_max_gid < max_gid))) {
        max_gid = cur_max_gid;
    }
    if ((strlen(start_gid) < 16)) {
        printf("bad start gid\n");
        return;
    }
    if ((strlen(max_gid) < 16)) {
        printf("bad max gid\n");
        return;
    }
    cur_peel_gid = start_gid;
    log_cur_progress("\n------------------------------------\n");
    log_cur_progress(sprintf("Start peel data:\nstart time : %O\nstart gid : %O\nmax gid : %O\n", time_desc(), start_gid, max_gid));
    notice_auxiliary_module(1);
    if ((!load_online_list())) {
        return;
    }
    create_thread(([ "name": "peel_data_thread", "stack_size": 512 ]), "daemon", max_gid, peel_interval);

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        18
   Code segments found:         18
   Successfully decoded:        18
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */