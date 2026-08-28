/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: communicated.o
 * Source: /daemons/communicated.c
 * Inherits: (none)
 * Functions: 3 names, 3 code segments found
 * Strings: 1, Variables: 0
 *********************************************************************/

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void destruct();
private void process_command(string cmd, int client_no, array args);
private void create();

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    pkt_destruct_analyser();

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=19, nargs=3, insts=19
// --------------------------------------------------------------------
private void process_command(string cmd, int client_no, array args) {
    /* ---- Enhanced Pseudocode ---- */
    mixed agent;

    if ((!is_object(agent = find_agent_by_client(client_no)))) {
        return;
    }
    agent->process_command(cmd, args);

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=12, nargs=0, insts=12
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    pkt_init_analyser(65536, 1024);
    load_comm_desc((: process_command :), 1);

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