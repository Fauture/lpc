/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: stalld.o
 * Source: /aaa/daemons/stalld.c
 * Inherits: (none)
 * Functions: 2 names, 2 code segments found
 * Strings: 7, Variables: 0
 *********************************************************************/

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void recycle_account_stall(string account);
private void _recycle_account_stall(string account);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=18, nargs=1, insts=18
// --------------------------------------------------------------------
void recycle_account_stall(string account) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _recycle_account_stall, account :));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=41, nargs=1, insts=41
// --------------------------------------------------------------------
private void _recycle_account_stall(string account) {
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
        agent->send_message(9344, account);
    }  // end loop (verified)

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        2
   Code segments found:         2
   Successfully decoded:        2
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */