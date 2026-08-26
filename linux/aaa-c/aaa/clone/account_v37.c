/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: account.o
 * Source: /aaa/clone/account.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/agent.c(), /feature/common/ob_source.c()
 * Functions: 7 names, 7 code segments found
 * Strings: 5, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/agent.c";
inherit "/feature/common/ob_source.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

int query_privilege();
mixed query_auth_key();
private void destruct();
string query_account();
private void create(mapping m);
int is_account();
mixed query_password();

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
int query_privilege() {
    /* ---- Enhanced Pseudocode ---- */
    return atoi(query("privilege"));

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=7, nargs=0, insts=7
// --------------------------------------------------------------------
mixed query_auth_key() {
    /* ---- Enhanced Pseudocode ---- */
    return query("auth_key");

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=4, nargs=0, insts=4
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=7, nargs=0, insts=7
// --------------------------------------------------------------------
string query_account() {
    /* ---- Enhanced Pseudocode ---- */
    return query("account");

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=14, nargs=1, insts=14
// --------------------------------------------------------------------
private void create(mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!is_clone())) {
        return;
    }
    replace_entire_dbase(m);
    set_cmd_module("/aaa/daemons/accountd.c");

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int is_account() {
    /* ---- Enhanced Pseudocode ---- */
    return 1;

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=7, nargs=0, insts=7
// --------------------------------------------------------------------
mixed query_password() {
    /* ---- Enhanced Pseudocode ---- */
    return query("password");

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        7
   Code segments found:         7
   Successfully decoded:        7
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          2
      - /inherit/misc/agent.c()
      - /feature/common/ob_source.c()
   ================================================================ */