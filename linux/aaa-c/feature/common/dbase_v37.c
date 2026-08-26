/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: dbase.o
 * Source: /feature/common/dbase.c
 * Inherits: (none)
 * Functions: 14 names, 14 code segments found
 * Strings: 1, Variables: 2
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   mapping dbase;
   mapping tmp_dbase;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void delete(string path);
mixed query_temp(string path);
mixed query(string path, mixed raw);
mixed add_temp_ex(string path, mixed val);
mapping query_entire_temp_dbase();
mapping query_entire_dbase();
mixed add_temp(string path, mixed val);
mixed set(string path, mixed val);
mixed set_temp(string path, mixed val);
void replace_entire_temp_dbase(mapping new_temp_dbase);
mixed add(string path, mixed val);
void delete_temp(string path);
mixed add_ex(string path, mixed val);
void replace_entire_dbase(mapping new_dbase);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   dbase = ([ ]);  // /feature/common/dbase.c:108
   tmp_dbase = ([ ]);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
void delete(string path) {
    /* ---- Enhanced Pseudocode ---- */
    express_delete(path, dbase);

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
mixed query_temp(string path) {
    /* ---- Enhanced Pseudocode ---- */
    return express_query(path, tmp_dbase);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=30, nargs=2, insts=30
// --------------------------------------------------------------------
mixed query(string path, mixed raw) {
    /* ---- Enhanced Pseudocode ---- */
    mixed data;

    if (((is_undefined(data = express_query(path, dbase)) && (!raw)) && is_clone())) {
        data = express_query(path, (object_name(this_object(), 1))->query_entire_dbase());
    }
    return data;

}

// --------------------------------------------------------------------
// @code  nc=16, nargs=2, insts=16
// --------------------------------------------------------------------
mixed add_temp_ex(string path, mixed val) {
    /* ---- Enhanced Pseudocode ---- */
    if (query_temp(path)) {
        return add_temp(path, val);
    }
    return set_temp(path, val);

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
mapping query_entire_temp_dbase() {
    /* ---- Enhanced Pseudocode ---- */
    return tmp_dbase;

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
mapping query_entire_dbase() {
    /* ---- Enhanced Pseudocode ---- */
    return dbase;

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=2, insts=9
// --------------------------------------------------------------------
mixed add_temp(string path, mixed val) {
    /* ---- Enhanced Pseudocode ---- */
    return express_add(path, tmp_dbase, val);

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=2, insts=9
// --------------------------------------------------------------------
mixed set(string path, mixed val) {
    /* ---- Enhanced Pseudocode ---- */
    return express_set(path, dbase, val);

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=9, nargs=2, insts=9
// --------------------------------------------------------------------
mixed set_temp(string path, mixed val) {
    /* ---- Enhanced Pseudocode ---- */
    return express_set(path, tmp_dbase, val);

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
void replace_entire_temp_dbase(mapping new_temp_dbase) {
    /* ---- Enhanced Pseudocode ---- */
    tmp_dbase = new_temp_dbase;

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=2, insts=9
// --------------------------------------------------------------------
mixed add(string path, mixed val) {
    /* ---- Enhanced Pseudocode ---- */
    return express_add(path, dbase, val);

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
void delete_temp(string path) {
    /* ---- Enhanced Pseudocode ---- */
    express_delete(path, tmp_dbase);

}

// --------------------------------------------------------------------
// @code  nc=16, nargs=2, insts=16
// --------------------------------------------------------------------
mixed add_ex(string path, mixed val) {
    /* ---- Enhanced Pseudocode ---- */
    if (query(path)) {
        return add(path, val);
    }
    return set(path, val);

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
void replace_entire_dbase(mapping new_dbase) {
    /* ---- Enhanced Pseudocode ---- */
    dbase = new_dbase;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        14
   Code segments found:         14
   Successfully decoded:        14
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */