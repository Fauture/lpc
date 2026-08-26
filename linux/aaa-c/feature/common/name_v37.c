/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: name.o
 * Source: /feature/common/name.c
 * Inherits: (none)
 * Functions: 3 names, 3 code segments found
 * Strings: 2, Variables: 0
 *********************************************************************/

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

int is_original_name();
mixed get_name();
void set_name(string name);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=20, nargs=0, insts=20
// --------------------------------------------------------------------
int is_original_name() {
    /* ---- Enhanced Pseudocode ---- */
    if (((object_name(this_object(), 1))->query("name") == this_object()->query("name"))) {
        return 1;
    }
    return 0;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=7, nargs=0, insts=7
// --------------------------------------------------------------------
mixed get_name() {
    /* ---- Enhanced Pseudocode ---- */
    return query("name");

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
void set_name(string name) {
    /* ---- Enhanced Pseudocode ---- */
    set("name", name);

}

/* ================================================================
   DECLARED FUNCTIONS (no bytecode - likely inherited or forward decl)
   ================================================================ */
/*
   void replace_entire_dbase(mapping new_dbase);  // inherited from parent or forward declaration
   void delete_temp(string path);  // inherited from parent or forward declaration
   mixed add(string path, mixed val);  // inherited from parent or forward declaration
   mixed set_temp(string path, mixed val);  // inherited from parent or forward declaration
   mixed set(string path, mixed val);  // inherited from parent or forward declaration
   mixed add_temp(string path, mixed val);  // inherited from parent or forward declaration
   mapping query_entire_dbase();  // inherited from parent or forward declaration
   mapping query_entire_temp_dbase();  // inherited from parent or forward declaration
   mixed query(string path, mixed raw);  // inherited from parent or forward declaration
   mixed query_temp(string path);  // inherited from parent or forward declaration
   void delete(string path);  // inherited from parent or forward declaration
*/

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        3
   Code segments found:         3
   Successfully decoded:        3
   Unresolved/minimal:          0
   Functions without code:      11

   Note: Functions without code are usually inherited from the
   parent object (), forward declarations, or
   simple property accessor methods that the compiler optimizes away.
   ================================================================ */