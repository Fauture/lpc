/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: user.o
 * Source: /daemons/data_save/user.c
 * Inherits: (none)
 * Functions: 5 names, 5 code segments found
 * Strings: 24, Variables: 0
 *********************************************************************/

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed convert_to_save_mapping(mapping &memory_map);
mixed convert_to_save_data(string memory_text);
mixed convert_to_memory_data(string save_text);
mixed convert_to_memory_mapping(mapping &save_map);
private void create();

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=273, nargs=1, insts=273
// --------------------------------------------------------------------
mixed convert_to_save_mapping(mapping &memory_map) {
    /* ---- Enhanced Pseudocode ---- */
    int pos;
    mixed mp;
    mixed mi;
    mixed mu;
    mixed mpp;
    mapping m_ob;
    mixed m_data;
    mixed item_convert_ob;
    mixed pet_convert_ob;
    mixed guard_convert_ob;

    item_convert_ob = "/daemons/datasaved.c"->query_entry("item");
    pet_convert_ob = "/daemons/datasaved.c"->query_entry("monster");
    guard_convert_ob = "/daemons/datasaved.c"->query_entry("guard");
    if (is_mapping(mp = memory_map["pets"])) {
        loop (pos in keys(mp)) {
            if ((!is_mapping(m_ob = mp[pos]))) {
                continue;
            }
            m_data = m_ob["data"];
            pet_convert_ob->convert_to_save_mapping(&m_data);
            mp[pos] = sprintf("%s:%s:%s:%s", m_ob["name"], m_ob["combat"], m_ob["visible"], save_string(m_data));
        }  // end loop (verified)
    }
    if (is_mapping(mpp = memory_map["practice_pets"])) {
        loop (pos in keys(mpp)) {
            if ((!is_mapping(m_ob = mpp[pos]))) {
                continue;
            }
            m_data = m_ob["data"];
            pet_convert_ob->convert_to_save_mapping(&m_data);
            mpp[pos] = sprintf("%s:%s", m_ob["name"], save_string(m_data));
        }  // end loop (verified)
    }
    if (is_mapping(mi = memory_map["carry"])) {
        loop (pos in keys(mi)) {
            if ((!is_mapping(m_ob = mi[pos]))) {
                continue;
            }
            m_data = m_ob["data"];
            item_convert_ob->convert_to_save_mapping(&m_data);
            mi[pos] = sprintf("%s:%s", m_ob["name"], save_string(m_data));
        }  // end loop (verified)
    }
    if (is_mapping(mp = memory_map["guards"])) {
        loop (pos in keys(mp)) {
            if ((!is_mapping(m_ob = mp[pos]))) {
                continue;
            }
            m_data = m_ob["data"];
            guard_convert_ob->convert_to_save_mapping(&m_data);
            mp[pos] = sprintf("%s:%s", m_ob["combat"], save_string(m_data));
        }  // end loop (verified)
    }
    if (is_mapping(mu = memory_map["upgrade_equip"])) {
        loop (pos in keys(mu)) {
            if ((!is_mapping(m_ob = mu[pos]))) {
                continue;
            }
            m_data = m_ob["data"];
            item_convert_ob->convert_to_save_mapping(&m_data);
            mu[pos] = sprintf("%s:%s", m_ob["name"], save_string(m_data));
        }  // end loop (verified)
    }
    return memory_map;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=274, nargs=1, insts=274
// --------------------------------------------------------------------
mixed convert_to_save_data(string memory_text) {
    /* ---- Enhanced Pseudocode ---- */
    int pos;
    mixed type;
    mixed item_convert_ob;
    mixed pet_convert_ob;
    mixed guard_convert_ob;
    mixed data;
    mixed name;
    mixed combat;
    mixed visible;
    mixed converted_data;
    mapping memory_map;
    mixed mp;
    mixed mi;
    mixed ms;
    mixed mu;

    item_convert_ob = "/daemons/datasaved.c"->query_entry("item");
    pet_convert_ob = "/daemons/datasaved.c"->query_entry("monster");
    guard_convert_ob = "/daemons/datasaved.c"->query_entry("guard");
    memory_map = restore_value(memory_text);
    if (is_mapping(mp = memory_map["pets"])) {
        loop (pos in keys(mp)) {
            data = mp[pos];
            if ((sscanf(data, "%s:%s:%s:%s", &name, &combat, &visible, &data) != 4)) {
                continue;
            }
            converted_data = pet_convert_ob->convert_to_save_data(data);
            mp[pos] = sprintf("%s:%s:%s:%s", name, combat, visible, converted_data);
        }  // end loop (verified)
    }
    if (is_mapping(mi = memory_map["carry"])) {
        loop (pos in keys(mi)) {
            data = mi[pos];
            if ((sscanf(data, "%s:%s", &name, &data) != 2)) {
                continue;
            }
            converted_data = item_convert_ob->convert_to_save_data(data);
            mi[pos] = sprintf("%s:%s", name, converted_data);
        }  // end loop (verified)
    }
    if (is_mapping(mu = memory_map["upgrade_equip"])) {
        loop (pos in keys(mu)) {
            data = mu[pos];
            if ((sscanf(data, "%s:%s", &name, &data) != 2)) {
                continue;
            }
            converted_data = item_convert_ob->convert_to_save_data(data);
            mu[pos] = sprintf("%s:%s", name, converted_data);
        }  // end loop (verified)
    }
    if (is_mapping(ms = memory_map["stall"])) {
        loop (pos in keys(ms)) {
            data = ms[pos];
            if ((sscanf(data, "%d:%s:%s", &type, &name, &data) != 3)) {
                continue;
            }
            converted_data = item_convert_ob->convert_to_save_data(data);
            ms[pos] = sprintf("%d:%s:%s", type, name, converted_data);
        }  // end loop (verified)
    }
    if (is_mapping(mp = memory_map["guards"])) {
        loop (pos in keys(mp)) {
            data = mp[pos];
            if ((sscanf(data, "%s:%s", &combat, &data) != 2)) {
                continue;
            }
            converted_data = guard_convert_ob->convert_to_save_data(data);
            mp[pos] = sprintf("%s:%s", combat, converted_data);
        }  // end loop (verified)
    }
    return save_string(memory_map);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=274, nargs=1, insts=274
// --------------------------------------------------------------------
mixed convert_to_memory_data(string save_text) {
    /* ---- Enhanced Pseudocode ---- */
    int pos;
    mixed type;
    mixed item_convert_ob;
    mixed pet_convert_ob;
    mixed guard_convert_ob;
    mixed data;
    mixed name;
    mixed combat;
    mixed visible;
    mixed converted_data;
    mapping save_map;
    mixed mp;
    mixed mi;
    mixed ms;
    mixed mu;

    item_convert_ob = "/daemons/datasaved.c"->query_entry("item");
    pet_convert_ob = "/daemons/datasaved.c"->query_entry("monster");
    guard_convert_ob = "/daemons/datasaved.c"->query_entry("guard");
    save_map = restore_value(save_text);
    if (is_mapping(mp = save_map["pets"])) {
        loop (pos in keys(mp)) {
            data = mp[pos];
            if ((sscanf(data, "%s:%s:%s:%s", &name, &combat, &visible, &data) != 4)) {
                continue;
            }
            converted_data = pet_convert_ob->convert_to_memory_data(data);
            mp[pos] = sprintf("%s:%s:%s:%s", name, combat, visible, converted_data);
        }  // end loop (verified)
    }
    if (is_mapping(mi = save_map["carry"])) {
        loop (pos in keys(mi)) {
            data = mi[pos];
            if ((sscanf(data, "%s:%s", &name, &data) != 2)) {
                continue;
            }
            converted_data = item_convert_ob->convert_to_memory_data(data);
            mi[pos] = sprintf("%s:%s", name, converted_data);
        }  // end loop (verified)
    }
    if (is_mapping(mu = save_map["upgrade_equip"])) {
        loop (pos in keys(mu)) {
            data = mu[pos];
            if ((sscanf(data, "%s:%s", &name, &data) != 2)) {
                continue;
            }
            converted_data = item_convert_ob->convert_to_memory_data(data);
            mu[pos] = sprintf("%s:%s", name, converted_data);
        }  // end loop (verified)
    }
    if (is_mapping(ms = save_map["stall"])) {
        loop (pos in keys(ms)) {
            data = ms[pos];
            if ((sscanf(data, "%d:%s:%s", &type, &name, &data) != 3)) {
                continue;
            }
            converted_data = item_convert_ob->convert_to_memory_data(data);
            ms[pos] = sprintf("%d:%s:%s", type, name, converted_data);
        }  // end loop (verified)
    }
    if (is_mapping(mp = save_map["guards"])) {
        loop (pos in keys(mp)) {
            data = mp[pos];
            if ((sscanf(data, "%s:%s", &combat, &data) != 2)) {
                continue;
            }
            converted_data = guard_convert_ob->convert_to_memory_data(data);
            mp[pos] = sprintf("%s:%s", combat, converted_data);
        }  // end loop (verified)
    }
    return save_string(save_map);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=288, nargs=1, insts=288
// --------------------------------------------------------------------
mixed convert_to_memory_mapping(mapping &save_map) {
    /* ---- Enhanced Pseudocode ---- */
    int pos;
    mixed data;
    mixed name;
    mixed combat;
    mixed visible;
    mixed mp;
    mixed mi;
    mixed mu;
    mixed mpp;
    mixed item_convert_ob;
    mixed pet_convert_ob;
    mixed guard_convert_ob;

    item_convert_ob = "/daemons/datasaved.c"->query_entry("item");
    pet_convert_ob = "/daemons/datasaved.c"->query_entry("monster");
    guard_convert_ob = "/daemons/datasaved.c"->query_entry("guard");
    if (is_mapping(mp = save_map["pets"])) {
        loop (pos in keys(mp)) {
            data = mp[pos];
            if ((sscanf(data, "%s:%s:%s:%s", &name, &combat, &visible, &data) != 4)) {
                continue;
            }
            data = restore_value(data);
            pet_convert_ob->convert_to_memory_mapping(&data);
            mp[pos] = ([ "name": name, "combat": combat, "visible": visible, "data": data ]);
        }  // end loop (verified)
    }
    if (is_mapping(mpp = save_map["practice_pets"])) {
        loop (pos in keys(mpp)) {
            data = mpp[pos];
            if ((sscanf(data, "%s:%s", &name, &data) != 2)) {
                continue;
            }
            data = restore_value(data);
            pet_convert_ob->convert_to_memory_mapping(&data);
            mpp[pos] = ([ "name": name, "data": data ]);
        }  // end loop (verified)
    }
    if (is_mapping(mi = save_map["carry"])) {
        loop (pos in keys(mi)) {
            data = mi[pos];
            if ((sscanf(data, "%s:%s", &name, &data) != 2)) {
                continue;
            }
            data = restore_value(data);
            item_convert_ob->convert_to_memory_mapping(&data);
            mi[pos] = ([ "name": name, "data": data ]);
        }  // end loop (verified)
    }
    if (is_mapping(mp = save_map["guards"])) {
        loop (pos in keys(mp)) {
            data = mp[pos];
            if ((sscanf(data, "%s:%s", &combat, &data) != 2)) {
                continue;
            }
            data = restore_value(data);
            guard_convert_ob->convert_to_memory_mapping(&data);
            mp[pos] = ([ "combat": combat, "data": data ]);
        }  // end loop (verified)
    }
    if (is_mapping(mu = save_map["upgrade_equip"])) {
        loop (pos in keys(mu)) {
            data = mu[pos];
            if ((sscanf(data, "%s:%s", &name, &data) != 2)) {
                continue;
            }
            data = restore_value(data);
            item_convert_ob->convert_to_memory_mapping(&data);
            mu[pos] = ([ "name": name, "data": data ]);
        }  // end loop (verified)
    }
    return save_map;

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=10, nargs=0, insts=10
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/datasaved.c"->register_entry("user", this_object());

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