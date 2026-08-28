/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_create_pet_deposit.o
 * Source: /dba/daemons/dbaserver/cmd_create_pet_deposit.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 3 names, 3 code segments found
 * Strings: 10, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed get_db_type();
int main(mixed agent, int cookie, string pet_iid, string status, string branch, buffer content);
private string convert_mapping_to_sql_cmd(mapping m);

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
// @code  nc=110, nargs=6, insts=110
// --------------------------------------------------------------------
int main(mixed agent, int cookie, string pet_iid, string status, string branch, buffer content) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed ret;
    mixed sql_cmd;
    mapping m;

    if ((!is_object(agent))) {
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        agent->send_message(21513, cookie, -2, 0);
        return 0;
    }
    if (sizeof(content)) {
        try {
            m = restore_value((string)content);
        } catch { /* error handled */ }
    }
    if ((0 || (sizeof(m) == 0))) {
        agent->send_message(21513, cookie, -4, 0);
        return 0;
    }
    m["pet_iid"] = pet_iid;
    m["status"] = atoi(status);
    sql_cmd = ("insert into pet_deposit " + convert_mapping_to_sql_cmd(m));
    ret = (((*f_execute)(cno, sql_cmd) >= 1) ? 0 : -1);
    agent->send_message(21513, cookie, ret, 0);
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=75, nargs=1, insts=75
// --------------------------------------------------------------------
private string convert_mapping_to_sql_cmd(mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    int index;
    mixed arr;
    mixed key_list;
    mixed value_list;
    mixed q_list;
    mixed sql_cmd;

    key_list = allocate_array(sizeof(m));
    value_list = allocate_array(sizeof(m));
    q_list = allocate_array(sizeof(m));
    index = 0;
    loop (arr in m) {
        key_list[index] = arr[0];
        q_list[index] = "?";
        value_list[index] = arr[1];
        ++index;
    }  // end loop (verified)
    sql_cmd = sprintf("(%s) values (%s)", implode(key_list, ","), implode(q_list, ","));
    return mysql_bind_parameters_arr(sql_cmd, value_list);

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        3
   Code segments found:         3
   Successfully decoded:        3
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          1
      - /inherit/misc/handler.c()
   ================================================================ */