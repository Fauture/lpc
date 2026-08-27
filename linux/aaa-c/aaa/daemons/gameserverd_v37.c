/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: gameserverd.o
 * Source: /aaa/daemons/gameserverd.c
 * Inherits: (none)
 * Inherited programs: /feature/daemon/eternal_data.c()
 * Functions: 7 names, 7 code segments found
 * Strings: 13, Variables: 7
 *********************************************************************/

inherit "/feature/daemon/eternal_data.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   mapping all_conns;
   int enable_select_optimal_server;
   /* mixed global_0; */
   int max_wait_login_num;
   int optimal_login_server_timeout;
   int timer_check_wait_login_interval;
   int wait_login_info_timeout;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void check_wait_login();
private void destruct();
private void create();
mixed select_optimal_server(string dist, string server);
mapping get_all_conns();
void set_wait_login_num(string server, int num);
int assure_connection(string server);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   all_conns = ([ ]);  // /aaa/daemons/gameserverd.c:19
   max_wait_login_num = 50;  // /aaa/daemons/gameserverd.c:196
   enable_select_optimal_server = 1;
   wait_login_info_timeout = 15;
   timer_check_wait_login_interval = 5;
   optimal_login_server_timeout = 5;

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  validator: verify data
// @code  nc=51, nargs=0, insts=51
// --------------------------------------------------------------------
private void check_wait_login() {
    /* ---- Enhanced Pseudocode ---- */
    mixed cur_ti;
    int server;
    mapping m;

    cur_ti = time();
    loop (server in keys(limit_login_server)) {
        if ((!is_mapping(m = limit_login_server[server]))) {
            continue;
        }
        if (((cur_ti - m["time"]) < wait_login_info_timeout)) {
            continue;
        }
        map_delete(limit_login_server, server);
    }  // end loop (verified)
    if ((sizeof(limit_login_server) && (!get_timer("tCheckWaitLogin")))) {
        set_timer(timer_check_wait_login_interval, "tCheckWaitLogin", (: check_wait_login :));
    }

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
// @brief  constructor: initialize object state
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    annouce_eternal_data("all_conns", &all_conns);

}

// --------------------------------------------------------------------
// @brief  query: database retrieval
// @code  nc=113, nargs=2, insts=113
// --------------------------------------------------------------------
mixed select_optimal_server(string dist, string server) {
    /* ---- Enhanced Pseudocode ---- */
    int online;
    int min_online;
    mixed client_no;
    int min_online_server;
    mapping server_list;

    if ((!enable_select_optimal_server)) {
        return;
    }
    if ((!is_mapping(limit_login_server[server]))) {
        return;
    }
    if ((is_string(optimal_login_server) && ((time() - optimal_login_server_time) < optimal_login_server_timeout))) {
        return optimal_login_server;
    }
    min_online = 2147483647;
    server_list = (mapping)"/aaa/daemons/configd.c"->get_server_list_by_dist(dist);
    loop (server in (keys(server_list) - keys(limit_login_server))) {
        online = (int)server_list[server]["online"];
        if ((online && (online < min_online))) {
            client_no = assure_connection(server);
            if ((client_no == -1)) {
                continue;
            }
            if ((!is_object(find_agent_by_client(client_no)))) {
                continue;
            }
            if ((!server_list[server]["available"])) {
                continue;
            }
            min_online = online;
            min_online_server = server;
        }
    }  // end loop (verified)
    if (is_string(min_online_server)) {
        optimal_login_server = min_online_server;
        optimal_login_server_time = time();
    }
    return min_online_server;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
mapping get_all_conns() {
    /* ---- Enhanced Pseudocode ---- */
    return all_conns;

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=28, nargs=2, insts=28
// --------------------------------------------------------------------
void set_wait_login_num(string server, int num) {
    /* ---- Enhanced Pseudocode ---- */
    if ((num < max_wait_login_num)) {
        return;
    }
    limit_login_server[server] = ([ "time": time(), "wait_num": num ]);
    if ((!get_timer("tCheckWaitLogin"))) {
        set_timer(wait_login_info_timeout, "tCheckWaitLogin", (: check_wait_login :));
    }

}

// --------------------------------------------------------------------
// @brief  verification: confirm/guarantee state
// @code  nc=92, nargs=1, insts=92
// --------------------------------------------------------------------
int assure_connection(string server) {
    /* ---- Enhanced Pseudocode ---- */
    mixed fd;
    mixed client_no;
    mapping server_info;

    if ((is_int(client_no = all_conns[server]) && (get_client_info(client_no)["alive"] == 1))) {
        return client_no;
    }
    server_info = "/aaa/daemons/configd.c"->get_server_info(server);
    if ((((!is_mapping(server_info)) || (server_info["ip"] == "")) || (server_info["port"] == ""))) {
        return -1;
    }
    fd = socket(2);
    if ((connect(fd, server_info["ip"], server_info["port"]) != 0)) {
        close_socket(fd);
        return -1;
    }
    if (((client_no = bind_to_client(fd)) < 0)) {
        close_socket(fd);
        return -1;
    }
    all_conns[server] = client_no;
    return client_no;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        7
   Code segments found:         7
   Successfully decoded:        7
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          1
      - /feature/daemon/eternal_data.c()
   ================================================================ */