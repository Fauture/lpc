/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_notice_level_up.o
 * Source: /aaa/daemons/server/cmd_l_notice_level_up.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 8, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _main(mixed agent, string account, int channel, int action, int ti, int did);
int main(mixed agent, string account, int channel, int action, int ti, int did);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=13, nargs=6, insts=13
// --------------------------------------------------------------------
private void _main(mixed agent, string account, int channel, int action, int ti, int did) {
    /* ---- Enhanced Pseudocode ---- */
    "/aaa/daemons/easyutild.c"->notice_level_up(account, channel, action, ti, did);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=40, nargs=6, insts=40
// --------------------------------------------------------------------
int main(mixed agent, string account, int channel, int action, int ti, int did) {
    /* ---- Enhanced Pseudocode ---- */
    if (((!"/aaa/daemons/easyutild.c"->get_notice_level_up_status()) || (!is_string("/aaa/daemons/configd.c"->get_global_config("notice_level_up_url"))))) {
        return 1;
    }
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _main, agent, account, channel, action, ti, did :));
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