/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: share_store.o
 * Source: /daemons/data_save/share_store.c
 * Inherits: (none)
 * Functions: 5 names, 5 code segments found
 * Strings: 10, Variables: 0
 *********************************************************************/

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed convert_to_save_mapping(mapping &save_map);
mixed convert_to_save_data(string memory_text);
mixed convert_to_memory_data(string save_text);
mixed convert_to_memory_mapping(mapping &memory_map);
private void create();

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=46, nargs=1, insts=46
// --------------------------------------------------------------------
mixed convert_to_save_mapping(mapping &save_map) {
    /* ---- Enhanced Pseudocode ---- */
    int pos;
    mixed data;
    mixed item_convert_ob;

    item_convert_ob = "/daemons/datasaved.c"->query_entry("item");
    loop (pos in keys(save_map)) {
        if ((!is_mapping(data = save_map[pos]["item_data"]))) {
            continue;
        }
        item_convert_ob->convert_to_save_mapping(&data);
        save_map[pos]["item_data"] = save_string(data);
    }  // end loop (verified)
    return save_map;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=56, nargs=1, insts=56
// --------------------------------------------------------------------
mixed convert_to_save_data(string memory_text) {
    /* ---- Enhanced Pseudocode ---- */
    mixed memory_map;
    mapping m;
    mixed item_convert_ob;
    int str_pos;

    item_convert_ob = "/daemons/datasaved.c"->query_entry("item");
    memory_map = restore_value(memory_text);
    loop (str_pos in keys(memory_map)) {
        m = memory_map[str_pos];
        if (((!is_mapping(m)) || (!is_string(m["item_data"])))) {
            continue;
        }
        m["item_data"] = item_convert_ob->convert_to_save_data(m["item_data"]);
    }  // end loop (verified)
    return save_string(memory_map);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=56, nargs=1, insts=56
// --------------------------------------------------------------------
mixed convert_to_memory_data(string save_text) {
    /* ---- Enhanced Pseudocode ---- */
    mixed save_map;
    mapping m;
    mixed item_convert_ob;
    int str_pos;

    item_convert_ob = "/daemons/datasaved.c"->query_entry("item");
    save_map = restore_value(save_text);
    loop (str_pos in keys(save_map)) {
        m = save_map[str_pos];
        if (((!is_mapping(m)) || (!is_string(m["item_data"])))) {
            continue;
        }
        m["item_data"] = item_convert_ob->convert_to_memory_data(m["item_data"]);
    }  // end loop (verified)
    return save_string(save_map);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=56, nargs=1, insts=56
// --------------------------------------------------------------------
mixed convert_to_memory_mapping(mapping &memory_map) {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr;
    mapping info;
    mixed data;
    mixed m_data;
    mixed item_convert_ob;

    item_convert_ob = "/daemons/datasaved.c"->query_entry("item");
    loop (arr in memory_map) {
        info = arr[1];
        if (((!is_mapping(info)) || (!is_string(data = info["item_data"])))) {
            continue;
        }
        m_data = restore_value(data);
        item_convert_ob->convert_to_memory_mapping(&m_data);
        info["item_data"] = m_data;
    }  // end loop (verified)
    return memory_map;

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=10, nargs=0, insts=10
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/datasaved.c"->register_entry("share_store", this_object());

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        5
   Code segments found:         5
   Successfully decoded:        5
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */