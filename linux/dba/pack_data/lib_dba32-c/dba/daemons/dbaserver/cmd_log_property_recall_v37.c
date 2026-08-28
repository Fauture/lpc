/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_log_property_recall.o
 * Source: /dba/daemons/dbaserver/cmd_log_property_recall.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 6, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
int main(object agent, string type, string owner, string ob_iid, string ob_name, int ob_type, string data, int replace, int flag, string depend, string memo);

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
// @code  nc=121, nargs=11, insts=121
// --------------------------------------------------------------------
int main(object agent, string type, string owner, string ob_iid, string ob_name, int ob_type, string data, int replace, int flag, string depend, string memo) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    int take;
    mixed cur_time;
    mixed arr;
    mixed sql_cmd;

    if ((!is_object(agent))) {
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        return 0;
    }
    if (flag) {
        sql_cmd = mysql_bind_parameters("delete from property_recall where iid = ? and take = ?", ob_iid, 0);
        (*f_execute)(cno, sql_cmd);
    } else {
        sql_cmd = mysql_bind_parameters("select id from property_recall where iid = ? and take = ?", ob_iid, 0);
        arr = mysql_select(cno, sql_cmd);
        if ((!arr)) {
            return 0;
        }
    }
    if (((sizeof(arr) != 0) && (!flag))) {
        take = 2;
    } else {
        take = 0;
    }
    cur_time = time();
    sql_cmd = mysql_bind_parameters("insert into property_recall (update_time, type, owner, iid, name, take, ob_type, exchange, depend ,data, checksum, memo) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", ctime(cur_time, 3), type, owner, ob_iid, ob_name, take, ob_type, replace, depend, data, int32((create_check_sum(cur_time, owner, ob_iid, (string)take, data) ^ -1418120497)), memo);
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