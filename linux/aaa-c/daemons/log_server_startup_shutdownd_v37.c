/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: log_server_startup_shutdownd.o
 * Source: /daemons/log_server_startup_shutdownd.c
 * Inherits: (none)
 * Functions: 1 names, 1 code segments found
 * Strings: 11, Variables: 1
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   mapping server_names;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void record_log(string action, string period);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   server_names = ([ 7 : "ccs", 9 : "spa", 5 : "gs" ]);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=52, nargs=2, insts=52
// --------------------------------------------------------------------
void record_log(string action, string period) {
    /* ---- Enhanced Pseudocode ---- */
    mixed para1;
    string current_path;
    mixed address;

    current_path = (address = "");
    para1 = period;
    if ((is_mapping(get_system_info()) && ("startup" == action))) {
        current_path = (string)get_system_info()["current_path"];
    }
    if (("/daemons/filed.c"->get_dba_client_no() < 1)) {
        return;
    }
    "/daemons/filed.c"->log_to_db("important_log", 1, server_names[2], action, para1, "1.52.1211", address, current_path);

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