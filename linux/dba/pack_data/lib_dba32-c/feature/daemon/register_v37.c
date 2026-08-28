/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: register.o
 * Source: /feature/daemon/register.c
 * Inherits: (none)
 * Functions: 6 names, 6 code segments found
 * Strings: 0, Variables: 1
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   mapping all_entries;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mapping query_all_entries();
int register_entry(mixed index = 0, mixed value = 0, int para = 1);
private void destruct();
void empty_entries();
mixed query_entry(mixed index);
void remove_entry(mixed index);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   all_entries = ([ ]);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
mapping query_all_entries() {
    /* ---- Enhanced Pseudocode ---- */
    return all_entries;

}

// --------------------------------------------------------------------
// @code  nc=28, nargs=3, insts=28
// --------------------------------------------------------------------
int register_entry(mixed index, mixed value, int para) {
    /* ---- Enhanced Pseudocode ---- */
    if (((!is_undefined(all_entries[index])) && (!para))) {
        return 0;
    }
    all_entries[index] = value;
    return 1;

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=14, nargs=0, insts=14
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    mixed index;

    loop (index in keys(all_entries)) {
        remove_entry(index);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
void empty_entries() {
    /* ---- Enhanced Pseudocode ---- */
    all_entries = ([ ]);

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
mixed query_entry(mixed index) {
    /* ---- Enhanced Pseudocode ---- */
    return all_entries[index];

}

// --------------------------------------------------------------------
// @code  nc=38, nargs=1, insts=38
// --------------------------------------------------------------------
void remove_entry(mixed index) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ob;

    if ((((is_object(ob = index) || is_object(ob = get_function_owner(ob))) || is_object(ob = all_entries[index])) || is_object(ob = get_function_owner(ob)))) {
        destruct_object(ob);
    }
    map_delete(all_entries, index);

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        6
   Code segments found:         6
   Successfully decoded:        6
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */