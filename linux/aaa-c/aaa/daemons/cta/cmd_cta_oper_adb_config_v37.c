/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_cta_oper_adb_config.o
 * Source: /aaa/daemons/cta/cmd_cta_oper_adb_config.c
 * Inherits: (none)
 * Functions: 7 names, 7 code segments found
 * Strings: 39, Variables: 0
 *********************************************************************/

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void query_protect_phone(mapping paras);
private void _main(int client_no, mapping paras);
private void create();
int main(int client_no, mapping paras);
private int valid_cmd(mapping paras);
private void delete_protect_phone(mapping paras);
private void set_protect_phone(mapping paras);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=158, nargs=1, insts=158
// --------------------------------------------------------------------
private void query_protect_phone(mapping paras) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed arr;
    mixed sql_cmd;
    mixed content;
    mapping m;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 0)) {
        paras["result"] = 1;
        paras["prompt"] = "Failed to connect to adb!";
        return;
    }
    sql_cmd = mysql_bind_parameters("select * from config where dist = ? and section = ? and name = ?", paras["dist"], "global", paras["name"]);
    arr = mysql_select(cno, sql_cmd);
    if ((!is_array(arr))) {
        paras["result"] = 1;
        paras["prompt"] = "Failed to operate adb!";
        return;
    }
    paras["result"] = 0;
    if ((sizeof(arr) < 1)) {
        paras["prompt"] = "No data in adb.config!";
        return;
    }
    content = arr[0]["value"];
    if (((!is_string(content)) || (!sizeof(content)))) {
        paras["prompt"] = "Bad data in adb.config!";
        return;
    }
    try {
        m = restore_value(content);
    } catch { /* error handled */ }

    // --- label L0117 ---
    if ((((!is_mapping(m)) || (!sizeof(m))) || (!is_string(m["tip"])))) {
        paras["prompt"] = "Bad data in adb.config!";
        return;
    }
    paras["prompt"] = "success";
    paras["protect_phone"] = m["tip"];

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=48, nargs=2, insts=48
// --------------------------------------------------------------------
private void _main(int client_no, mapping paras) {
    /* ---- Enhanced Pseudocode ---- */
    do {
        if ((!valid_cmd(paras))) {
            break;
        }
        switch (paras["oper"]) {
            case "query":
            query_protect_phone(paras);
            break;
            case "set":
            set_protect_phone(paras);
            break;
            case "delete":
            delete_protect_phone(paras);
            break;
            default:
            break;
        }  // end switch

        // --- label L0031 ---
    } while (0);

    // --- label L0034 ---
    paras["msg"] = "oper_adb_config";
    "/daemons/ctad.c"->send_to_cta_ex(client_no, paras);

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=9, nargs=0, insts=9
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    register_daemon_data("/daemons/ctacommandd.c", "oper_adb_config", "/aaa/daemons/cta/cmd_cta_oper_adb_config.c");

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=15, nargs=2, insts=15
// --------------------------------------------------------------------
int main(int client_no, mapping paras) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch(random(10), (: _main, client_no, paras :));
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=69, nargs=1, insts=69
// --------------------------------------------------------------------
private int valid_cmd(mapping paras) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!is_string(paras["dist"]))) {
        paras["result"] = 1;
        paras["prompt"] = "区组配置不对\n";
        return 0;
    }
    if ((!is_string(paras["name"]))) {
        paras["result"] = 1;
        paras["prompt"] = "Name missed!";
        return 0;
    }
    if (("protect_phone" != paras["name"])) {
        paras["result"] = 1;
        paras["prompt"] = "Bad name! Name must be protect_phone!";
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=58, nargs=1, insts=58
// --------------------------------------------------------------------
private void delete_protect_phone(mapping paras) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed sql_cmd;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 0)) {
        paras["result"] = 1;
        paras["prompt"] = "Failed to connect to adb!";
        return;
    }
    sql_cmd = mysql_bind_parameters("delete from config where dist = ? and section = ? and name = ?", paras["dist"], "global", paras["name"]);
    (*f_execute)(cno, sql_cmd);
    paras["result"] = 0;
    paras["prompt"] = "success";

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=186, nargs=1, insts=186
// --------------------------------------------------------------------
private void set_protect_phone(mapping paras) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    int ret;
    mixed arr;
    mixed sql_cmd_insert;
    mixed sql_cmd_update;
    mixed content;
    mixed str;
    mixed key;
    mixed value;
    mixed m;

    content = paras["protect_phone"];
    if (((!is_string(content)) || (!sizeof(content)))) {
        paras["result"] = 1;
        paras["prompt"] = "Bad data for protect_phone!";
        return;
    }
    m = ([ ]);
    arr = explode(content, "|");
    loop (str in arr) {
        if ((sscanf(str, "%s=%s", &key, &value) < 2)) {
            continue;
        }
        m[trim(key)] = trim(value);
    }  // end loop (verified)
    if ((!sizeof(m))) {
        paras["result"] = 1;
        paras["prompt"] = "No data for protect_phone!";
        return;
    }
    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 0)) {
        paras["result"] = 1;
        paras["prompt"] = "Failed to connect to adb!";
        return;
    }
    sql_cmd_insert = mysql_bind_parameters("insert into config(dist, section, name, value) values(?, ?, ?, ?)", paras["dist"], "global", paras["name"], save_string(m));
    sql_cmd_update = mysql_bind_parameters("update config set value = ? where dist = ? and section = ? and name = ?", save_string(m), paras["dist"], "global", paras["name"]);
    ret = (*f_execute)(cno, sql_cmd_insert);
    if ((ret < 1)) {
        ret = (*f_execute)(cno, sql_cmd_update);
    }
    if ((ret < 1)) {
        paras["result"] = 1;
        paras["prompt"] = "Failed to operate adb!";
        return;
    }
    paras["result"] = 0;
    paras["prompt"] = "success";

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        7
   Code segments found:         7
   Successfully decoded:        7
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */