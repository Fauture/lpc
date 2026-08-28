/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: httpd_handler.o
 * Source: /inherit/misc/httpd_handler.c
 * Inherits: (none)
 * Functions: 1 names, 1 code segments found
 * Strings: 6, Variables: 0
 *********************************************************************/

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void create();

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=35, nargs=0, insts=35
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    mixed cmd_name;

    if ((this_object() == find_object("/inherit/misc/httpd_handler.c"))) {
        return;
    }
    cmd_name = explode(object_name(), "/")[<1][0..<3];
    cmd_name = replace_string(cmd_name, "_", "");
    "/daemons/http_serverd.c"->register_entry(cmd_name, this_object());

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        1
   Code segments found:         1
   Successfully decoded:        1
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */