/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_aaa_shutdown.o
 * Source: /aaa/daemons/server/cmd_aaa_shutdown.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 5, Variables: 2
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   int flag_received;
   /* mixed global_0; */

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void _main(mixed agent);
void main(mixed server);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   flag_received = 0;

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=13, nargs=1, insts=13
// --------------------------------------------------------------------
void _main(mixed agent) {
    /* ---- Enhanced Pseudocode ---- */
    printf("[1;32m½ÓÊÕÀ´×ÔCCSµÄÔ¶¶Ë¹Ø»úÇëÇó£¬×¼±¸°²È«¹Ø»ú\n[2;37;0m");
    "/aaa/daemons/aaaserverd.c"->stop_service();
    safe_reboot();

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=19, nargs=1, insts=19
// --------------------------------------------------------------------
void main(mixed server) {
    /* ---- Enhanced Pseudocode ---- */
    if (flag_received) {
        return;
    }
    flag_received = 1;
    "/daemons/threadd.c"->dispatch(random(10), (: _main, server :));

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        2
   Code segments found:         2
   Successfully decoded:        2
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          1
      - /inherit/misc/handler.c()
   ================================================================ */