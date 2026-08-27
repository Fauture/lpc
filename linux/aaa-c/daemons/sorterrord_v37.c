/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: sorterrord.o
 * Source: /daemons/sorterrord.c
 * Inherits: (none)
 * Functions: 24 names, 24 code segments found
 * Strings: 92, Variables: 4
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   mapping all_errors;
   int errorQ;
   int g_log_mode;
   int tid;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void save_error_info(string new_file_name, mapping one_error, array org_file);
private mixed analyse_stack(mixed arr_at);
private mixed filter_files(array files, int log_mode);
private int is_same_error(string file_name, mapping one_err);
private mixed is_stack(string one_row);
private mixed is_variable(string one_row);
private mixed filter_row(string one_row);
private void process_msg_ex(mixed msg);
private mixed analyse_error_info(array org_file);
private void destruct();
private void do_init();
private string get_new_file_name(string file_name);
int stop_log_schedule();
int start_log_schedule();
private int is_contain_str_sizeof(string one_row);
private void create();
private string get_error_date(string file_name);
private void do_log_error(mapping m);
private void process_msg(mixed msg);
private void daemon();
private mixed read_log(int fno);
private mixed is_argument(string one_row);
void sort_error(string file_path);
private mixed get_files(string file_path, string suffix = ".txt");

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   all_errors = ([ ]);  // /daemons/sorterrord.c:1089
   g_log_mode = 0;

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=151, nargs=3, insts=151
// --------------------------------------------------------------------
private void save_error_info(string new_file_name, mapping one_error, array org_file) {
    /* ---- Enhanced Pseudocode ---- */
    int func_num;
    mixed temp;
    mixed str_error;
    mixed str_object;
    mixed str_stack;

    func_num = 0;
    str_error = (("\n出错原因  :" + one_error["err_reason"]) + "\n");
    str_object = (("出错对象  :" + one_error["err_object"]) + "\n");
    str_stack = replace_string(one_error["stack"], "()", "() -> ");
    log_file(new_file_name, (str_error + str_object));
    log_file(new_file_name, (("\n调用栈的顺序:\n" + str_stack) + "\n调用栈:\n"));
    loop (temp in one_error["err_info"]) {
        if ((sizeof(temp) != 5)) {
            continue;
        }
        ++func_num;
        if ((temp[2] != "NO_LINE")) {
            log_file(new_file_name, sprintf("\n%d. %s in %s at line#%d\n", func_num, temp[0], temp[1], temp[2]));
        } else {
            log_file(new_file_name, sprintf("\n%d. %s in %s\n", func_num, temp[0], temp[1]));
        }
        log_file(new_file_name, "参数：\n");
        log_file(new_file_name, implode(temp[3], "\n"));
        log_file(new_file_name, "\n");
        log_file(new_file_name, "变量：\n");
        log_file(new_file_name, implode(temp[4], "\n"));
        log_file(new_file_name, "\n");
    }  // end loop (verified)
    log_file(new_file_name, "\n-------------------------------------------------------\n");
    log_file(new_file_name, "\n\n################第 1 次出错################\n\n");
    log_file(new_file_name, implode(org_file, "\n"));

}

// --------------------------------------------------------------------
// @code  nc=161, nargs=1, insts=161
// --------------------------------------------------------------------
private mixed analyse_stack(mixed arr_at) {
    /* ---- Enhanced Pseudocode ---- */
    mixed line_num;
    int i;
    mixed skip_num;
    int flag;
    mixed arr_arg;
    mixed arr_var;

    arr_arg = ({});
    arr_var = ({});
    flag = 0;
    i = 0;
    if (((line_num = sizeof(arr_at)) < 0)) {
        return 0;
    }
    while (i < line_num) {
        if ((skip_num = is_contain_str_sizeof(arr_at[i]))) {
            if (is_variable(arr_at[i])) {
                flag = 0;
            }
            if (is_argument(arr_at[i])) {
                flag = 1;
            }
            if (flag) {
                arr_arg += arr_at[i..(skip_num + i)];
            } else {
                arr_var += arr_at[i..(skip_num + i)];
            }
            i += (skip_num + 1);
            continue;
        }
        if (is_argument(arr_at[i])) {
            arr_arg += ({ arr_at[i] });
            flag = 1;
            ++i;
            continue;
        }
        if (is_variable(arr_at[i])) {
            arr_var += ({ arr_at[i] });
            ++i;
            flag = 0;
            continue;
        }
        if (flag) {
            arr_arg += ({ arr_at[i] });
        } else {
            arr_var += ({ arr_at[i] });
        }
        ++i;
    }  // end loop (verified)
    return ({ arr_arg, arr_var });

}

// --------------------------------------------------------------------
// @code  nc=142, nargs=2, insts=142
// --------------------------------------------------------------------
private mixed filter_files(array files, int log_mode) {
    /* ---- Enhanced Pseudocode ---- */
    mixed t;
    mixed filtered_files;
    mixed start_date;
    mixed end_date;
    mixed log_date;
    mixed file_name;
    mixed m;

    switch (log_mode) {
        case 1:
        t = (time() - 86400);
        start_date = ctime(t, 2);
        end_date = start_date;
        break;
        case 2:
        t = (time() - 604800);
        start_date = ctime(t, 2);
        t = (time() - 86400);
        end_date = ctime(t, 2);
        break;
        default:
        return files;
    }  // end switch

    // --- label L0053 ---
    filtered_files = ({});
    loop (file_name in files) {
        if ((strsrch(file_name, (("/aaa/log/log_" + _server_name) + "_")) != 0)) {
            continue;
        }
        if ((strsrch(file_name, ".txt") != (sizeof(file_name) - sizeof(".txt")))) {
            continue;
        }
        log_date = get_error_date(file_name);
        if (((strcmp(log_date, start_date) < 0) || (strcmp(log_date, end_date) > 0))) {
            continue;
        }
        if ((file_size(file_name) >= (is_test() ? 5242880 : 2097152))) {
            m = ([ "action": log_date, "para1": "BIG NUMBER", "para2": "too large size of log file", "para3": "", "memo": "" ]);
            do_log_error(m);
            continue;
        }
        filtered_files += ({ file_name });
    }  // end loop (verified)
    return filtered_files;

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=112, nargs=2, insts=112
// --------------------------------------------------------------------
private int is_same_error(string file_name, mapping one_err) {
    /* ---- Enhanced Pseudocode ---- */
    mixed num;
    mixed mk;
    mixed stack;

    do {
        if ((!is_mapping(all_errors[file_name]))) {
            all_errors[file_name] = ([ ]);
            break;
        }
        loop (mk in all_errors[file_name]) {
            if ((!is_string(stack = mk[0]))) {
                continue;
            }
            if (strcmp(one_err["stack"], stack)) {
                continue;
            }
            ++all_errors[file_name][one_err["stack"]][2];
            num = all_errors[file_name][one_err["stack"]][2];
            all_errors[file_name][one_err["stack"]][3] += sprintf("%d. %s\n", num, one_err["occured_time"]);
            return num;
        }  // end loop (verified)
    } while (0);

    // --- label L0086 ---
    all_errors[file_name][one_err["stack"]] = ({ one_err["err_reason"], one_err["err_object"], 1, sprintf("发生的时间：\n1. %s\n", one_err["occured_time"]) });
    return 0;

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=51, nargs=1, insts=51
// --------------------------------------------------------------------
private mixed is_stack(string one_row) {
    /* ---- Enhanced Pseudocode ---- */
    mixed err_line;
    mixed err_file;
    mixed err_func;

    if ((strsrch(one_row, "Call from") != -1)) {
        return ({});
    }
    if ((sscanf(one_row, "At EFUN: %s in %s", &err_func, &err_file) == 2)) {
        err_func += "()";
        return ({ err_func, err_file, "NO_LINE" });
    }
    if ((sscanf(one_row, "At %s:%d(%s)%*s", &err_file, &err_line, &err_func) == 4)) {
        err_func += "()";
        return ({ err_func, err_file, err_line });
    }
    return 0;

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=15, nargs=1, insts=15
// --------------------------------------------------------------------
private mixed is_variable(string one_row) {
    /* ---- Enhanced Pseudocode ---- */
    mixed str;

    if ((sscanf(one_row, "Variable %s", &str) != 1)) {
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=25, nargs=1, insts=25
// --------------------------------------------------------------------
private mixed filter_row(string one_row) {
    /* ---- Enhanced Pseudocode ---- */
    mixed minute;

    if ((sscanf(one_row, "%*s:%d:%*s", &minute) != 3)) {
        return one_row;
    }
    if ((sizeof(one_row) == 25)) {
        return 0;
    }
    return one_row[25..<1];

}

// --------------------------------------------------------------------
// @code  nc=146, nargs=1, insts=146
// --------------------------------------------------------------------
private void process_msg_ex(mixed msg) {
    /* ---- Enhanced Pseudocode ---- */
    mixed org_file;
    mixed mk;
    mixed date;
    int stack;
    int file_name;
    mixed m;
    mapping one_error_info;

    if ((!is_mapping(msg))) {
        return;
    }
    file_name = msg["file"];
    date = get_error_date(file_name);
    if ((msg["error_info"] == "QUIT_FILE")) {
        if ((!is_mapping(all_errors[file_name]))) {
            map_delete(all_errors, file_name);
            return;
        }
        loop (mk in all_errors[file_name]) {
            stack = mk[0];
            m = ([ "action": date, "para1": (string)all_errors[file_name][stack][2], "para2": all_errors[file_name][stack][0], "para3": all_errors[file_name][stack][1], "memo": save_string(([ "err_stack": replace_string(stack, "()", "() -> "), "err_detail": all_errors[file_name][stack][4] ])) ]);
            do_log_error(m);
        }  // end loop (verified)
        map_delete(all_errors, file_name);
        return;
    }
    org_file = msg["error_info"];
    one_error_info = analyse_error_info(org_file);
    if ((!one_error_info)) {
        printf("error: Sort one error faild!\n");
        return;
    }
    if ((!is_same_error(file_name, one_error_info))) {
        all_errors[file_name][one_error_info["stack"]] += ({ implode(org_file, "\n") });
    }

}

// --------------------------------------------------------------------
// @code  nc=218, nargs=1, insts=218
// --------------------------------------------------------------------
private mixed analyse_error_info(array org_file) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    mixed all_rows;
    mixed one_row;
    mixed back_analyse;
    mixed one_stack_info;
    mixed error_info;
    mixed prep_stack_info;
    mixed arr_arg_var;
    string str_error;
    string str_object;
    string str_stack;
    string str_time;
    mapping one_error;

    i = 0;
    str_time = "";
    str_error = "";
    str_object = "";
    str_stack = "";
    one_error = ([ ]);
    error_info = ({});
    arr_arg_var = ({});
    prep_stack_info = ({});

    // --- label L0038 ---
    while ((sscanf(org_file[i], "%s Error: %s", &str_time, &str_error) != 2)) {
        if ((strsrch(org_file[i], "Call from") != -1)) {
            return 0;
        }
        ++i;
    }  // end loop (verified)
    ++i;

    // --- label L0070 ---
    while ((sscanf(org_file[i], "%*sObject: %s", &str_object) != 2)) {
        if ((strsrch(org_file[i], "Current thread:") == -1)) {
            ++i;
        } else {
            break;
        }
    }  // end loop (verified)
    ++i;
    all_rows = sizeof(org_file);

    // --- label L0107 ---
    while (i < all_rows) {
        if ((!one_row = filter_row(org_file[i]))) {
            ++i;
            continue;
        }
        if ((one_stack_info = is_stack(one_row))) {
            if (sizeof(one_stack_info)) {
                str_stack = (one_stack_info[0] + str_stack);
            }
            if ((back_analyse = analyse_stack(arr_arg_var))) {
                prep_stack_info += back_analyse;
                error_info += ({ prep_stack_info });
            }
            prep_stack_info = one_stack_info;
            arr_arg_var = ({});
            ++i;
            continue;
        }
        arr_arg_var += ({ one_row });
        ++i;
    }  // end loop (verified)
    one_error["occured_time"] = str_time;
    one_error["err_reason"] = str_error;
    one_error["err_object"] = str_object;
    one_error["stack"] = str_stack;
    one_error["err_info"] = error_info;
    return one_error;

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=25, nargs=0, insts=25
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    send_to_queue(errorQ, "QUIT_MESSAGE");

    // --- label L0006 ---
    while ((get_thread_id("tanalyseerror") != -1)) {
        sleep(0.1);
    }  // end loop (verified)
    delete_queue(errorQ);
    "/daemons/scheduled.c"->delete_scheme("统计服务器报错信息");

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=136, nargs=0, insts=136
// --------------------------------------------------------------------
private void do_init() {
    /* ---- Enhanced Pseudocode ---- */
    int log_mode;
    int flag;
    mixed log_servers;
    mixed text;
    mixed server;
    mapping config_m;

    if ((!is_string(text = "/aaa/daemons/configd.c"->read_etc("global", "log_error")))) {
        return;
    }
    try {
        config_m = restore_value(text);
    } catch { /* error handled */ }

    // --- label L0023 ---
    if (((!is_mapping(config_m)) || (!sizeof(config_m)))) {
        return;
    }
    do {
        if (("*" == config_m["log_servers"])) {
            break;
        }
        if ((!is_array(log_servers = config_m["log_servers"]))) {
            return;
        }
        flag = 0;
        loop (server in log_servers) {
            if ((((((stricmp(server, _server_name) == 0) || (0 && ("GS*" == toupper(server)))) || (0 && ("GS*" == toupper(server)))) || (0 && ("CCS*" == toupper(server)))) || (0 && ("SPA*" == toupper(server))))) {
                flag = 1;
                break;
            }
        }  // end loop (verified)
        if ((!flag)) {
            return;
        }
    } while (0);

    // --- label L0116 ---
    if ((!is_int(log_mode = config_m["log_mode"]))) {
        log_mode = 1;
    }
    g_log_mode = log_mode;
    start_log_schedule();

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=82, nargs=1, insts=82
// --------------------------------------------------------------------
private string get_new_file_name(string file_name) {
    /* ---- Enhanced Pseudocode ---- */
    int file_num;
    mixed new_file_name;
    mixed temp_file_name1;
    mixed temp_file_name2;
    mapping lt;

    lt = local_time();
    file_num = 0;
    sscanf(file_name, "%s.txt", &new_file_name);
    new_file_name += "/";
    assure_file(new_file_name);
    temp_file_name1 = (("sort_err_" + sprintf("%d_%d_%d_%d_%d_%d", lt["year"], lt["mon"], lt["mday"], lt["hour"], lt["min"], lt["sec"])) + ".txt");
    temp_file_name2 = temp_file_name1;

    // --- label L0051 ---
    while ((file_size((new_file_name + temp_file_name2)) != -1)) {
        ++file_num;
        temp_file_name2 = replace_string(temp_file_name1, ".txt", (sprintf("_%d", file_num) + ".txt"));
    }  // end loop (verified)
    return (new_file_name + temp_file_name2);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=13, nargs=0, insts=13
// --------------------------------------------------------------------
int stop_log_schedule() {
    /* ---- Enhanced Pseudocode ---- */
    g_log_mode = 0;
    return "/daemons/scheduled.c"->delete_scheme("统计服务器报错信息");

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=70, nargs=0, insts=70
// --------------------------------------------------------------------
int start_log_schedule() {
    /* ---- Enhanced Pseudocode ---- */
    mixed start_time;
    string time_setting;

    if ((!g_log_mode)) {
        g_log_mode = 1;
    }
    switch (g_log_mode) {
        case 1:
        start_time = ((_today_zero_time + 86400) + 21600);
        time_setting = "per_day";
        break;
        case 2:
        start_time = ((get_monday_zero_time(time()) + 604800) + 21600);
        time_setting = "per_week";
        break;
        default:
        start_time = ((_today_zero_time + 86400) + 21600);
        time_setting = "per_day";
        break;
    }  // end switch

    // --- label L0056 ---
    "/daemons/scheduled.c"->create_scheme("统计服务器报错信息", (: sort_error, "/aaa/log/" :), start_time, time_setting, 1);
    return 1;

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=17, nargs=1, insts=17
// --------------------------------------------------------------------
private int is_contain_str_sizeof(string one_row) {
    /* ---- Enhanced Pseudocode ---- */
    mixed num;

    if ((sscanf(one_row, "%*s/* sizeof() == %d */", &num) != 2)) {
        return 0;
    }
    return (num + 1);

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=20, nargs=0, insts=20
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    require_uid();
    set_euid(0);
    tid = create_thread("tanalyseerror", "daemon");
    errorQ = create_queue("qanalyseerror");

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=52, nargs=1, insts=52
// --------------------------------------------------------------------
private string get_error_date(string file_name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed year;
    mixed month;
    mixed day;
    mixed date;

    date = file_name[sizeof((("/aaa/log/log_" + _server_name) + "_"))..(sizeof((("/aaa/log/log_" + _server_name) + "_")) + 9)];
    if (((sizeof(date) != 10) || (sscanf(date, "%d-%d-%d", &year, &month, &day) != 3))) {
        return "";
    }
    if ((sprintf("%04d-%02d-%02d", year, month, day) != date)) {
        return "";
    }
    return date;

}

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=30, nargs=1, insts=30
// --------------------------------------------------------------------
private void do_log_error(mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!sizeof(m))) {
        return;
    }
    "/daemons/filed.c"->log_debug(_server_name, "SORT_ERROR_D", m["action"], m["para1"], m["para2"], m["para3"], m["memo"]);

}

// --------------------------------------------------------------------
// @code  nc=179, nargs=1, insts=179
// --------------------------------------------------------------------
private void process_msg(mixed msg) {
    /* ---- Enhanced Pseudocode ---- */
    mixed fno;
    mixed occured_times;
    mixed org_file;
    mixed mk;
    int stack;
    int file_name;
    mixed new_file;
    string str_content;
    mixed str;
    mapping one_error_info;

    if ((!is_mapping(msg))) {
        return;
    }
    file_name = msg["file"];
    if ((msg["error_info"] == "QUIT_FILE")) {
        loop (mk in all_errors[file_name]) {
            stack = mk[0];
            str_content = "--------------------错误日志分析-----------------------\n";
            str_content += (sprintf("发生的次数: %d 次\n", all_errors[file_name][stack][2]) + all_errors[file_name][stack][3]);
            fno = fopen(all_errors[file_name][stack][4], "r");

            // --- label L0068 ---
            while ((str = read_log(fno))) {
                str_content += str;
            }  // end loop (verified)
            fclose(fno);
            fno = fopen(all_errors[file_name][stack][4], "r+");
            fwrite(fno, str_content);
            fclose(fno);
        }  // end loop (verified)
        return;
    }
    org_file = msg["error_info"];
    one_error_info = analyse_error_info(org_file);
    if ((!one_error_info)) {
        printf("error: Sort one error faild!\n");
        return;
    }
    if ((!occured_times = is_same_error(file_name, one_error_info))) {
        new_file = get_new_file_name(file_name);
        save_error_info(new_file, one_error_info, org_file);
        all_errors[file_name][one_error_info["stack"]] += ({ new_file });
        return;
    }
    new_file = all_errors[file_name][one_error_info["stack"]][4];
    log_file(new_file, sprintf("\n\n################第 %d 次出错################\n\n", occured_times));
    log_file(new_file, implode(org_file, "\n"));

}

// --------------------------------------------------------------------
// @code  nc=43, nargs=0, insts=43
// --------------------------------------------------------------------
private void daemon() {
    /* ---- Enhanced Pseudocode ---- */
    mixed msg;

    set_as_daemon();

    // --- label L0004 ---
    while (1) {
        if ((receive_from_queue(errorQ, &msg) == 0)) {
            if ((msg == "QUIT_MESSAGE")) {
                return;
            }
            if (g_log_mode) {
                try {
                    process_msg_ex(msg);
                } catch { /* error handled */ }

                // --- label L0027 ---
            } else {
                try {
                    process_msg(msg);
                } catch { /* error handled */ }

                // --- label L0037 ---
            }
        }
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @code  nc=18, nargs=1, insts=18
// --------------------------------------------------------------------
private mixed read_log(int fno) {
    /* ---- Enhanced Pseudocode ---- */
    mixed buf;

    if ((sizeof((buf = fread(fno, 262144))) > 0)) {
        return (string)buf;
    }
    return 0;

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=15, nargs=1, insts=15
// --------------------------------------------------------------------
private mixed is_argument(string one_row) {
    /* ---- Enhanced Pseudocode ---- */
    mixed str;

    if ((sscanf(one_row, "Argument %s", &str) != 1)) {
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=203, nargs=1, insts=203
// --------------------------------------------------------------------
void sort_error(string file_path) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    mixed fno;
    int start_row;
    int end_row;
    mixed files;
    mixed org_file;
    mixed arr_content;
    mixed one_row;
    int file_name;
    mixed str_content;
    mapping m;

    files = get_files(file_path);
    files = filter_files(files, g_log_mode);
    if ((!sizeof(files))) {
        printf("error: Not find the file: %s\n", file_path);
        if (g_log_mode) {
            do_log_error(([ ]));
        }
        return;
    }
    all_errors = ([ ]);
    loop (file_name in files) {
        fno = fopen(file_name, "r");
        all_errors[file_name] = ([ ]);
        org_file = ({});

        // --- label L0053 ---
        while ((str_content = read_log(fno))) {
            if (sizeof(org_file)) {
                str_content = (implode(org_file, "\n") + str_content);
            }
            arr_content = explode(str_content, "\n");
            i = 0;
            start_row = i;
            end_row = i;
            loop (one_row in arr_content) {
                if ((strsrch(one_row, "Call from") == -1)) {
                    ++i;
                    continue;
                }
                end_row = i;
                org_file = arr_content[start_row..end_row];
                ++i;
                start_row = i;
                m = ([ ]);
                m["file"] = file_name;
                m["error_info"] = org_file;
                send_to_queue(errorQ, m);
                org_file = ({});
                sleep(0.1);
            }  // end loop (verified)
            if (((i - start_row) > 1)) {
                org_file = arr_content[start_row..(i - 1)];
            } else {
                org_file = ({});
            }
            start_row = 0;
            end_row = 0;
        }  // end loop (verified)
        m = ([ "file": file_name, "error_info": "QUIT_FILE" ]);
        send_to_queue(errorQ, m);
        fclose(fno);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=88, nargs=2, insts=88
// --------------------------------------------------------------------
private mixed get_files(string file_path, string suffix) {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr_file;
    mixed arr_txt_file;
    mixed file_name;

    arr_txt_file = ({});
    if ((!sizeof(file_path))) {
        return 0;
    }
    if ((strsrch(file_path, suffix) != -1)) {
        if ((strsrch(file_path, "/") < 0)) {
            file_path = ("gs/log/" + file_path);
        }
        if ((file_size(file_path) == -1)) {
            return 0;
        }
        return ({ file_path });
    }
    if ((strsrch(file_path, "/") == -1)) {
        file_path += "/";
    }
    if ((!arr_file = get_dir((file_path + "*")))) {
        return 0;
    }
    loop (file_name in arr_file) {
        if ((strsrch(file_name, suffix) == -1)) {
            continue;
        }
        arr_txt_file += ({ (file_path + file_name) });
    }  // end loop (verified)
    return arr_txt_file;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        24
   Code segments found:         24
   Successfully decoded:        24
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */