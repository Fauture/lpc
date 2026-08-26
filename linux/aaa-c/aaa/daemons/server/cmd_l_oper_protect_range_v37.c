/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_oper_protect_range.o
 * Source: /aaa/daemons/server/cmd_l_oper_protect_range.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 9, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _main(object agent, string account, string oper, int range);
void main(object agent, string account, string oper, int range);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=53, nargs=4, insts=53
// --------------------------------------------------------------------
private void _main(object agent, string account, string oper, int range) {
    /* ---- Enhanced Pseudocode ---- */
    int result;

    switch (oper) {
        case "get":
        result = "/aaa/daemons/accountd.c"->query_protect_range(account);
        if ((result < 0)) {
            return;
        }
        break;
        case "set":
        result = 0;
        if ("/aaa/daemons/accountd.c"->is_protect_actived(account, get_auth_normal_bits())) {
            result = "/aaa/daemons/accountd.c"->record_protect_range(account, range);
        }
        break;
        default:
        return;
    }  // end switch

    // --- label L0042 ---
    agent->send_message(21761, account, oper, range, result);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=21, nargs=4, insts=21
// --------------------------------------------------------------------
void main(object agent, string account, string oper, int range) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _main, agent, account, oper, range :));

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