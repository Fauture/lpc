/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_query_ip_area.o
 * Source: /aaa/daemons/server/cmd_l_query_ip_area.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 12, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void query_ip_area(mixed agent, string account, string digital_ip, string module);
void main(object agent, string account, string digital_ip, string module);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=77, nargs=4, insts=77
// --------------------------------------------------------------------
private void query_ip_area(mixed agent, string account, string digital_ip, string module) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed sql_cmd;
    string result;
    mapping map;
    mixed ip_int;

    if ((!is_object(agent))) {
        return;
    }
    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 1)) {
        return;
    }
    ip_int = to_unsigned_str(atoinet(digital_ip));
    sql_cmd = mysql_bind_parameters("select * from ip_info where start_ip <= ? and end_ip >= ?", ip_int, ip_int);
    map = mysql_select_single_result(cno, sql_cmd);
    if ((!is_object(agent))) {
        return;
    }
    if ((!is_mapping(map))) {
        result = "";
    } else {
        result = sprintf("%s@%s@%s", map["country"], map["province"], map["city"]);
    }
    agent->send_message(9107, account, digital_ip, module, result);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=15, nargs=4, insts=15
// --------------------------------------------------------------------
void main(object agent, string account, string digital_ip, string module) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch(random(10), (: query_ip_area, agent, account, digital_ip, module :));

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