/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_get_local_encrypt_info.o
 * Source: /aaa/daemons/server/cmd_l_get_local_encrypt_info.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 11, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void get_local_encrypt_info(mixed agent, string account);
void main(object agent, string account);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=86, nargs=2, insts=86
// --------------------------------------------------------------------
private void get_local_encrypt_info(mixed agent, string account) {
    /* ---- Enhanced Pseudocode ---- */
    int state;
    string password;
    mapping m;

    if ((!is_object(agent))) {
        return;
    }
    password = "";
    m = "/aaa/daemons/accountd.c"->query_account_info(account);
    if (is_mapping(m)) {
        account = m["account"];
        password = m["password"];
    }
    do {
        state = 0;
        m = "/aaa/daemons/accountd.c"->query_local_encrypt_info(account);
        if ((!is_mapping(m))) {
            break;
        }
        if ((!is_string(m["local_encrypt_info"]))) {
            break;
        }
        if ((!(int)m["state"])) {
            break;
        }
        state = 1;
    } while (0);

    // --- label L0071 ---
    if ((!is_object(agent))) {
        return;
    }
    agent->send_message(5013, account, password, state);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=19, nargs=2, insts=19
// --------------------------------------------------------------------
void main(object agent, string account) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: get_local_encrypt_info, agent, account :));

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