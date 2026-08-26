/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_query_account_state.o
 * Source: /aaa/daemons/server/cmd_l_query_account_state.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 9, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void query_account_state(mixed agent, string account);
void main(object agent, string account);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=54, nargs=2, insts=54
// --------------------------------------------------------------------
private void query_account_state(mixed agent, string account) {
    /* ---- Enhanced Pseudocode ---- */
    string state;
    mapping account_info;

    if ((!is_object(agent))) {
        return;
    }
    state = "";
    do {
        account_info = "/aaa/daemons/accountd.c"->query_account_info(account);
        if ((!is_mapping(account_info))) {
            break;
        }
        state = account_info["locked"];
    } while (0);

    // --- label L0032 ---
    if ((!is_object(agent))) {
        return;
    }
    if ((state != "1")) {
        state = "0";
    }
    agent->send_message(9099, account, state);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=19, nargs=2, insts=19
// --------------------------------------------------------------------
void main(object agent, string account) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: query_account_state, agent, account :));

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