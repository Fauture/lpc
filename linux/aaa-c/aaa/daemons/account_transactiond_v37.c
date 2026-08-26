/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: account_transactiond.o
 * Source: /aaa/daemons/account_transactiond.c
 * Inherits: (none)
 * Functions: 15 names, 15 code segments found
 * Strings: 33, Variables: 5
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   mapping check_info_func_map;
   int precost_lock_effective_interval;
   int precost_lock_enabled;
   mapping precost_lock_list;
   mapping query_info_func_map;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

int update_account_info(int cno, mapping info, mixed fields);
void delete_precost_lock(string account);
int insert_account_coin_trade_info(int cno, mapping info);
string calc_account_coin_trade_checksum(mapping coin_trade_info);
private void clear_timeout_precost_lock();
private void destruct();
void set_precost_lock(string account, mixed lock);
int is_precost_locked(string account);
mixed query_account_info(int cno, string account);
int update_account_coin_trade_info(int cno, mapping info, mixed fields);
int end_account_transaction(int cno, int ret);
private void create();
int start_account_transaction(int cno, string account, string table = "account");
mixed query_account_coin_trade_info(int cno, string account);
mixed do_account_transaction(int cno, string account, string table, function proc_func, mixed fields);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   precost_lock_enabled = 1;  // /aaa/daemons/account_transactiond.c:28
   precost_lock_effective_interval = 60;  // /aaa/daemons/account_transactiond.c:54
   precost_lock_list = ([ ]);
   query_info_func_map = ([ "account" : (: query_account_info :), "account_coin_trade" : (: query_account_coin_trade_info :) ]);
   check_info_func_map = ([ "account" : (: call_other, "/aaa/daemons/accountd.c", "calc_account_checksum" :), "account_coin_trade" : (: calc_account_coin_trade_checksum :) ]);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=118, nargs=3, insts=118
