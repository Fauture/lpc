/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_set_account_info.o
 * Source: /aaa/daemons/server/cmd_l_set_account_info.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 9, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _main(string account, string key, string value);
void main(object agent, string account, string key, string value);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=46, nargs=3, insts=46
// --------------------------------------------------------------------
private void _main(string account, string key, string value) {
    /* ---- Enhanced Pseudocode ---- */
    mixed account_info;

    if ((((key == "account") || (key == "gold_coin")) || (key == "silver_coin"))) {
        printf("This module can't be used to add coin.");
        return;
    }
    if ((!is_mapping(account_info = "/aaa/daemons/accountd.c"->query_account_info(account)))) {
        return;
    }
    account_info[key] = value;
    "/aaa/daemons/accountd.c"->update_account_info(account_info, ({ key }));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=20, nargs=4, insts=20
// --------------------------------------------------------------------
void main(object agent, string account, string key, string value) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _main, account, key, value :));

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