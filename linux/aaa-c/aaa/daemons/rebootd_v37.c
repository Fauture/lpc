/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: rebootd.o
 * Source: /aaa/daemons/rebootd.c
 * Inherits: (none)
 * Functions: 3 names, 3 code segments found
 * Strings: 7, Variables: 0
 *********************************************************************/

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void reboot_daemon();
void reboot();
private void create();

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=23, nargs=0, insts=23
// --------------------------------------------------------------------
void reboot_daemon() {
    /* ---- Enhanced Pseudocode ---- */
    broadcast("[Reboot]: 开始关闭 AAA(%s) 服务器\n", _server_name);
    set_server_status(1);
    broadcast("[Reboot]: 停止 AAA 服务\n");
    "/aaa/daemons/aaaserverd.c"->stop_service();
    broadcast("AAA shutdown.\n");
    shutdown();

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
void reboot() {
    /* ---- Enhanced Pseudocode ---- */
    create_thread("tReboot", "reboot_daemon");

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=9, nargs=0, insts=9
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    require_uid();
    set_euid(0);

}

/* ================================================================
   DECLARED FUNCTIONS (no bytecode - likely inherited or forward decl)
   ================================================================ */
/*
   void reboot_daemons();  // inherited from parent or forward declaration
*/

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        3
   Code segments found:         3
   Successfully decoded:        3
   Unresolved/minimal:          0
   Functions without code:      1

   Note: Functions without code are usually inherited from the
   parent object (), forward declarations, or
   simple property accessor methods that the compiler optimizes away.
   ================================================================ */