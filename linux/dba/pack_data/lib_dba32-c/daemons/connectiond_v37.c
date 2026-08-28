/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: connectiond.o
 * Source: /daemons/connectiond.c
 * Inherits: (none)
 * Inherited programs: /feature/daemon/eternal_data.c()
 * Functions: 12 names, 12 code segments found
 * Strings: 23, Variables: 4
 *********************************************************************/

inherit "/feature/daemon/eternal_data.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   mapping auth_func;
   array ban_func;
   array disconnect_func;
   /* mixed global_0; */

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void connection_lost(object agent);
private mixed auth_in_config_list(string list_name, object agent, string name, int cookie);
void connection_disconnect(object agent);
private void do_when_new_connection(int client_no);
void register_disconnect(function func);
void register_auth(string cmd, function func);
void register_ban(function func);
private void destruct();
private mixed auth_in_config_info(string info_name, object agent, string name, int cookie);
void process_command(object agent, string cmd, array args);
private void create();
mixed auth_connection(object agent, int type, string name, int cookie);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   auth_func = ([ ]);  // /daemons/connectiond.c:16
   ban_func = ({ });
   disconnect_func = ({ });

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=7, nargs=1, insts=7
// --------------------------------------------------------------------
void connection_lost(object agent) {
    /* ---- Enhanced Pseudocode ---- */
    destruct_object(agent);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=50, nargs=4, insts=50
// --------------------------------------------------------------------
private mixed auth_in_config_list(string list_name, object agent, string name, int cookie) {
    /* ---- Enhanced Pseudocode ---- */
    mapping server_info;
    mixed server_list;

    if ((!find_object("/daemons/configd.c"))) {
        return 0;
    }
    if ((!is_array(server_list = "/daemons/configd.c"->get_config(list_name)))) {
        return 0;
    }
    loop (server_info in server_list) {
        if (((0 <= member_array(agent->get_peer_ip(), explode(server_info["ip"], ","))) && (server_info["name"] == name))) {
            return server_info;
        }
    }  // end loop (verified)
    return 0;

}

// --------------------------------------------------------------------
// @code  nc=24, nargs=1, insts=24
// --------------------------------------------------------------------
void connection_disconnect(object agent) {
    /* ---- Enhanced Pseudocode ---- */
    mixed f;

    clean_array(disconnect_func, 9);
    loop (f in disconnect_func) {
        if (is_function(f)) {
            (*f)(agent);
        }
    }  // end loop (verified)
    destruct_object(agent);

}

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=71, nargs=1, insts=71
// --------------------------------------------------------------------
private void do_when_new_connection(int client_no) {
    /* ---- Enhanced Pseudocode ---- */
    mixed func;
    string prompt;
    mixed ret;
    mixed agent;

    agent = clone_object("/clone/misc/unauthed_agent.c");
    agent->set_client_no(client_no);
    agent->set_cmd_module("/daemons/connectiond.c");
    clean_array(ban_func, 9);
    prompt = "连接服务器失败。（原因：1006）";
    loop (func in ban_func) {
        try {
            ret = (*func)(agent, &prompt);
        } catch { /* error handled */ }

        // --- label L0042 ---
        if (0 || ret) {
            agent->send_message(8165, prompt, 1);
            agent->to_be_disconnect(1);
        }
    }  // end loop (verified)
    set_client_attrib(client_no, ([ "type": "internal", "peek": 20 ]));

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=1, insts=9
// --------------------------------------------------------------------
void register_disconnect(function func) {
    /* ---- Enhanced Pseudocode ---- */
    disconnect_func += ({ func });

}

// --------------------------------------------------------------------
// @code  nc=10, nargs=2, insts=10
// --------------------------------------------------------------------
void register_auth(string cmd, function func) {
    /* ---- Enhanced Pseudocode ---- */
    auth_func[cmd] = func;

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=1, insts=9
// --------------------------------------------------------------------
void register_ban(function func) {
    /* ---- Enhanced Pseudocode ---- */
    ban_func += ({ func });

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=4, nargs=0, insts=4
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=44, nargs=4, insts=44
// --------------------------------------------------------------------
private mixed auth_in_config_info(string info_name, object agent, string name, int cookie) {
    /* ---- Enhanced Pseudocode ---- */
    mapping server_info;

    if ((!find_object("/daemons/configd.c"))) {
        return 0;
    }
    if ((!is_mapping(server_info = "/daemons/configd.c"->get_config(info_name)))) {
        return 0;
    }
    if (((0 <= member_array(agent->get_peer_ip(), explode(server_info["ip"], ","))) && (server_info["name"] == name))) {
        return server_info;
    }
    return 0;

}

// --------------------------------------------------------------------
// @code  nc=48, nargs=3, insts=48
// --------------------------------------------------------------------
void process_command(object agent, string cmd, array args) {
    /* ---- Enhanced Pseudocode ---- */
    if (enable_stat_cmds) {
        cmds_flow_in[cmd] = ((int)cmds_flow_in[cmd] + 1);
    }
    if ((!auth_func[cmd])) {
        destruct_object(agent);
        return;
    }
    (*find_function_arg(auth_func[cmd], agent, args))();
    if (enable_stat_cmds) {
        cmds_processed[cmd] = ((int)cmds_processed[cmd] + 1);
    }

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=15, nargs=0, insts=15
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    annouce_eternal_data("auth_func", &auth_func);
    annouce_eternal_data("ban_func", &ban_func);
    register_post_new_connection((: do_when_new_connection :));

}

// --------------------------------------------------------------------
// @code  nc=54, nargs=4, insts=54
// --------------------------------------------------------------------
mixed auth_connection(object agent, int type, string name, int cookie) {
    /* ---- Enhanced Pseudocode ---- */
    switch (type) {
        case 5:
        case 18:
        return auth_in_config_list("game_server_list", agent, name, cookie);
        case 7:
        return (auth_in_config_list("ccs_list", agent, name, cookie) || auth_in_config_info("ccs_info", agent, name, cookie));
        case 11:
        return auth_in_config_list("cta_list", agent, name, cookie);
        case 9:
        if (0) {
            return auth_in_config_info("spa_info", agent, name, cookie);
        }
        return ([ ]);
        default:
        return 0;
    }  // end switch

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        12
   Code segments found:         12
   Successfully decoded:        12
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          1
      - /feature/daemon/eternal_data.c()
   ================================================================ */