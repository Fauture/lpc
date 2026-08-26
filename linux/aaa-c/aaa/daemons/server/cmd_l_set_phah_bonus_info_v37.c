/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_set_phah_bonus_info.o
 * Source: /aaa/daemons/server/cmd_l_set_phah_bonus_info.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 3 names, 3 code segments found
 * Strings: 11, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _main(mixed agent, string account, string gid);
void main(object agent, string account, string gid);
private mapping _do_set_phah_bonus_info(string account, string gid);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=28, nargs=3, insts=28
// --------------------------------------------------------------------
private void _main(mixed agent, string account, string gid) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ret;

    if ((!is_object(agent))) {
        return;
    }
    ret = _do_set_phah_bonus_info(account, gid);
    if ((!is_object(agent))) {
        return;
    }
    agent->send_message(64923, gid, save_string(ret));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=20, nargs=3, insts=20
// --------------------------------------------------------------------
void main(object agent, string account, string gid) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _main, agent, account, gid :));

}

// --------------------------------------------------------------------
// @brief  internal handler: sub-action processor
// @code  nc=80, nargs=2, insts=80
// --------------------------------------------------------------------
private mapping _do_set_phah_bonus_info(string account, string gid) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    int ret;
    mixed ti;
    mixed sql_cmd;
    mixed bonus_info;

    if (((cno = "/daemons/dbd.c"->assure_connection("adb")) < 0)) {
        return ([ "ret": -2 ]);
    }
    ti = time();
    bonus_info = ([ "time": ctime(ti, 3), "gid": gid ]);
    sql_cmd = mysql_bind_parameters("update phone_auth set bonus_info=?, update_time=? where account=? and state=1", save_string(bonus_info), ctime(ti, 3), account);
    ret = (*f_execute)(cno, sql_cmd);
    if ((ret < 0)) {
        return ([ "ret": -2 ]);
    }
    if ((ret == 0)) {
        return ([ "ret": -1 ]);
    }
    bonus_info = ([ "time": ti, "gid": gid ]);
    return ([ "ret": 0, "bonus_info": bonus_info ]);

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