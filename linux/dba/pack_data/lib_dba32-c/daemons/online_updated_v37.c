/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: online_updated.o
 * Source: /daemons/online_updated.c
 * Inherits: (none)
 * Functions: 18 names, 18 code segments found
 * Strings: 66, Variables: 6
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   mapping convert_list;
   mapping convert_list_ex;
   int enable_delete_file;
   int start_all_online_update;
   mapping system_startup_update_list;
   mapping update_state_list;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private mixed do_update_by_etc_file(string update_name, mapping m);
private void delete_update_file(string file_name, array maked_dirs);
private mixed read_update_file(string server_ver, int server_type, string update_name);
private void do_update(mapping update_info);
void make_update_file(string config_file, string update_name);
mixed get_update_state(mixed update_name);
private void do_update_cmd(string update_name, mapping m);
private int calc_checksum(mapping m);
void start_online_update(mixed update_name);
private int make_dir(string path, array &maked_dir);
private mixed do_update_by_code_file(string update_name, mapping m);
private mixed get_config(string config_file);
int write_update_file_to_dbase(string file_path);
private int check_update(mapping m);
private void do_update_by_cmd(string update_name, mapping m);
private void create();
private void do_all_online_update();
void set_delete_file(int flag);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   update_state_list = ([ ]);  // /daemons/online_updated.c:27
   start_all_online_update = 0;  // /daemons/online_updated.c:34
   system_startup_update_list = ([ ]);  // /daemons/online_updated.c:38
   convert_list = ([ 48 : 68, 49 : 50, 50 : 57, 51 : 54, 52 : 69, 53 : 49, 54 : 51, 55 : 56, 56 : 48, 57 : 52, 65 : 67, 66 : 53, 67 : 70, 68 : 55, 69 : 65, 70 : 66 ]);
   convert_list_ex = ([ 68 : 48, 50 : 49, 57 : 50, 54 : 51, 69 : 52, 49 : 53, 51 : 54, 56 : 55, 48 : 56, 52 : 57, 67 : 65, 53 : 66, 70 : 67, 55 : 68, 65 : 69, 66 : 70 ]);
   enable_delete_file = 1;

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=87, nargs=2, insts=87
// --------------------------------------------------------------------
private mixed do_update_by_etc_file(string update_name, mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    mixed maked_dirs;
    mixed file;

    maked_dirs = ({});
    if ((strlen(m["file_content"]) < 1)) {
        update_state_list[update_name] = 8;
        broadcast("Empty file content %s.\n", m["file_path"]);
        return;
    }
    make_dir(m["file_path"], &maked_dirs);
    file = (m["file_path"] + m["file_name"]);
    delete_file(file);
    write_file(file, m["file_content"]);
    map_delete(m, "file_content");
    if ((file_size(file) < 0)) {
        update_state_list[update_name] = 9;
        delete_update_file(file, maked_dirs);
        broadcast("Write etc file %s failed.\n", file);
        return;
    }
    do_update_cmd(update_name, m);
    return (: delete_update_file, file, maked_dirs :);

}

// --------------------------------------------------------------------
// @code  nc=29, nargs=2, insts=29
// --------------------------------------------------------------------
private void delete_update_file(string file_name, array maked_dirs) {
    /* ---- Enhanced Pseudocode ---- */
    int i;

    if ((!enable_delete_file)) {
        return;
    }
    delete_file(file_name);
    for (i = (sizeof(maked_dirs) - 1); i >= 0; i--) {
        rmdir(maked_dirs[i]);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=48, nargs=3, insts=48
// --------------------------------------------------------------------
private mixed read_update_file(string server_ver, int server_type, string update_name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ret;
    mixed sql_cmd;

    sql_cmd = mysql_bind_parameters("select * from online_update where server_ver = ? and server_type = ? and update_name = ?", server_ver, server_type, update_name);
    ret = "/daemons/filed.c"->read_log("ddb", sql_cmd, 0, 100, 1800);
    if ((!is_array(ret))) {
        update_state_list[update_name] = 1;
        broadcast("Read update content failed\n");
        return;
    }
    if ((!sizeof(ret))) {
        return ([ ]);
    }
    return ret[0];

}

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=265, nargs=1, insts=265
// --------------------------------------------------------------------
private void do_update(mapping update_info) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    int update_flag;
    int update_name;
    string update_content;
    mixed update_list;
    mapping m;
    mixed ret_map;

    update_flag = 1;
    update_name = update_info["update_name"];
    if (((int)update_info["checksum"] != calc_checksum(update_info))) {
        update_state_list[update_name] = 2;
        broadcast(sprintf("Find bad checksum : %s.\n", update_name));
        return;
    }
    update_content = update_info["content"];
    for (i = 0; i <= ((sizeof(update_content) / 100) - 1); i++) {
        if (convert_list_ex[update_content[i]]) {
            put_sub_data(&update_content, i, convert_list_ex[update_content[i]]);
        }
    }  // end loop (verified)
    update_content = restore_value(update_content);
    update_content = (string)lzss_decompress(update_content);
    if ((!is_mapping(update_list = restore_value(update_content)))) {
        update_state_list[update_name] = 3;
        broadcast("Restore update content failed.\n");
        return;
    }
    if ((sizeof(update_list) > 10)) {
        update_state_list[update_name] = 4;
        broadcast("Update step > %d.\n", 10);
        return;
    }
    if ((!check_update(update_list))) {
        update_state_list[update_name] = 5;
        broadcast("Update server_type or server_ver error.\n");
        return;
    }
    ret_map = ([ ]);
    for (i = 0; i <= 10; i++) {
        if ((!is_mapping(m = update_list[i]))) {
            continue;
        }
        switch (m["update_type"]) {
            case 1:
            do_update_by_cmd(update_name, m);
            break;
            case 2:
            ret_map[i] = do_update_by_code_file(update_name, m);
            break;
            case 3:
            ret_map[i] = do_update_by_etc_file(update_name, m);
            break;
            default:
            update_flag = 0;
            break;
        }  // end switch

        // --- label L0191 ---
        if ((!update_flag)) {
            break;
        }
        if (is_int(update_state_list[update_name])) {
            update_flag = 0;
            break;
        }
    }  // end loop (verified)
    for (i = 0; i <= 10; i++) {
        try {
            if (is_function(ret_map[i])) {
                (*ret_map[i])();
            }
        } catch { /* error handled */ }

        // --- label L0233 ---
    }  // end loop (verified)
    if (update_flag) {
        if (start_all_online_update) {
            map_delete(system_startup_update_list, update_name);
            if ((!sizeof(system_startup_update_list))) {
                online_update_status = 1;
                start_all_online_update = 0;
            }
        }
        printf("OU_TRACE_BACK: %O", update_name);
    }

}

// --------------------------------------------------------------------
// @code  nc=334, nargs=2, insts=334
// --------------------------------------------------------------------
void make_update_file(string config_file, string update_name) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    int j;
    string content;
    mixed update_content;
    int para;
    mixed sql_cmd;
    mapping config_map;
    mapping update_list;
    mapping m;

    if ((!is_mapping(config_map = get_config(config_file)))) {
        return;
    }
    if ((sizeof(config_map) > 10)) {
        printf("Update step > %s.\n", 10);
        return;
    }
    if (((config_map["update_limit"]["server_type"] != 5) && (config_map["update_limit"]["server_type"] != 18))) {
        printf("Update only support GS.\n");
        return;
    }
    update_list = ([ "update_name": update_name, "server_ver": config_map["update_limit"]["server_ver"], "server_type": config_map["update_limit"]["server_type"] ]);
    for (i = 0; i <= 10; i++) {
        if ((!is_mapping(m = config_map[("step" + i)]))) {
            continue;
        }
        if ((is_string(m["file_path"]) && strlen(m["file_path"]))) {
            if ((!is_buffer(content = read_file((m["file_path"] + m["file_name"]), "b")))) {
                printf("Read update file %s failed\n", (m["file_path"] + m["file_name"]));
                return;
            }
            content = (string)content;
            if ((strlen(content) > 258048)) {
                printf("Update file too big.\n");
                return;
            }
            m["file_content"] = content;
        }
        if (is_array(m["cmd_para"])) {
            for (j = 0; j <= (sizeof(m["cmd_para"]) - 1); j++) {
                para = m["cmd_para"][j];
                if (((!is_string(para)) || (!is_mapping(config_map[para])))) {
                    continue;
                }
                if (is_array(config_map[para][para])) {
                    m["cmd_para"][j] = config_map[para][para];
                } else {
                    m["cmd_para"][j] = config_map[para];
                }
            }  // end loop (verified)
            m["cmd_para"] = save_string(m["cmd_para"]);
        }
        update_list[i] = m;
    }  // end loop (verified)
    update_content = save_string(update_list);
    update_content = save_string(lzss_compress(update_content));
    if ((strlen(update_content) > 258048)) {
        printf("Update file too big.\n");
        return;
    }
    for (i = 0; i <= ((sizeof(update_content) / 100) - 1); i++) {
        if (convert_list[update_content[i]]) {
            put_sub_data(&update_content, i, convert_list[update_content[i]]);
        }
    }  // end loop (verified)
    update_list["content"] = update_content;
    delete_file((("/online_update_file/" + update_name) + ".txt"));
    delete_file((("/online_update_file/" + update_name) + "_config.txt"));
    mkdir("/online_update_file");
    sql_cmd = mysql_bind_parameters("insert into ddb.online_update(update_name, server_ver,server_type, checksum, content, memo) values (?, ?, ?, ?, ?, ?);", update_list["update_name"], update_list["server_ver"], update_list["server_type"], calc_checksum(update_list), update_content, "");
    write_file((("/online_update_file/" + update_name) + ".txt"), sql_cmd);
    printf("Make update file Ok.\n");

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
mixed get_update_state(mixed update_name) {
    /* ---- Enhanced Pseudocode ---- */
    return update_state_list[update_name];

}

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=180, nargs=2, insts=180
// --------------------------------------------------------------------
private void do_update_cmd(string update_name, mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    switch (var_type(m["update_cmd"])) {
        case "string":
        if ((m["update_cmd"] != "update")) {
            broadcast("Update type %d Only support update\n", m["update_type"]);
            update_state_list[update_name] = 10;
            return;
        }
        (*m["update_cmd"])(m["cmd_para"]);
        if (((m["update_type"] == 3) || (m["update_type"] == 1))) {
            break;
        }
        if ((!(m["cmd_para"])->finished_online_update())) {
            broadcast("Do %s failed\n", m["update_cmd"]);
            update_state_list[update_name] = 11;
            return;
        }
        break;
        case "array":
        if ((!is_string(m["cmd_para"]))) {
            call_other_arg(m["update_cmd"][0], m["update_cmd"][1], ({}));
        } else {
            call_other_arg(m["update_cmd"][0], m["update_cmd"][1], restore_value(m["cmd_para"]));
        }
        break;
        default:
        if ((m["update_type"] == 3)) {
            m["update_cmd"] = "load etc file";
            break;
        }
        return;
    }  // end switch

    // --- label L0128 ---
    broadcast(sprintf("Finished %s(%O).\n", (is_array(m["update_cmd"]) ? m["update_cmd"][1] : m["update_cmd"]), (is_string(m["cmd_para"]) ? m["cmd_para"] : save_string(m["cmd_para"]))));
    update_state_list[update_name] = time_desc();
    "/daemons/filed.c"->log_to_db("db_log", 1, "online_update", "finish_update", update_name, "", "", save_string(m));

}

// --------------------------------------------------------------------
// @code  nc=23, nargs=1, insts=23
// --------------------------------------------------------------------
private int calc_checksum(mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    return int32((create_check_sum(1, m["update_name"], m["server_ver"], (string)m["server_type"], m["content"]) ^ 20101218));

}

// --------------------------------------------------------------------
// @code  nc=93, nargs=1, insts=93
// --------------------------------------------------------------------
void start_online_update(mixed update_name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed m;

    if (is_string(update_name)) {
        if (start_all_online_update) {
            broadcast("Can't update when Do system up update.\n");
            return;
        }
        if (is_int(update_state_list[update_name])) {
            broadcast(sprintf("%s is in updating.\n", update_name));
            return;
        }
        update_state_list[update_name] = -1;
        m = read_update_file("1.60.0912", 4, update_name);
        if (((!is_mapping(m)) || (!sizeof(m)))) {
            update_state_list[update_name] = 1;
            broadcast("Online update read content failed.\n");
            return;
        }
        do_update(m);
        return;
    }
    if (start_all_online_update) {
        broadcast(printf("Can't update before finished system startup update.\n"));
        return;
    }
    if ((online_update_status == 1)) {
        return;
    }
    online_update_status = 0;
    start_all_online_update = 1;
    try {
        do_all_online_update();
    } catch { /* error handled */ }

    // --- label L0090 ---

}

// --------------------------------------------------------------------
// @code  nc=62, nargs=2, insts=62
// --------------------------------------------------------------------
private int make_dir(string path, array &maked_dir) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    mixed arr;
    mixed file;

    arr = explode(path, "/");
    file = arr[0];
    i = 1;

    // --- label L0018 ---
    while ((i < sizeof(arr))) {
        switch (file_size(file)) {
            case -2:
            break;
            case -1:
            if (mkdir(file)) {
                maked_dir += ({ file });
            }
            break;
            default:
            return 0;
        }  // end switch

        // --- label L0043 ---
        file += ("/" + arr[i]);
        ++i;
    }  // end loop (verified)
    return 1;

}

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=87, nargs=2, insts=87
// --------------------------------------------------------------------
private mixed do_update_by_code_file(string update_name, mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    mixed maked_dirs;
    mixed file;

    maked_dirs = ({});
    if ((strlen(m["file_content"]) < 1)) {
        update_state_list[update_name] = 6;
        broadcast("Empty file content %s.\n", m["file_path"]);
        return;
    }
    make_dir(m["file_path"], &maked_dirs);
    file = (m["file_path"] + m["file_name"]);
    delete_file(file);
    write_file(file, m["file_content"]);
    map_delete(m, "file_content");
    if ((file_size(file) < 0)) {
        update_state_list[update_name] = 7;
        delete_update_file(file, maked_dirs);
        broadcast("Write etc file %s failed.\n", file);
        return;
    }
    do_update_cmd(update_name, m);
    return (: delete_update_file, file, maked_dirs :);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=60, nargs=1, insts=60
// --------------------------------------------------------------------
private mixed get_config(string config_file) {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr;
    mixed form_index;
    mixed config_content;
    mixed form_value;

    form_index = ({});
    form_value = ([ ]);
    if ((!is_string(config_content = read_file(config_file)))) {
        printf("Read file %s failed\n", config_file);
        return;
    }
    arr = mapping_array(explode(config_content, "\n"), (: trim :));
    if ((!sizeof(arr))) {
        printf("Empty file: %s\n", config_file);
        return;
    }
    create_data_from2(arr, &form_index, &form_value);
    if ((!is_mapping(form_value))) {
        printf("Bad config file: %s\n", config_file);
        return;
    }
    return form_value;

}

// --------------------------------------------------------------------
// @brief  data writer: persist information
// @code  nc=37, nargs=1, insts=37
// --------------------------------------------------------------------
int write_update_file_to_dbase(string file_path) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;

    if ((!is_string(sql_cmd = read_file(file_path)))) {
        printf("read file failed\n");
        return 0;
    }
    if (((cno = "/daemons/dbd.c"->assure_connection("ddb")) == -1)) {
        printf("connect to dba failed\n");
        return 0;
    }
    return (*f_execute)(cno, sql_cmd);

}

