/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: start_telnet_service.o
 * Source: /cmds/cta/start_telnet_service.c
 * Inherits: (none)
 * Functions: 3 names, 3 code segments found
 * Strings: 12, Variables: 0
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
// @code  nc=38, nargs=2, insts=38
// --------------------------------------------------------------------
private void _main(int client_no, mapping paras) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ret;

    ret = "/aaa/daemons/configd.c"->start_telnet_service();
    if ((!ret)) {
        paras["prompt"] = "failed";
    } else {
        paras["prompt"] = "succeed";
    }
    paras["msg"] = "start_telnet_service";
    "/daemons/ctad.c"->send_to_cta(client_no, paras);

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=9, nargs=0, insts=9
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    register_daemon_data("/daemons/ctacommandd.c", "start_telnet_service", "/cmds/cta/start_telnet_service.c");

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