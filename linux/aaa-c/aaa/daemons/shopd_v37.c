/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: shopd.o
 * Source: /aaa/daemons/shopd.c
 * Inherits: (none)
 * Functions: 3 names, 3 code segments found
 * Strings: 9, Variables: 0
 *********************************************************************/

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _pend_account_shop(string account);
private void _pend_account_shop2(string account);
void pend_account_shop(string account);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=37, nargs=1, insts=37
// --------------------------------------------------------------------
private void _pend_account_shop(string account) {
    /* ---- Enhanced Pseudocode ---- */
    mixed server_lists;
    mixed server;
    mixed agent;

    server_lists = keys((mapping)"/aaa/daemons/configd.c"->get_server_list());
    loop (server in server_lists) {
        agent = "/aaa/daemons/accountd.c"->assure_server_agent(server);
        if ((!is_object(agent))) {
            continue;
        }
        agent->send_message(9216, account);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=41, nargs=1, insts=41
// --------------------------------------------------------------------
private void _pend_account_shop2(string account) {
    /* ---- Enhanced Pseudocode ---- */
    mixed mks;
    mixed server;
    mixed agent;
    mixed all_conns;

    all_conns = "/aaa/daemons/gameserverd.c"->get_all_conns();
    loop (mks in all_conns) {
        server = mks[0];
        agent = "/aaa/daemons/accountd.c"->assure_server_agent(server);
        if ((!is_object(agent))) {
            continue;
        }
        agent->send_message(9216, account);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=18, nargs=1, insts=18
// --------------------------------------------------------------------
void pend_account_shop(string account) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _pend_account_shop2, account :));

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        3
   Code segments found:         3
   Successfully decoded:        3
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */