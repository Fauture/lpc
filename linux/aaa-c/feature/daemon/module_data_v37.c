/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: module_data.o
 * Source: /feature/daemon/module_data.c
 * Inherits: (none)
 * Functions: 2 names, 2 code segments found
 * Strings: 0, Variables: 1
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   int load_data_ok;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

int check_module_data_ok();
void load_module_data_ok();

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   load_data_ok = 0;

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  validator: verify data
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int check_module_data_ok() {
    /* ---- Enhanced Pseudocode ---- */
    return load_data_ok;

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
void load_module_data_ok() {
    /* ---- Enhanced Pseudocode ---- */
    load_data_ok = 1;

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