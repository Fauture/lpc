/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: item.o
 * Source: /daemons/data_save/item.c
 * Inherits: (none)
 * Inherited programs: /feature/common/dbase.c()
 * Functions: 6 names, 6 code segments found
 * Strings: 236, Variables: 4
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

   org_key_map = ([ ]);  // /daemons/data_save/item.c:308
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
// @code  nc=942, nargs=0, insts=942
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/datasaved.c"->register_entry("item", this_object());
    set_convert_key("extra_desc", 1);
    set_convert_key("str", 2);
    set_convert_key("phy_power", 3);
    set_convert_key("accurate", 4);
    set_convert_key("con", 5);
    set_convert_key("life", 6);
    set_convert_key("max_life", 7);
    set_convert_key("def", 8);
    set_convert_key("penetrate", 9);
    set_convert_key("wiz", 10);
    set_convert_key("mag_power", 11);
    set_convert_key("mana", 12);
    set_convert_key("max_mana", 13);
    set_convert_key("dex", 14);
    set_convert_key("speed", 15);
    set_convert_key("parry", 16);
    set_convert_key("polar", 17);
    set_convert_key("attrib_point", 18);
    set_convert_key("polar_point", 19);
    set_convert_key("stamina", 20);
    set_convert_key("max_stamina", 21);
    set_convert_key("tao", 22);
    set_convert_key("friend", 23);
    set_convert_key("exp", 24);
    set_convert_key("pot", 25);
    set_convert_key("level", 26);
    set_convert_key("name", 27);
    set_convert_key("desc", 28);
    set_convert_key("metal", 30);
    set_convert_key("wood", 31);
    set_convert_key("water", 32);
    set_convert_key("fire", 33);
    set_convert_key("earth", 34);
    set_convert_key("durability", 35);
    set_convert_key("resist_metal", 36);
    set_convert_key("resist_wood", 37);
    set_convert_key("resist_water", 39);
    set_convert_key("resist_fire", 40);
    set_convert_key("resist_earth", 41);
    set_convert_key("resist_poison", 42);
    set_convert_key("resist_frozen", 43);
    set_convert_key("resist_sleep", 44);
    set_convert_key("resist_forgotten", 45);
    set_convert_key("resist_confusion", 46);
    set_convert_key("amount", 47);
    set_convert_key("attrib", 48);
    set_convert_key("value", 49);
    set_convert_key("resist_point", 50);
    set_convert_key("double_hit", 51);
    set_convert_key("stunt", 52);
    set_convert_key("counter_attack", 53);
    set_convert_key("rebuild_level", 54);
    set_convert_key("color", 55);
    set_convert_key("counter_attack_rate", 56);
    set_convert_key("double_hit_rate", 57);
    set_convert_key("stunt_rate", 58);
    set_convert_key("damage_sel", 59);
    set_convert_key("damage_sel_rate", 60);
    set_convert_key("item_type", 61);
    set_convert_key("exp_to_next_level", 62);
    set_convert_key("item_unique", 63);
    set_convert_key("all_attrib", 64);
    set_convert_key("all_polar", 65);
    set_convert_key("all_resist_polar", 66);
    set_convert_key("all_resist_except", 67);
    set_convert_key("all_skill", 68);
    set_convert_key("req_con", 69);
    set_convert_key("req_wiz", 70);
    set_convert_key("req_str", 71);
    set_convert_key("req_dex", 72);
    set_convert_key("penetrate_rate", 73);
    set_convert_key("nimbus", 74);
    set_convert_key("extra_life", 75);
    set_convert_key("extra_mana", 76);
    set_convert_key("metal_mag_power", 77);
    set_convert_key("wood_mag_power", 78);
    set_convert_key("water_mag_power", 79);
    set_convert_key("fire_mag_power", 80);
    set_convert_key("earth_mag_power", 81);
    set_convert_key("card_level", 82);
    set_convert_key("max_card_amount", 83);
    set_convert_key("card_amount", 84);
    set_convert_key("ignore_resist_metal", 85);
    set_convert_key("ignore_resist_wood", 86);
    set_convert_key("ignore_resist_water", 87);
    set_convert_key("ignore_resist_fire", 88);
    set_convert_key("ignore_resist_earth", 89);
    set_convert_key("ignore_resist_forgotten", 90);
    set_convert_key("ignore_resist_poison", 91);
    set_convert_key("ignore_resist_frozen", 92);
    set_convert_key("ignore_resist_sleep", 93);
    set_convert_key("ignore_resist_confusion", 94);
    set_convert_key("super_excluse_metal", 95);
    set_convert_key("super_excluse_wood", 96);
    set_convert_key("super_excluse_water", 97);
    set_convert_key("super_excluse_fire", 98);
    set_convert_key("super_excluse_earth", 99);
    set_convert_key("B_skill_low_cost", 100);
    set_convert_key("C_skill_low_cost", 101);
    set_convert_key("D_skill_low_cost", 102);
    set_convert_key("super_poison", 103);
    set_convert_key("super_sleep", 104);
    set_convert_key("super_forgotten", 105);
    set_convert_key("super_confusion", 106);
    set_convert_key("super_frozen", 107);
    set_convert_key("enhanced_metal", 108);
    set_convert_key("enhanced_wood", 109);
    set_convert_key("enhanced_water", 110);
    set_convert_key("enhanced_fire", 111);
    set_convert_key("enhanced_earth", 112);
    set_convert_key("mag_dodge", 113);
    set_convert_key("jinguang-zhaxian_counter_att_rate", 114);
    set_convert_key("zhaiye-feihua_counter_att_rate", 115);
    set_convert_key("dishui-chuanshi_counter_att_rate", 116);
    set_convert_key("juhuo-fentian_counter_att_rate", 117);
    set_convert_key("luotu-feiyan_counter_att_rate", 118);
    set_convert_key("ignore_all_resist_polar", 119);
    set_convert_key("ignore_all_resist_except", 200);
    set_convert_key("release_forgotten", 201);
    set_convert_key("release_poison", 202);
    set_convert_key("release_frozen", 203);
    set_convert_key("release_sleep", 204);
    set_convert_key("release_confusion", 205);
    set_convert_key("source", 206);
    set_convert_key("quality", 207);
    set_convert_key("life_effect", 208);
    set_convert_key("mana_effect", 209);
    set_convert_key("attack_effect", 210);
    set_convert_key("speed_effect", 211);
    set_convert_key("phy_effect", 212);
    set_convert_key("mag_effect", 213);
    set_convert_key("phy_absorb", 214);
    set_convert_key("mag_absorb", 215);
    set_convert_key("add_max_life", 216);
    set_convert_key("add_max_mana", 217);
    set_convert_key("add_user_level", 218);
    set_convert_key("add_random_skill", 219);
    set_convert_key("phy_rebuild_level", 220);
    set_convert_key("mag_rebuild_level", 221);
    set_convert_key("upgrade_type", 222);
    set_convert_key("locked", 223);
    set_convert_key("suit_polar", 224);
    set_convert_key("suit_enabled", 225);
    set_convert_key("gift", 226);
    set_convert_key("suit_name", 227);
    set_convert_key("suit_level", 228);
    set_convert_key("prop", 229);
    set_convert_key("prop1", 230);
    set_convert_key("prop2", 231);
    set_convert_key("position", 232);
    set_convert_key("iid", 233);
    set_convert_key("prop3", 234);
    set_convert_key("prop_suit", 235);
    set_convert_key("prop4", 236);
    set_convert_key("prop_recg", 237);
    set_convert_key("ignore_double_hit_rate", 238);
    set_convert_key("ignore_stunt_rate", 239);
    set_convert_key("seal_flag", 240);
    set_convert_key("prop_seal", 241);
    set_convert_key("prop_seal_life", 242);
    set_convert_key("seal_icon", 243);
    set_convert_key("ignore_mag_dodge", 244);
    set_convert_key("enhanced_phy", 245);
    set_convert_key("prop_bind", 246);
    set_convert_key("basic_prop", 247);
    set_convert_key("evolve_level", 248);
    set_convert_key("prop_seal_skill", 249);
    set_convert_key("extra_upgrade_attrib", 250);
    set_convert_key("upgrade_com", 251);
    set_convert_key("super_obstruct", 252);
    set_convert_key("prop_rebuild", 253);
    set_convert_key("prop_seal_base", 254);
    set_convert_key("repair_ver", 255);
    set_convert_key("perfect_degree", 256);
    set_convert_key("property_bind", 257);
    set_convert_key("rebuild_cumulate", 258);
    set_convert_key("req_level", 259);
    set_convert_key("identified", 260);
    set_convert_key("prop2_color", 261);
    set_convert_key("recognize", 262);
    set_convert_key("recognized", 263);
    set_convert_key("org_prop_suit", 264);
    set_convert_key("suit_evolve_num", 265);
    set_convert_key("limit_can_exchange", 266);
    set_convert_key("min_req_level", 267);
    set_convert_key("max_req_level", 268);
    set_convert_key("super_light", 269);
    set_convert_key("mstunt_rate", 270);
    set_convert_key("ignore_mstunt_rate", 271);
    set_convert_key("prop_resonance", 272);
    set_convert_key("provenance", 273);
    set_convert_key("ignore_phy_absorb", 274);
    set_convert_key("rank_desc", 275);
    set_convert_key("seal", 276);
    set_convert_key("seal_prop", 277);
    set_convert_key("item_polar", 278);
    set_convert_key("rate", 279);
    set_convert_key("friend_score", 280);
    set_convert_key("master", 281);
    set_convert_key("icon", 282);
    set_convert_key("important", 283);
    set_convert_key("resist_repress", 284);
    set_convert_key("resist_melt", 285);
    set_convert_key("resist_cage", 286);
    set_convert_key("resist_lock", 287);
    set_convert_key("resist_lost", 288);
    set_convert_key("ignore_resist_repress", 289);
    set_convert_key("ignore_resist_melt", 290);
    set_convert_key("ignore_resist_cage", 291);
    set_convert_key("ignore_resist_lock", 292);
    set_convert_key("ignore_resist_lost", 293);
    set_convert_key("release_repress", 294);
    set_convert_key("release_melt", 295);
    set_convert_key("release_cage", 296);
    set_convert_key("release_lock", 297);
    set_convert_key("release_lost", 298);
    set_convert_key("super_repress", 299);
    set_convert_key("super_melt", 300);
    set_convert_key("super_cage", 301);
    set_convert_key("super_lock", 302);
    set_convert_key("super_lost", 303);
    set_convert_key("religion_convert_end_time", 304);
    set_convert_key("base_dodge", 305);
    set_convert_key("stun_impr", 306);
    set_convert_key("org_owner", 307);
    set_convert_key("own_ti", 308);
    set_convert_key("origin", 309);
    set_convert_key("last_exchange_owner", 310);
    set_convert_key("last_exchange_time", 311);
    set_convert_key("rec_intimacy", 312);
    set_convert_key("prop_basic", 313);
    set_convert_key("prop_extra", 314);
    set_convert_key("prop_basic_cp", 315);

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