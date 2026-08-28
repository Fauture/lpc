/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: encryptd.o
 * Source: /daemons/encryptd.c
 * Inherits: (none)
 * Functions: 2 names, 2 code segments found
 * Strings: 2, Variables: 0
 *********************************************************************/

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed decrypt(string text, string key);
string gen_dynamic_key();

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=47, nargs=2, insts=47
// --------------------------------------------------------------------
mixed decrypt(string text, string key) {
    /* ---- Enhanced Pseudocode ---- */
    mixed buf;
    string str;

    try {
        buf = restore_value(sprintf(":%s:", text));
    } catch { /* error handled */ }

    // --- label L0014 ---
    if ((0 || (!is_buffer(buf)))) {
        return 0;
    }
    try {
        str = (string)des(buf, 1, key);
    } catch { /* error handled */ }

    // --- label L0035 ---
    if ((0 || (!is_string(str)))) {
        return 0;
    }
    return str;

}

// --------------------------------------------------------------------
// @code  nc=91, nargs=0, insts=91
// --------------------------------------------------------------------
string gen_dynamic_key() {
    /* ---- Enhanced Pseudocode ---- */
    return sprintf("%c%c%c%c%c%c%c", (random(2) ? (65 + random(26)) : (97 + random(26))), (random(2) ? (65 + random(26)) : (97 + random(26))), (random(2) ? (65 + random(26)) : (97 + random(26))), (random(2) ? (65 + random(26)) : (97 + random(26))), (random(2) ? (65 + random(26)) : (97 + random(26))), (random(2) ? (65 + random(26)) : (97 + random(26))), (random(2) ? (65 + random(26)) : (97 + random(26))));

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