/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: store.o
 * Source: /daemons/data_save/store.c
 * Inherits: (none)
 * Functions: 5 names, 5 code segments found
 * Strings: 14, Variables: 0
 *********************************************************************/

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed convert_to_save_mapping(mapping &memory_mapping);
mixed convert_to_save_data(string memory_text);
mixed convert_to_memory_data(string save_text);
mixed convert_to_memory_mapping(mapping &save_map);
private void create();

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=79, nargs=1, insts=79
// --------------------------------------------------------------------
mixed convert_to_save_mapping(mapping &memory_mapping) {
    /* ---- Enhanced Pseudocode ---- */
    int pos;
    mixed arr;
    mapping m_data;
    mapping m_ob;
    mixed item_convert_ob;
    mixed pet_convert_ob;

    item_convert_ob = "/daemons/datasaved.c"->query_entry("item");
    pet_convert_ob = "/daemons/datasaved.c"->query_entry("monster");
    loop (arr in memory_mapping) {
        if (((!is_int(pos = arr[0])) || (!is_mapping(m_ob = arr[1])))) {
            continue;
        }
        m_data = m_ob["data"];
        if (is_mapping(m_data["attrib"])) {
            pet_convert_ob->convert_to_save_mapping(&m_data);
        } else {
            item_convert_ob->convert_to_save_mapping(&m_data);
        }
        memory_mapping[pos] = sprintf("%s:%s", m_ob["name"], save_string(m_data));
    }  // end loop (verified)
    return memory_mapping;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=88, nargs=1, insts=88
// --------------------------------------------------------------------
mixed convert_to_save_data(string memory_text) {
    /* ---- Enhanced Pseudocode ---- */
    mixed memory_map;
    mapping data_map;
    int pos;
    mixed name;
    mixed data;
    mixed converted_data;
    mixed item_convert_ob;
    mixed pet_convert_ob;

    item_convert_ob = "/daemons/datasaved.c"->query_entry("item");
    pet_convert_ob = "/daemons/datasaved.c"->query_entry("monster");
    memory_map = restore_value(memory_text);
    loop (pos in keys(memory_map)) {
        data = memory_map[pos];
        if ((!is_string(data))) {
            continue;
        }
        if ((sscanf(data, "%s:%s", &name, &data) != 2)) {
            continue;
        }
        data_map = restore_value(data);
        if (is_mapping(data_map["attrib"])) {
            converted_data = pet_convert_ob->convert_to_save_data(data);
        } else {
            converted_data = item_convert_ob->convert_to_save_data(data);
        }
        memory_map[pos] = sprintf("%s:%s", name, converted_data);
    }  // end loop (verified)
    return save_string(memory_map);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=88, nargs=1, insts=88
// --------------------------------------------------------------------
mixed convert_to_memory_data(string save_text) {
    /* ---- Enhanced Pseudocode ---- */
    mixed save_map;
    mapping data_map;
    int pos;
    mixed name;
    mixed data;
    mixed converted_data;
    mixed item_convert_ob;
    mixed pet_convert_ob;

    item_convert_ob = "/daemons/datasaved.c"->query_entry("item");
    pet_convert_ob = "/daemons/datasaved.c"->query_entry("monster");
    save_map = restore_value(save_text);
    loop (pos in keys(save_map)) {
        data = save_map[pos];
        if ((!is_string(data))) {
            continue;
        }
        if ((sscanf(data, "%s:%s", &name, &data) != 2)) {
            continue;
        }
        data_map = restore_value(data);
        if (is_mapping(data_map["attrib"])) {
            converted_data = pet_convert_ob->convert_to_memory_data(data);
        } else {
            converted_data = item_convert_ob->convert_to_memory_data(data);
        }
        save_map[pos] = sprintf("%s:%s", name, converted_data);
    }  // end loop (verified)
    return save_string(save_map);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=82, nargs=1, insts=82
// --------------------------------------------------------------------
mixed convert_to_memory_mapping(mapping &save_map) {
    /* ---- Enhanced Pseudocode ---- */
    int pos;
    mixed data;
    mixed arr;
    mixed name;
    mapping data_map;
    mixed item_convert_ob;
    mixed pet_convert_ob;

    item_convert_ob = "/daemons/datasaved.c"->query_entry("item");
    pet_convert_ob = "/daemons/datasaved.c"->query_entry("monster");
    loop (arr in save_map) {
        if ((!is_int(pos = arr[0]))) {
            continue;
        }
        data = arr[1];
        if ((sscanf(data, "%s:%s", &name, &data) != 2)) {
            continue;
        }
        data_map = restore_value(data);
        if (is_mapping(data_map["attrib"])) {
            pet_convert_ob->convert_to_memory_mapping(&data_map);
        } else {
            item_convert_ob->convert_to_memory_mapping(&data_map);
        }
        save_map[pos] = ([ "name": name, "data": data_map ]);
    }  // end loop (verified)
    return save_map;

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=10, nargs=0, insts=10
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/datasaved.c"->register_entry("store", this_object());

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