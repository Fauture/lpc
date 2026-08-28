/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: party_members.o
 * Source: /daemons/data_save/party_members.c
 * Inherits: (none)
 * Inherited programs: /feature/common/dbase.c()
 * Functions: 8 names, 8 code segments found
 * Strings: 40, Variables: 4
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

private mixed get_org_key(int new_key);
mixed convert_to_save_mapping(mapping &memory_mapping);
mixed convert_to_save_data(string memory_text);
private mixed get_new_key(mixed org_key);
mixed convert_to_memory_data(string save_text);
private void set_convert_key(mixed org_key, int new_key);
mixed convert_to_memory_mapping(mapping &save_mapping);
private void create();

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   org_key_map = ([ ]);  // /daemons/data_save/party_members.c:164
   new_key_map = ([ ]);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
private mixed get_org_key(int new_key) {
    /* ---- Enhanced Pseudocode ---- */
    return new_key_map[new_key];

}

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
// @code  nc=62, nargs=1, insts=62
// --------------------------------------------------------------------
mixed convert_to_save_data(string memory_text) {
    /* ---- Enhanced Pseudocode ---- */
    mixed org_map;
    mapping m;
    int gid;
    mixed new_key;

    org_map = restore_value(memory_text);
    if ((!is_mapping(org_map))) {
        return memory_text;
    }
    loop (gid in keys(org_map)) {
        m = org_map[gid];
        if ((!is_mapping(m))) {
            continue;
        }
        if (is_string(m["family"])) {
            new_key = get_new_key(m["family"]);
            if (is_int(new_key)) {
                m["family"] = new_key;
            }
        }
        convert_to_save_mapping(&m);
    }  // end loop (verified)
    return save_string(org_map);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
private mixed get_new_key(mixed org_key) {
    /* ---- Enhanced Pseudocode ---- */
    return org_key_map[org_key];

}

// --------------------------------------------------------------------
// @code  nc=63, nargs=1, insts=63
// --------------------------------------------------------------------
mixed convert_to_memory_data(string save_text) {
    /* ---- Enhanced Pseudocode ---- */
    mixed new_map;
    mapping m;
    int gid;
    mixed org_key;

    new_map = restore_value(save_text);
    if ((!is_mapping(new_map))) {
        return save_text;
    }
    loop (gid in keys(new_map)) {
        m = new_map[gid];
        if ((!is_mapping(m))) {
            continue;
        }
        convert_to_memory_mapping(&m);
        if ((!is_int(m["family"]))) {
            continue;
        }
        org_key = get_org_key(m["family"]);
        if (is_string(org_key)) {
            m["family"] = org_key;
        }
    }  // end loop (verified)
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
// @code  nc=158, nargs=0, insts=158
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/datasaved.c"->register_entry("party_members", this_object());
    set_convert_key("party_job", 1);
    set_convert_key("gender", 2);
    set_convert_key("polar", 3);
    set_convert_key("total_score", 4);
    set_convert_key("party_desc", 5);
    set_convert_key("level", 6);
    set_convert_key("logout_time", 7);
    set_convert_key("family", 8);
    set_convert_key("title", 9);
    set_convert_key("reputation", 10);
    set_convert_key("name", 11);
    set_convert_key("party_activity", 12);
    set_convert_key("ÎåÁúÉ½ÔÆÏö¶´", 13);
    set_convert_key("ÖÕÄÏÉ½ÓñÖù¶´", 14);
    set_convert_key("·ï»ËÉ½¶·ãÚ¹¬", 15);
    set_convert_key("Ç¬ÔªÉ½½ğ¹â¶´", 16);
    set_convert_key("÷¼÷ÃÉ½°×¹Ç¶´", 17);
    set_convert_key("total_died", 20);
    set_convert_key("icon", 21);
    set_convert_key("party_contrib", 22);
    set_convert_key("portrait", 23);
    set_convert_key("benediction", 24);
    set_convert_key("join_time", 25);
    set_convert_key("value", 26);
    set_convert_key("ti_last_add", 27);
    set_convert_key("contributor", 28);
    set_convert_key("start_time", 29);
    set_convert_key("week_act", 30);
    set_convert_key("cm_act", 31);
    set_convert_key("lm_act", 32);
    set_convert_key("lw_salary", 33);
    set_convert_key("salary_rate", 34);
    set_convert_key("power", 35);
    set_convert_key("add_act_ti", 36);
    set_convert_key("war_times", 37);
    set_convert_key("last_war", 38);
    set_convert_key("lw_act", 39);

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        8
   Code segments found:         8
   Successfully decoded:        8
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          1
      - /feature/common/dbase.c()
   ================================================================ */