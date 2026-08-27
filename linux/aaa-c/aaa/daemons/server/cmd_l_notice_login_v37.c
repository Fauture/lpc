/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_notice_login.o
 * Source: /aaa/daemons/server/cmd_l_notice_login.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 8, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _main(mixed agent, string account, mapping para);
int main(mixed agent, string account, string str_para);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=3, insts=10
// --------------------------------------------------------------------
private void _main(mixed agent, string account, mapping para) {
    /* ---- Enhanced Pseudocode ---- */
    "/aaa/daemons/easyutild.c"->notice_login(account, para);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=55, nargs=3, insts=55
// --------------------------------------------------------------------
int main(mixed agent, string account, string str_para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed para;

    if (((!"/aaa/daemons/easyutild.c"->get_notice_login_status()) || (!is_string("/aaa/daemons/configd.c"->get_global_config("notice_login_url"))))) {
        return 1;
    }
    try {
        para = restore_value(str_para);
    } catch { /* error handled */ }

    // --- label L0027 ---
    if ((0 || (!is_mapping(para)))) {
        return 1;
    }
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _main, agent, account, para :));
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