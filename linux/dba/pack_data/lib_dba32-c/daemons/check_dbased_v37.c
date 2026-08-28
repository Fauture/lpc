/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: check_dbased.o
 * Source: /daemons/check_dbased.c
 * Inherits: (none)
 * Functions: 5 names, 5 code segments found
 * Strings: 28, Variables: 1
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   mapping dbase_config_xml;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void check_db_config(string dbase, int step = 2);
void release_data();
private void destruct();
private void load_db_config();
private void create();

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  validator: verify data
// @code  nc=384, nargs=2, insts=384
// --------------------------------------------------------------------
private void check_db_config(string dbase, int step) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    mixed cno;
    mixed vno;
    int fields_fail;
    mixed record_num;
    mixed mks;
    mixed mks_ex;
    mixed dbase_info;
    mixed fields;
    mixed fields_config;
    mixed field_arr;
    mixed msg;
    mixed msg_ex;
    mixed dbase_map;
    mixed table_map;
    mapping table;
    mixed fields_info;
    mapping fields_map;
    mapping m;

    if (((cno = "/daemons/dbd.c"->assure_connection(dbase)) < 0)) {
        return;
    }
    if ((1 == step)) {
        return;
    }
    dbase_map = dbase_config_xml["_childs_ex"];
    if (is_mapping(dbase_map)) {
    }
    if ((!is_array(dbase_info = dbase_map[(("ctdb" == dbase) ? "tdb" : dbase)]))) {
        return;
    }
    msg = (msg_ex = "");
    loop (table in dbase_info) {
        table_map = table["_childs_ex"];
        loop (mks in table_map) {
            if (((!is_array(mks)) || (!is_array(mks[1])))) {
                continue;
            }
            vno = (*f_query)(cno, sprintf("select * from %s limit 1", mks[0]));
            if ((vno == -1)) {
                msg += sprintf("the dbase %s.%s don't config\n", dbase, mks[0]);
                continue;
            }
            fields = (*f_field_name)(vno);
            (*f_finalize)(vno);
            if ((!sizeof(fields))) {
                msg += sprintf("the dbase %s.%s don't config\n", dbase, mks[0]);
                continue;
            }
            fields_fail = 0;
            fields_config = keys(mks[1][0]["_childs_ex"]);
            if (sizeof((field_arr = (fields - fields_config)))) {
                fields_fail = 1;
                msg += sprintf("the dbase %s.%s exist unnecessary fields %s!\n", dbase, mks[0], implode(field_arr, ","));
            }
            if (sizeof((field_arr = (fields_config - fields)))) {
                fields_fail = 1;
                msg += sprintf("the dbase %s.%s lack necessary fields %s!\n", dbase, mks[0], implode(field_arr, ","));
            }
            if (fields_fail) {
                continue;
            }
            fields_info = ([ ]);
            loop (mks_ex in mks[1][0]["_childs_ex"]) {
                loop (fields_map in mks_ex[1]) {
                    if (is_undefined(fields_map["field_length"])) {
                        fields_info[mks_ex[0]] = ([ "Type": sprintf("%s", fields_map["field_type"]), "Default": fields_map["field_default"] ]);
                    } else {
                        fields_info[mks_ex[0]] = ([ "Type": sprintf("%s(%d)", fields_map["field_type"], (int)fields_map["field_length"]), "Default": fields_map["field_default"] ]);
                    }
                }  // end loop (verified)
            }  // end loop (verified)
            vno = (*f_query)(cno, sprintf("describe %s", mks[0]));
            record_num = (*f_num_records)(vno);
            for (i = 0; i <= (record_num - 1); i++) {
                if (((*f_fetch_record)(vno) != 0)) {
                    break;
                }
                m = (*f_this_record)(vno);
                if ((is_mapping(fields_info[m["Field"]]) && (strsrch(m["Type"], (string)fields_info[m["Field"]]["Type"]) > -1))) {
                    continue;
                }
                try {
                    msg_ex += sprintf("the field %s.%s.%s \ndatabase_config£º%O, \nver_config£º%O\n\n", dbase, mks[0], m["Field"], m, fields_info[m["Field"]]);
                } catch { /* error handled */ }

                // --- label L0355 ---
            }  // end loop (verified)
        }  // end loop (verified)
    }  // end loop (verified)
    if (sizeof(msg)) {
        error("%O", msg);
    }
    if (sizeof(msg_ex)) {
        catch(error("%O", msg_ex));

        // --- label L0381 ---
    }

}

// --------------------------------------------------------------------
// @code  nc=7, nargs=0, insts=7
// --------------------------------------------------------------------
void release_data() {
    /* ---- Enhanced Pseudocode ---- */
    release_tixml_parse_ex(dbase_config_xml);

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    release_data();

}

// --------------------------------------------------------------------
// @code  nc=26, nargs=0, insts=26
// --------------------------------------------------------------------
private void load_db_config() {
    /* ---- Enhanced Pseudocode ---- */
    string dbase_config;

    dbase_config = "";
    dbase_config = read_etc_file("dbase_config.xml", "a", "/etc/");
    dbase_config_xml = tixml_parse(dbase_config);
    if (is_mapping(dbase_config_xml)) {
        tixml_parse_ex(dbase_config_xml);
    }

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=24, nargs=0, insts=24
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    require_uid();
    set_euid(0);
    load_db_config();
    if ((!is_mapping(dbase_config_xml))) {
        error("dbase_config.xml is bad! please check it!!!");
    }
    check_db_config("ddb");
    check_db_config("ldb");

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