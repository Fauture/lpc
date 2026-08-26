/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_verify_password.o
 * Source: /aaa/daemons/server/cmd_l_verify_password.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 9, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void verify_password(mixed agent, string server, string account, string gid, string password, string type);
void main(object agent, string server, string account, string gid, string password, string type);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  verification: confirm/guarantee state
// @code  nc=78, nargs=6, insts=78
// --------------------------------------------------------------------
private void verify_password(mixed agent, string server, string account, string gid, string password, string type) {
    /* ---- Enhanced Pseudocode ---- */
    int result;
    mixed new_md5password;
    mapping account_info;

    if ((!is_object(agent))) {
        return;
    }
    result = 0;
    do {
        account_info = "/aaa/daemons/accountd.c"->query_account_info(account);
        if ((!is_mapping(account_info))) {
            break;
        }
        new_md5password = "/aaa/daemons/accountd.c"->calc_password_new_md5(account_info["account"], password);
        if ((type == "password")) {
            result = (stricmp(account_info["password"], new_md5password) == 0);
        } else {
            result = (stricmp(account_info["protect"], password) == 0);
        }
    } while (0);

    // --- label L0062 ---
    if ((!is_object(agent))) {
        return;
    }
    agent->send_message(13161, account, gid, type, result);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=23, nargs=6, insts=23
// --------------------------------------------------------------------
void main(object agent, string server, string account, string gid, string password, string type) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: verify_password, agent, server, account, gid, password, type :));

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