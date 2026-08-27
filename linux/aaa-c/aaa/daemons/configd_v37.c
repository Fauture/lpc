/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: configd.o
 * Source: /aaa/daemons/configd.c
 * Inherits: (none)
 * Inherited programs: /feature/daemon/register.c(), /feature/daemon/handler_module.c(), /feature/daemon/eternal_data.c()
 * Functions: 50 names, 50 code segments found
 * Strings: 46, Variables: 12
 *********************************************************************/

inherit "/feature/daemon/register.c";
inherit "/feature/daemon/handler_module.c";
inherit "/feature/daemon/eternal_data.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   int _last_update_day;
   int _last_update_time;
   mapping config_list;
   /* mixed global_0; */
   /* mixed global_1; */
   mixed global_config;
   array internal_addrs;
   mapping key_field_map;
   mapping privilege_group;
   mapping server_list_dup;
   int update_config_daemon_id;
   int updating_server_status;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_spa_list();
private void fetch_privilege_group(mixed cno);
private mixed unlock_when_updated();
mixed get_csa_info_by_dist(string dist);
void update_online_num(string server, int num);
mixed get_csa_list();
mixed get_my_info();
private void do_init_after_loaded();
private void update_spa_list();
mixed get_ddb_list_by_spa(string spa);
mixed get_dist_info(string dist);
mixed get_aaa_info(string aaa);
private void update_csa_list();
int is_csp_compete_dist();
mixed get_tts_list();
private void update_config_daemon();
int check_internal_address(mixed addr);
private void fetch_config_list(int cno, string list);
private void destruct();
private void update_config();
private void update_internal_address();
mixed get_ahserver_info(string ahserver);
int is_csl_compete_dist();
string analyse_privilege(string pgclass, string privilege);
mixed get_spa_info_by_dist(string dist);
mixed get_ddb_info_by_dist(string dist);
private mixed lock_when_updating();
mixed get_aaa_list();
mixed get_ccs_list();
private int update_telnet_server();
private void create();
mixed get_cta_list();
private array update_internal_address_by_type(string server_type);
mixed get_server_list_by_dist(string dist);
int is_updating_server_status();
mixed get_ldba_list_by_dist(string dist);
private string expand_privilege_group(mapping all_group, string privilege, mixed got);
mixed get_server_info(string server);
mixed get_server_list();
mixed get_dba_list_by_dist(string dist);
private void fetch_global_config(int cno);
mixed get_info_by_type(int type, string name);
mixed get_dist_list();
private void fetch_server_status();
mixed get_tts_info_by_dist(string dist);
int start_telnet_service();
mixed query_account_service_in_dist(string account, string dist);
mixed get_global_config(string name);
private int check_for_ban(object unauthed_agent, string &prompt);
mixed get_ccs_info_by_dist(string dist);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   update_config_daemon_id = 65535;  // /aaa/daemons/configd.c:24
   internal_addrs = ({ });  // /aaa/daemons/configd.c:33
   config_list = ([ ]);  // /aaa/daemons/configd.c:37
   global_config = ([ ]);  // /aaa/daemons/configd.c:41
   privilege_group = ([ ]);  // /aaa/daemons/configd.c:45
   updating_server_status = 0;
   key_field_map = ([ "aaa" : "aaa", "ccs" : "ccs", "cta" : "cta", "dba" : "dba", "tts" : "tts", "ddb" : "ddb", "spa" : "spa", "csa" : "csa", "ldba" : "ldba", "district" : "dist", "server" : "server", "ahserver" : "ahserver" ]);
   _last_update_time = 0;
   _last_update_day = 0;

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
mixed get_spa_list() {
    /* ---- Enhanced Pseudocode ---- */
    return config_list["spa"];

}

// --------------------------------------------------------------------
// @code  nc=102, nargs=1, insts=102
// --------------------------------------------------------------------
private void fetch_privilege_group(mixed cno) {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr;
    mixed mks;
    int pgclass;
    int pggroup;
    mixed pgprivilege;
    mixed m;
    mapping data;

    arr = mysql_select(cno, "select * from privilege_group", 2147483647);
    if ((sizeof(arr) <= 0)) {
        return;
    }
    loop (data in arr) {
        pgclass = data["class"];
        pggroup = data["privilege_group"];
        pgprivilege = data["privilege"];
        if (is_undefined(privilege_group[pgclass])) {
            privilege_group[pgclass] = ([ pggroup: pgprivilege ]);
        } else {
            privilege_group[pgclass][pggroup] = pgprivilege;
        }
    }  // end loop (verified)
    loop (pgclass in keys(privilege_group)) {
        m = privilege_group[pgclass];
        loop (mks in m) {
            m[mks[0]] = expand_privilege_group(m, mks[1]);
        }  // end loop (verified)
        privilege_group[pgclass] = m;
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @code  nc=12, nargs=0, insts=12
// --------------------------------------------------------------------
private mixed unlock_when_updated() {
    /* ---- Enhanced Pseudocode ---- */
    updating_server_status = 0;
    server_list_dup = 0;
    return;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=33, nargs=1, insts=33
// --------------------------------------------------------------------
mixed get_csa_info_by_dist(string dist) {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;

    if (((!is_mapping(config_list["district"])) || (!is_mapping(m = config_list["district"][dist])))) {
        return 0;
    }
    return (mapping)config_list["csa"][m["csa"]];

}

// --------------------------------------------------------------------
// @code  nc=26, nargs=2, insts=26
// --------------------------------------------------------------------
void update_online_num(string server, int num) {
    /* ---- Enhanced Pseudocode ---- */
    mixed server_list;

    if ((is_mapping(server_list = config_list["server"]) && is_mapping(server_list[server]))) {
        server_list[server]["online"] = (string)num;
    }

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
mixed get_csa_list() {
    /* ---- Enhanced Pseudocode ---- */
    return config_list["csa"];

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=11, nargs=0, insts=11
// --------------------------------------------------------------------
mixed get_my_info() {
    /* ---- Enhanced Pseudocode ---- */
    return get_aaa_info(trim(read_ini("Me", "Name", INI_FILE)));

}

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=19, nargs=0, insts=19
// --------------------------------------------------------------------
private void do_init_after_loaded() {
    /* ---- Enhanced Pseudocode ---- */
    if ((!is_object(find_object("/daemons/connectiond.c")))) {
        register_post_init((: do_init_after_loaded :));
        return;
    }
    "/daemons/connectiond.c"->register_ban((: check_for_ban :));

}

// --------------------------------------------------------------------
// @code  nc=75, nargs=0, insts=75
// --------------------------------------------------------------------
private void update_spa_list() {
    /* ---- Enhanced Pseudocode ---- */
    mixed mks;
    int set_spa;
    mixed dist;
    mixed spa;
    mixed new_spa;

    dist = config_list["district"];
    spa = config_list["spa"];
    if ((!is_mapping(dist))) {
        config_list["spa"] = ([ ]);
    }
    new_spa = ([ ]);
    loop (mks in dist) {
        set_spa = mks[1]["spa"];
        if ((!spa[set_spa])) {
            continue;
        }
        new_spa[set_spa] = spa[set_spa];
        new_spa[set_spa]["dist"] = mks[0];
    }  // end loop (verified)
    config_list["spa"] = new_spa;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=55, nargs=1, insts=55
// --------------------------------------------------------------------
mixed get_ddb_list_by_spa(string spa) {
    /* ---- Enhanced Pseudocode ---- */
    mixed mks;
    mixed arr;
    mixed dist;
    mixed all_dist;
    mixed ddb_info;

    if ((!is_mapping(all_dist = get_dist_list()))) {
        return 0;
    }
    arr = ({});
    loop (mks in all_dist) {
        if ((mks[1]["spa"] != spa)) {
            continue;
        }
        dist = mks[0];
        ddb_info = get_ddb_info_by_dist(dist);
        if ((!is_mapping(ddb_info))) {
            continue;
        }
        arr += ({ ddb_info });
    }  // end loop (verified)
    return arr;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=12, nargs=1, insts=12
// --------------------------------------------------------------------
mixed get_dist_info(string dist) {
    /* ---- Enhanced Pseudocode ---- */
    return (mapping)config_list["district"][dist];

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
mixed get_aaa_info(string aaa) {
    /* ---- Enhanced Pseudocode ---- */
    return get_info_by_type(2, aaa);

}

// --------------------------------------------------------------------
// @code  nc=75, nargs=0, insts=75
// --------------------------------------------------------------------
private void update_csa_list() {
    /* ---- Enhanced Pseudocode ---- */
    mixed mks;
    int set_csa;
    mixed dist;
    mixed csa;
    mixed new_csa;

    dist = config_list["district"];
    csa = config_list["csa"];
    if ((!is_mapping(dist))) {
        config_list["csa"] = ([ ]);
    }
    new_csa = ([ ]);
    loop (mks in dist) {
        set_csa = mks[1]["csa"];
        if ((!csa[set_csa])) {
            continue;
        }
        new_csa[set_csa] = csa[set_csa];
        new_csa[set_csa]["dist"] = mks[0];
    }  // end loop (verified)
    config_list["csa"] = new_csa;

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=16, nargs=0, insts=16
// --------------------------------------------------------------------
int is_csp_compete_dist() {
    /* ---- Enhanced Pseudocode ---- */
    return (8 == (atoi(read_ini("Me", "Partywar_Compete", INI_FILE)) ? 8 : 4));

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
mixed get_tts_list() {
    /* ---- Enhanced Pseudocode ---- */
    return config_list["tts"];

}

// --------------------------------------------------------------------
// @code  nc=21, nargs=0, insts=21
// --------------------------------------------------------------------
private void update_config_daemon() {
    /* ---- Enhanced Pseudocode ---- */
    set_as_daemon();

    // --- label L0004 ---
    while (1) {
        try {
            update_config();
        } catch { /* error handled */ }

        // --- label L0013 ---
        sleep(180);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  validator: verify data
// @code  nc=23, nargs=1, insts=23
// --------------------------------------------------------------------
int check_internal_address(mixed addr) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ip;

    ip = atoinet(addr);
    if ((is_array(internal_addrs) && (member_array(ip, internal_addrs) != -1))) {
        return 1;
    }
    return 0;

}

// --------------------------------------------------------------------
// @code  nc=110, nargs=2, insts=110
// --------------------------------------------------------------------
private void fetch_config_list(int cno, string list) {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr;
    mixed m;
    mapping data;

    m = ([ ]);
    arr = mysql_select(cno, ("select * from " + list), 2147483647);
    if ((!arr)) {
        return;
    }
    loop (data in arr) {
        if (is_string(data["port"])) {
            data["port"] = atoi(data["port"]);
        }
        if (is_string(data["id"])) {
            data["id"] = atoi(data["id"]);
        }
        if (is_string(data["enable"])) {
            data["enable"] = atoi(data["enable"]);
        }
        if (is_string(data["password"])) {
            data["password"] = get_raw_password(data["password"]);
        }
        data["name"] = data[key_field_map[list]];
        m[data[key_field_map[list]]] = data;
    }  // end loop (verified)
    config_list[list] = m;

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=7, nargs=0, insts=7
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    delete_thread(update_config_daemon_id);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=91, nargs=0, insts=91
// --------------------------------------------------------------------
private void update_config() {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;

    reset_log_file_name(_server_name);
    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno == -1)) {
        printf("[1;31mÁ¬½Ó ADB Êý¾Ý¿âÊ§°Ü\n[2;37;0m");
        return;
    }
    fetch_global_config(cno);
    fetch_config_list(cno, "aaa");
    update_telnet_server();
    fetch_config_list(cno, "ccs");
    fetch_config_list(cno, "cta");
    fetch_config_list(cno, "dba");
    fetch_config_list(cno, "ldba");
    fetch_config_list(cno, "ddb");
    fetch_config_list(cno, "tts");
    fetch_config_list(cno, "district");
    lock_when_updating();
    fetch_config_list(cno, "server");
    fetch_server_status();
    unlock_when_updated();
    fetch_config_list(cno, "spa");
    update_spa_list();
    fetch_config_list(cno, "csa");
    update_csa_list();
    fetch_config_list(cno, "ahserver");
    fetch_privilege_group(cno);
    update_internal_address();

}

// --------------------------------------------------------------------
// @code  nc=62, nargs=0, insts=62
// --------------------------------------------------------------------
private void update_internal_address() {
    /* ---- Enhanced Pseudocode ---- */
    mixed results;

    results = ({});
    results += update_internal_address_by_type("cta");
    results += update_internal_address_by_type("dba");
    results += update_internal_address_by_type("aaa");
    results += update_internal_address_by_type("spa");
    results += update_internal_address_by_type("csa");
    results += update_internal_address_by_type("ccs");
    results += update_internal_address_by_type("tts");
    results += update_internal_address_by_type("ldba");
    results += update_internal_address_by_type("server");
    results += update_internal_address_by_type("ahserver");
    internal_addrs = results;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=12, nargs=1, insts=12
// --------------------------------------------------------------------
mixed get_ahserver_info(string ahserver) {
    /* ---- Enhanced Pseudocode ---- */
    return (mapping)config_list["ahserver"][ahserver];

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=16, nargs=0, insts=16
// --------------------------------------------------------------------
int is_csl_compete_dist() {
    /* ---- Enhanced Pseudocode ---- */
    return (2 == (atoi(read_ini("Me", "League_Compete", INI_FILE)) ? 2 : 1));

}

// --------------------------------------------------------------------
// @code  nc=17, nargs=2, insts=17
// --------------------------------------------------------------------
string analyse_privilege(string pgclass, string privilege) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!privilege_group[pgclass])) {
        return privilege;
    }
    return expand_privilege_group(privilege_group[pgclass], privilege);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=33, nargs=1, insts=33
// --------------------------------------------------------------------
mixed get_spa_info_by_dist(string dist) {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;

    if (((!is_mapping(config_list["district"])) || (!is_mapping(m = config_list["district"][dist])))) {
        return 0;
    }
    return (mapping)config_list["spa"][m["spa"]];

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=72, nargs=1, insts=72
// --------------------------------------------------------------------
mixed get_ddb_info_by_dist(string dist) {
    /* ---- Enhanced Pseudocode ---- */
    mixed mks;
    mapping ddb_info;

    if ((!is_mapping(config_list["ddb"]))) {
        return 0;
    }
    loop (mks in config_list["ddb"]) {
        if ((mks[1]["dist"] == dist)) {
            ddb_info = mks[1];
            if ((((((!is_mapping(ddb_info)) || (!is_string(ddb_info["ip"]))) || (!is_int(ddb_info["port"]))) || (ddb_info["ip"] == "")) || (ddb_info["port"] == 0))) {
                continue;
            }
            return ddb_info;
        }
    }  // end loop (verified)
    return 0;

}

// --------------------------------------------------------------------
// @code  nc=15, nargs=0, insts=15
// --------------------------------------------------------------------
private mixed lock_when_updating() {
    /* ---- Enhanced Pseudocode ---- */
    server_list_dup = deep_dup(config_list["server"]);
    updating_server_status = 1;
    return;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
mixed get_aaa_list() {
    /* ---- Enhanced Pseudocode ---- */
    return config_list["aaa"];

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
mixed get_ccs_list() {
    /* ---- Enhanced Pseudocode ---- */
    return config_list["ccs"];

}

// --------------------------------------------------------------------
// @code  nc=77, nargs=0, insts=77
// --------------------------------------------------------------------
private int update_telnet_server() {
    /* ---- Enhanced Pseudocode ---- */
    mixed pwd;
    mapping aaa_info;

    if ((!is_mapping(aaa_info = get_my_info()))) {
        return 0;
    }
    if (((!is_int(aaa_info["port"])) || (!is_string(aaa_info["aaa"])))) {
        return 0;
    }
    if (((time() / 86400) == _last_update_day)) {
        return 0;
    }
    if (((time() - _last_update_time) < 300)) {
        return 0;
    }
    _last_update_time = time();
    if (((!is_string(pwd = get_telnet_password(aaa_info["aaa"]))) || (!strlen(pwd)))) {
        return 0;
    }
    _last_update_day = (time() / 86400);
    return telnet_on((aaa_info["port"] + 1000), pwd);

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=26, nargs=0, insts=26
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    require_uid();
    set_euid(0);
    _server_name = trim(read_ini("Me", "Name", INI_FILE));
    update_config_daemon_id = create_thread(0, (: update_config_daemon :));
    register_post_init((: do_init_after_loaded :));

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
mixed get_cta_list() {
    /* ---- Enhanced Pseudocode ---- */
    return config_list["cta"];

}

// --------------------------------------------------------------------
// @code  nc=61, nargs=1, insts=61
// --------------------------------------------------------------------
private array update_internal_address_by_type(string server_type) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    mixed addr;
    mixed arr;
    mixed m;
    mapping server_info;

    m = config_list[server_type];
    if ((!is_mapping(m))) {
        return ({});
    }
    i = 0;
    arr = allocate_array(sizeof(m));
    loop (server_info in values(m)) {
        addr = atoinet((string)server_info["ip"]);
        if ((!addr)) {
            continue;
        }
        arr[i++] = addr;
    }  // end loop (verified)
    clean_array(arr, 2);
    return arr;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=63, nargs=1, insts=63
// --------------------------------------------------------------------
mixed get_server_list_by_dist(string dist) {
    /* ---- Enhanced Pseudocode ---- */
    mixed mks;
    mixed server_map;
    mixed m;
    mapping data;

    if (is_mapping(server_map = get_server_list())) {
        m = ([ ]);
        loop (mks in server_map) {
            if ((!is_mapping(data = mks[1]))) {
                continue;
            }
            if ((data["dist"] != dist)) {
                continue;
            }
            if ((data["enable"] != 1)) {
                continue;
            }
            m[mks[0]] = data;
        }  // end loop (verified)
    }
    return 0;

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int is_updating_server_status() {
    /* ---- Enhanced Pseudocode ---- */
    return updating_server_status;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=41, nargs=1, insts=41
// --------------------------------------------------------------------
mixed get_ldba_list_by_dist(string dist) {
    /* ---- Enhanced Pseudocode ---- */
    mixed mks;
    mixed arr;

    if ((!is_mapping(config_list["ldba"]))) {
        return 0;
    }
    arr = ({});
    loop (mks in config_list["ldba"]) {
        if ((mks[1]["dist"] == dist)) {
            arr += ({ mks[1] });
        }
    }  // end loop (verified)
    return arr;

}

// --------------------------------------------------------------------
// @code  nc=98, nargs=3, insts=98
// --------------------------------------------------------------------
private string expand_privilege_group(mapping all_group, string privilege, mixed got) {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr;
    mixed expand_got;
    int str;
    string ret;
    mixed tmp;

    if ((!got)) {
        expand_got = ({});
    } else {
        expand_got = got;
    }
    ret = "";
    arr = explode(privilege, "/");
    loop (str in arr) {
        str = trim(str);
        if ((member_array(str, expand_got) >= 0)) {
            continue;
        }
        if ((!all_group[str])) {
            ret += (str + "/");
            expand_got += ({ str });
        } else {
            tmp = expand_privilege_group(all_group, all_group[str], &expand_got);
            if ((strlen(tmp) > 0)) {
                tmp += "/";
            }
            ret += tmp;
        }
    }  // end loop (verified)
    got = expand_got;
    return ret[0..<2];

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
mixed get_server_info(string server) {
    /* ---- Enhanced Pseudocode ---- */
    return get_info_by_type(5, server);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=12, nargs=0, insts=12
// --------------------------------------------------------------------
mixed get_server_list() {
    /* ---- Enhanced Pseudocode ---- */
    return (updating_server_status ? server_list_dup : config_list["server"]);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=41, nargs=1, insts=41
// --------------------------------------------------------------------
mixed get_dba_list_by_dist(string dist) {
    /* ---- Enhanced Pseudocode ---- */
    mixed mks;
    mixed arr;

    if ((!is_mapping(config_list["dba"]))) {
        return 0;
    }
    arr = ({});
    loop (mks in config_list["dba"]) {
        if ((mks[1]["dist"] == dist)) {
            arr += ({ mks[1] });
        }
    }  // end loop (verified)
    return arr;

}

// --------------------------------------------------------------------
// @code  nc=31, nargs=1, insts=31
// --------------------------------------------------------------------
private void fetch_global_config(int cno) {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr;
    mapping data;

    arr = mysql_select(cno, "select * from config where section = 'global'", 2147483647);
    if ((!arr)) {
        return;
    }
    loop (data in arr) {
        global_config[data["name"]] = data["value"];
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=97, nargs=2, insts=97
// --------------------------------------------------------------------
mixed get_info_by_type(int type, string name) {
    /* ---- Enhanced Pseudocode ---- */
    switch (type) {
        case 5:
        case 18:
        return (mapping)get_server_list()[name];
        case 7:
        return (mapping)config_list["ccs"][name];
        case 4:
        return (mapping)config_list["dba"][name];
        case 19:
        return (mapping)config_list["ldba"][name];
        case 9:
        return (mapping)config_list["spa"][name];
        case 2:
        return (mapping)config_list["aaa"][name];
        case 11:
        return (mapping)config_list["cta"][name];
        case 14:
        return (mapping)config_list["csa"][name];
        case 16:
        return (mapping)config_list["tts"][name];
        default:
        return;
    }  // end switch

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
mixed get_dist_list() {
    /* ---- Enhanced Pseudocode ---- */
    return config_list["district"];

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=62, nargs=0, insts=62
// --------------------------------------------------------------------
private void fetch_server_status() {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed arr;
    mixed server;
    mapping data;

    if (((cno = "/daemons/dbd.c"->assure_connection("mdb")) < 0)) {
        return;
    }
    arr = mysql_select(cno, "select * from server_status", 2147483647);
    if ((!arr)) {
        return;
    }
    server = config_list["server"];
    loop (data in arr) {
        if (is_mapping(server[data["server"]])) {
            data << server[data["server"]];
            server[data["server"]] = data;
        }
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=34, nargs=1, insts=34
// --------------------------------------------------------------------
mixed get_tts_info_by_dist(string dist) {
    /* ---- Enhanced Pseudocode ---- */
    mixed mks;

    if ((!is_mapping(config_list["tts"]))) {
        return 0;
    }
    loop (mks in config_list["tts"]) {
        if ((mks[1]["dist"] == dist)) {
            return mks[1];
        }
    }  // end loop (verified)
    return 0;

}

// --------------------------------------------------------------------
// @code  nc=14, nargs=0, insts=14
// --------------------------------------------------------------------
int start_telnet_service() {
    /* ---- Enhanced Pseudocode ---- */
    _last_update_time = 0;
    _last_update_day = 0;
    return update_telnet_server();

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=35, nargs=2, insts=35
// --------------------------------------------------------------------
mixed query_account_service_in_dist(string account, string dist) {
    /* ---- Enhanced Pseudocode ---- */
    mixed sql_cmd;
    mixed cno;
    mixed service;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno == -1)) {
        printf("[1;31mÁ¬½Ó ADB Êý¾Ý¿âÊ§°Ü\n[2;37;0m");
        return;
    }
    sql_cmd = mysql_bind_parameters("select * from service where account = ? and dist = ?", account, dist);
    service = mysql_select_single_result(cno, sql_cmd);
    return service;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
mixed get_global_config(string name) {
    /* ---- Enhanced Pseudocode ---- */
    return global_config[name];

}

// --------------------------------------------------------------------
// @brief  validator: verify data
// @code  nc=52, nargs=2, insts=52
// --------------------------------------------------------------------
private int check_for_ban(object unauthed_agent, string &prompt) {
    /* ---- Enhanced Pseudocode ---- */
    int client_no;
    mixed addr;
    mapping client_info;
    mapping my_info;

    if ((!is_mapping(my_info = get_my_info()))) {
        return 1;
    }
    if ((my_info["attrib"] != "internal")) {
        return 0;
    }
    client_no = unauthed_agent->get_client_no();
    if ((client_no < 0)) {
        return 1;
    }
    client_info = get_client_info(client_no);
    addr = atoinet(client_info["remote_address"]);
    if ((!check_internal_address(addr))) {
        return 1;
    }
    return 0;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=34, nargs=1, insts=34
// --------------------------------------------------------------------
mixed get_ccs_info_by_dist(string dist) {
    /* ---- Enhanced Pseudocode ---- */
    mixed mks;

    if ((!is_mapping(config_list["ccs"]))) {
        return 0;
    }
    loop (mks in config_list["ccs"]) {
        if ((mks[1]["dist"] == dist)) {
            return mks[1];
        }
    }  // end loop (verified)
    return 0;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        50
   Code segments found:         50
   Successfully decoded:        50
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          3
      - /feature/daemon/register.c()
      - /feature/daemon/handler_module.c()
      - /feature/daemon/eternal_data.c()
   ================================================================ */