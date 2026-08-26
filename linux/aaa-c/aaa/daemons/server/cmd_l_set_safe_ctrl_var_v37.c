/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_set_safe_ctrl_var.o
 * Source: /aaa/daemons/server/cmd_l_set_safe_ctrl_var.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 10, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _main(object agent, string module, string account, string dist_name, int value);
void main(object agent, string module, string account, string dist_name, int value);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=53, nargs=5, insts=53
// --------------------------------------------------------------------
private void _main(object agent, string module, string account, string dist_name, int value) {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;

    m = ([ ]);
    m["account"] = account;
    m["protect_type"] = "total_switch";
    m["protect_info"] = sprintf("%d", value);
    if ("/aaa/daemons/accountd.c"->insert_safe_ctrl(m)) {
        agent->send_message(9117, module, account, 1, value);
        return;
    }
    agent->send_message(9117, module, account, 0, value);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=22, nargs=5, insts=22
// --------------------------------------------------------------------
void main(object agent, string module, string account, string dist_name, int value) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _main, agent, module, account, dist_name, value :));

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