// --------------------------------------------------------------------
int update_account_info(int cno, mapping info, mixed fields) {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr;
    mixed mks;
    string str;
    mixed checksum;
    mixed sql_cmd;

    checksum = "/aaa/daemons/accountd.c"->calc_account_checksum(info);
    info["checksum"] = checksum;
    str = "update account set ";
    arr = ({});
    if (is_array(fields)) {
        loop (mks in info) {
            if ((member_array(mks[0], fields) > -1)) {
                str += sprintf("%s = ?, ", mks[0]);
                arr += ({ mks[1] });
            }
        }  // end loop (verified)
    } else {
        loop (mks in info) {
            str += sprintf("%s = ?, ", mks[0]);
            arr += ({ mks[1] });
        }  // end loop (verified)
    }
    str = (str[0..<3] + " where account = ?");
    arr += ({ info["account"] });
    sql_cmd = mysql_bind_parameters_arr(str, arr);
    if (((*f_execute)(cno, sql_cmd) < 1)) {
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
void delete_precost_lock(string account) {
    /* ---- Enhanced Pseudocode ---- */
    map_delete(precost_lock_list, account);

}

// --------------------------------------------------------------------
// @code  nc=53, nargs=2, insts=53
// --------------------------------------------------------------------
int insert_account_coin_trade_info(int cno, mapping info) {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr;
    mixed checksum;
    mixed sql_cmd;

    checksum = calc_account_coin_trade_checksum(info);
    info["checksum"] = checksum;
    arr = allocate_array(sizeof(info), "?");
    sql_cmd = sprintf("insert into account_coin_trade(%s) values(%s)", implode(keys(info), ","), implode(arr, ","));
    sql_cmd = mysql_bind_parameters_arr(sql_cmd, values(info));
    if (((*f_execute)(cno, sql_cmd) < 1)) {
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=35, nargs=1, insts=35
// --------------------------------------------------------------------
string calc_account_coin_trade_checksum(mapping coin_trade_info) {
    /* ---- Enhanced Pseudocode ---- */
    mixed account;
    mixed gold_coin;
    mixed str;

    account = coin_trade_info["account"];
    gold_coin = sprintf("%08X", atoi(coin_trade_info["gold_coin"]));
    str = ((account + gold_coin) + "ABCDEF");
    return save_string(md5(str))[1..<2];

}

// --------------------------------------------------------------------
// @code  nc=58, nargs=0, insts=58
// --------------------------------------------------------------------
private void clear_timeout_precost_lock() {
    /* ---- Enhanced Pseudocode ---- */
    mixed mks;
    mixed lock;
    int account;

    loop (mks in precost_lock_list) {
        if ((!is_string(account = mks[0]))) {
            precost_lock_list[account] = 0;
            continue;
        }
        if ((!is_int(lock = mks[1]))) {
            continue;
        }
        if ((lock == -1)) {
            continue;
        }
        if ((time() <= (lock + precost_lock_effective_interval))) {
            continue;
        }
        precost_lock_list[account] = 0;
    }  // end loop (verified)
    clean_mapping_by_value(precost_lock_list, 2);

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=17, nargs=0, insts=17
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/scheduled.c"->delete_scheme("sClearPrecostLock");
    if (sizeof(precost_lock_list)) {
        register_daemon_data("/aaa/daemons/account_transactiond.c", "precost_lock_list", precost_lock_list);
    }

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=10, nargs=2, insts=10
// --------------------------------------------------------------------
void set_precost_lock(string account, mixed lock) {
    /* ---- Enhanced Pseudocode ---- */
    precost_lock_list[account] = lock;

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=37, nargs=1, insts=37
// --------------------------------------------------------------------
int is_precost_locked(string account) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!precost_lock_enabled)) {
        return 0;
    }
    if ((!is_int(precost_lock_list[account]))) {
        return 0;
    }
    if ((precost_lock_list[account] == -1)) {
        return 1;
    }
    if ((time() <= (precost_lock_list[account] + precost_lock_effective_interval))) {
        return 1;
    }
    return 0;

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=14, nargs=2, insts=14
// --------------------------------------------------------------------
mixed query_account_info(int cno, string account) {
    /* ---- Enhanced Pseudocode ---- */
    mixed sql_cmd;

    sql_cmd = mysql_bind_parameters("select * from account where account = ? limit 1", account);
    return mysql_select_single_result(cno, sql_cmd);

}

// --------------------------------------------------------------------
// @code  nc=116, nargs=3, insts=116
// --------------------------------------------------------------------
int update_account_coin_trade_info(int cno, mapping info, mixed fields) {
    /* ---- Enhanced Pseudocode ---- */
    mixed mks;
    mixed arr;
    string str;
    mixed checksum;
    mixed sql_cmd;

    checksum = calc_account_coin_trade_checksum(info);
    info["checksum"] = checksum;
    str = "update account_coin_trade set ";
    arr = ({});
    if (is_array(fields)) {
        loop (mks in info) {
            if ((member_array(mks[0], fields) > -1)) {
                str += sprintf("%s = ?, ", mks[0]);
                arr += ({ mks[1] });
            }
        }  // end loop (verified)
    } else {
        loop (mks in info) {
            str += sprintf("%s = ?, ", mks[0]);
            arr += ({ mks[1] });
        }  // end loop (verified)
    }
    str = (str[0..<3] + ", update_time = date_format(CURRENT_TIMESTAMP, '%Y%m%d%H%i%s') where account = ?");
    arr += ({ info["account"] });
    sql_cmd = mysql_bind_parameters_arr(str, arr);
    if (((*f_execute)(cno, sql_cmd) < 1)) {
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=28, nargs=2, insts=28
// --------------------------------------------------------------------
int end_account_transaction(int cno, int ret) {
    /* ---- Enhanced Pseudocode ---- */
    if (((*f_execute)(cno, (ret ? "commit" : "rollback")) != 0)) {
        return 0;
    }
    if (((*f_execute)(cno, "set autocommit=1") != 0)) {
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=36, nargs=0, insts=36
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;

    if (is_mapping(m = get_daemon_data("/aaa/daemons/account_transactiond.c"))) {
        if (is_mapping(m["precost_lock_list"])) {
            precost_lock_list = m["precost_lock_list"];
            unregister_daemon_data("/aaa/daemons/account_transactiond.c", "precost_lock_list");
        }
    }
    "/daemons/scheduled.c"->create_scheme("sClearPrecostLock", (: clear_timeout_precost_lock :), time(), 300, 1);

}

// --------------------------------------------------------------------
// @code  nc=38, nargs=3, insts=38
// --------------------------------------------------------------------
int start_account_transaction(int cno, string account, string table) {
    /* ---- Enhanced Pseudocode ---- */
    mixed sql_cmd;

    if (((*f_execute)(cno, "set autocommit=0") != 0)) {
        return 0;
    }
    sql_cmd = sprintf("update %s set account = account where account = ?", table);
    sql_cmd = mysql_bind_parameters(sql_cmd, account);
    if (((*f_execute)(cno, sql_cmd) < 0)) {
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=14, nargs=2, insts=14
// --------------------------------------------------------------------
mixed query_account_coin_trade_info(int cno, string account) {
    /* ---- Enhanced Pseudocode ---- */
    mixed sql_cmd;

    sql_cmd = mysql_bind_parameters("select * from account_coin_trade where account = ? limit 1", account);
    return mysql_select_single_result(cno, sql_cmd);

}

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=105, nargs=5, insts=105
// --------------------------------------------------------------------
mixed do_account_transaction(int cno, string account, string table, function proc_func, mixed fields) {
    /* ---- Enhanced Pseudocode ---- */
    int ret;
    mixed m;
    string failed_reason;
    mapping info;
    mixed query_info_func;
    mixed check_info_func;

    query_info_func = query_info_func_map[table];
    check_info_func = check_info_func_map[table];
    if (((!is_function(query_info_func)) || (!is_function(check_info_func)))) {
        return "no_func";
    }
    ret = 0;
    do {
        if ((!start_account_transaction(cno, account))) {
            failed_reason = "start_transaction_failed";
            break;
        }
        try {
            if ((!is_mapping(info = (*query_info_func)(cno, account)))) {
                failed_reason = "query_info_failed";
                break;
            }
            if ((info["checksum"] != (*check_info_func)(info))) {
                failed_reason = "check_info_failed";
                break;
            }
            if ((!(*proc_func)(&m, info, fields))) {
                break;
            }
            ret = 1;
        } catch { /* error handled */ }
        goto L0088;

        // --- label L0088 ---
    } while (0);

    // --- label L0092 ---
    end_account_transaction(cno, ret);
    if (is_string(failed_reason)) {
        return failed_reason;
    }
    return m;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        15
   Code segments found:         15
   Successfully decoded:        15
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */