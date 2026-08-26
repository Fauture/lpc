/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: handler_module.o
 * Source: /feature/daemon/handler_module.c
 * Inherits: (none)
 * Functions: 4 names, 4 code segments found
 * Strings: 2, Variables: 0
 *********************************************************************/

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void connection_lost(object agent);
void connection_disconnect(object agent);
void process_command(object agent, string cmd, array args);
void load_all_handlers(string path);

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
// @code  nc=7, nargs=1, insts=7
// --------------------------------------------------------------------
void connection_disconnect(object agent) {
    /* ---- Enhanced Pseudocode ---- */
    destruct_object(agent);

}

// --------------------------------------------------------------------
// @code  nc=21, nargs=3, insts=21
// --------------------------------------------------------------------
void process_command(object agent, string cmd, array args) {
    /* ---- Enhanced Pseudocode ---- */
    mixed handler;

    if ((!is_object(handler = query_entry(cmd)))) {
        return;
    }
    call_other_arg(handler, "main", (({ agent }) + args));

}

// --------------------------------------------------------------------
// @code  nc=10, nargs=1, insts=10
// --------------------------------------------------------------------
void load_all_handlers(string path) {
    /* ---- Enhanced Pseudocode ---- */
    load_all(path, ([ "module": this_object() ]));

}

/* ================================================================
   DECLARED FUNCTIONS (no bytecode - likely inherited or forward decl)
   ================================================================ */
/*
   mixed query_entry(mixed index);  // inherited from parent or forward declaration
   int register_entry(mixed index, mixed value, int para);  // inherited from parent or forward declaration
   mapping query_all_entries();  // inherited from parent or forward declaration
*/

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        4
   Code segments found:         4
   Successfully decoded:        4
   Unresolved/minimal:          0
   Functions without code:      3

   Note: Functions without code are usually inherited from the
   parent object (), forward declarations, or
   simple property accessor methods that the compiler optimizes away.
   ================================================================ */