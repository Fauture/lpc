/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: agent.o
 * Source: /feature/common/agent.c
 * Inherits: (none)
 * Functions: 9 names, 9 code segments found
 * Strings: 3, Variables: 1
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   mixed cmd_module;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

int is_agent();
mixed get_cmd_module();
void comm_connection_disconnect();
void comm_connection_lost();
void relay_agent(object agent);
void set_cmd_module(mixed new_cmd_module);
private void destruct();
void comm_set_client_no(int client_no);
void process_command(string cmd, array args);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int is_agent() {
    /* ---- Enhanced Pseudocode ---- */
    return 1;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
mixed get_cmd_module() {
    /* ---- Enhanced Pseudocode ---- */
    return cmd_module;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=16, nargs=0, insts=16
// --------------------------------------------------------------------
void comm_connection_disconnect() {
    /* ---- Enhanced Pseudocode ---- */
    if ((get_client_no() != -1)) {
        remove_client_agent(get_client_no());
    }
    cmd_module->connection_disconnect(this_object());

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=16, nargs=0, insts=16
// --------------------------------------------------------------------
void comm_connection_lost() {
    /* ---- Enhanced Pseudocode ---- */
    if ((get_client_no() != -1)) {
        remove_client_agent(get_client_no());
    }
    cmd_module->connection_lost(this_object());

}

// --------------------------------------------------------------------
// @code  nc=10, nargs=1, insts=10
// --------------------------------------------------------------------
void relay_agent(object agent) {
    /* ---- Enhanced Pseudocode ---- */
    remove_client_agent(get_client_no());
    relay_comm(agent);

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
void set_cmd_module(mixed new_cmd_module) {
    /* ---- Enhanced Pseudocode ---- */
    cmd_module = new_cmd_module;

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=11, nargs=0, insts=11
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    if ((get_client_no() != -1)) {
        remove_client_agent(get_client_no());
    }

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
void comm_set_client_no(int client_no) {
    /* ---- Enhanced Pseudocode ---- */
    set_client_agent(client_no, this_object());

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=11, nargs=2, insts=11
// --------------------------------------------------------------------
void process_command(string cmd, array args) {
    /* ---- Enhanced Pseudocode ---- */
    cmd_module->process_command(this_object(), cmd, args);

}

/* ================================================================
   DECLARED FUNCTIONS (no bytecode - likely inherited or forward decl)
   ================================================================ */
/*
   void relay_comm(object comm);  // inherited from parent or forward declaration
   string get_peer_ip();  // inherited from parent or forward declaration
   void set_client_no(int new_client_no);  // inherited from parent or forward declaration
   int get_client_no();  // inherited from parent or forward declaration
   int is_comm();  // inherited from parent or forward declaration
   void set_debug_flag(int new_debug_flag);  // inherited from parent or forward declaration
   varargs void send_message(int msg_no);  // inherited from parent or forward declaration
*/

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        9
   Code segments found:         9
   Successfully decoded:        9
   Unresolved/minimal:          0
   Functions without code:      7

   Note: Functions without code are usually inherited from the
   parent object (), forward declarations, or
   simple property accessor methods that the compiler optimizes away.
   ================================================================ */