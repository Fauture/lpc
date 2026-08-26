/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: gid.o
 * Source: /feature/common/gid.c
 * Inherits: (none)
 * Functions: 3 names, 3 code segments found
 * Strings: 2, Variables: 1
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   string gid;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void set_gid(string _gid);
private void destruct();
mixed get_gid();

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=14, nargs=1, insts=14
// --------------------------------------------------------------------
void set_gid(string _gid) {
    /* ---- Enhanced Pseudocode ---- */
    gid = _gid;
    this_object()->set("gid", gid);

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=10, nargs=0, insts=10
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    if (is_string(gid)) {
        remove_gid_object(get_gid());
    }

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
mixed get_gid() {
    /* ---- Enhanced Pseudocode ---- */
    return gid;

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