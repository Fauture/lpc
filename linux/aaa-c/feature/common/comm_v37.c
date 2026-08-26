/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: comm.o
 * Source: /feature/common/comm.c
 * Inherits: (none)
 * Functions: 12 names, 12 code segments found
 * Strings: 8, Variables: 5
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   int client_no;
   int debug_flag;
   string peer_ip;
   int temp_flag;
   int timer_id;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void connection_lost();
varargs void send_message(int msg_no);
void set_debug_flag(int new_debug_flag);
int is_comm();
int get_client_no();
void set_client_no(int new_client_no);
void to_be_disconnect(int timeout);
private void destruct();
string get_peer_ip();
private void disconnect();
private void create(mapping m);
void relay_comm(object comm);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   client_no = 65535;  // /feature/common/comm.c:10
   peer_ip = "Unknown";
   timer_id = 65535;

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=12, nargs=0, insts=12
// --------------------------------------------------------------------
void connection_lost() {
    /* ---- Enhanced Pseudocode ---- */
    this_object()->comm_connection_lost();
    client_no = -1;

}

// --------------------------------------------------------------------
// @code  nc=32, nargs=1, insts=32
// --------------------------------------------------------------------
varargs void send_message(int msg_no) {
    /* ---- Enhanced Pseudocode ---- */
    mixed buf;

    buf = pkt_pad_interface(msg_no, ($<)[1..<1]);
    if (debug_flag) {
        temp_flag = debug_on();
    }
    send_to_client(client_no, msg_no, buf);
    if (debug_flag) {
        debug_on(temp_flag);
    }

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
void set_debug_flag(int new_debug_flag) {
    /* ---- Enhanced Pseudocode ---- */
    debug_flag = new_debug_flag;

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int is_comm() {
    /* ---- Enhanced Pseudocode ---- */
    return 1;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int get_client_no() {
    /* ---- Enhanced Pseudocode ---- */
    return client_no;

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=27, nargs=1, insts=27
// --------------------------------------------------------------------
void set_client_no(int new_client_no) {
    /* ---- Enhanced Pseudocode ---- */
    client_no = new_client_no;
    if ((!is_string(peer_ip = get_client_info(client_no)["remote_address"]))) {
        peer_ip = "Unknown";
    }
    this_object()->comm_set_client_no(client_no);

}

// --------------------------------------------------------------------
// @brief  converter: transform data
// @code  nc=23, nargs=1, insts=23
// --------------------------------------------------------------------
void to_be_disconnect(int timeout) {
    /* ---- Enhanced Pseudocode ---- */
    if ((timer_id != -1)) {
        delete_timer(timer_id);
        timer_id = -1;
    }
    timer_id = set_timer(timeout, 0, (: disconnect :));

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=12, nargs=0, insts=12
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    if ((client_no != -1)) {
        close_client(client_no);
    }

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
string get_peer_ip() {
    /* ---- Enhanced Pseudocode ---- */
    return peer_ip;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=34, nargs=0, insts=34
// --------------------------------------------------------------------
private void disconnect() {
    /* ---- Enhanced Pseudocode ---- */
    if ((client_no != -1)) {
        reset_cmd_cost_stat(client_no);
    }
    this_object()->comm_connection_disconnect();
    close_client(client_no);
    client_no = -1;
    if ((timer_id != -1)) {
        delete_timer(timer_id);
        timer_id = -1;
    }

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=6, nargs=1, insts=6
// --------------------------------------------------------------------
private void create(mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    return ([ ]);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=40, nargs=1, insts=40
// --------------------------------------------------------------------
void relay_comm(object comm) {
    /* ---- Enhanced Pseudocode ---- */
    mixed peek;

    if ((client_no == -1)) {
        return;
    }
    peek = get_client_info(client_no)["peek"];
    set_client_attrib(client_no, ([ "peek": 0 ]));
    comm->set_client_no(client_no);
    client_no = -1;
    set_client_attrib(comm->get_client_no(), ([ "peek": peek ]));

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        12
   Code segments found:         12
   Successfully decoded:        12
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */