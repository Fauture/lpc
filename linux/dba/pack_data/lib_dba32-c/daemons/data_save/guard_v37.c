/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: guard.o
 * Source: /daemons/data_save/guard.c
 * Inherits: (none)
 * Inherited programs: /feature/common/dbase.c()
 * Functions: 6 names, 6 code segments found
 * Strings: 112, Variables: 4
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

   org_key_map = ([ ]);  // /daemons/data_save/guard.c:190
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
// @code  nc=27, nargs=1, insts=27
// --------------------------------------------------------------------
mixed convert_to_save_data(string memory_text) {
    /* ---- Enhanced Pseudocode ---- */
    mapping org_map;
    mixed attrib_map;

    org_map = restore_value(memory_text);
    attrib_map = org_map["attrib"];
    if ((!is_mapping(attrib_map))) {
        return memory_text;
    }
    convert_to_save_mapping(&attrib_map);
    return save_string(org_map);

}

// --------------------------------------------------------------------
// @code  nc=27, nargs=1, insts=27
// --------------------------------------------------------------------
mixed convert_to_memory_data(string save_text) {
    /* ---- Enhanced Pseudocode ---- */
    mapping new_map;
    mixed attrib_map;

    new_map = restore_value(save_text);
    attrib_map = new_map["attrib"];
    if ((!is_mapping(attrib_map))) {
        return save_text;
    }
    convert_to_memory_mapping(&attrib_map);
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
// @code  nc=442, nargs=0, insts=442
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/datasaved.c"->register_entry("guard", this_object());
    set_convert_key("resist_earth", 1);
    set_convert_key("life", 2);
    set_convert_key("def_effect", 3);
    set_convert_key("phy_effect", 5);
    set_convert_key("resist_wood", 6);
    set_convert_key("exp_to_next_level", 7);
    set_convert_key("longevity", 8);
    set_convert_key("con", 9);
    set_convert_key("resist_forgotten", 10);
    set_convert_key("str", 11);
    set_convert_key("wiz", 12);
    set_convert_key("resist_poison", 13);
    set_convert_key("pet_life_shape", 14);
    set_convert_key("dex", 15);
    set_convert_key("def", 16);
    set_convert_key("special_skill", 17);
    set_convert_key("sub_type", 18);
    set_convert_key("init", 19);
    set_convert_key("resist_metal", 20);
    set_convert_key("phy_power", 21);
    set_convert_key("mag_power", 22);
    set_convert_key("attack_eff", 23);
    set_convert_key("pet_mana_shape", 24);
    set_convert_key("init_level", 25);
    set_convert_key("resist_already", 26);
    set_convert_key("monster_type", 27);
    set_convert_key("shape", 28);
    set_convert_key("resist_confusion", 29);
    set_convert_key("dodge", 30);
    set_convert_key("mag_effect", 31);
    set_convert_key("resist_sleep", 32);
    set_convert_key("iid", 33);
    set_convert_key("resist_point", 34);
    set_convert_key("rank", 35);
    set_convert_key("speed", 36);
    set_convert_key("mana", 37);
    set_convert_key("resist_water", 38);
    set_convert_key("name", 39);
    set_convert_key("accurate", 40);
    set_convert_key("pet_mag_shape", 41);
    set_convert_key("attrib_point", 42);
    set_convert_key("resist_fire", 43);
    set_convert_key("level", 44);
    set_convert_key("pet_phy_shape", 45);
    set_convert_key("resist_frozen", 46);
    set_convert_key("life_effect", 47);
    set_convert_key("max_life", 48);
    set_convert_key("exp", 49);
    set_convert_key("pet_speed_shape", 50);
    set_convert_key("max_mana", 51);
    set_convert_key("speed_effect", 52);
    set_convert_key("mana_effect", 53);
    set_convert_key("loyalty", 54);
    set_convert_key("gender", 55);
    set_convert_key("combat_guard", 56);
    set_convert_key("mag_absorb", 57);
    set_convert_key("attack_effect", 58);
    set_convert_key("use_skill", 59);
    set_convert_key("phy_absorb", 60);
    set_convert_key("icon", 61);
    set_convert_key("polar", 62);
    set_convert_key("money", 63);
    set_convert_key("tao_effect", 64);
    set_convert_key("no_cost_mana", 65);
    set_convert_key("power", 66);
    set_convert_key("intimacy", 67);
    set_convert_key("tao", 68);
    set_convert_key("appear_icon", 69);
    set_convert_key("fake_suit_icon", 70);
    set_convert_key("stunt_rate", 71);
    set_convert_key("portrait", 72);
    set_convert_key("recognize", 73);
    set_convert_key("recognized", 74);
    set_convert_key("double_hit_rate", 75);
    set_convert_key("double_hit", 76);
    set_convert_key("guard_damage", 77);
    set_convert_key("jinguang-zhaxian", 78);
    set_convert_key("daoguang-jianying", 79);
    set_convert_key("jinhong-guanri", 80);
    set_convert_key("liuguang-yicai", 81);
    set_convert_key("nitian-canren", 82);
    set_convert_key("zhaiye-feihua", 83);
    set_convert_key("feiliu-xianshi", 84);
    set_convert_key("pangen-cuojie", 85);
    set_convert_key("luoying-binfen", 86);
    set_convert_key("guiwu-kuteng", 87);
    set_convert_key("dishui-chuanshi", 88);
    set_convert_key("yuhen-yunchou", 89);
    set_convert_key("xuanhe-xieshui", 90);
    set_convert_key("nubo-kuangtao", 91);
    set_convert_key("jiaohai-fanjiang", 92);
    set_convert_key("juhuo-fentian", 93);
    set_convert_key("xinghuo-liaoyuan", 94);
    set_convert_key("yantian-huoyu", 95);
    set_convert_key("jiaojin-lishi", 96);
    set_convert_key("lianyu-huohai", 97);
    set_convert_key("luotu-feiyan", 98);
    set_convert_key("tumo-chenmai", 99);
    set_convert_key("shanbeng-dilie", 100);
    set_convert_key("tianta-dixian", 101);
    set_convert_key("shipo-tianjing", 102);
    set_convert_key("ignore_phy_absorb", 103);
    set_convert_key("religion", 104);
    set_convert_key("rec_intimacy", 105);
    set_convert_key("repair_ver", 106);
    set_convert_key("vigour", 107);
    set_convert_key("max_vigour", 108);
    set_convert_key("tactics_type", 109);

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