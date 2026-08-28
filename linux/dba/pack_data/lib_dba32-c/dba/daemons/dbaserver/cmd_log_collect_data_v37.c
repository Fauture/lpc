/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_log_collect_data.o
 * Source: /dba/daemons/dbaserver/cmd_log_collect_data.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 65, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
int main(object agent, string account, string gid, string para, string memo);

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
// @brief  interaction: calls other objects
// @code  nc=378, nargs=5, insts=378
// --------------------------------------------------------------------
int main(object agent, string account, string gid, string para, string memo) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;
    mapping m;

    if ((!is_object(agent))) {
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ldb");
    if ((cno == -1)) {
        return 0;
    }
    if ((!is_mapping(m = restore_value(para)))) {
        return 0;
    }
    switch (m["type"]) {
        case 1:
        sql_cmd = mysql_bind_parameters("insert into collect_data_log (account, gid, cpu_id, cpu_info, disk_sequence, mac_address, memory_size, video_card, video_memory, bios, privilege, para1, para2, memo, update_time) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", account, gid, m["cpu_id"], m["cpu_info"], m["disk_sequence"], m["mac_address"], (int)m["memory_size"], m["video_card"], (int)m["video_memory"], m["bios"], (int)m["privilege"], m["os"], m["client_version"], memo, ctime(time(), 3));
        break;
        case 2:
        case 5:
        case 9:
        case 10:
        case 11:
        case 12:
        sql_cmd = mysql_bind_parameters("insert into client_performance_log (record_interval, account, gid, mac, ip, insider, lv, type, cpu_cost, mem_cost, render_frame_rate, cur_scene, cur_room_user, cur_room_name, cur_process_num, minimize_window, button_type, button_name, timeslice, button_times, directx_version, prever_download, para1, para2, para3, para4, para5, memo, update_time) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", m["record_interval"], m["account"], m["gid"], m["mac"], m["ip"], m["insider"], m["lv"], m["type"], m["cpu_cost"], m["mem_cost"], m["render_frame_rate"], m["cur_scene"], m["cur_room_user"], m["cur_room_name"], m["cur_process_num"], m["minimize_window"], m["button_type"], m["button_name"], m["timeslice"], m["button_times"], m["directx_version"], m["prever_download"], m["para1"], m["para2"], m["para3"], m["para4"], m["para5"], memo, ctime(time(), 3));
        break;
        case 3:
        sql_cmd = mysql_bind_parameters("insert into client_action_log (account, gid, mac, ip, insider, lv, tao, gold_coin, in_party, send_num, send_gids, receive_num, receive_gids, current_channel, troop_channel, party_channel, team_channel, world_channel, whoop_channel, para1, para2, para3, memo, update_time) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", m["account"], m["gid"], m["mac"], m["ip"], m["insider"], m["lv"], m["tao"], m["gold_coin"], m["in_party"], m["send_num"], m["send_gids"], m["receive_num"], m["receive_gids"], m["current_channel"], m["troop_channel"], m["party_channel"], m["team_channel"], m["world_channel"], m["whoop_channel"], m["para1"], m["para2"], m["para3"], m["memo"], ctime(time(), 3));
        break;
        case 4:
        sql_cmd = mysql_bind_parameters("insert into combat_log (account, gid, name, mac, ip, insider, lv, tao, gold_coin, in_party, all_round, abnormal_round, abnormal_degree, main_attribute, ratio_attribute, para1, para2, para3, para4, para5, memo, update_time) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", m["account"], m["gid"], m["name"], m["mac"], m["ip"], m["insider"], m["lv"], m["tao"], m["gold_coin"], m["in_party"], m["all_round"], m["abnormal_round"], m["abnormal_degree"], m["main_attribute"], m["ratio_attribute"], m["para1"], m["para2"], m["para3"], m["para4"], m["para5"], m["memo"], ctime(time(), 3));
        default:
        return 0;
    }  // end switch

    // --- label L0374 ---
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