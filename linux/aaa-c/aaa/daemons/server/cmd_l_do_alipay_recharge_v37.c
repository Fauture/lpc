/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_do_alipay_recharge.o
 * Source: /aaa/daemons/server/cmd_l_do_alipay_recharge.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 5, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _main(mixed agent, string dist, string account, string gid, int rmb);
void main(mixed agent, string dist, string account, string gid, int rmb);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=30, nargs=5, insts=30
// --------------------------------------------------------------------
private void _main(mixed agent, string dist, string account, string gid, int rmb) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ret;

    if ((!is_object(agent))) {
        return;
    }
    ret = "/aaa/daemons/express_recharged.c"->do_alipay_recharge(dist, account, rmb);
    if ((!is_object(agent))) {
        return;
    }
    agent->send_message(25473, gid, ret);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=27, nargs=5, insts=27
// --------------------------------------------------------------------
void main(mixed agent, string dist, string account, string gid, int rmb) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!is_object(agent))) {
        return;
    }
    "/daemons/threadd.c"->dispatch((sizeof(account) ? account[0] : 0), (: _main, agent, dist, account, gid, rmb :));

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