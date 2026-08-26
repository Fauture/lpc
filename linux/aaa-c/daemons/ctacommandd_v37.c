/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: ctacommandd.o
 * Source: /daemons/ctacommandd.c
 * Inherits: (none)
 * Inherited programs: /feature/daemon/register.c(), /feature/daemon/handler_module.c()
 * Functions: 6 names, 6 code segments found
 * Strings: 22, Variables: 0
 *********************************************************************/

inherit "/feature/daemon/register.c";
inherit "/feature/daemon/handler_module.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void init();
private mixed get_args(buffer buf);
override void process_command(object agent, string cmd, array args);
private void create();
int cta_command(int client_no, buffer buf);
void notify_daemon_data(mixed index, mixed val);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=26, nargs=0, insts=26
// --------------------------------------------------------------------
private void init() {
    /* ---- Enhanced Pseudocode ---- */
    mixed m;
    mixed mks;

    if ((!is_mapping(m = get_daemon_data("/daemons/ctacommandd.c")))) {
        return;
    }
    loop (mks in m) {
        notify_daemon_data(mks[0], mks[1]);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=103, nargs=1, insts=103
// --------------------------------------------------------------------
private mixed get_args(buffer buf) {
    /* ---- Enhanced Pseudocode ---- */
    int pairs;
    int pos;
    mixed len;
    mixed m;
    mixed key_len;
    int val_len;
    string key;
    string val;

    pos = 0;
    len = sizeof(buf);
    pairs = (int)buf[pos..(pos + 1)];
    pos += 2;
    m = allocate_mapping(pairs);

    // --- label L0030 ---
    while (pos < len) {
        key_len = buf[pos];
        ++pos;
        key = (string)buf[pos..((pos + key_len) - 1)];
        pos += key_len;
        val_len = (int)buf[pos..(pos + 1)];
        pos += 2;
        val = (string)buf[pos..((pos + val_len) - 1)];
        pos += val_len;
        m[key] = val;
    }  // end loop (verified)
    return m;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=12, nargs=3, insts=12
// --------------------------------------------------------------------
override void process_command(object agent, string cmd, array args) {
    /* ---- Enhanced Pseudocode ---- */
    cta_command(agent->get_client_no(), args[0]);

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=10, nargs=0, insts=10
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    load_all_path("/cmds/cta/");
    register_post_init((: init :));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=197, nargs=2, insts=197
// --------------------------------------------------------------------
int cta_command(int client_no, buffer buf) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    int ret_val;
    mixed cmd_ob;
    mixed pw_arr;
    int name;
    mixed pw;
    string ret_str;
    mixed privilege;
    mixed cmd;
    mapping paras;
    mixed cta_info;

    ret_val = 0;
    do {
        if ((!is_mapping(paras = get_args(buf)))) {
            ret_str = "Bad parameters buf 1...\n";
            break;
        }
        cmd = paras["cmd"];
        if ((!is_string(cmd))) {
            ret_str = "Bad parameters buf 2...\n";
            break;
        }
        cmd_ob = query_entry(cmd);
        if ((!is_object(cmd_ob))) {
            return 0;
        }
        name = paras["cta"];
        pw = paras["pw"];
        pw = save_string(md5(pw))[1..<2];
        cta_info = "/aaa/daemons/configd.c"->get_cta_list();
        if ((!is_mapping(cta_info[name]))) {
            ret_str = "No such cta user info in db ...\n";
            break;
        }
        pw_arr = explode(cta_info[name]["access_adb_password"], ",");
        privilege = cta_info[name]["privilege"];
        for (i = 0; i <= (sizeof(pw_arr) - 1); i++) {
            if (("" == pw_arr[i])) {
                pw_arr[i] = 1;
            }
        }  // end loop (verified)
        clean_array(pw_arr, 4);
        if ((0 > member_array(pw, pw_arr))) {
            ret_str = "Input wrong password...\n";
            break;
        }
        if (((privilege != "*") && (strsrch(privilege, cmd) < 0))) {
            ret_str = "没有许可权";
            break;
        }
        ret_val = 1;
    } while (0);

    // --- label L0171 ---
    if ((!ret_val)) {
        paras["error"] = ret_str;
        "/daemons/ctad.c"->send_to_cta(client_no, paras);
        return 0;
    }
    return cmd_ob->main(client_no, paras);

}

// --------------------------------------------------------------------
// @brief  notification: send message
// @code  nc=20, nargs=2, insts=20
// --------------------------------------------------------------------
void notify_daemon_data(mixed index, mixed val) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ob;

    if ((!is_object(ob = find_object(val)))) {
        ob = load_object(val);
    }
    register_entry(index, ob);

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        6
   Code segments found:         6
   Successfully decoded:        6
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          2
      - /feature/daemon/register.c()
      - /feature/daemon/handler_module.c()
   ================================================================ */