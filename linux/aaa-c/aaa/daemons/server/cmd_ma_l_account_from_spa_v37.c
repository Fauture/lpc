/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_ma_l_account_from_spa.o
 * Source: /aaa/daemons/server/cmd_ma_l_account_from_spa.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 11, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void auth_admintool(mixed agent, string account, string password, string mac_addr, int cookie, string peer_ip);
void main(object agent, string account, string password, string mac_addr, int cookie, string peer_ip);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=41, nargs=6, insts=41
// --------------------------------------------------------------------
private void auth_admintool(mixed agent, string account, string password, string mac_addr, int cookie, string peer_ip) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ret;
    mapping info;

    if ((!is_object(agent))) {
        return;
    }
    info = ([ ]);
    ret = "/aaa/daemons/admintoold.c"->auth_admintool(agent, account, password, mac_addr, &info, peer_ip);
    agent->send_message(16631, ret, info["privilege"], info["prompt"], cookie, account, "", 0);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=26, nargs=6, insts=26
// --------------------------------------------------------------------
void main(object agent, string account, string password, string mac_addr, int cookie, string peer_ip) {
    /* ---- Enhanced Pseudocode ---- */
    if ((agent->query("type") != 9)) {
        return;
    }
    "/daemons/threadd.c"->dispatch(random(10), (: auth_admintool, agent, account, password, mac_addr, cookie, peer_ip :));

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