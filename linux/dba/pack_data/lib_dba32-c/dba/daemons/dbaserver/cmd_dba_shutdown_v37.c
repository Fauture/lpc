/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_dba_shutdown.o
 * Source: /dba/daemons/dbaserver/cmd_dba_shutdown.c
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

int _main(mixed agent);
int main(mixed agent);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   flag_received = 0;

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=15, nargs=1, insts=15
// --------------------------------------------------------------------
int _main(mixed agent) {
    /* ---- Enhanced Pseudocode ---- */
    printf("[1;32m½ÓÊÜµ½CCSµÄÔ¶³Ì¹Ø»úÇëÇó£¬×¼±¸°²È«¹Ø»ú¡£\n[2;37;0m");
    "/dba/daemons/dbaserverd.c"->stop_service();
    safe_reboot();
    return 0;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=20, nargs=1, insts=20
// --------------------------------------------------------------------
int main(mixed agent) {
    /* ---- Enhanced Pseudocode ---- */
    if (flag_received) {
        return 1;
    }
    flag_received = 1;
    "/daemons/threadd.c"->dispatch_service((: _main, agent :));
    return 1;

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