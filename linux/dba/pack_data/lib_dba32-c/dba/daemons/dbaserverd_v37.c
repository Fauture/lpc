/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: dbaserverd.o
 * Source: /dba/daemons/dbaserverd.c
 * Inherits: (none)
 * Inherited programs: /feature/daemon/register.c(), /feature/daemon/eternal_data.c(), /feature/daemon/handler_module.c()
 * Functions: 17 names, 17 code segments found
 * Strings: 66, Variables: 8
 *********************************************************************/

inherit "/feature/daemon/register.c";
inherit "/feature/daemon/eternal_data.c";
inherit "/feature/daemon/handler_module.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   mapping all_conns;
   mapping data_length_stat_conf;
   mapping data_length_stat_info;
   int data_length_stat_status;
   mapping db_record_info;
   /* mixed global_0; */
   /* mixed global_1; */
   int part_db_request;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

override void connection_lost(object agent);
private void auth_internal_connection(object agent, int type, string name, int cookie);
private void remove_agent(object agent);
void update_data_length_stat_info(string path, string name, string branch, string content);
private void do_when_new_connection(object agent, mapping client_info, int type, string name, int cookie);
private void load_data_length_stat_info();
void stop_service(mixed client_no_arr);
private void destruct();
private void record_ini_info();
private void do_init();
void save();
void wait_finish_service();
override void process_command(object agent, string cmd, array args);
private void create();
private void save_data_length_stat_info();
void start_service();
mapping query_all_conns();

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   all_conns = ([ ]);  // /dba/daemons/dbaserverd.c:24
   db_record_info = ([ "MDB" : "mdb_host", "LDB" : "ldb_host" ]);
   part_db_request = 1;
   data_length_stat_status = 1;
   data_length_stat_conf = ([ ]);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=10, nargs=1, insts=10
// --------------------------------------------------------------------
override void connection_lost(object agent) {
    /* ---- Enhanced Pseudocode ---- */
    remove_agent(agent);
    ::connection_lost(agent);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=30, nargs=4, insts=30
// --------------------------------------------------------------------
private void auth_internal_connection(object agent, int type, string name, int cookie) {
    /* ---- Enhanced Pseudocode ---- */
    mixed client_info;

    client_info = "/daemons/connectiond.c"->auth_connection(agent, type, name, cookie);
    if ((!is_mapping(client_info))) {
        destruct_object(agent);
        return;
    }
    do_when_new_connection(agent, client_info, type, name, cookie);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=1, insts=10
// --------------------------------------------------------------------
private void remove_agent(object agent) {
    /* ---- Enhanced Pseudocode ---- */
    map_delete(all_conns, agent->get_name());

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=173, nargs=4, insts=173
// --------------------------------------------------------------------
void update_data_length_stat_info(string path, string name, string branch, string content) {
    /* ---- Enhanced Pseudocode ---- */
    int ori_length;
    int length;
    mixed entry;
    int key;
    mixed memo;
    mixed para;

    if (((!data_length_stat_status) || (!is_mapping(data_length_stat_info)))) {
        return;
    }
    switch (path) {
        case "user":
        key = sprintf("%s_%s", path, branch);
        break;
        default:
        key = path;
        break;
    }  // end switch

    // --- label L0029 ---
    if ((!is_int(ori_length = data_length_stat_conf[key]))) {
        ori_length = 64512;
    }
    if (is_mapping(data_length_stat_info[key])) {
        ori_length = maximum((int)data_length_stat_info[key]["length"], ori_length);
    }
    length = strlen(content);
    if ((length > ori_length)) {
        data_length_stat_info[key] = ([ "path": path, "name": name, "branch": branch, "length": length ]);
    }
    if ((length <= 64512)) {
        return;
    }
    catch(error("Too large data(length=%d)!\n", length));

    // --- label L0095 ---
    write_file(("/dba/log/" + sprintf("too_large_data_%s_%s_%s.txt", path, name, branch)), content);
    if ((!is_object(entry = query_entry("cmd_log_debug")))) {
        return;
    }
    para = ([ "type": "SORT_ERROR_D", "action": ctime(time(), 2), "para1": "1", "para2": sprintf("Too large data(length=%d)!", length), "para3": "/dba/daemons/dbaserverd.c" ]);
    memo = sprintf("path=%s, name=%s, branch=%s, length=%d, ori_length=%d, content = %s", path, name, branch, length, ori_length, content);
    if ((!part_db_request)) {
        "/daemons/threadd.c"->dispatch_service((: call_other_arg, entry, "_main", ({ _server_name, memo, save_string(para) }) :));
    } else {
        "/daemons/threadd.c"->dispatch_ldb_service((: call_other_arg, entry, "_main", ({ _server_name, memo, save_string(para) }) :));
    }

}

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=66, nargs=5, insts=66
// --------------------------------------------------------------------
private void do_when_new_connection(object agent, mapping client_info, int type, string name, int cookie) {
    /* ---- Enhanced Pseudocode ---- */
    mixed dba_agent;

    if (all_conns[name]) {
        destruct_object(agent);
        return;
    }
    dba_agent = clone_object("/clone/misc/mixed_agent.c");
    agent->relay_agent(dba_agent);
    dba_agent->set_name(name);
    set_client_attrib(dba_agent->get_client_no(), ([ "type": "internal", "size": 131000, "peek": -1 ]));
    destruct_object(agent);
    dba_agent->set_cmd_module("/dba/daemons/dbaserverd.c");
    if ((type == 11)) {
        dba_agent->set_cmd_module("/daemons/ctacommandd.c");
    }
    dba_agent->send_message(14850, type, name, 1, cookie);

}

// --------------------------------------------------------------------
// @code  nc=47, nargs=0, insts=47
// --------------------------------------------------------------------
private void load_data_length_stat_info() {
    /* ---- Enhanced Pseudocode ---- */
    mixed m;
    mixed content;

    if ((!data_length_stat_status)) {
        return;
    }
    if ((!is_string(content = read_file("/dba/log/data_length_stat_info.txt")))) {
        data_length_stat_info = ([ ]);
        return;
    }
    try {
        m = restore_value(content);
    } catch { /* error handled */ }

    // --- label L0029 ---
    if ((0 || (!is_mapping(m)))) {
        data_length_stat_info = ([ ]);
        return;
    }
    data_length_stat_info = m;

}

// --------------------------------------------------------------------
// @code  nc=60, nargs=1, insts=60
// --------------------------------------------------------------------
void stop_service(mixed client_no_arr) {
    /* ---- Enhanced Pseudocode ---- */
    mixed mks;

    printf("[1;32mStop %s service\n[2;37;0m", "DBA");
    start_service_on(0);
    if ((!is_array(client_no_arr))) {
        loop (mks in client_agents) {
            remove_agent(mks[1]);
            destruct_object(mks[1]);
        }  // end loop (verified)
    } else {
        loop (mks in client_agents) {
            if ((member_array(mks[0], client_no_arr) >= 0)) {
                continue;
            }
            remove_agent(mks[1]);
            destruct_object(mks[1]);
        }  // end loop (verified)
    }

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    save();

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=50, nargs=0, insts=50
// --------------------------------------------------------------------
private void record_ini_info() {
    /* ---- Enhanced Pseudocode ---- */
    mixed mks;
    mixed host;
    mixed port;

    loop (mks in db_record_info) {
        host = read_ini(mks[0], "Host", INI_FILE);
        host = inettoa(resolve(host)[0]);
        port = read_ini(mks[0], "Port", INI_FILE);
        "/daemons/configd.c"->write_etc("global", mks[1], sprintf("%s:%s", host, port));
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=10, nargs=0, insts=10
// --------------------------------------------------------------------
private void do_init() {
    /* ---- Enhanced Pseudocode ---- */
    start_service();
    record_ini_info();
    load_data_length_stat_info();

}

// --------------------------------------------------------------------
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
void save() {
    /* ---- Enhanced Pseudocode ---- */
    save_data_length_stat_info();

}

// --------------------------------------------------------------------
// @code  nc=11, nargs=0, insts=11
// --------------------------------------------------------------------
void wait_finish_service() {
    /* ---- Enhanced Pseudocode ---- */

    // --- label L0002 ---
    while (sizeof(all_conns)) {
        sleep(10);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=99, nargs=3, insts=99
// --------------------------------------------------------------------
override void process_command(object agent, string cmd, array args) {
    /* ---- Enhanced Pseudocode ---- */
    int timeout;
    mixed entry;

    if (get_comm_cmd_attrib(cmd, "critical")) {
        timeout = -1;
    } else {
        timeout = 15;
    }
    if (get_comm_cmd_attrib(cmd, "sequence")) {
        "/daemons/threadd.c"->dispatch(args[0], (: ::process_command, agent, cmd, args[1..<0] :), timeout);
        return;
    }
    if ((!part_db_request)) {
        "/daemons/threadd.c"->dispatch_service((: ::process_command, agent, cmd, args :), timeout);
        return;
    }
    if ((!is_object(entry = query_entry(cmd)))) {
        return;
    }
    switch (entry->get_db_type()) {
        case "ddb":
        "/daemons/threadd.c"->dispatch_ddb_service((: ::process_command, agent, cmd, args :), timeout);
        return;
        case "ldb":
        "/daemons/threadd.c"->dispatch_ldb_service((: ::process_command, agent, cmd, args :), timeout);
        return;
        default:
        "/daemons/threadd.c"->dispatch_service((: ::process_command, agent, cmd, args :), timeout);
        return;
    }  // end switch

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=25, nargs=0, insts=25
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    require_uid();
    set_euid(0);
    annouce_eternal_data("all_conns", &all_conns);
    load_all_handlers("/dba/daemons/dbaserver/");
    register_post_init((: do_init :));
    "/daemons/connectiond.c"->register_auth("cmd_internal_auth", (: auth_internal_connection :));

}

// --------------------------------------------------------------------
// @code  nc=24, nargs=0, insts=24
// --------------------------------------------------------------------
private void save_data_length_stat_info() {
    /* ---- Enhanced Pseudocode ---- */
    if ((!data_length_stat_status)) {
        return;
    }
    if (((!is_mapping(data_length_stat_info)) || (sizeof(data_length_stat_info) < 1))) {
        return;
    }
    write_file("/dba/log/data_length_stat_info.txt", save_string(data_length_stat_info));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=36, nargs=0, insts=36
// --------------------------------------------------------------------
void start_service() {
    /* ---- Enhanced Pseudocode ---- */
    mixed port;


    // --- label L0002 ---
    while ((!port = "/daemons/configd.c"->get_config("my_port"))) {
        "/daemons/configd.c"->wait_config(3);
    }  // end loop (verified)
    if ((!start_service_on(port))) {
        broadcast("%s failed to listen on port: %d\n", "DBA", port);
        shutdown();
        return;
    }
    printf("[1;32mStart %s service on port: %d\n[2;37;0m", "DBA", port);

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
mapping query_all_conns() {
    /* ---- Enhanced Pseudocode ---- */
    return all_conns;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        17
   Code segments found:         17
   Successfully decoded:        17
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          3
      - /feature/daemon/register.c()
      - /feature/daemon/eternal_data.c()
      - /feature/daemon/handler_module.c()
   ================================================================ */