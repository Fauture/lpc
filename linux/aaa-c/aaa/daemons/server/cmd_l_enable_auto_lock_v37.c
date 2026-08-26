/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_enable_auto_lock.o
 * Source: /aaa/daemons/server/cmd_l_enable_auto_lock.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 7, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void enable_auto_lock(mixed agent, string account, int flag);
void main(object agent, string account, int flag);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=83, nargs=3, insts=83
// --------------------------------------------------------------------
private void enable_auto_lock(mixed agent, string account, int flag) {
    /* ---- Enhanced Pseudocode ---- */
    int result;
    int auto_lock;
    mapping account_info;

    if ((!is_object(agent))) {
        return;
    }
    do {
        auto_lock = 3;
        result = 2;
        account_info = "/aaa/daemons/accountd.c"->query_account_info(account);
        if ((!is_mapping(account_info))) {
            break;
        }
        auto_lock = atoi(account_info["auto_lock"]);
        if ((auto_lock == flag)) {
            result = 1;
            break;
        }
        account_info["auto_lock"] = flag;
        if ((!"/aaa/daemons/accountd.c"->update_account_info(account_info))) {
            break;
        }
        result = 1;
        auto_lock = flag;
    } while (0);

    // --- label L0068 ---
    if ((!is_object(agent))) {
        return;
    }
    agent->send_message(13197, account, result, auto_lock);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=20, nargs=3, insts=20
// --------------------------------------------------------------------
void main(object agent, string account, int flag) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: enable_auto_lock, agent, account, flag :));

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