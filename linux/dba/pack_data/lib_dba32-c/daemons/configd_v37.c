/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: configd.o
 * Source: /daemons/configd.c
 * Inherits: (none)
 * Inherited programs: /feature/daemon/register.c(), /feature/daemon/handler_module.c()
 * Functions: 39 names, 39 code segments found
 * Strings: 84, Variables: 35
 *********************************************************************/

inherit "/feature/daemon/register.c";
inherit "/feature/daemon/handler_module.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   int AAA_DOMAIN_NAME;
   int AAA_DOMAIN_PORT;
   int CS_ACTIVITY_ROLE;
   int CS_ARENA_ROLE;
   int CS_CELEBRITY_ROLE;
   int CS_CITYWAR_ROLE;
   int CS_EVERYONE_PK_ROLE;
   int CS_LEAGUE_ROLE;
   int CS_PARTYWAR_ROLE;
   int CS_SAMSUNG_ROLE;
   int CS_SDDH_ROLE;
   int CS_XGAME_ROLE;
   int MY_NAME;
   int _last_update_day;
   int _last_update_time;
   array aaa_addrs;
   int aaa_client_no;
   int activity_compete_dist;
   int arena_compete_dist;
   int celebrity_compete_dist;
   mapping config;
   object config_agent;
   array cta_addrs;
   int current_cookie;
   array dist_aaa_addrs;
   mapping dist_aaa_map;
   int ev_config;
   int everyone_pk_compete_dist;
   array game_server_addrs;
   /* mixed global_0; */
   mapping oper_map;
   int samsung_compete_dist;
   mapping server_id_name_list;
   mapping server_name_id_list;
   mapping setting;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_my_info();
int is_activity_compete_dist();
mixed get_ldba_address();
mixed get_server_addrs(string server);
int is_csp_compete_dist();
int check_internal_address(mixed addr);
int write_etc(string section, string key, string value, string version = "1.60.0905");
private void destruct();
void set_config(string name, mixed val);
mixed get_config(string name);
private void _get_config_without_aaa();
void msg_cfg_write_etc(int cookie, int result);
void msg_cfg_read_etc(int cookie, int result);
int is_samsung_compete_dist();
int is_arena_compete_dist();
int check_config(string config, int type);
int is_sddh_compete_dist();
mixed get_setting(string name);
int is_celebrity_compete_dist();
int is_csl_compete_dist();
private void _config_daemon();
mixed read_etc(string section, string key, string version = "1.60.0905");
int is_stand_alone();
int is_xgame_compete_dist();
int is_everyone_pk_compete_dist();
private int update_telnet_server();
int get_server_id_by_name(string name);
int get_int_setting(string name);
private void create();
void msg_cfg_etc_page_data(int cookie, buffer content);
mixed get_dba_address();
string get_server_name_by_id(int id);
int try_to_connect_aaa();
private void assure_config_agent();
mixed get_tts_address();
mapping get_aaa_address();
int start_telnet_service();
int is_citywar_compete_dist();
int wait_config(mixed timeout = -1);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   dist_aaa_map = ([ ]);  // /daemons/configd.c:81
   dist_aaa_addrs = ({ });  // /daemons/configd.c:85
   config = ([ ]);  // /daemons/configd.c:89
   setting = ([ ]);  // /daemons/configd.c:95
   aaa_client_no = 65535;
   ev_config = 65535;
   oper_map = ([ ]);
   arena_compete_dist = 0;
   celebrity_compete_dist = 0;
   samsung_compete_dist = 0;
   everyone_pk_compete_dist = 0;
   activity_compete_dist = 0;
   server_name_id_list = ([ ]);
   server_id_name_list = ([ ]);
   current_cookie = 123456;
   _last_update_time = 0;
   _last_update_day = 0;

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=42, nargs=0, insts=42
// --------------------------------------------------------------------
mixed get_my_info() {
    /* ---- Enhanced Pseudocode ---- */
    string dist;
    string name;
    int port;
    mapping m;

    dist = (string)config["my_dist"];
    name = (string)config["my_name"];
    port = (int)config["my_port"];
    m = (mapping)config["my_setting"];
    return ([ "dist": dist, "name": name, "port": port, "my_setting": m ]);

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int is_activity_compete_dist() {
    /* ---- Enhanced Pseudocode ---- */
    return activity_compete_dist;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=36, nargs=0, insts=36
// --------------------------------------------------------------------
mixed get_ldba_address() {
    /* ---- Enhanced Pseudocode ---- */
    mixed ldba_list;

    if (is_stand_alone()) {
        return ([ "ip": trim(read_ini("Me", "LDBA_Addr", INI_FILE)), "port": atoi(read_ini("Me", "LDBA_Port", INI_FILE)) ]);
    }
    if ((!sizeof((ldba_list = get_config("ldba_list"))))) {
        return 0;
    }
    return ldba_list[random(sizeof(ldba_list))];

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=30, nargs=1, insts=30
// --------------------------------------------------------------------
mixed get_server_addrs(string server) {
    /* ---- Enhanced Pseudocode ---- */
    mixed server_list;
    mapping m;

    if ((!sizeof((server_list = get_config("game_server_list"))))) {
        return _server_name;
    }
    loop (m in server_list) {
        if ((server == m["name"])) {
            return m["ip"];
        }
    }  // end loop (verified)
    return;

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
int is_csp_compete_dist() {
    /* ---- Enhanced Pseudocode ---- */
    return (8 == CS_PARTYWAR_ROLE);

}

// --------------------------------------------------------------------
// @brief  validator: verify data
// @code  nc=99, nargs=1, insts=99
// --------------------------------------------------------------------
int check_internal_address(mixed addr) {
    /* ---- Enhanced Pseudocode ---- */
    int ip;

    ip = atoinet(addr);
    if ((is_array(aaa_addrs) && (member_array(ip, aaa_addrs) != -1))) {
        return 1;
    }
    if ((is_array(dist_aaa_addrs) && (member_array(ip, dist_aaa_addrs) != -1))) {
        return 1;
    }
    if ((is_array(game_server_addrs) && (member_array(ip, game_server_addrs) != -1))) {
        return 1;
    }
    if ((is_array(cta_addrs) && (member_array(ip, cta_addrs) != -1))) {
        return 1;
    }
    if ((is_mapping(config["ccs_info"]) && (atoinet(config["ccs_info"]["ip"]) == ip))) {
        return 1;
    }
    if ((is_mapping(config["spa_info"]) && (atoinet(config["spa_info"]["ip"]) == ip))) {
        return 1;
    }
    if ((ip == 2130706433)) {
        return 1;
    }
    return 0;

}

// --------------------------------------------------------------------
// @brief  data writer: persist information
// @code  nc=126, nargs=4, insts=126
// --------------------------------------------------------------------
int write_etc(string section, string key, string value, string version) {
    /* ---- Enhanced Pseudocode ---- */
    int cookie;
    mixed thread_id;
    mapping m;

    if ((!sizeof(get_aaa_address()))) {
        return -1;
    }
    thread_id = get_thread_id();

    // --- label L0014 ---
    while ((try_to_connect_aaa() == -1)) {
        wait_event(("ev_readetcd" + (thread_id / 32)), (1 << (thread_id % 32)), 0, 1);
    }  // end loop (verified)
    assure_config_agent();
    wait_event(("ev_readetcd" + (thread_id / 32)), (1 << (thread_id % 32)), 0, 0);
    cookie = thread_id;
    m = ([ ]);
    m["result"] = 0;
    m["tid"] = thread_id;
    oper_map[cookie] = m;
    send_to_client(aaa_client_no, 17524, _dist_name, section, key, pad_buffer(make32(strlen(value)), (buffer)value), version, make32(cookie));
    wait_event(("ev_readetcd" + (thread_id / 32)), (1 << (thread_id % 32)), 0, 60);
    m = oper_map[cookie];
    map_delete(oper_map, cookie);
    return m["result"];

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=85, nargs=0, insts=85
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    int i;

    printf("Stop CONFIG_D.all services.\n");
    debug_delete_thread("tconfig");
    printf("CONFIG_D.daemon done.\n");
    if ((ev_config != 1)) {
        delete_event(ev_config);
        ev_config = -1;
    }
    delete_mutex("mconfig");
    for (i = 0; i <= ((((get_system_info()["max_threads"] + 32) - 1) / 32) - 1); i++) {
        delete_event(("ev_readetcd" + i));
    }  // end loop (verified)
    if ((aaa_client_no != -1)) {
        close_client(aaa_client_no);
    }
    if ((!sizeof(config))) {
        printf("[1;33mCONFIG_D could not get the correct configurate from AAA agent.\n[2;37;0m");
    }
    if (is_object(config_agent)) {
        destruct_object(config_agent);
    }
    if (sizeof(config)) {
        register_daemon_data("/daemons/configd.c", "config", config);
    }
    printf("CONFIG_D.all services done.\n");

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=300, nargs=2, insts=300
// --------------------------------------------------------------------
void set_config(string name, mixed val) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    int index;
    int size;
    mixed ip_list;
    mixed temp_arr;
    mixed f;
    //mixed <loop_1>;
    mixed aaa_info;
    int addr;

    size = 0;
    ip_list = ({});
    loop (f in values((mapping)get_daemon_data("/daemons/configd.c"))) {
        if ((!is_function(f))) {
            continue;
        }
        try {
            (*f)(name, val);
        } catch { /* error handled */ }

        // --- label L0033 ---
    }  // end loop (verified)
    switch (name) {
        case "my_port":
        raise_event(ev_config, 32768);
        break;
        case "game_server_list":
        game_server_addrs = allocate_array(i = sizeof(val));

        // --- label L0059 ---
        while (i--) {
            game_server_addrs[i] = atoinet(val[i]["ip"]);
            server_name_id_list[val[i]["name"]] = val[i]["id"];
            server_id_name_list[val[i]["id"]] = val[i]["name"];
        }  // end loop (verified)
        break;
        case "my_name":
        if ((val != config["my_name"])) {
            catch(error("Bad server name received from AAA.\n"));

            // --- label L0121 ---
        }
        reset_log_file_name(val);
        return;
        case "my_dist":
        _dist_name = val;
        break;
        case "my_setting":
        setting << val;
        val = setting;
        break;
        case "cta_list":
        i = sizeof(val);

        // --- label L0148 ---
        while (i--) {
            if ((0 > strsrch(val[i]["ip"], ","))) {
                ip_list += ({ val[i]["ip"] });
                ++size;
                continue;
            }
            temp_arr = explode(val[i]["ip"], ",");
            for (index = 0; index <= (sizeof(temp_arr) - 1); index++) {
                if (((!is_string(temp_arr[index])) || ("" == temp_arr[index]))) {
                    continue;
                }
                ip_list += ({ temp_arr[index] });
                ++size;
            }  // end loop (verified)
        }  // end loop (verified)
        cta_addrs = allocate_array(size);

        // --- label L0236 ---
        while (size--) {
            cta_addrs[size] = atoinet(ip_list[size]);
        }  // end loop (verified)
        break;
        case "aaa_info":
        dist_aaa_addrs = ({});
        dist_aaa_map = ([ ]);
        loop (aaa_info in values(val)) {
            addr = atoinet(aaa_info[0]);
            dist_aaa_addrs += ({ addr });
            dist_aaa_map[addr] = aaa_info[2];
        }  // end loop (verified)
        break;
        default:
    }  // end switch

    // --- label L0292 ---
    config[name] = val;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
mixed get_config(string name) {
    /* ---- Enhanced Pseudocode ---- */
    return config[name];

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=48, nargs=0, insts=48
// --------------------------------------------------------------------
private void _get_config_without_aaa() {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/configd.c"->set_config("my_dist", "local");
    "/daemons/configd.c"->set_config("my_name", config["my_name"]);
    "/daemons/configd.c"->set_config("my_port", config["my_port"]);
    "/daemons/configd.c"->set_config("my_id", 0);
    "/daemons/configd.c"->set_config("my_setting", ([ ]));
    config["aaa_disabled"] = 1;
    raise_event(ev_config, 32768);

}

// --------------------------------------------------------------------
// @code  nc=33, nargs=2, insts=33
// --------------------------------------------------------------------
void msg_cfg_write_etc(int cookie, int result) {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;

    m = oper_map[cookie];
    if ((!is_mapping(m))) {
        return;
    }
    m["result"] = result;
    raise_event(("ev_readetcd" + (cookie / 32)), (1 << (cookie % 32)));

}

// --------------------------------------------------------------------
// @code  nc=42, nargs=2, insts=42
// --------------------------------------------------------------------
void msg_cfg_read_etc(int cookie, int result) {
    /* ---- Enhanced Pseudocode ---- */
    mixed tid;
    mapping m;

    m = oper_map[cookie];
    if (((!is_mapping(m)) || (!is_int(tid = m["tid"])))) {
        return;
    }
    m["result"] = result;
    raise_event(("ev_readetcd" + (tid / 32)), (1 << (tid % 32)));

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int is_samsung_compete_dist() {
    /* ---- Enhanced Pseudocode ---- */
    return samsung_compete_dist;

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int is_arena_compete_dist() {
    /* ---- Enhanced Pseudocode ---- */
    return arena_compete_dist;

}

// --------------------------------------------------------------------
// @brief  validator: verify data
// @code  nc=32, nargs=2, insts=32
// --------------------------------------------------------------------
int check_config(string config, int type) {
    /* ---- Enhanced Pseudocode ---- */
    mixed value;

    if ((type == 4)) {
        return 1;
    }
    try {
        value = restore_value(config);
    } catch { /* error handled */ }

    // --- label L0018 ---
    return 0;
    if ((var_raw_type(value) != type)) {
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
int is_sddh_compete_dist() {
    /* ---- Enhanced Pseudocode ---- */
    return (512 == CS_SDDH_ROLE);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
mixed get_setting(string name) {
    /* ---- Enhanced Pseudocode ---- */
    return setting[name];

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int is_celebrity_compete_dist() {
    /* ---- Enhanced Pseudocode ---- */
    return celebrity_compete_dist;

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
int is_csl_compete_dist() {
    /* ---- Enhanced Pseudocode ---- */
    return (2 == CS_LEAGUE_ROLE);

}

// --------------------------------------------------------------------
// @code  nc=212, nargs=0, insts=212
// --------------------------------------------------------------------
private void _config_daemon() {
    /* ---- Enhanced Pseudocode ---- */
    mapping aaa_addr;

    set_as_daemon();
    printf("CONFIG_D.daemon start...\n");
    require_uid();
    set_euid(0);
    if (((config["my_name"] = MY_NAME) == "")) {
        printf("Not found Me.Name in server.ini, failed to start CONFIG_D.\n");
        shutdown();
        suspend_thread();
    }
    _server_name = config["my_name"];
    printf("My configuration:\n  Name: %s\n", config["my_name"]);
    while (1) {
        update_telnet_server();
        aaa_addr = get_aaa_address();
        if ((!sizeof(aaa_addr))) {
            _get_config_without_aaa();
            sleep(15);
            continue;
        }
        if ((try_to_connect_aaa() == -1)) {
            printf("CONFIG_D connect to AAA failed, %s:%d.\n", aaa_addr["ip"], aaa_addr["port"]);
            sleep(15);
            continue;
        }
        assure_config_agent();
        wait_event(ev_config, 32768, 0, 0);
        send_to_client(aaa_client_no, 9336, 4, config["my_name"]);
        send_to_client(aaa_client_no, 9050, 4, config["my_name"]);
        send_to_client(aaa_client_no, 9056, 4, config["my_name"]);
        send_to_client(aaa_client_no, 9070, 4, config["my_name"]);
        send_to_client(aaa_client_no, 9058, 4, config["my_name"]);
        send_to_client(aaa_client_no, 9054, 4, config["my_name"]);
        send_to_client(aaa_client_no, 9324, 4, config["my_name"]);
        send_to_client(aaa_client_no, 9062, 4, config["my_name"]);
        send_to_client(aaa_client_no, 16626, 4, config["my_name"], "*");
        send_to_client(aaa_client_no, 9338, 4, config["my_name"], "*");
        send_to_client(aaa_client_no, 9326, 4, config["my_name"]);
        send_to_client(aaa_client_no, 9342, 4, config["my_name"]);
        send_to_client(aaa_client_no, 9330, 4, config["my_name"]);
        send_to_client(aaa_client_no, 9060, 4, config["my_name"]);
        wait_event(ev_config, 32768, 0, 30);
        update_telnet_server();
        sleep(180);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @code  nc=147, nargs=3, insts=147
// --------------------------------------------------------------------
mixed read_etc(string section, string key, string version) {
    /* ---- Enhanced Pseudocode ---- */
    int cookie;
    mapping m;
    mixed thread_id;

    if ((!sizeof(get_aaa_address()))) {
        return -1;
    }
    thread_id = get_thread_id();

    // --- label L0014 ---
    while ((try_to_connect_aaa() == -1)) {
        wait_event(("ev_readetcd" + (thread_id / 32)), (1 << (thread_id % 32)), 0, 1);
    }  // end loop (verified)
    assure_config_agent();
    wait_event(("ev_readetcd" + (thread_id / 32)), (1 << (thread_id % 32)), 0, 0);
    ++current_cookie;
    cookie = (current_cookie & -1);
    m = ([ ]);
    m["content"] = 0;
    m["result"] = 0;
    m["tid"] = thread_id;
    oper_map[cookie] = m;
    send_to_client(aaa_client_no, 17520, _dist_name, section, key, version, make32(cookie));
    wait_event(("ev_readetcd" + (thread_id / 32)), (1 << (thread_id % 32)), 0, 60);
    m = oper_map[cookie];
    map_delete(oper_map, cookie);
    if ((1 != m["result"])) {
        return -1;
    }
    if ((!is_string(m["content"]))) {
        return -1;
    }
    return m["content"];

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=10, nargs=0, insts=10
// --------------------------------------------------------------------
int is_stand_alone() {
    /* ---- Enhanced Pseudocode ---- */
    return atoi(read_ini("Me", "Standalone", INI_FILE));

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
int is_xgame_compete_dist() {
    /* ---- Enhanced Pseudocode ---- */
    return (131072 == CS_XGAME_ROLE);

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int is_everyone_pk_compete_dist() {
    /* ---- Enhanced Pseudocode ---- */
    return everyone_pk_compete_dist;

}

// --------------------------------------------------------------------
// @code  nc=69, nargs=0, insts=69
// --------------------------------------------------------------------
private int update_telnet_server() {
    /* ---- Enhanced Pseudocode ---- */
    mixed pwd;

    if ((!config["my_port"])) {
        return 0;
    }
    if ((!is_string(config["my_dist"]))) {
        return 0;
    }
    if (((time() / 86400) == _last_update_day)) {
        return 0;
    }
    if (((time() - _last_update_time) < 300)) {
        return 0;
    }
    _last_update_time = time();
    if (((!is_string(pwd = get_telnet_password(config["my_dist"]))) || (!strlen(pwd)))) {
        return 0;
    }
    _last_update_day = (time() / 86400);
    return telnet_on((config["my_port"] + 1000), pwd);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=16, nargs=1, insts=16
// --------------------------------------------------------------------
int get_server_id_by_name(string name) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!is_int(server_name_id_list[name]))) {
        return -1;
    }
    return server_name_id_list[name];

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=9, nargs=1, insts=9
// --------------------------------------------------------------------
int get_int_setting(string name) {
    /* ---- Enhanced Pseudocode ---- */
    return (int)setting[name];

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=385, nargs=0, insts=385
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    mixed host_addrs;
    mixed host_name;
    int bad_config;
    int i;
    //mixed <loop_1>;
    mapping m;

    bad_config = 0;
    AAA_DOMAIN_NAME = trim(read_ini("Me", "AAA_Addr", INI_FILE));
    AAA_DOMAIN_PORT = atoi(read_ini("Me", "AAA_Port", INI_FILE));
    MY_NAME = trim(read_ini("Me", "Name", INI_FILE));
    CS_LEAGUE_ROLE = (atoi(read_ini("Me", "League_Compete", INI_FILE)) ? 2 : 1);
    CS_PARTYWAR_ROLE = (atoi(read_ini("Me", "Partywar_Compete", INI_FILE)) ? 8 : 4);
    CS_ARENA_ROLE = (atoi(read_ini("Me", "Arena_Compete", INI_FILE)) ? 32 : 16);
    CS_SDDH_ROLE = (atoi(read_ini("Me", "Shidaodahui_Compete", INI_FILE)) ? 512 : 256);
    CS_CELEBRITY_ROLE = (atoi(read_ini("Me", "Celebrity_Compete", INI_FILE)) ? 2048 : 1024);
    CS_SAMSUNG_ROLE = (atoi(read_ini("Me", "Samsung_Compete", INI_FILE)) ? 8192 : 4096);
    CS_CITYWAR_ROLE = (atoi(read_ini("Me", "Citywar_Compete", INI_FILE)) ? 32768 : 16384);
    CS_XGAME_ROLE = (atoi(read_ini("Me", "XGame_Compete", INI_FILE)) ? 131072 : 65536);
    CS_EVERYONE_PK_ROLE = (atoi(read_ini("Me", "Everyone_Pk_Compete", INI_FILE)) ? 524288 : 262144);
    CS_ACTIVITY_ROLE = (atoi(read_ini("Me", "Activity_Compete", INI_FILE)) ? 2097152 : 1048576);
    if ((AAA_DOMAIN_NAME == "")) {
        printf("Miss field \"AAA_Addr\" in section \"Me\", expected domain name for AAA server.\n");
        bad_config = 1;
    }
    if ((AAA_DOMAIN_PORT == 0)) {
        printf("Miss field \"AAA_Port\" in section \"Me\", expected tcp port for AAA server.\n");
        bad_config = 1;
    }
    if ((MY_NAME == "")) {
        printf("Miss field \"Name\" in section \"Me\", expected server name.\n");
        bad_config = 1;
    }
    aaa_addrs = ({});
    if (is_stand_alone()) {
        printf("In stand alone mode, AAA is disabled.\n");
        config["dist"] = trim(read_ini("Me", "Dist", INI_FILE));
        config["my_port"] = atoi(read_ini("Me", "Port", INI_FILE));
    } else {
        loop (host_name in explode(AAA_DOMAIN_NAME, ",")) {
            host_addrs = resolve(trim(host_name));
            if ((!sizeof(host_addrs))) {
                printf("Bad AAA address: %s\n", host_name);
            } else {
                aaa_addrs += host_addrs;
            }
        }  // end loop (verified)
        if ((!sizeof(aaa_addrs))) {
            printf("AAA address is not indicated.\n");
            bad_config = 1;
        }
    }
    if (bad_config) {
        printf((("[1;31mBad configuration, please edit your " + INI_FILE) + "\n[2;37;0mFor example:\n[1;32m[Me]\nAAA_Addr = aaa.g-bits.com\nAAA_Port = 8001\nCTA_Addr = 10.2.48.84\nName = server_doing\n[2;37;0m"));
        suspend_thread();
    }
    m = get_daemon_data("/daemons/configd.c");
    if (((is_mapping(m) && is_mapping(m["config"])) && (sizeof(m["config"]) > 0))) {
        config = m["config"];
        unregister_daemon_data("/daemons/configd.c", "config");
    }
    ev_config = create_event("ev_config");
    create_mutex("mconfig");
    for (i = 0; i <= ((((get_system_info()["max_threads"] + 32) - 1) / 32) - 1); i++) {
        create_event(("ev_readetcd" + i));
    }  // end loop (verified)
    printf("Start CONFIG_D.all services.\n");
    set_driver_caption(MY_NAME);
    reset_log_file_name(MY_NAME);
    load_all_handlers("/daemons/config/");
    create_thread("tconfig", "_config_daemon");
    arena_compete_dist = (32 == CS_ARENA_ROLE);
    celebrity_compete_dist = (2048 == CS_CELEBRITY_ROLE);
    samsung_compete_dist = (8192 == CS_SAMSUNG_ROLE);
    everyone_pk_compete_dist = (524288 == CS_EVERYONE_PK_ROLE);
    activity_compete_dist = (2097152 == CS_ACTIVITY_ROLE);

}

// --------------------------------------------------------------------
// @code  nc=40, nargs=2, insts=40
// --------------------------------------------------------------------
void msg_cfg_etc_page_data(int cookie, buffer content) {
    /* ---- Enhanced Pseudocode ---- */
    string data;
    mapping m;

    m = oper_map[cookie];
    if ((!is_mapping(m))) {
        return;
    }
    data = (string)lzss_decompress(content);
    if ((!is_string(m["content"]))) {
        m["content"] = data;
    } else {
        m["content"] += data;
    }

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=36, nargs=0, insts=36
// --------------------------------------------------------------------
mixed get_dba_address() {
    /* ---- Enhanced Pseudocode ---- */
    mixed dba_list;

    if (is_stand_alone()) {
        return ([ "ip": trim(read_ini("Me", "DBA_Addr", INI_FILE)), "port": atoi(read_ini("Me", "DBA_Port", INI_FILE)) ]);
    }
    if ((!sizeof((dba_list = get_config("dba_list"))))) {
        return 0;
    }
    return dba_list[random(sizeof(dba_list))];

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=16, nargs=1, insts=16
// --------------------------------------------------------------------
string get_server_name_by_id(int id) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!is_string(server_id_name_list[id]))) {
        return "";
    }
    return server_id_name_list[id];

}

// --------------------------------------------------------------------
// @code  nc=99, nargs=0, insts=99
// --------------------------------------------------------------------
int try_to_connect_aaa() {
    /* ---- Enhanced Pseudocode ---- */
    mixed fd;
    mapping m;
    mapping aaa_addr;

    do {
        if ((aaa_client_no >= 0)) {
            m = get_client_info(aaa_client_no);
            if ((is_mapping(m = get_client_info(aaa_client_no)) && m["alive"])) {
                break;
            }
        }
        aaa_client_no = -1;
        if ((take_mutex("mconfig") != 0)) {
            break;
        }
        aaa_addr = get_aaa_address();
        fd = socket();
        if (((connect(fd, aaa_addr["ip"], aaa_addr["port"]) != 0) || ((aaa_client_no = bind_to_client(fd)) < 0))) {
            close_socket(fd);
            give_mutex("mconfig");
            break;
        }
        set_client_attrib(aaa_client_no, ([ "type": "internal", "peek": -1, "size": 131000 ]));
        send_to_client(aaa_client_no, 14849, 4, config["my_name"], make32(0));
        give_mutex("mconfig");
    } while (0);

    // --- label L0095 ---
    return aaa_client_no;

}

// --------------------------------------------------------------------
// @brief  verification: confirm/guarantee state
// @code  nc=24, nargs=0, insts=24
// --------------------------------------------------------------------
private void assure_config_agent() {
    /* ---- Enhanced Pseudocode ---- */
    if ((!is_object(config_agent))) {
        config_agent = clone_object("/clone/misc/mixed_agent.c");
        config_agent->set_client_no(aaa_client_no);
        config_agent->set_cmd_module("/daemons/configd.c");
    }

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
mixed get_tts_address() {
    /* ---- Enhanced Pseudocode ---- */
    return config["tts_info"];

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=21, nargs=0, insts=21
// --------------------------------------------------------------------
mapping get_aaa_address() {
    /* ---- Enhanced Pseudocode ---- */
    if ((!sizeof(aaa_addrs))) {
        return ([ ]);
    }
    return ([ "ip": inettoa(aaa_addrs[random(sizeof(aaa_addrs))]), "port": AAA_DOMAIN_PORT ]);

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
// @brief  predicate: boolean check
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
int is_citywar_compete_dist() {
    /* ---- Enhanced Pseudocode ---- */
    return (32768 == CS_CITYWAR_ROLE);

}

// --------------------------------------------------------------------
// @code  nc=20, nargs=1, insts=20
// --------------------------------------------------------------------
int wait_config(mixed timeout) {
    /* ---- Enhanced Pseudocode ---- */
    if (config["my_port"]) {
        return 1;
    }
    return (wait_event(ev_config, 32768, 0, timeout) == 0);

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        39
   Code segments found:         39
   Successfully decoded:        39
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          2
      - /feature/daemon/register.c()
      - /feature/daemon/handler_module.c()
   ================================================================ */