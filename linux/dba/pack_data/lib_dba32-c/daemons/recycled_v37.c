/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: recycled.o
 * Source: /daemons/recycled.c
 * Inherits: (none)
 * Functions: 3 names, 3 code segments found
 * Strings: 15, Variables: 0
 *********************************************************************/

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void delete_empty_vendue_store();
int specific_vendue_store(mapping record);
void delete_specific_records(string select_cmd, function specific_func);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=14, nargs=0, insts=14
// --------------------------------------------------------------------
void delete_empty_vendue_store() {
    /* ---- Enhanced Pseudocode ---- */
    mixed sql_cmd;

    sql_cmd = mysql_bind_parameters("select * from data where path = ?", "vendue_store");
    delete_specific_records(sql_cmd, (: specific_vendue_store :));

}

// --------------------------------------------------------------------
// @code  nc=20, nargs=1, insts=20
// --------------------------------------------------------------------
int specific_vendue_store(mapping record) {
    /* ---- Enhanced Pseudocode ---- */
    mixed m;

    m = restore_value(record["content"]);
    if ((sizeof(m) <= 2)) {
        return 1;
    }
    return 0;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=183, nargs=2, insts=183
// --------------------------------------------------------------------
void delete_specific_records(string select_cmd, function specific_func) {
    /* ---- Enhanced Pseudocode ---- */
    int vno;
    int cno;
    mixed record_num;
    int i;
    int n;
    int j;
    mixed sql_cmd;
    mapping m;
    mixed arr;
    mixed records;

    cno = "/daemons/dbd.c"->assure_connection("ddb");
    if ((cno < 0)) {
        error("TYPE_DDB connection failed!");
        return;
    }
    vno = (*f_query)(cno, select_cmd);
    if ((vno < 0)) {
        error("Error when mysql_query");
        return;
    }
    record_num = (*f_num_records)(vno);
    records = allocate_array(record_num);
    n = 0;
    for (i = 0; i <= (record_num - 1); i++) {
        if (((*f_fetch_record)(vno) != 0)) {
            error(sprintf("Error when fetch %d record", i));
            return;
        }
        m = (*f_this_record)(vno);
        if ((*specific_func)(m)) {
            records[n++] = ({ m["path"], m["name"], m["branch"] });
        }
    }  // end loop (verified)
    (*f_finalize)(vno);
    i = 0;

    // --- label L0111 ---
    while (i < n) {
        sql_cmd = sprintf("delete from data where");
        j = i;

        // --- label L0124 ---
        while (((j < (i + 500)) && (j < n))) {
            arr = records[j];
            if ((j > i)) {
                sql_cmd += " or";
            }
            sql_cmd += mysql_bind_parameters(" (path = ? and name = ? and branch = ?)", arr[0], arr[1], arr[2]);
            ++j;
        }  // end loop (verified)
        (*f_execute)(cno, sql_cmd);
        i += 500;
    }  // end loop (verified)

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        3
   Code segments found:         3
   Successfully decoded:        3
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */