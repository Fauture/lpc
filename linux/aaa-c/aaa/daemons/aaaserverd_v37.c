/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: aaaserverd.o
 * Source: /aaa/daemons/aaaserverd.c
 * Inherits: (none)
 * Functions: 7 names, 7 code segments found
 * Strings: 10, Variables: 2
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   int in_service;
   mapping my_info;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private mixed get_my_info();
int stop_service();
private void destruct();
private void do_init();
void wait_finish_service();
private void create();
int start_service();

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   in_service = 0;

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
private mixed get_my_info() {
    /* ---- Enhanced Pseudocode ---- */
    return "/aaa/daemons/configd.c"->get_my_info();

}

// --------------------------------------------------------------------
// @code  nc=30, nargs=0, insts=30
// --------------------------------------------------------------------
int stop_service() {
    /* ---- Enhanced Pseudocode ---- */
    mixed agent;

    if ((!start_service_on(0))) {
        return 0;
    }
    in_service = 0;
    clean_mapping_by_value(client_agents, 10);
    loop (agent in values(client_agents)) {
        destruct_object(agent);
    }  // end loop (verified)
    return 1;

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
// @code  nc=26, nargs=0, insts=26
// --------------------------------------------------------------------
private void do_init() {
    /* ---- Enhanced Pseudocode ---- */
    do {
        my_info = get_my_info();
        if ((!is_mapping(my_info))) {
            printf("[1;31mªÒ»° AAA ≈‰÷√◊ —∂ ß∞‹£¨…‘∫Û÷ÿ ‘\n[2;37;0m");
            sleep(3);
        }
    } while ((!is_mapping(my_info)));
    start_service();

}

// --------------------------------------------------------------------
// @code  nc=22, nargs=0, insts=22
// --------------------------------------------------------------------
void wait_finish_service() {
    /* ---- Enhanced Pseudocode ---- */
    clean_mapping_by_value(client_agents, 10);

    // --- label L0006 ---
    while ((sizeof(client_agents) > 0)) {
        sleep(10);
        clean_mapping_by_value(client_agents, 10);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=20, nargs=0, insts=20
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    require_uid();
    set_euid(0);
    register_post_init((: do_init :));
    _server_name = trim(read_ini("Me", "Name", INI_FILE));

}

// --------------------------------------------------------------------
// @code  nc=54, nargs=0, insts=54
// --------------------------------------------------------------------
int start_service() {
    /* ---- Enhanced Pseudocode ---- */
    int port;

    if ((!is_int(my_info["id"]))) {
        printf("[1;31m»±…Ÿ AAA id ◊÷∂Œ≈‰÷√£¨AAA æ‹æ¯Ã·π©∑˛ŒÒ\n[2;37;0m");
        return 0;
    }
    port = atoi(my_info["port"]);
    if ((port <= 0)) {
        port = 8001;
    }
    if ((!start_service_on(port))) {
        broadcast("AAA failed to listen on port: %d\n", port);
        shutdown();
        return 0;
    }
    in_service = 1;
    printf("[1;32mStart AAA service on port: %d\n[2;37;0m", port);
    return 1;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        7
   Code segments found:         7
   Successfully decoded:        7
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */