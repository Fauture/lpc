/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_set_safe_ctrl_str.o
 * Source: /aaa/daemons/server/cmd_l_set_safe_ctrl_str.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 5, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _main(object agent, string module, string account, string dist_name, string content);
void main(object agent, string module, string account, string dist_name, buffer content);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=41, nargs=5, insts=41
// --------------------------------------------------------------------
private void _main(object agent, string module, string account, string dist_name, string content) {
    /* ---- Enhanced Pseudocode ---- */
    mixed m;

    m = restore_value(content);
    if ((!is_mapping(m))) {
        return;
    }
    if ("/aaa/daemons/accountd.c"->insert_safe_ctrl(m)) {
        agent->send_message(9116, module, account, 1, save_string(m));
        return;
    }
    agent->send_message(9116, module, account, 0, save_string(m));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=23, nargs=5, insts=23
// --------------------------------------------------------------------
void main(object agent, string module, string account, string dist_name, buffer content) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _main, agent, module, account, dist_name, (string)content :));

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