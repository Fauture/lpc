/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_force_write_log.o
 * Source: /dba/daemons/dbaserver/cmd_force_write_log.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 1 names, 1 code segments found
 * Strings: 3, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

int main(mixed agent, string table_name);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=21, nargs=2, insts=21
// --------------------------------------------------------------------
int main(mixed agent, string table_name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed entry;

    if ((!is_object(entry = "/dba/daemons/batch_logd.c"->query_entry(table_name)))) {
        return 0;
    }
    entry->force_write_log();
    return 1;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        1
   Code segments found:         1
   Successfully decoded:        1
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          1
      - /inherit/misc/handler.c()
   ================================================================ */