/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: module_stat.o
 * Source: /feature/common/module_stat.c
 * Inherits: (none)
 * Functions: 6 names, 6 code segments found
 * Strings: 8, Variables: 0
 *********************************************************************/

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void destruct();
mixed get_objects();
private void create();
mixed get_variables();
void size_variables();
void size_objects();

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=20, nargs=0, insts=20
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    mixed index;

    if ((find_object("/feature/common/module_stat.c") == this_object())) {
        return;
    }
    index = object_name(this_object());
    "/daemons/statmoduled.c"->remove_entry(index);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=9, nargs=0, insts=9
// --------------------------------------------------------------------
mixed get_objects() {
    /* ---- Enhanced Pseudocode ---- */
    printf("统计该模块使用的object类型的数据，需要重载本函数\n");
    return ({});

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=30, nargs=0, insts=30
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    mixed index;

    if ((find_object("/feature/common/module_stat.c") == this_object())) {
        return;
    }
    index = object_name(this_object());
    if ((!is_object(find_object("/daemons/statmoduled.c")))) {
        load_object("/daemons/statmoduled.c");
    }
    "/daemons/statmoduled.c"->register_entry(index, this_object(), 1);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=9, nargs=0, insts=9
// --------------------------------------------------------------------
mixed get_variables() {
    /* ---- Enhanced Pseudocode ---- */
    return "/daemons/statmoduled.c"->get_variables(this_object());

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=9, nargs=0, insts=9
// --------------------------------------------------------------------
void size_variables() {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/statmoduled.c"->size_variables(this_object());

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=9, nargs=0, insts=9
// --------------------------------------------------------------------
void size_objects() {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/statmoduled.c"->size_objects(this_object());

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