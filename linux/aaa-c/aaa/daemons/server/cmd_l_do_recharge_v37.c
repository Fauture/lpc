/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_do_recharge.o
 * Source: /aaa/daemons/server/cmd_l_do_recharge.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 14, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _after_do_recharge(mixed agent, string gid, string order_id, string pay_type, int result, int result_code, int gold);
void main(mixed agent, string gid, string order_id, string card_no, string card_pwd, string user_name, string char_name, string server_name, string sales_name, string sales_ip, int value, string pay_type, string card_type);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=20, nargs=7, insts=20
// --------------------------------------------------------------------
private void _after_do_recharge(mixed agent, string gid, string order_id, string pay_type, int result, int result_code, int gold) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!is_object(agent))) {
        return;
    }
    agent->send_message(13093, gid, order_id, pay_type, result, result_code, gold);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=39, nargs=13, insts=39
// --------------------------------------------------------------------
void main(mixed agent, string gid, string order_id, string card_no, string card_pwd, string user_name, string char_name, string server_name, string sales_name, string sales_ip, int value, string pay_type, string card_type) {
    /* ---- Enhanced Pseudocode ---- */
    mixed info;

    info = ([ "order_id": order_id, "card_no": card_no, "card_pwd": card_pwd, "user_name": user_name, "char_name": char_name, "server_name": server_name, "sales_name": sales_name, "user_ip": sales_ip, "par_value": value, "pay_type": pay_type, "card_type": (int)card_type ]);
    "/aaa/daemons/express_recharged.c"->do_recharge(info, (: _after_do_recharge, agent, gid :));

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