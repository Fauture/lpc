/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_read_log.o
 * Source: /dba/daemons/dbaserver/cmd_read_log.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 7, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
int main(mixed agent, int cookie, string db, string sql_cmd, int start_pos, int count);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=4, nargs=0, insts=4
// --------------------------------------------------------------------
mixed get_db_type() {
    /* ---- Enhanced Pseudocode ---- */
    return;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=149, nargs=6, insts=149
// --------------------------------------------------------------------
int main(mixed agent, int cookie, string db, string sql_cmd, int start_pos, int count) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    mixed cno;
    mixed arr;
    mixed str;

    if ((!is_object(agent))) {
        catch(error("Agent is destructed.\n"));

        // --- label L0014 ---
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection(db);
    if ((cno == -1)) {
        agent->send_message(9243, cookie, -2, 0);
        return 0;
    }
    if ((count > 2048)) {
        count = 2048;
    }
    if ((count != 0)) {
        sql_cmd += (((" limit " + maximum((start_pos - 1), 0)) + ",") + count);
    }
    arr = mysql_select(cno, sql_cmd, count);
    if ((!is_object(agent))) {
        catch(error("Agent is destructed 2.\n"));

        // --- label L0085 ---
        return 0;
    }
    if ((!arr)) {
        agent->send_message(9243, cookie, -2, 0);
        return 0;
    }
    str = save_string(arr);
    i = 0;

    // --- label L0112 ---
    while ((i < sizeof(str))) {
        agent->send_message(9229, cookie, lzss_compress(str[i..((i + 49152) - 1)]));
        i += 49152;
    }  // end loop (verified)
    agent->send_message(9243, cookie, 0, 0);
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