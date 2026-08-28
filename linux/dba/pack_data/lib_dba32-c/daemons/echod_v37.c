/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: echod.o
 * Source: /daemons/echod.c
 * Inherits: (none)
 * Functions: 3 names, 3 code segments found
 * Strings: 1, Variables: 0
 *********************************************************************/

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void echo_daemon();
private void destruct();
private void create();

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=23, nargs=0, insts=23
// --------------------------------------------------------------------
private void echo_daemon() {
    /* ---- Enhanced Pseudocode ---- */
    mixed client_no;

    set_as_daemon();
    while (1) {
        loop (client_no in get_all_client_no()) {
            send_to_client(client_no, 8402, make32(tick()));
        }  // end loop (verified)
        sleep(10);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=7, nargs=0, insts=7
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    debug_delete_thread("techo");

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    create_thread("techo", (: echo_daemon :));

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