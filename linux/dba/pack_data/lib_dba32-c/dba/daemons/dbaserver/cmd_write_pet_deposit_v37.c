/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_write_pet_deposit.o
 * Source: /dba/daemons/dbaserver/cmd_write_pet_deposit.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 3 names, 3 code segments found
 * Strings: 8, Variables: 0
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
// @code  nc=103, nargs=6, insts=103
// --------------------------------------------------------------------
int main(mixed agent, int cookie, string pet_iid, string status, string branch, buffer content) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed ret;
    mixed sql_cmd;
    mixed m;

    if ((!is_object(agent))) {
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno == -1)) {
        agent->send_message(21509, cookie, -2, 0);
        return 0;
    }
    if (sizeof(content)) {
        try {
            m = restore_value((string)content);
        } catch { /* error handled */ }
    }
    if ((0 || (sizeof(m) == 0))) {
        agent->send_message(21509, cookie, -4, 0);
        return 0;
    }
    sql_cmd = (("update pet_deposit set " + convert_mapping_to_sql_cmd(m)) + mysql_bind_parameters(" where pet_iid=? and status=?", pet_iid, atoi(status)));
    ret = (((*f_execute)(cno, sql_cmd) >= 1) ? 0 : -1);
    agent->send_message(21509, cookie, ret, 0);
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=63, nargs=1, insts=63
// --------------------------------------------------------------------
private string convert_mapping_to_sql_cmd(mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    int index;
    mixed arr;
    mixed field_list;
    mixed value_list;

    field_list = allocate_array(sizeof(m));
    value_list = allocate_array(sizeof(m));
    index = 0;
    loop (arr in m) {
        if ((!is_string(arr[0]))) {
            continue;
        }
        field_list[index] = sprintf("%s=?", arr[0]);
        value_list[index] = arr[1];
        ++index;
    }  // end loop (verified)
    return mysql_bind_parameters_arr(implode(field_list, ","), value_list);

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