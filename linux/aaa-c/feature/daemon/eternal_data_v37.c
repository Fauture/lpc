/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: eternal_data.o
 * Source: /feature/daemon/eternal_data.c
 * Inherits: (none)
 * Functions: 3 names, 3 code segments found
 * Strings: 2, Variables: 1
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   array eternal_data;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void annouce_eternal_data(string var_name, mixed &var);
private void destruct();
private void create();

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   eternal_data = ([ ]);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=22, nargs=2, insts=22
// --------------------------------------------------------------------
void annouce_eternal_data(string var_name, mixed &var) {
    /* ---- Enhanced Pseudocode ---- */
    if (eternal_data[var_name]) {
        var = eternal_data[var_name];
    } else {
        eternal_data[var_name] = var;
    }

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=37, nargs=0, insts=37
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    mixed obj_name;
    mixed mks;

    if ((is_clone() || ((obj_name = object_name()) == "/feature/daemon/eternal_data.c"))) {
        return;
    }
    loop (mks in eternal_data) {
        eternal_data[mks[0]] = get_obj_var(this_object(), mks[0]);
    }  // end loop (verified)
    register_daemon_data(obj_name, "eternal_data", eternal_data);

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=39, nargs=0, insts=39
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;
    mixed obj_name;

    if ((is_clone() || ((obj_name = object_name()) == "/feature/daemon/eternal_data.c"))) {
        return;
    }
    if ((!is_mapping(m = get_daemon_data(obj_name)))) {
        return;
    }
    if ((!is_mapping(m["eternal_data"]))) {
        return;
    }
    eternal_data = m["eternal_data"];
    unregister_daemon_data(obj_name, "eternal_data");

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        3
   Code segments found:         3
   Successfully decoded:        3
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */