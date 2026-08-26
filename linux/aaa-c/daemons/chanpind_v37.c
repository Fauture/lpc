/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: chanpind.o
 * Source: /daemons/chanpind.c
 * Inherits: (none)
 * Functions: 2 names, 2 code segments found
 * Strings: 5, Variables: 0
 *********************************************************************/

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed read(string path, string name, mixed branch);
int write(string path, string name, mixed branch, mixed content);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=11, nargs=3, insts=11
// --------------------------------------------------------------------
mixed read(string path, string name, mixed branch) {
    /* ---- Enhanced Pseudocode ---- */
    return "/daemons/filed.c"->read_content(path, name, branch);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=19, nargs=4, insts=19
// --------------------------------------------------------------------
int write(string path, string name, mixed branch, mixed content) {
    /* ---- Enhanced Pseudocode ---- */
    if (is_undefined(content)) {
        error("%O\n", "Argument 4 undefined.");
    }
    return "/daemons/filed.c"->write_content(path, name, branch, content);

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