// --------------------------------------------------------------------
// @brief  validator: verify data
// @code  nc=21, nargs=1, insts=21
// --------------------------------------------------------------------
private int check_update(mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    if (((m["server_type"] != 4) || (m["server_ver"] != "1.60.0912"))) {
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=8, nargs=2, insts=8
// --------------------------------------------------------------------
private void do_update_by_cmd(string update_name, mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    do_update_cmd(update_name, m);

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=9, nargs=0, insts=9
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    require_uid();
    set_euid(0);

}

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=121, nargs=0, insts=121
// --------------------------------------------------------------------
private void do_all_online_update() {
    /* ---- Enhanced Pseudocode ---- */
    mixed all_update_list;
    mixed sql_cmd;
    int update_name;
    mapping m;

    sql_cmd = mysql_bind_parameters("select update_name from online_update where server_ver = ? and server_type = ? order by id", "1.60.0912", 4);
    all_update_list = "/daemons/filed.c"->read_log("ddb", sql_cmd, 0, 100, 1800);
    if ((!is_array(all_update_list))) {
        update_state_list["system_up"] = 1;
        broadcast("Online update read content failed.\n");
        return;
    }
    if ((!sizeof(all_update_list))) {
        online_update_status = 1;
        start_all_online_update = 0;
        broadcast("No online update.\n");
        return;
    }
    system_startup_update_list = ([ ]);
    loop (m in all_update_list) {
        system_startup_update_list[m["update_name"]] = 1;
    }  // end loop (verified)
    loop (m in all_update_list) {
        update_name = m["update_name"];
        update_state_list[update_name] = -1;
        m = read_update_file("1.60.0912", 4, update_name);
        if (((!is_mapping(m)) || (!sizeof(m)))) {
            update_state_list[update_name] = 1;
            broadcast("Online update read content failed.\n");
            return;
        }
        do_update(m);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
void set_delete_file(int flag) {
    /* ---- Enhanced Pseudocode ---- */
    enable_delete_file = flag;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        18
   Code segments found:         18
   Successfully decoded:        18
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */