/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_do_inquiry.o
 * Source: /aaa/daemons/server/cmd_l_do_inquiry.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 3, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _after_do_inquiry(mixed agent, string gid, string order_id, string pay_type, int result, int result_code, int gold);
void main(mixed agent, string gid, string order_id);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=20, nargs=7, insts=20
// --------------------------------------------------------------------
private void _after_do_inquiry(mixed agent, string gid, string order_id, string pay_type, int result, int result_code, int gold) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!is_object(agent))) {
        return;
    }
    agent->send_message(13095, gid, order_id, pay_type, result, result_code, gold);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=13, nargs=3, insts=13
// --------------------------------------------------------------------
void main(mixed agent, string gid, string order_id) {
    /* ---- Enhanced Pseudocode ---- */
    "/aaa/daemons/express_recharged.c"->do_inquiry(order_id, 0, (: _after_do_inquiry, agent, gid :));

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