/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_query_ip_area.o
 * Source: /aaa/daemons/server/cmd_l_query_ip_area.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 12, Variables: 2
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   int enable;
   /* mixed global_0; */

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void query_ip_area(mixed agent, string account, string digital_ip, string module);
void main(object agent, string account, string digital_ip, string module);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   enable = 1;

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=107, nargs=4, insts=107
// --------------------------------------------------------------------
private void query_ip_area(mixed agent, string account, string digital_ip, string module) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed ip_int;
    mixed head_ip_int;
    mixed sql_cmd;
    string result;
    mixed head_ip_uint_str;
    mixed ip_uint_str;
    mapping map;

    if ((!is_object(agent))) {
        return;
    }
    if ((!enable)) {
        agent->send_message(9107, account, digital_ip, module, "");
        return;
    }
    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 1)) {
        return;
    }
    ip_int = atoinet(digital_ip);
    head_ip_int = (ip_int & -16777216);
    ip_uint_str = to_unsigned_str(ip_int);
    head_ip_uint_str = to_unsigned_str(head_ip_int);
    sql_cmd = mysql_bind_parameters("select * from ip_info force index (start_ip) where start_ip between ? and ? and end_ip >= ?", head_ip_uint_str, ip_uint_str, ip_uint_str);
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