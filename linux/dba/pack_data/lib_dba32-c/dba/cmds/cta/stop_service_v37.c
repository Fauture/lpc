/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: stop_service.o
 * Source: /dba/cmds/cta/stop_service.c
 * Inherits: (none)
 * Functions: 2 names, 2 code segments found
 * Strings: 9, Variables: 0
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
    register_daemon_data("/daemons/ctacommandd.c", "stop_service", "/dba/cmds/cta/stop_service.c");

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=30, nargs=2, insts=30
// --------------------------------------------------------------------
int main(int client_no, mapping paras) {
    /* ---- Enhanced Pseudocode ---- */
    "/dba/daemons/dbaserverd.c"->stop_service(({ client_no }));
    paras["msg"] = "stop_service";
    paras["prompt"] = "DBA所有服务已经被停止";
    "/daemons/ctad.c"->send_to_cta(client_no, paras);
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