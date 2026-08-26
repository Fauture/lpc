/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_request_ip_card.o
 * Source: /aaa/daemons/server/cmd_l_request_ip_card.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 11, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void request_ip_card(mixed agent, string account, string gid, string type);
void main(object agent, string account, string gid, string type);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=117, nargs=4, insts=117
// --------------------------------------------------------------------
private void request_ip_card(mixed agent, string account, string gid, string type) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    int result;
    string card_no;
    mixed arr;
    mixed select_sql_cmd;
    mixed update_sql_cmd;

    if ((!is_object(agent))) {
        return;
    }
    result = 3;
    card_no = "";
    do {
        cno = "/daemons/dbd.c"->assure_connection("adb");
        if ((-1 == cno)) {
            printf("[1;31mÁ¬½Ó ADB Êý¾Ý¿âÊ§°Ü\n[2;37;0m");
            break;
        }
        select_sql_cmd = mysql_bind_parameters("select * from ip_card where type = ? and used = ?", type, 1);
        arr = mysql_select(cno, select_sql_cmd);
        if ((sizeof(arr) <= 0)) {
            break;
        }
        card_no = arr[0]["card_no"];
        update_sql_cmd = mysql_bind_parameters("update ip_card set used = ?, account = ?, gid = ?, request_time = ? where card_no = ? and type = ? and used = ?", 1, account, gid, ctime(time(), 3), card_no, type, 0);
        if (((*f_execute)(cno, update_sql_cmd) <= 0)) {
            break;
        }
        if ((sizeof(mysql_select(cno, select_sql_cmd)) <= 0)) {
            result = 2;
        } else {
            result = 1;
        }
    } while (0);

    // --- label L0101 ---
    if ((!is_object(agent))) {
        return;
    }
    agent->send_message(21355, gid, type, card_no, result);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=21, nargs=4, insts=21
// --------------------------------------------------------------------
void main(object agent, string account, string gid, string type) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: request_ip_card, agent, account, gid, type :));

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