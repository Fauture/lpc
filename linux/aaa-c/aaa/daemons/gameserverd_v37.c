/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: gameserverd.o
 * Source: /aaa/daemons/gameserverd.c
 * Inherits: (none)
 * Inherited programs: /feature/daemon/eternal_data.c()
 * Functions: 4 names, 4 code segments found
 * Strings: 7, Variables: 2
 *********************************************************************/

inherit "/feature/daemon/eternal_data.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   mapping all_conns;
   /* mixed global_0; */

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void destruct();
private void create();
mapping get_all_conns();
int assure_connection(string server);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   all_conns = ([ ]);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

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
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
mapping get_all_conns() {
    /* ---- Enhanced Pseudocode ---- */
    return all_conns;

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
   Total function names:        4
   Code segments found:         4
   Successfully decoded:        4
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          1
      - /feature/daemon/eternal_data.c()
   ================================================================ */