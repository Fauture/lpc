/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_query_citys.o
 * Source: /aaa/daemons/server/cmd_l_query_citys.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 8, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _main(mixed agent, string gid, string module, string country, string province);
void main(mixed agent, string gid, string module, string country, string province);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=79, nargs=5, insts=79
// --------------------------------------------------------------------
private void _main(mixed agent, string gid, string module, string country, string province) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed sql_cmd;
    mixed citys;
    mapping city;
    mixed city_name;
    mixed city_arr;

    if ((!is_object(agent))) {
        return;
    }
    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 1)) {
        return;
    }
    sql_cmd = mysql_bind_parameters("select distinct city from ip_info where country = ? and province = ?", country, province);
    citys = mysql_select(cno, sql_cmd, 50);
    if ((!is_array(citys))) {
        return;
    }
    city_arr = ({});
    loop (city in citys) {
        city_name = trim(city["city"]);
        if ((!sizeof(city_name))) {
            continue;
        }
        city_arr += ({ city_name });
    }  // end loop (verified)
    agent->send_message(9121, gid, module, country, province, save_string(city_arr));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=20, nargs=5, insts=20
// --------------------------------------------------------------------
void main(mixed agent, string gid, string module, string country, string province) {
    /* ---- Enhanced Pseudocode ---- */
    mixed f;

    f = (: _main, agent, gid, module, country, province :);
    "/daemons/threadd.c"->dispatch(random(10), f);

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