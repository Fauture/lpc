/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: monster.o
 * Source: /daemons/data_save/monster.c
 * Inherits: (none)
 * Inherited programs: /feature/common/dbase.c()
 * Functions: 6 names, 6 code segments found
 * Strings: 202, Variables: 4
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

   org_key_map = ([ ]);  // /daemons/data_save/monster.c:290
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
// @code  nc=39, nargs=1, insts=39
// --------------------------------------------------------------------
mixed convert_to_save_data(string memory_text) {
    /* ---- Enhanced Pseudocode ---- */
    mapping org_map;
    mixed attrib_map;

    org_map = restore_value(memory_text);
    if ((!is_mapping(attrib_map = org_map["attrib"]))) {
        return memory_text;
    }
    convert_to_save_mapping(&attrib_map);
    if ((!is_mapping(attrib_map = org_map["skills"]))) {
        return save_string(org_map);
    }
    convert_to_save_mapping(&attrib_map);
    return save_string(org_map);

}

// --------------------------------------------------------------------
// @code  nc=39, nargs=1, insts=39
// --------------------------------------------------------------------
mixed convert_to_memory_data(string save_text) {
    /* ---- Enhanced Pseudocode ---- */
    mapping new_map;
    mixed attrib_map;

    new_map = restore_value(save_text);
    if ((!is_mapping(attrib_map = new_map["attrib"]))) {
        return save_text;
    }
    convert_to_memory_mapping(&attrib_map);
    if ((!is_mapping(attrib_map = new_map["skills"]))) {
        return save_string(new_map);
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
// @code  nc=798, nargs=0, insts=798
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/datasaved.c"->register_entry("monster", this_object());
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
    set_convert_key("total", 55);
    set_convert_key("attrib_already", 56);
    set_convert_key("repair_ver", 57);
    set_convert_key("penetrate", 58);
    set_convert_key("full_extra_mana", 59);
    set_convert_key("godbook_skills", 60);
    set_convert_key("nimbus", 61);
    set_convert_key("pet_upgraded", 62);
    set_convert_key("intimacy", 63);
    set_convert_key("evolve", 64);
    set_convert_key("last_train_ti", 65);
    set_convert_key("basic_effect", 66);
    set_convert_key("feisheng_effect", 67);
    set_convert_key("locked", 68);
    set_convert_key("medicine", 69);
    set_convert_key("modify_effect", 70);
    set_convert_key("money", 71);
    set_convert_key("enchant", 72);
    set_convert_key("enchant_effect", 73);
    set_convert_key("extra_mana", 74);
    set_convert_key("martialfeed", 75);
    set_convert_key("full_extra_life", 76);
    set_convert_key("extra_life", 77);
    set_convert_key("last_apply", 78);
    set_convert_key("martial", 79);
    set_convert_key("medicine_used", 80);
    set_convert_key("sanqingwan", 81);
    set_convert_key("baohua-yuluwan", 82);
    set_convert_key("phy_absorb", 83);
    set_convert_key("upgrade", 84);
    set_convert_key("create_time", 85);
    set_convert_key("got_exp", 86);
    set_convert_key("train_times", 87);
    set_convert_key("req_level", 88);
    set_convert_key("top_data", 89);
    set_convert_key("mag_absorb", 90);
    set_convert_key("extra_attrib", 91);
    set_convert_key("init_attrib", 92);
    set_convert_key("has_upgraded", 93);
    set_convert_key("absorb_medicine_time", 94);
    set_convert_key("old_effect", 95);
    set_convert_key("jiuzhuan-xianlinglu", 96);
    set_convert_key("mag_rebuild_level", 97);
    set_convert_key("add_intimacy", 98);
    set_convert_key("recognized", 99);
    set_convert_key("recognize", 100);
    set_convert_key("join_combat", 101);
    set_convert_key("strengthen_effect", 102);
    set_convert_key("boss_monster", 103);
    set_convert_key("mount_attrib", 104);
    set_convert_key("mount_type", 105);
    set_convert_key("last_lv_up", 106);
    set_convert_key("mag", 107);
    set_convert_key("phy", 108);
    set_convert_key("initial_phy_effect", 109);
    set_convert_key("feedback", 110);
    set_convert_key("ti_last_apply", 111);
    set_convert_key("apply_num", 112);
    set_convert_key("capacity_level", 113);
    set_convert_key("ignore_def", 114);
    set_convert_key("ruyi-quan", 115);
    set_convert_key("juhuo-fentian", 116);
    set_convert_key("luotu-feiyan", 117);
    set_convert_key("sheming-yiji", 118);
    set_convert_key("tumo-chenmai", 119);
    set_convert_key("shiwan-huoji", 120);
    set_convert_key("feiliu-xianshi", 121);
    set_convert_key("mantian-xuewu", 122);
    set_convert_key("zhaiye-feihua", 123);
    set_convert_key("jiaojin-lishi", 124);
    set_convert_key("qiankun-zhao", 125);
    set_convert_key("tiansheng-shenli", 126);
    set_convert_key("bianchang-moji", 127);
    set_convert_key("fanzhuan-qiankun", 128);
    set_convert_key("tianta-dixian", 129);
    set_convert_key("shensheng-zhiguang", 130);
    set_convert_key("luoying-binfen", 131);
    set_convert_key("xinghuo-liaoyuan", 132);
    set_convert_key("dishui-chuanshi", 133);
    set_convert_key("yuhen-yunchou", 134);
    set_convert_key("fangwei-dujian", 135);
    set_convert_key("youshui-zhishe", 136);
    set_convert_key("jinguang-zhaxian", 137);
    set_convert_key("siwang-chanmian", 138);
    set_convert_key("nubo-kuangtao", 139);
    set_convert_key("bamiao-zhuzhang", 140);
    set_convert_key("daoguang-jianying", 141);
    set_convert_key("liuguang-yicai", 142);
    set_convert_key("shenlong-zhao", 143);
    set_convert_key("passive_skills", 144);
    set_convert_key("xiangfu-xiangcheng", 145);
    set_convert_key("jianren-buba", 146);
    set_convert_key("fali-pingzhang", 147);
    set_convert_key("shangru-gusui", 148);
    set_convert_key("liehun-jingpo", 149);
    set_convert_key("wujin-shixue", 150);
    set_convert_key("tiandao-lunhui", 151);
    set_convert_key("heian-lunhui", 152);
    set_convert_key("log_owner_gid", 153);
    set_convert_key("max_morph_stat", 154);
    set_convert_key("morph_mag_times", 155);
    set_convert_key("morph_mag_stat", 156);
    set_convert_key("morph_phy_times", 157);
    set_convert_key("morph_phy_stat", 158);
    set_convert_key("morph_speed_times", 159);
    set_convert_key("morph_speed_stat", 160);
    set_convert_key("morph_mana_times", 161);
    set_convert_key("morph_mana_stat", 162);
    set_convert_key("morph_life_times", 163);
    set_convert_key("morph_life_stat", 164);
    set_convert_key("pet_dress_inventory", 165);
    set_convert_key("pet_dress_info", 166);
    set_convert_key("eclosion", 167);
    set_convert_key("eclosion_nimbus", 168);
    set_convert_key("max_eclosion_nimbus", 169);
    set_convert_key("eclosion_rank", 170);
    set_convert_key("eclosion_effect", 171);
    set_convert_key("eclosion_name", 172);
    set_convert_key("dunwu_nimbus", 173);
    set_convert_key("dunwu_left_times", 174);
    set_convert_key("dunwu_total_times", 175);
    set_convert_key("ignore_phy_absorb", 176);
    set_convert_key("mount_category", 177);
    set_convert_key("resist_repress", 178);
    set_convert_key("resist_melt", 179);
    set_convert_key("resist_cage", 180);
    set_convert_key("resist_lock", 181);
    set_convert_key("resist_lost", 182);
    set_convert_key("phy_rebuild_level", 183);
    set_convert_key("phy_rebuild_rate", 184);
    set_convert_key("mag_rebuild_rate", 185);
    set_convert_key("dunwu_rate", 186);
    set_convert_key("rec_att", 187);
    set_convert_key("org_owner", 188);
    set_convert_key("own_ti", 189);
    set_convert_key("master", 190);
    set_convert_key("inherit_exp", 191);
    set_convert_key("last_exchange_owner", 192);
    set_convert_key("last_exchange_time", 193);
    set_convert_key("rec_intimacy", 194);
    set_convert_key("exp_ex", 195);
    set_convert_key("prop_basic", 196);
    set_convert_key("prop_extra", 197);
    set_convert_key("prop_basic_cp", 198);

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