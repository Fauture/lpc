/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_get_sys_envelope.o
 * Source: /dba/daemons/dbaserver/cmd_get_sys_envelope.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c(), /feature/daemon/eternal_data.c()
 * Functions: 10 names, 10 code segments found
 * Strings: 34, Variables: 4
 *********************************************************************/

inherit "/inherit/misc/handler.c";
inherit "/feature/daemon/eternal_data.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   mapping all_system_envelopes;
   /* mixed global_0; */
   /* mixed global_1; */
   int save_interval;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
private int apply_envelope_coin(string iid, int type, int cur_coin, int cur_num, mapping memo);
private int random_coin(int total_coin, int cur_coin, int total_num, int cur_num, int reward_type);
void save();
private int sort_as_coin(array gainer_info);
private string get_update_sql_cmd(mapping m);
private void create();
int main(mixed agent, int cookie, string iid, string user_name, string para);
private void record_a_gainer(mapping envelope, array gainer_info);
private void get_envelope(object agent, int cookie, string iid, mapping m, string user_name);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   all_system_envelopes = ([ ]);  // /dba/daemons/dbaserver/cmd_get_sys_envelope.c:364
   save_interval = 500;

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
mixed get_db_type() {
    /* ---- Enhanced Pseudocode ---- */
    return "ddb";

}

// --------------------------------------------------------------------
// @code  nc=92, nargs=5, insts=92
// --------------------------------------------------------------------
private int apply_envelope_coin(string iid, int type, int cur_coin, int cur_num, mapping memo) {
    /* ---- Enhanced Pseudocode ---- */
    mixed coin;
    mixed a;

    if ((1 == cur_num)) {
        return cur_coin;
    }
    if ((((cur_coin >= (50000 + (100 * (cur_num - 1)))) && is_array(memo["big_bonus"])) && (member_array((memo["used"] + 1), memo["big_bonus"]) != -1))) {
        return 50000;
    }
    coin = random_coin(memo["coin"], cur_coin, memo["amount"], cur_num, memo["sub_type"]);
    if ((coin >= ((memo["coin"] / memo["amount"]) * 2.95))) {
        a = (3000 + random(17000));
        if ((cur_coin >= (a + (100 * (cur_num - 1))))) {
            coin = a;
        }
    }
    return coin;

}

