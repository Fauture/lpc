/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: get_server_version.o
 * Source: /cmds/cta/get_server_version.c
 * Inherits: (none)
 * Functions: 3 names, 3 code segments found
 * Strings: 9, Variables: 0
 *********************************************************************/

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _main(int client_no, mapping paras);
private void create();
int main(int client_no, mapping paras);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=16, nargs=2, insts=16
// --------------------------------------------------------------------
private void _main(int client_no, mapping paras) {
    /* ---- Enhanced Pseudocode ---- */
    paras["version"] = "1.52.1211";
    "/daemons/ctad.c"->send_to_cta(client_no, paras);

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=9, nargs=0, insts=9
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    register_daemon_data("/daemons/ctacommandd.c", "get_server_version", "/cmds/cta/get_server_version.c");

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=13, nargs=2, insts=13
// --------------------------------------------------------------------
int main(int client_no, mapping paras) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_service((: _main, client_no, paras :));
    return 1;

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