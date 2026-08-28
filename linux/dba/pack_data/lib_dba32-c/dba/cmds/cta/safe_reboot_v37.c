/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: safe_reboot.o
 * Source: /dba/cmds/cta/safe_reboot.c
 * Inherits: (none)
 * Functions: 2 names, 2 code segments found
 * Strings: 8, Variables: 0
 *********************************************************************/

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void create();
int main(int client_no, mapping paras);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=9, nargs=0, insts=9
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    register_daemon_data("/daemons/ctacommandd.c", "safe_reboot", "/dba/cmds/cta/safe_reboot.c");

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=30, nargs=2, insts=30
// --------------------------------------------------------------------
int main(int client_no, mapping paras) {
    /* ---- Enhanced Pseudocode ---- */
    paras["msg"] = "safe_reboot";
    paras["prompt"] = "DBA即将安全重启";
    "/daemons/ctad.c"->send_to_cta(client_no, paras);
    destruct_object(find_agent_by_client(client_no));
    safe_reboot();
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
   ================================================================ */