/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_manage_vip_sms.o
 * Source: /aaa/daemons/server/cmd_l_manage_vip_sms.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 4, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _main(mixed agent, string dist, string gid, string account, int oper, string para);
int main(mixed agent, string dist, string gid, string account, int oper, string para);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=14, nargs=6, insts=14
// --------------------------------------------------------------------
private void _main(mixed agent, string dist, string gid, string account, int oper, string para) {
    /* ---- Enhanced Pseudocode ---- */
    "/aaa/daemons/vip_smsd.c"->manage_vip_sms(agent, dist, gid, account, oper, para);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=25, nargs=6, insts=25
// --------------------------------------------------------------------
int main(mixed agent, string dist, string gid, string account, int oper, string para) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _main, agent, dist, gid, account, oper, para :));
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