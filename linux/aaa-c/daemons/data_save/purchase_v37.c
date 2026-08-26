/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: purchase.o
 * Source: /daemons/data_save/purchase.c
 * Inherits: (none)
 * Inherited programs: /feature/common/dbase.c()
 * Functions: 6 names, 6 code segments found
 * Strings: 14, Variables: 4
 *********************************************************************/

inherit "/feature/common/dbase.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   /* mixed global_0; */
   /* mixed global_1; */
   mapping new_key_map;
   mapping org_key_map;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed convert_to_save_mapping(mapping &memory_mapping);
mixed convert_to_save_data(string memory_text);
mixed convert_to_memory_data(string save_text);
private void set_convert_key(mixed org_key, int new_key);
mixed convert_to_memory_mapping(mapping &save_mapping);
private void create();

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   org_key_map = ([ ]);  // /daemons/data_save/purchase.c:84
   new_key_map = ([ ]);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=49, nargs=1, insts=49
// --------------------------------------------------------------------
mixed convert_to_save_mapping(mapping &memory_mapping) {
    /* ---- Enhanced Pseudocode ---- */
    int org_key;
    int new_key;
    mixed org_value;

    loop (org_key in keys(memory_mapping)) {
        org_value = memory_mapping[org_key];
        if (is_mapping(org_value)) {
            convert_to_save_mapping(&org_value);
        }
        if (is_int(new_key = org_key_map[org_key])) {
            memory_mapping[org_key] = 0;
            memory_mapping[new_key] = org_value;
        }
    }  // end loop (verified)
    clean_mapping_by_value(memory_mapping);
    return memory_mapping;

}

// --------------------------------------------------------------------
// @code  nc=15, nargs=1, insts=15
// --------------------------------------------------------------------
mixed convert_to_save_data(string memory_text) {
    /* ---- Enhanced Pseudocode ---- */
    mixed org_map;

    org_map = restore_value(memory_text);
    convert_to_save_mapping(&org_map);
    return save_string(org_map);

}

// --------------------------------------------------------------------
// @code  nc=15, nargs=1, insts=15
// --------------------------------------------------------------------
mixed convert_to_memory_data(string save_text) {
    /* ---- Enhanced Pseudocode ---- */
    mixed new_map;

    new_map = restore_value(save_text);
    convert_to_memory_mapping(&new_map);
    return save_string(new_map);

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=16, nargs=2, insts=16
// --------------------------------------------------------------------
private void set_convert_key(mixed org_key, int new_key) {
    /* ---- Enhanced Pseudocode ---- */
    org_key_map[org_key] = new_key;
    new_key_map[new_key] = org_key;

}

// --------------------------------------------------------------------
// @code  nc=49, nargs=1, insts=49
// --------------------------------------------------------------------
mixed convert_to_memory_mapping(mapping &save_mapping) {
    /* ---- Enhanced Pseudocode ---- */
    int new_key;
    int org_key;
    mixed org_value;

    loop (new_key in keys(save_mapping)) {
        org_value = save_mapping[new_key];
        if (is_mapping(org_value)) {
            convert_to_memory_mapping(&org_value);
        }
        if (is_string(org_key = new_key_map[new_key])) {
            save_mapping[new_key] = 0;
            save_mapping[org_key] = org_value;
        }
    }  // end loop (verified)
    clean_mapping_by_value(save_mapping);
    return save_mapping;

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=54, nargs=0, insts=54
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/datasaved.c"->register_entry("purchase", this_object());
    set_convert_key("need_data", 1);
    set_convert_key("order_id", 2);
    set_convert_key("owner", 3);
    set_convert_key("type", 4);
    set_convert_key("amount", 5);
    set_convert_key("sub_type", 6);
    set_convert_key("end_time", 7);
    set_convert_key("price", 8);
    set_convert_key("sub_amount", 9);
    set_convert_key("name", 10);
    set_convert_key("tax", 11);

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        6
   Code segments found:         6
   Successfully decoded:        6
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          1
      - /feature/common/dbase.c()
   ================================================================ */