/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: shop_counter.o
 * Source: /daemons/data_save/shop_counter.c
 * Inherits: (none)
 * Functions: 3 names, 3 code segments found
 * Strings: 9, Variables: 0
 *********************************************************************/

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed convert_to_save_data(string memory_text);
mixed convert_to_memory_data(string save_text);
private void create();

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=6, nargs=1, insts=6
// --------------------------------------------------------------------
mixed convert_to_save_data(string memory_text) {
    /* ---- Enhanced Pseudocode ---- */
    return memory_text;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=90, nargs=1, insts=90
// --------------------------------------------------------------------
mixed convert_to_memory_data(string save_text) {
    /* ---- Enhanced Pseudocode ---- */
    mixed save_map;
    mapping ob_data_map;
    int pos;
    mapping data;
    mixed converted_data;
    mixed ob_data;
    mixed item_convert_ob;
    mixed pet_convert_ob;

    item_convert_ob = "/daemons/datasaved.c"->query_entry("item");
    pet_convert_ob = "/daemons/datasaved.c"->query_entry("monster");
    save_map = restore_value(save_text);
    loop (pos in keys(save_map)) {
        data = save_map[pos];
        if ((!is_mapping(data))) {
            continue;
        }
        if ((!is_string(ob_data = data["ob_data"]))) {
            continue;
        }
        ob_data_map = restore_value(ob_data);
        if ((!is_mapping(ob_data_map))) {
            continue;
        }
        if (is_mapping(ob_data_map["attrib"])) {
            converted_data = pet_convert_ob->convert_to_memory_data(ob_data);
        } else {
            converted_data = item_convert_ob->convert_to_memory_data(ob_data);
        }
        data["ob_data"] = converted_data;
    }  // end loop (verified)
    return save_string(save_map);

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=10, nargs=0, insts=10
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/datasaved.c"->register_entry("shop_counter", this_object());

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