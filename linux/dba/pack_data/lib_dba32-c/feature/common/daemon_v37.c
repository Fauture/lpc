/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: daemon.o
 * Source: /feature/common/daemon.c
 * Inherits: (none)
 * Functions: 2 names, 2 code segments found
 * Strings: 3, Variables: 0
 *********************************************************************/

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

string get_name();
private void create();

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=20, nargs=0, insts=20
// --------------------------------------------------------------------
string get_name() {
    /* ---- Enhanced Pseudocode ---- */
    mixed name;
    mixed dname;

    name = object_name(this_object());
    if ((!sscanf(name, "/dba/daemons/%s.c", &dname))) {
        error("Bad daemon.\n");
    }
    return dname;

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    this_object()->restore();

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