// --------------------------------------------------------------------
// @code  nc=34, nargs=5, insts=34
// --------------------------------------------------------------------
private int random_coin(int total_coin, int cur_coin, int total_num, int cur_num, int reward_type) {
    /* ---- Enhanced Pseudocode ---- */
    if ((1 == cur_num)) {
        return cur_coin;
    }
    if ((0 == reward_type)) {
        return (cur_coin / cur_num);
    }
    return random(100, minimum(((total_coin / total_num) * 3), (cur_coin - (100 * (cur_num - 1)))));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=106, nargs=0, insts=106
// --------------------------------------------------------------------
void save() {
    /* ---- Enhanced Pseudocode ---- */
    int try_times;
    mixed cno;
    mixed mks;
    mapping m;

    if (((cno = "/daemons/dbd.c"->assure_connection("ddb")) == -1)) {
        catch(error("Fail content to database.\n"));

        // --- label L0019 ---
        return;
    }
    try_times = 20;

    // --- label L0026 ---
    while ((sizeof(all_system_envelopes) && (try_times > 0))) {
        loop (mks in all_system_envelopes) {
            if ((!is_mapping(m = mks[1]))) {
                continue;
            }
            if ((!m["update"])) {
                all_system_envelopes[mks[0]] = 0;
                continue;
            }
            if (((*f_execute)(cno, get_update_sql_cmd(m)) >= 1)) {
                all_system_envelopes[mks[0]] = 0;
            }
        }  // end loop (verified)
        --try_times;
        clean_mapping_by_value(all_system_envelopes, 7);
    }  // end loop (verified)
    if (sizeof(all_system_envelopes)) {
        catch(error("Save system envelope info fail.\n"));

        // --- label L0103 ---
    }

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
private int sort_as_coin(array gainer_info) {
    /* ---- Enhanced Pseudocode ---- */
    return gainer_info[2];

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=71, nargs=1, insts=71
// --------------------------------------------------------------------
private string get_update_sql_cmd(mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    mixed checksum;
    mixed sql_cmd;

    checksum = int32((create_check_sum(1, sprintf("%s$%s$%d$%d$%d$%d$%d$%d$%d$%d", m["name"], m["iid"], m["state"], m["type"], m["total_coin"], m["cur_coin"], m["total_num"], m["cur_num"], m["deadline"], m["use"])) ^ -1418120497));
    sql_cmd = mysql_bind_parameters("update red_envelope set state = ?, cur_coin = ?, cur_num = ? , checksum = ?, memo = ? where iid = ? and `use` = ?", m["state"], m["cur_coin"], m["cur_num"], checksum, save_string(m["memo"]), m["iid"], m["use"]);
    return sql_cmd;

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    annouce_eternal_data("all_system_envelopes", &all_system_envelopes);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=298, nargs=5, insts=298
// --------------------------------------------------------------------
int main(mixed agent, int cookie, string iid, string user_name, string para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed checksum;
    mixed ret;
    mixed sql_cmd;
    mapping envelope_info;
    mixed memo;
    mapping m;

    if ((!is_object(agent))) {
        catch(error("Agent is destructed.\n"));

        // --- label L0014 ---
        return 0;
    }
    if ((!is_mapping(envelope_info = all_system_envelopes[iid]))) {
        cno = "/daemons/dbd.c"->assure_connection("ddb");
        if ((cno == -1)) {
            agent->send_message(11831, cookie, -2, 0);
            return 0;
        }
        sql_cmd = mysql_bind_parameters("select * from red_envelope where iid=? and `use`=? and `type`=?", iid, 1, 4);
        ret = mysql_select(cno, sql_cmd);
        if ((!is_object(agent))) {
            catch(error("Agent is destructed 2.\n"));

            // --- label L0074 ---
            return 0;
        }
        if ((!is_array(ret))) {
            agent->send_message(11831, cookie, -2, 0);
            return 0;
        }
        if ((sizeof(ret) != 1)) {
            agent->send_message(11831, cookie, 0, save_string(([ "ret": 2 ])));
            return 0;
        }
        m = ret[0];
        checksum = int32((create_check_sum(1, sprintf("%s$%s$%d$%d$%d$%d$%d$%d$%d$%d", m["name"], iid, (int)m["state"], (int)m["type"], (int)m["total_coin"], (int)m["cur_coin"], (int)m["total_num"], (int)m["cur_num"], (int)m["deadline"], 1)) ^ -1418120497));
        if ((checksum != (int)m["checksum"])) {
            agent->send_message(11831, cookie, 0, save_string(([ "ret": 13 ])));
            return 0;
        }
        if ((!is_mapping(memo = restore_value(m["memo"])))) {
            agent->send_message(11831, cookie, 0, save_string(([ "ret": 14 ])));
            return 0;
        }
        envelope_info = ([ "memo": memo, "type": (int)m["type"], "deadline": (int)m["deadline"], "state": (int)m["state"], "cur_coin": (int)m["cur_coin"], "cur_num": (int)m["cur_num"], "total_coin": (int)m["total_coin"], "total_num": (int)m["total_num"], "iid": iid, "name": m["name"], "use": 1 ]);
        all_system_envelopes[iid] = envelope_info;
    }
    switch (para) {
        case "get":
        get_envelope(agent, cookie, iid, envelope_info, user_name);
        break;
        case "view":
        agent->send_message(11831, cookie, 0, save_string(([ "ret": 15, "memo": envelope_info["memo"] ])));
        break;
        default:
        return 0;
    }  // end switch

    // --- label L0294 ---
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=29, nargs=2, insts=29
// --------------------------------------------------------------------
private void record_a_gainer(mapping envelope, array gainer_info) {
    /* ---- Enhanced Pseudocode ---- */
    mixed top_arr;
    mixed new_top_arr;

    top_arr = (envelope["top"] + ({ gainer_info }));
    new_top_arr = sort(top_arr, (: sort_as_coin :), -1);
    envelope["top"] = new_top_arr[0..19];

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=234, nargs=5, insts=234
// --------------------------------------------------------------------
private void get_envelope(object agent, int cookie, string iid, mapping m, string user_name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed coin;
    mixed cno;
    mapping memo;

    memo = m["memo"];
    if (((2 == m["state"]) || (time() >= m["deadline"]))) {
        agent->send_message(11831, cookie, 0, save_string(([ "ret": 3, "memo": memo ])));
        return;
    }
    if ((((3 == m["state"]) || (m["cur_coin"] < 1)) || (m["cur_num"] < 1))) {
        agent->send_message(11831, cookie, 0, save_string(([ "ret": 5, "memo": memo ])));
        return;
    }
    if (((coin = apply_envelope_coin(iid, m["type"], m["cur_coin"], m["cur_num"], memo)) < 1)) {
        agent->send_message(11831, cookie, 0, save_string(([ "ret": 12, "coin": coin, "memo": memo ])));
        return;
    }
    m["update"] = 1;
    m["cur_num"] -= 1;
    if ((m["cur_num"] < 1)) {
        m["state"] = 3;
    }
    if ((coin > m["cur_coin"])) {
        coin = m["cur_coin"];
    }
    m["cur_coin"] -= coin;
    if ((m["cur_coin"] < 1)) {
        m["state"] = 3;
    }
    record_a_gainer(memo, ({ user_name, time(), coin }));
    memo["cur_coin"] = m["cur_coin"];
    memo["used"] = (m["total_num"] - m["cur_num"]);
    agent->send_message(11831, cookie, 0, save_string(([ "ret": 1, "coin": coin, "memo": memo ])));
    if ((((3 == m["state"]) || ((m["cur_num"] % save_interval) == 0)) && ((cno = "/daemons/dbd.c"->assure_connection("ddb")) != -1))) {
        (*f_execute)(cno, get_update_sql_cmd(m));
    }

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        10
   Code segments found:         10
   Successfully decoded:        10
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          2
      - /inherit/misc/handler.c()
      - /feature/daemon/eternal_data.c()
   ================================================================ */