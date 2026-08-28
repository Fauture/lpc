/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: ctad.o
 * Source: /daemons/ctad.c
 * Inherits: (none)
 * Functions: 2 names, 2 code segments found
 * Strings: 0, Variables: 0
 *********************************************************************/

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

int send_to_cta(int client_no, mapping m);
int send_to_cta_ex(int client_no, mapping m);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=36, nargs=2, insts=36
// --------------------------------------------------------------------
int send_to_cta(int client_no, mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    mixed pairs;
    int key;
    mixed buf;

    pairs = sizeof(m);
    buf = make16(pairs);
    loop (key in keys(m)) {
        buf += pad_buffer((string)key, (string)m[key]);
    }  // end loop (verified)
    return send_to_client(client_no, 63748, buf);

}

// --------------------------------------------------------------------
// @code  nc=53, nargs=2, insts=53
// --------------------------------------------------------------------
int send_to_cta_ex(int client_no, mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    mixed pairs;
    mixed length;
    int key;
    mixed buf;
    string content;

    pairs = sizeof(m);
    buf = make16(pairs);
    loop (key in keys(m)) {
        content = (string)m[key];
        length = strlen(content);
        buf += pad_buffer((string)key);
        buf += pad_buffer(make32(length), (buffer)content);
    }  // end loop (verified)
    return send_to_client(client_no, 63749, buf);

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