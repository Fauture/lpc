/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: datasaved.o
 * Source: /daemons/datasaved.c
 * Inherits: (none)
 * Inherited programs: /feature/common/dbase.c(), /feature/common/name.c(), /feature/daemon/register.c()
 * Functions: 5 names, 5 code segments found
 * Strings: 5, Variables: 0
 *********************************************************************/

inherit "/feature/common/dbase.c";
inherit "/feature/common/name.c";
inherit "/feature/daemon/register.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed convert_to_save_mapping(string name, mapping &m_memory);
mixed convert_to_save_data(string name, string memory_text);
mixed convert_to_memory_data(string name, string save_text);
mixed convert_to_memory_mapping(string name, mapping &s_memory);
private void create();

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=30, nargs=2, insts=30
// --------------------------------------------------------------------
mixed convert_to_save_mapping(string name, mapping &m_memory) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ob;
    mixed ret;

    if ((!is_object(ob = query_entry(name)))) {
        return m_memory;
    }
    try {
        ret = ob->convert_to_save_mapping(&m_memory);
    } catch { /* error handled */ }

    // --- label L0023 ---
    return m_memory;
    return ret;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=30, nargs=2, insts=30
// --------------------------------------------------------------------
mixed convert_to_save_data(string name, string memory_text) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ob;
    mixed ret;

    if ((!is_object(ob = query_entry(name)))) {
        return memory_text;
    }
    try {
        ret = ob->convert_to_save_data(memory_text);
    } catch { /* error handled */ }

    // --- label L0023 ---
    return memory_text;
    return ret;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=30, nargs=2, insts=30
// --------------------------------------------------------------------
mixed convert_to_memory_data(string name, string save_text) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ob;
    mixed ret;

    if ((!is_object(ob = query_entry(name)))) {
        return save_text;
    }
    try {
        ret = ob->convert_to_memory_data(save_text);
    } catch { /* error handled */ }

    // --- label L0023 ---
    return save_text;
    return ret;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=30, nargs=2, insts=30
// --------------------------------------------------------------------
mixed convert_to_memory_mapping(string name, mapping &s_memory) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ob;
    mixed ret;

    if ((!is_object(ob = query_entry(name)))) {
        return s_memory;
    }
    try {
        ret = ob->convert_to_memory_mapping(&s_memory);
    } catch { /* error handled */ }

    // --- label L0023 ---
    return s_memory;
    return ret;

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=7, nargs=0, insts=7
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    load_all("/daemons/data_save/");

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        5
   Code segments found:         5
   Successfully decoded:        5
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          3
      - /feature/common/dbase.c()
      - /feature/common/name.c()
      - /feature/daemon/register.c()
   ================================================================ */