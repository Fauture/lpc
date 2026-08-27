/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_manage_mp_device.o
 * Source: /aaa/daemons/server/cmd_l_manage_mp_device.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 14, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _main(mixed agent, string gid, string account, int oper, string device, int os);
int main(mixed agent, string gid, string account, int oper, string device, int os);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=119, nargs=6, insts=119
// --------------------------------------------------------------------
private void _main(mixed agent, string gid, string account, int oper, string device, int os) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    int ret;
    mixed sql_cmd;
    mixed mp_device;

    if ((!is_object(agent))) {
        return;
    }
    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((!is_object(agent))) {
        return;
    }
    if ((cno < 0)) {
        agent->send_message(25479, gid, oper, -1, "");
        return;
    }
    switch (oper) {
        case 1:
        mp_device = ([ "account": account, "device": device, "os": os, "bind_time": ctime(time(), 3) ]);
        sql_cmd = convert_mapping_to_sql_cmd("replace", "mp_device", mp_device);
        break;
        case 2:
        mp_device = ([ "account": account, "device": device ]);
        sql_cmd = convert_mapping_to_sql_cmd("delete", "mp_device", mp_device);
        break;
        default:
        return;
    }  // end switch

    // --- label L0080 ---
    ret = (*f_execute)(cno, sql_cmd);
    if ((!is_object(agent))) {
        return;
    }
    if ((ret < 1)) {
        agent->send_message(25479, gid, oper, -1, "");
        return;
    }
    agent->send_message(25479, gid, oper, 0, save_string(mp_device));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=25, nargs=6, insts=25
// --------------------------------------------------------------------
int main(mixed agent, string gid, string account, int oper, string device, int os) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _main, agent, gid, account, oper, device, os :));
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