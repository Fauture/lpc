/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_get_catalog_ex.o
 * Source: /dba/daemons/dbaserver/cmd_get_catalog_ex.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 17, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
int main(mixed agent, int cookie, string path, string name, string branch, string condition, int start_pos, int count);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
mixed get_db_type() {
    /* ---- Enhanced Pseudocode ---- */
    return "ddb";

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=220, nargs=8, insts=220
// --------------------------------------------------------------------
int main(mixed agent, int cookie, string path, string name, string branch, string condition, int start_pos, int count) {
    /* ---- Enhanced Pseudocode ---- */
    mixed sql_cmd;
    mixed arr;
    mixed cno;
    mixed str;
    int i;

    if ((!is_object(agent))) {
        catch(error("Agent is destructed.\n"));

        // --- label L0014 ---
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        agent->send_message(9231, cookie, -2, 0);
        return 0;
    }
    if ((name == "*")) {
        sql_cmd = mysql_bind_parameters("select name, time from data where path=? and branch=''", path);
    } else {
        sql_cmd = mysql_bind_parameters("select branch as name, time from data where path=? and name=? and branch<>''", path, name);
    }
    if ((condition != "")) {
        sql_cmd += (" and " + condition);
    }
    if ((count > 1000000)) {
        count = 1000000;
    }
    if ((count != 0)) {
        sql_cmd += (((" limit " + maximum((start_pos - 1), 0)) + ",") + count);
    }
    arr = mysql_select(cno, sql_cmd, count);
    if ((!is_object(agent))) {
        catch(error("Agent is destructed 2.\n"));

        // --- label L0115 ---
        return 0;
    }
    if ((!arr)) {
        agent->send_message(9231, cookie, -2, 0);
        return 0;
    }
    for (i = 0; i <= (sizeof(arr) - 1); i++) {
        if ((!is_mapping(arr[i]))) {
            continue;
        }
        arr[i] = ((arr[i]["name"] + "|$") + time(arr[i]["time"]));
    }  // end loop (verified)
    str = implode(arr, "\$");
    i = 0;

    // --- label L0183 ---
    while ((i < sizeof(str))) {
        agent->send_message(9229, cookie, lzss_compress(str[i..((i + 49152) - 1)]));
        i += 49152;
    }  // end loop (verified)
    agent->send_message(9231, cookie, 0, 0);
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