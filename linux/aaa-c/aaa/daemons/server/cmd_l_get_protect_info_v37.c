/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_get_protect_info.o
 * Source: /aaa/daemons/server/cmd_l_get_protect_info.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 3 names, 3 code segments found
 * Strings: 26, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _main(object agent, string account, int dist_type);
void main(object agent, string account, int dist_type);
private int get_dur_time(mixed bind_list);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=455, nargs=3, insts=455
// --------------------------------------------------------------------
private void _main(object agent, string account, int dist_type) {
    /* ---- Enhanced Pseudocode ---- */
    int dur_day;
    mixed month_zero_time;
    int dur_time;
    mixed cno;
    int protect_day;
    mixed branch;
    mixed bind_time;
    int protect;
    mixed sql_cmd;
    string protects;
    mixed protect_types;
    mapping m;
    mapping protect_info;
    mapping info;

    protects = "";
    info = ([ ]);
    if (((cno = "/daemons/dbd.c"->assure_connection("adb")) < 0)) {
        return;
    }
    branch = get_offset_ym(time(), -1);
    sql_cmd = mysql_bind_parameters("select * from protect_bonus where account = ? and branch = ? limit 1", account, branch);
    m = mysql_select_single_result(cno, sql_cmd);
    if ((is_mapping(m) && (int)m["bonus_time"])) {
        if ((((!dist_type) && is_same_month(time(m["bonus_time"]), time())) || (dist_type && is_same_week(time(m["bonus_time"]), time())))) {
            info["bonus_flag"] = 1;
        }
    }
    month_zero_time = get_month_zero_time(time());
    protect_types = ([ "matrix": "D", "phone": "B", "passpod": "A" ]);
    info["dur_bind_time"] = ([ ]);
    info["bind_time"] = ([ ]);
    if (dist_type) {
        if (((!info["bonus_flag"]) && (!is_same_month(get_monday_zero_time(time()), time())))) {
            branch = get_offset_ym(time(), -2);
            sql_cmd = mysql_bind_parameters("select * from protect_bonus where account = ? and branch = ? limit 1", account, branch);
            m = mysql_select_single_result(cno, sql_cmd);
            if (((is_mapping(m) && (int)m["bonus_time"]) && is_same_week(time(m["bonus_time"]), time()))) {
                info["bonus_flag"] = 1;
            }
        }
        loop (protect in keys(protect_types)) {
            protect_info = "/aaa/daemons/accountd.c"->query_protect_info(account, protect);
            if ((!is_mapping(protect_info))) {
                continue;
            }
            if (((protect == "matrix") && (!(int)protect_info["enable"]))) {
                continue;
            }
            protects += protect_types[protect];
            if (((!is_string(bind_time = protect_info["bind_time"])) || (!sizeof(bind_time)))) {
                bind_time = time_to_string();
                protect_info["bind_time"] = time_to_string();
                "/aaa/daemons/accountd.c"->update_protect_info(protect_info);
            }
            if (((dur_time = (get_monday_zero_time(time()) - time(bind_time))) < 0)) {
                dur_time = 0;
            }
            protect_day = (dur_time / 86400);
            info["dur_bind_time"][protect_types[protect]] = protect_day;
            info["bind_time"][protect_types[protect]] = time(bind_time);
        }  // end loop (verified)
        info["protects"] = protects;
        agent->send_message(4641, account, save_string(info));
        return;
    }
    loop (protect in keys(protect_types)) {
        protect_info = "/aaa/daemons/accountd.c"->query_protect_info(account, protect);
        if ((!is_mapping(m))) {
            dur_day = 0;
        } else {
            dur_day = get_dur_time(m[sprintf("%s_time", protect)]);
        }
        do {
            if ((!is_mapping(protect_info))) {
                protect_day = dur_day;
                break;
            }
            if (((protect == "matrix") && (!(int)protect_info["enable"]))) {
                protect_day = dur_day;
                break;
            }
            protects += protect_types[protect];
            if (((!is_string(bind_time = protect_info["bind_time"])) || (!sizeof(bind_time)))) {
                bind_time = time_to_string();
                protect_info["bind_time"] = time_to_string();
                "/aaa/daemons/accountd.c"->update_protect_info(protect_info);
            }
            info["bind_time"][protect_types[protect]] = time(bind_time);
            dur_time = (((month_zero_time - time(bind_time)) / 86400) + 1);
            if ((dur_time > 30)) {
                protect_day = 30;
                break;
            }
            protect_day = (dur_day + ((dur_time < 0) ? 0 : dur_time));
        } while (0);

        // --- label L0427 ---
        info["dur_bind_time"][protect_types[protect]] = protect_day;
    }  // end loop (verified)
    info["protects"] = protects;
    agent->send_message(4641, account, save_string(info));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=20, nargs=3, insts=20
// --------------------------------------------------------------------
void main(object agent, string account, int dist_type) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _main, agent, account, dist_type :));

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=60, nargs=1, insts=60
// --------------------------------------------------------------------
private int get_dur_time(mixed bind_list) {
    /* ---- Enhanced Pseudocode ---- */
    int dur_time;
    mixed dur_info;
    mixed start_time;
    mixed end_time;
    mixed time_str;

    dur_time = 0;
    if (((!is_string(bind_list)) || (!sizeof(bind_list)))) {
        return dur_time;
    }
    dur_info = explode(bind_list, ",");
    loop (time_str in dur_info) {
        time_str = trim(time_str);
        if ((sscanf(time_str, "%s-%s", &start_time, &end_time) < 2)) {
            continue;
        }
        dur_time += (time(end_time) - time(start_time));
    }  // end loop (verified)
    return ((dur_time / 86400) + 1);

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