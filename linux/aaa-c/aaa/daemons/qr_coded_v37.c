/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: qr_coded.o
 * Source: /aaa/daemons/qr_coded.c
 * Inherits: (none)
 * Functions: 11 names, 11 code segments found
 * Strings: 49, Variables: 5
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   int interval_scan;
   int per_rows_scan;
   mapping qr_agent_list;
   int qr_enable;
   int timeout_qr_code;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void wakeup_qr_code_agent(string qr_id, string account);
private void _get_qr_code_info(mixed agent);
int get_qr_code_status(mixed url = 0);
int check_qr_code_login(mixed agent, string qr_id, string account);
private void destruct();
int test_write_qr_runtime(string qr_id, string account);
private int handle_result(mixed http_ret, string &qr_id, string &qr_code);
private void get_qr_code_info(mixed agent);
private void create();
private void clean_qr_agent(string qr_id);
private void scan_daemon();

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   qr_agent_list = ([ ]);  // /aaa/daemons/qr_coded.c:18
   timeout_qr_code = 60;
   interval_scan = 5;
   per_rows_scan = 2000;
   qr_enable = 1;

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=87, nargs=2, insts=87
// --------------------------------------------------------------------
private void wakeup_qr_code_agent(string qr_id, string account) {
    /* ---- Enhanced Pseudocode ---- */
    mixed agent;
    mapping qr_code_info;

    if ((1 != is_object(agent = qr_agent_list[qr_id]))) {
        return;
    }
    map_delete(qr_agent_list, qr_id);
    if ((!get_qr_code_status())) {
        return;
    }
    if ((agent->lite_query_temp("ok") == 1)) {
        return;
    }
    if ((((!is_mapping(qr_code_info = agent->lite_query_temp("qr_code"))) || (qr_id != qr_code_info["id"])) || qr_code_info["login"])) {
        return;
    }
    if (((time() - (int)qr_code_info["ti"]) > timeout_qr_code)) {
        return;
    }
    agent->delete_temp("destruct_callback/clean_qr_code");
    qr_code_info["account"] = account;
    qr_code_info["login"] = 1;
    agent->send_message(4991, qr_id, account);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=129, nargs=1, insts=129
// --------------------------------------------------------------------
private void _get_qr_code_info(mixed agent) {
    /* ---- Enhanced Pseudocode ---- */
    mixed http_ret;
    mixed url;
    string qr_id;
    string qr_code;
    mixed qr_code_info;

    qr_id = "";
    qr_code = "";
    if ((1 != is_object(agent))) {
        return;
    }
    if ((!get_qr_code_status(&url))) {
        agent->send_message(8165, "该功能已关闭！", 1);
        return;
    }
    if ((agent->lite_query_temp("ok") == 1)) {
        return;
    }
    if ((!agent->lite_query_temp("antibot_answer"))) {
        return;
    }
    if (is_mapping(agent->lite_query_temp("qr_code"))) {
        return;
    }
    url = sprintf("%s?type=GL", url);
    http_ret = "/daemons/http_clientd.c"->http_get_web(url);
    if ((1 != is_object(agent))) {
        return;
    }
    if ((!handle_result(http_ret, &qr_id, &qr_code))) {
        agent->send_message(8165, "获取二维码信息失败！", 1);
        return;
    }
    qr_code_info = ([ "id": qr_id, "code": qr_code, "ti": time() ]);
    agent->lite_set_temp("qr_code", qr_code_info);
    agent->set_temp("destruct_callback/clean_qr_code", (: clean_qr_agent, qr_id :));
    qr_agent_list[qr_id] = agent;
    agent->send_message(4989, qr_id, qr_code, timeout_qr_code);
    agent->to_be_disconnect(timeout_qr_code);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=29, nargs=1, insts=29
// --------------------------------------------------------------------
int get_qr_code_status(mixed url) {
    /* ---- Enhanced Pseudocode ---- */
    if ((((!qr_enable) || (!is_string(url = "/aaa/daemons/configd.c"->get_global_config("qr_code_url")))) || (strlen(url) < 1))) {
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  validator: verify data
// @code  nc=72, nargs=3, insts=72
// --------------------------------------------------------------------
int check_qr_code_login(mixed agent, string qr_id, string account) {
    /* ---- Enhanced Pseudocode ---- */
    mapping qr_code_info;

    if ((1 != is_object(agent))) {
        return 0;
    }
    if ((!get_qr_code_status())) {
        return 0;
    }
    if ((!is_mapping(qr_code_info = agent->lite_query_temp("qr_code")))) {
        return 0;
    }
    if ((stricmp(account, qr_code_info["account"]) != 0)) {
        return 0;
    }
    if ((qr_id != qr_code_info["id"])) {
        return 0;
    }
    if (((time() - (int)qr_code_info["ti"]) > timeout_qr_code)) {
        return 0;
    }
    if ((qr_code_info["login"] != 1)) {
        return 0;
    }
    qr_code_info["login"] = 2;
    return 1;

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=10, nargs=0, insts=10
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    if (get_thread_info("tScanQrRuntime")) {
        delete_thread("tScanQrRuntime");
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=31, nargs=2, insts=31
// --------------------------------------------------------------------
int test_write_qr_runtime(string qr_id, string account) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;

    return 0;
    sql_cmd = mysql_bind_parameters("insert into qr_runtime (qrid, account, update_time) values (?, ?, date_format(CURRENT_TIMESTAMP, '%Y%m%d%H%i%s'))", qr_id, account);
    return ((*f_execute)(cno, sql_cmd) == 1);

}

// --------------------------------------------------------------------
// @brief  event handler: respond to event
// @code  nc=92, nargs=3, insts=92
// --------------------------------------------------------------------
private int handle_result(mixed http_ret, string &qr_id, string &qr_code) {
    /* ---- Enhanced Pseudocode ---- */
    mixed content;
    mapping m;

    if (((!is_string(http_ret)) || (strlen(http_ret) < 1))) {
        return 0;
    }
    content = http_ret;
    content = replace_string(content, "{", "([");
    content = replace_string(content, "}", "])");
    try {
        m = restore_value(content);
    } catch { /* error handled */ }

    // --- label L0042 ---
    if ((0 || (!is_mapping(m)))) {
        return 0;
    }
    if (((m["Status"] != "success") || (!is_mapping(m["Data"])))) {
        return 0;
    }
    if (((!is_string(qr_id = m["Data"]["Id"])) || (!is_string(qr_code = m["Data"]["Content"])))) {
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=22, nargs=1, insts=22
// --------------------------------------------------------------------
private void get_qr_code_info(mixed agent) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!get_qr_code_status())) {
        agent->send_message(8165, "该功能已关闭！", 1);
        return;
    }
    "/daemons/threadd.c"->dispatch_service((: _get_qr_code_info, agent :));

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=14, nargs=0, insts=14
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/connectiond.c"->register_auth("cmd_l_get_qr_code_info", (: get_qr_code_info :));
    create_thread("tScanQrRuntime", "scan_daemon");

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
private void clean_qr_agent(string qr_id) {
    /* ---- Enhanced Pseudocode ---- */
    map_delete(qr_agent_list, qr_id);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=140, nargs=0, insts=140
// --------------------------------------------------------------------
private void scan_daemon() {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    int i;
    mixed size;
    mixed arr;
    mixed arr_ids;
    mixed scan_cmd;
    mixed del_cmd;
    mapping m;

    set_as_daemon();
    scan_cmd = mysql_bind_parameters("select * from qr_runtime order by update_time limit ?", per_rows_scan);

    // --- label L0010 ---
    while (1) {
        do {
            if ((!qr_enable)) {
                break;
            }
            if ((sizeof(qr_agent_list) < 1)) {
                break;
            }
            cno = "/daemons/dbd.c"->assure_connection("adb");
            if ((cno == -1)) {
                break;
            }
            arr = mysql_select(cno, scan_cmd);
            if ((!is_array(arr))) {
                break;
            }
            if (((size = sizeof(arr)) < 1)) {
                break;
            }
            arr_ids = allocate_array(size);
            for (i = 0; i <= (size - 1); i++) {
                if ((!is_mapping(m = arr[i]))) {
                    continue;
                }
                try {
                    wakeup_qr_code_agent(m["qrid"], m["account"]);
                } catch { /* error handled */ }

                // --- label L0093 ---
                arr_ids[i] = m["qrid"];
            }  // end loop (verified)
            clean_array(arr_ids, 4);
            del_cmd = sprintf("delete from qr_runtime where qrid in (%s)", implode(allocate_array(sizeof(arr_ids), "?"), ","));
            del_cmd = mysql_bind_parameters_arr(del_cmd, arr_ids);
            (*f_execute)(cno, del_cmd);
        } while (0);

        // --- label L0133 ---
        sleep(interval_scan);
    }  // end loop (verified)

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        11
   Code segments found:         11
   Successfully decoded:        11
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */