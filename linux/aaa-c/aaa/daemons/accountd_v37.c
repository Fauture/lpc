/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: accountd.o
 * Source: /aaa/daemons/accountd.c
 * Inherits: (none)
 * Inherited programs: /feature/common/dbase.c(), /feature/daemon/register.c(), /feature/daemon/handler_module.c(), /feature/daemon/eternal_data.c()
 * Functions: 144 names, 144 code segments found
 * Strings: 453, Variables: 30
 *********************************************************************/

inherit "/feature/common/dbase.c";
inherit "/feature/daemon/register.c";
inherit "/feature/daemon/handler_module.c";
inherit "/feature/daemon/eternal_data.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   int aa_rule_activate_account;
   int aa_rule_login;
   int account_active_status;
   int apex_enabled;
   mapping auth_key_account;
   int clean_protect_account_timer;
   int clean_protect_account_timer_ex;
   mapping client_seedA;
   int cur_auth_key;
   int fixed_city_status;
   int g_auth_method;
   /* mixed global_0; */
   /* mixed global_1; */
   /* mixed global_2; */
   /* mixed global_3; */
   array ignore_dist_list;
   int is_need_open_guard;
   int local_encrypt_enabled;
   int matrix_active_status;
   array matrix_list;
   int matrix_pic_enabled;
   int merge_limit_login;
   mapping protect_account;
   mapping protect_account_ex;
   mapping protect_index;
   int safe_tips_enbled;
   int timeout_for_choose_server;
   int timeout_for_protect_auth;
   array vector_x;
   array vector_y;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private mapping activate_account(string account, string md5password, string account_ip);
override void connection_lost(object agent);
mixed auth_matrix_protect(string account, string ip, string dist, string auth_info, mixed &prompt, mixed &build_info);
int check_matrix_info(mapping m);
mixed assure_server_agent(string server);
private string get_protect_key(string account, string ip);
int block_account(string dist, string account, int interval, string reason, int ignore_data, mixed &prompt = 0, int recycle_privilege = 0);
int is_under_control(mapping account_info);
private int pwd_protect_deny_fixed_ip(string account, mapping m, mixed param, mixed dist);
int update_account_info(mapping info, mixed fields, mixed valid_checksum);
int get_safe_tips_status();
int record_protect_range(string account, int range);
mixed query_app_bind_info(string account, int format = 0);
override void connection_disconnect(object agent);
int get_account_matrix_status(mixed matrix_info);
private void init_matrix();
private int pwd_protect_deny_fixed_tm(string account, mapping m, mixed param, mixed dist);
private int _check_client_apex_status(mapping account_info, string password);
string calc_password_new_md5(string account, string pwd);
private void log_protect_time(string account, string branch, string protect_type, string time_desc, int cno);
int get_account_controlled_safe_state(string account);
void log_bad_checksum_account(string account, mapping account_info);
mixed query_mp_device_info(string account);
string calc_account_checksum(mapping account_info);
private void set_protect_info_analysis_func(string type, mixed func);
mixed calc_local_encrypt_checksum(mapping m, string le_info);
private string calc_matrix_pwd_md5(string value1, string value2, string value3, string value4);
int control_account(string account, int oper, string password, string permit_ip, string permit_id, mixed &prompt);
int get_matrix_status();
int set_active_account_status(int status);
mixed auth_phone_protect(string account, string ip, string dist, string auth_info, mixed &prompt);
private int pwd_protect_fetch_phone(string account, mapping &m);
private void do_when_batch_block_account(array arr_account, string dist);
private int pwd_protect_fetch_sm(string account, mapping &m);
void refresh_matrix_pic(mixed agent, int index);
string get_place_by_ip(string ip_addr);
private void account_login(object agent, string account, string password, string mac_addr, string client_data, string passpod = "", string dist = "", int mobile_client = 0, string version = "", string qr_id = "");
private int pwd_protect_fetch_matrix(string account, mapping &m);
int delete_protect_info(string account, mixed protect_type);
void enable_safe_tips();
int update_protect_info(mapping m);
private int pwd_protect_deny_fixed_city(string account, mapping m, mixed param, mixed dist);
private int pwd_protect_update_phone(string account, mapping &m);
private int deny_by_pwd_protect(string account, string protect_type, mixed &prompt = 0, mixed param = 0, mixed dist = "");
int update_matrix_info(mapping m, int encode = 0);
int is_protect_actived(string account, int protect_flag, mixed account_ob);
int temp_block_account(string dist, string account, int interval, string reason, mixed &prompt);
void disable_local_encrypt();
void enable_apex();
private int pwd_protect_deny_sm(string account, mapping m, mixed param, mixed dist);
int insert_safe_ctrl(mapping m);
mixed query_phone_bind_info(string account, int format = 0);
private void do_when_block_account(string account, string dist);
void decode_matrix_info(mapping &m);
void disable_apex();
private void _timer_clean_client_seedA();
mixed query_local_encrypt_info(string account);
void get_extra_block_reason_msg(int check_left_day, string real_reason, string &breason);
int get_active_account_status();
void enable_matrix_pic();
private string encode_matrix_auth_text(string cookie, string pos1, string pos2, string pos3);
private string des_local_encrypt_pos(array arr_pos, string replace_str, string seed1, string seed2);
int update_protect_info_ex(mapping m);
private buffer build_local_encrypt_pos_buf(array arr_pos);
void _send_antibot_question(mixed agent, string md5_value, string checksum);
private int check_for_protect_account_ex(string account, mixed &ret);
private void destruct();
private void send_login_result(object agent, int result, int auth_key, string prompt);
private void log_antibot(string ip, string account, int bot_type, mixed bot_content);
int is_enable_matrix_info_for_gs(string account);
private mixed decode_matrix_auth_text(string auth_text);
mapping get_coin_info(mapping account_info);
void set_auth_method(int _method);
void client_auth_pic(mixed agent, string client_data);
int batch_block_account(string dist, array arr_account, int interval, string reason, int ignore_data, mixed &prompt);
mixed auth_sm_protect(string account, string ip, string dist, string auth_info, mixed &prompt);
mixed auth_passpod_protect(string account, string ip, string dist, string auth_info, mixed &prompt, mapping &extra_info);
private mixed get_value_from_matrix_text(string matrix_text, string pos);
private void _client_auth_pic(mixed agent, string client_data);
int get_guard_state();
int set_guard_state(int state);
void disable_safe_tips();
private void clean_protect_account();
mixed get_account_ob_by_auth_key(int auth_key);
private void clean_protect_account_ex();
private int generate_auth_key();
mixed query_password_protect_info(string account);
private int check_for_protect_account(string account, string ip, mixed &ret);
private void _process_command(mixed func, mixed agent, string cmd, array arr);
int is_apex_enabled();
private void _account_login(mixed agent, string account, string password, string mac_addr, string client_data, string passpod, string dist, string qr_id, int auth_verfy_code);
mixed query_account_info(string account);
void check_client_data(mixed agent, string client_data);
void enable_local_encrypt();
void add_coins(mapping account_info, string dist, string server, string account, int add_silver_coin, int add_gold_coin);
mixed query_protect_info(string account, string protect_type);
private int deny_by_pwd_protect_ex(string account, string protect_type, mixed &prompt, mixed param);
int query_protect_range(string account);
void disable_matrix_pic();
mixed query_phone_auth_info(string account, int format = 0);
private void do_when_password_error(mixed agent, string account, mixed &ret);
private void timer_clean_client_seedA();
mixed query_alipay_info(string account);
override void process_command(object agent, string cmd, array args);
private void cancel_pwd_protect(string account, string protect_type, mixed param, mixed dist);
private void create();
void set_fixed_city_status(int _status);
void encode_matrix_info(mapping &m);
private int pwd_protect_update_passpod(string account, mapping &m);
mixed calc_protect_info_checksum(mapping m);
private string generate_auth_matrix_cookie(string account);
buffer get_matrix_pic(array matrix);
mixed query_safe_ctrl(string account, string protect_type);
private void auth_all_protect(mixed agent, string account, mapping account_info, int qr_code_login = 0);
private int pwd_protect_update_sm(string account, mapping &m);
private int get_busy_time();
mixed query_pwd_protect_err_checksum(mixed arg);
private string md5_local_encrypt_pos(array arr_pos, string replace_str, string seed1, string seed2);
private int is_account_controlled_safe(object agent, string account, string &prompt);
private void _check_client_data(mixed agent, string client_data);
private int check_skip_protected(mixed agent, string mac_addr, mapping account_info);
void send_antibot_question(mixed agent, string md5_value, string checksum);
private mixed get_protect_info_analysis_func(string type);
int set_matrix_status(int status);
private int get_error_times();
mixed revise_pwd_protect_checksum(mixed arg);
private string generate_encrypt_cookie();
private mixed _generate_random_matrix(int count, mixed filters, mixed &prompt, mixed &is_out);
mixed calc_matrix_checksum(mapping m);
mixed query_matrix_info(string account, int decode = 0);
private int pwd_protect_update_matrix(string account, mapping &m);
private mixed check_account_info(mixed agent, string account, string password, string mac_addr, string client_data, string passpod, string dist, int qr_code_login);
private array get_random_pos(int file_length);
private int pwd_protect_deny_phone(string account, mapping m, mixed param, mixed dist);
int is_matrix_pic_enabled();
private void do_when_password_correct(mixed agent, string account);
void auth_local_encrypt(mixed agent, string account, string auth_text);
private int pwd_protect_deny_passpod(string account, mapping m, mixed param, mixed dist);
private int pwd_protect_fetch_passpod(string account, mapping &m);
private array generate_random_matrix(mixed agent, int count, mixed filters);
int get_fixed_city_status();
void auth_matrix(mixed agent, string account, string auth_text);
private int check_for_ban(object unauthed_agent, string &prompt);
int get_auth_method();

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   protect_account = ([ ]);  // /aaa/daemons/accountd.c:35
   protect_account_ex = ([ ]);  // /aaa/daemons/accountd.c:53
   auth_key_account = ([ ]);  // /aaa/daemons/accountd.c:64
   cur_auth_key = random(2147483647);  // /aaa/daemons/accountd.c:79
   account_active_status = 1;  // /aaa/daemons/accountd.c:121
   matrix_active_status = 1;  // /aaa/daemons/accountd.c:129
   matrix_pic_enabled = 1;  // /aaa/daemons/accountd.c:1859
   local_encrypt_enabled = 1;
   safe_tips_enbled = 1;
   apex_enabled = 0;
   client_seedA = ([ ]);
   timeout_for_choose_server = 30;
   timeout_for_protect_auth = 60;
   aa_rule_activate_account = 1;
   aa_rule_login = 0;
   merge_limit_login = 0;
   vector_x = ({ "A", "B", "C", "D", "E", "F", "G", "H", "I" });
   vector_y = ({ "1", "2", "3", "4", "5", "6", "7", "8", "9" });
   protect_index = ([ "sm" : "201", "phone" : "202", "passpod" : "203", "matrix" : "204" ]);
   ignore_dist_list = ({ "ÎÊµÀÒ»Çø", "ÎÊµÀ¶þÇø", "ÎÊµÀÈýÇø", "°æ±¾ÄÚ²â", "ÊÔ½£ÄÚ²â" });
   g_auth_method = 2;
   is_need_open_guard = 1;
   fixed_city_status = 1;

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=193, nargs=3, insts=193
// --------------------------------------------------------------------
private mapping activate_account(string account, string md5password, string account_ip) {
    /* ---- Enhanced Pseudocode ---- */
    mixed result;
    string prompt;
    mixed url;
    mapping ret;

    ret = ([ ]);
    ret = ([ ]);
    url = "/aaa/daemons/configd.c"->get_my_info()["activate_url"];
    if (((!is_string(url)) || (strlen(url) == 0))) {
        ret["prompt"] = "Ã»ÓÐÉè¶¨×Ô¶¯¼¤»î·þÎñÆ÷";
        ret["result"] = 1;
        return ret;
    }
    if ((url[<1] == 44)) {
        url = url[0..<2];
    }
    url = explode(url, ",");
    url = trim(url[random(sizeof(url))]);
    if ((url[<1] != 63)) {
        url += "?";
    }
    url += sprintf("UserName=%s&Password=%s&GameServer=%s&ClientIP=%s", account, md5password, "/aaa/daemons/configd.c"->get_my_info()["activate_aaa_ip"], account_ip);
    result = "/daemons/http_clientd.c"->http_get_web(url);
    if (((!is_string(result)) || (sscanf(result, "%d,%s", &result, &prompt) < 2))) {
        ret["result"] = 1;
        ret["prompt"] = "ÕÊºÅ×Ô¶¯¼¤»îÊ§°Ü£¬ÇëÉÔºóÖØÊÔ¡£";
        return ret;
    }
    prompt = iconv(prompt, "gbk", "utf-8");
    if ((!is_string(prompt))) {
        prompt = "Î´Öª´íÎó";
    }
    ret["prompt"] = prompt;
    switch (result) {
        case 0:
        ret["result"] = 0;
        break;
        case 2:
        ret["result"] = 2;
        break;
        case 6:
        ret["result"] = 3;
        break;
        default:
        ret["result"] = 1;
        break;
    }  // end switch

    // --- label L0189 ---
    return ret;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=39, nargs=1, insts=39
// --------------------------------------------------------------------
override void connection_lost(object agent) {
    /* ---- Enhanced Pseudocode ---- */
    mixed account;

    if (is_string(account = agent->lite_query("account"))) {
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "------ Á¬½ÓÖÐ¶Ï£¬µÇÂ¼½áÊø\n");
        }
    }
    map_delete(client_seedA, agent->get_client_no());
    ::connection_lost(agent);

}

// --------------------------------------------------------------------
// @code  nc=245, nargs=6, insts=245
// --------------------------------------------------------------------
mixed auth_matrix_protect(string account, string ip, string dist, string auth_info, mixed &prompt, mixed &build_info) {
    /* ---- Enhanced Pseudocode ---- */
    int matrix_status;
    mixed matrix_random;
    mixed matrix_text;
    mixed matrix_cookie;
    mixed auth_text;
    mixed value_1;
    mixed value_2;
    mixed value_3;
    mapping m;
    mapping matrix_info;
    mixed is_out;

    if ((!get_matrix_status())) {
        matrix_status = 2;
    } else {
        matrix_info = query_matrix_info(account);
        matrix_status = get_account_matrix_status(matrix_info);
    }
    if ((2 == matrix_status)) {
        return 1;
    }
    if ((matrix_status == 1)) {
        prompt = "ÑéÖ¤ÃÜ±£Òì³££¬ÇëÉÔºóÖØÊÔ";
        return -1;
    }
    if ((!sizeof(auth_info))) {
        matrix_cookie = generate_auth_matrix_cookie(account);
        matrix_random = _generate_random_matrix(3, matrix_info["used_matrix"], &prompt, &is_out);
        build_info = ([ "matrix_cookie": matrix_cookie, "matrix_random": matrix_random ]);
        return -2;
    }
    try {
        auth_info = restore_value(auth_info);
    } catch { /* error handled */ }

    // --- label L0078 ---
    if ((0 || (!is_mapping(auth_info)))) {
        return -1;
    }
    matrix_random = auth_info["matrix_random"];
    if (((!is_array(matrix_random)) || (sizeof(matrix_random) != 3))) {
        return -1;
    }
    if ((!is_string(auth_text = auth_info["client"]))) {
        return -1;
    }
    m = decode_matrix_auth_text(auth_text);
    if ((!is_mapping(m))) {
        return -1;
    }
    if ((m["matrix_cookie"] != auth_info["matrix_cookie"])) {
        return -1;
    }
    if ((!get_matrix_status())) {
        matrix_status = 2;
    } else {
        matrix_info = query_matrix_info(account);
        matrix_status = get_account_matrix_status(matrix_info);
    }
    if ((1 == matrix_status)) {
        return -1;
    }
    if ((2 != matrix_status)) {
        decode_matrix_info(&matrix_info);
        matrix_text = matrix_info["matrix"];
        value_1 = get_value_from_matrix_text(matrix_text, matrix_random[0]);
        value_2 = get_value_from_matrix_text(matrix_text, matrix_random[1]);
        value_3 = get_value_from_matrix_text(matrix_text, matrix_random[2]);
        if ((calc_matrix_pwd_md5(value_1, value_2, value_3, m["matrix_cookie"]) != m["MD5"])) {
            return -1;
        }
        if (is_array(matrix_info["used_matrix"])) {
            matrix_info["used_matrix"] -= matrix_random;
            matrix_info["used_matrix"] += matrix_random;
        } else {
            matrix_info["used_matrix"] = matrix_random;
        }
        update_matrix_info(matrix_info, 1);
    }
    return 0;

}

// --------------------------------------------------------------------
// @brief  validator: verify data
// @code  nc=12, nargs=1, insts=12
// --------------------------------------------------------------------
int check_matrix_info(mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    return (calc_matrix_checksum(m) == toupper(m["checksum"]));

}

// --------------------------------------------------------------------
// @brief  verification: confirm/guarantee state
// @code  nc=72, nargs=1, insts=72
// --------------------------------------------------------------------
mixed assure_server_agent(string server) {
    /* ---- Enhanced Pseudocode ---- */
    mixed client_no;
    mixed agent;

    client_no = "/aaa/daemons/gameserverd.c"->assure_connection(server);
    if ((client_no == -1)) {
        return 0;
    }
    if (is_object(agent = find_agent_by_client(client_no))) {
        return agent;
    }
    agent = clone_object("/clone/misc/mixed_agent.c");
    agent->set_client_no(client_no);
    agent->set_name(server);
    agent->set_cmd_module("/aaa/daemons/accountd.c");
    agent->lite_set_temp("ok", 1);
    set_client_attrib(client_no, ([ "type": "Internal", "size": 131000, "peek": -1 ]));
    agent->send_message(14849, 2, _server_name, make32(0));
    return agent;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=9, nargs=2, insts=9
// --------------------------------------------------------------------
private string get_protect_key(string account, string ip) {
    /* ---- Enhanced Pseudocode ---- */
    return sprintf("%s@%s", account, ip);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=220, nargs=7, insts=220
// --------------------------------------------------------------------
int block_account(string dist, string account, int interval, string reason, int ignore_data, mixed &prompt, int recycle_privilege) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ti;
    int ret;
    string msg;
    string btime;
    mapping account_info;

    ret = 0;
    do {
        if ((!account_info = query_account_info(account))) {
            msg = "ÎÞ·¨»ñÈ¡ÕÊºÅÐÅÏ¢£¬¿ÉÄÜÊý¾Ý¿âÁ¬½ÓÊ§°Ü»òÕßÕÊºÅ²»´æÔÚ";
            break;
        }
        if ((account_info["checksum"] != calc_account_checksum(account_info))) {
            msg = "ÕÊºÅÊý¾Ý±»ÆÆ»µ£¬ÔÝÊ±ÎÞ·¨·â±ÕÕÊºÅ¡££¨Ô­Òò£º1009£©";
            break;
        }
        if (recycle_privilege) {
            account_info["privilege"] = 0;
        }
        if ((-2 == interval)) {
            account_info["blocked_reason"] = reason;
            if ((!update_account_info(account_info, 0, account_info["checksum"]))) {
                msg = "²Ù×÷Ê§°Ü£¬¿ÉÄÜÊý¾Ý¿âÒì³£";
            } else {
                msg = "²Ù×÷³É¹¦¡£";
                ret = 1;
            }
            prompt = msg;
            return ret;
        }
        ti = time();
        if ((interval < 0)) {
            interval = 2147483647;
        } else {
            interval += ti;
        }
        if ((ti == interval)) {
            btime = "";
        } else {
            btime = ctime(interval, 3);
        }
        if (((account_info["blocked_time"] != btime) || (account_info["blocked_reason"] != reason))) {
            account_info["blocked_time"] = btime;
            account_info["blocked_reason"] = reason;
            if ((!update_account_info(account_info, 0, account_info["checksum"]))) {
                if ((!sizeof(btime))) {
                    msg = "½â·âÕÊºÅ²Ù×÷Ê§°Ü£¬¿ÉÄÜÊý¾Ý¿âÒì³£";
                } else {
                    msg = "·â±ÕÕÊºÅ²Ù×÷Ê§°Ü£¬¿ÉÄÜÊý¾Ý¿âÒì³£";
                }
                break;
            }
        }
        if ((!sizeof(btime))) {
            msg = "½â·âÕÊºÅ²Ù×÷³É¹¦";
        } else {
            if ((!ignore_data)) {
                try {
                    do_when_block_account(account, dist);
                } catch { /* error handled */ }

                // --- label L0185 ---
            }
            msg = "·â±ÕÕÊºÅ²Ù×÷³É¹¦";
        }
        ret = 1;
        "/daemons/threadd.c"->dispatch(random(10), (: call_other, "/aaa/daemons/vip_smsd.c", "do_when_block_account", dist, account, btime :));
    } while (0);

    // --- label L0210 ---
    prompt = msg;
    return ret;

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=11, nargs=1, insts=11
// --------------------------------------------------------------------
int is_under_control(mapping account_info) {
    /* ---- Enhanced Pseudocode ---- */
    return (sizeof(account_info["org_password"]) > 0);

}

// --------------------------------------------------------------------
// @code  nc=84, nargs=4, insts=84
// --------------------------------------------------------------------
private int pwd_protect_deny_fixed_ip(string account, mapping m, mixed param, mixed dist) {
    /* ---- Enhanced Pseudocode ---- */
    mixed protect_ip;
    mixed user_ip;
    mixed conv_ip1;
    mixed conv_ip2;
    mixed first_ip;
    mixed last_ip;
    mixed cur_ip;

    if (((!is_string(m["protect_info"])) || (!is_string(param)))) {
        return 0;
    }
    protect_ip = trim(m["protect_info"]);
    user_ip = trim(param);
    conv_ip1 = replace_string(protect_ip, "*", "0");
    conv_ip2 = replace_string(protect_ip, "*", "255");
    first_ip = atoinet(conv_ip1);
    last_ip = atoinet(conv_ip2);
    cur_ip = atoinet(user_ip);
    if ((((!first_ip) || (!last_ip)) || (!cur_ip))) {
        return 0;
    }
    if (((first_ip <= cur_ip) && (cur_ip <= last_ip))) {
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=156, nargs=3, insts=156
// --------------------------------------------------------------------
int update_account_info(mapping info, mixed fields, mixed valid_checksum) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed mks;
    mixed arr;
    string str;
    mixed checksum;
    mixed sql_cmd;

    if (((cno = "/daemons/dbd.c"->assure_connection("adb")) == -1)) {
        return 0;
    }
    checksum = calc_account_checksum(info);
    info["checksum"] = checksum;
    str = "update account set ";
    arr = ({});
    if (is_array(fields)) {
        loop (mks in info) {
            if ((member_array(mks[0], fields) > -1)) {
                str += sprintf("%s = ?, ", mks[0]);
                arr += ({ mks[1] });
            }
        }  // end loop (verified)
    } else {
        loop (mks in info) {
            str += sprintf("%s = ?, ", mks[0]);
            arr += ({ mks[1] });
        }  // end loop (verified)
    }
    if (is_string(valid_checksum)) {
        str = (str[0..<3] + " where account = ? and checksum = ?");
        arr += ({ info["account"], valid_checksum });
    } else {
        str = (str[0..<3] + " where account = ?");
        arr += ({ info["account"] });
    }
    sql_cmd = mysql_bind_parameters_arr(str, arr);
    if (((*f_execute)(cno, sql_cmd) <= 0)) {
        printf("update_account_info mysql error: %s\n", (*f_get_error)(cno));
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int get_safe_tips_status() {
    /* ---- Enhanced Pseudocode ---- */
    return safe_tips_enbled;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=51, nargs=2, insts=51
// --------------------------------------------------------------------
int record_protect_range(string account, int range) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;

    if (((cno = "/daemons/dbd.c"->assure_connection("adb")) < 0)) {
        return -1;
    }
    sql_cmd = mysql_bind_parameters("insert into protect_range(account, p_range) values (?,?)", account, range);
    if (((*f_execute)(cno, sql_cmd) == 1)) {
        return 1;
    }
    sql_cmd = mysql_bind_parameters("update protect_range set p_range=? where account=?", range, account);
    if (((*f_execute)(cno, sql_cmd) < 0)) {
        return -1;
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=73, nargs=2, insts=73
// --------------------------------------------------------------------
mixed query_app_bind_info(string account, int format) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed arr;
    mixed sql_cmd;
    mapping m;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 0)) {
        return -1;
    }
    sql_cmd = mysql_bind_parameters("select * from app_bind where account = ?", account);
    arr = mysql_select(cno, sql_cmd, 1);
    if ((!is_array(arr))) {
        return -1;
    }
    if ((sizeof(arr) < 1)) {
        return ([ ]);
    }
    m = arr[0];
    if (format) {
        map_delete(m, "account");
        map_delete(m, "update_time");
        m["bind_time"] = time(m["bind_time"]);
    }
    return m;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=33, nargs=1, insts=33
// --------------------------------------------------------------------
override void connection_disconnect(object agent) {
    /* ---- Enhanced Pseudocode ---- */
    mixed account;

    if (is_string(account = agent->lite_query("account"))) {
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "------ ·þÎñÆ÷Ö÷¶¯¹Ø±ÕÁ¬½Ó£¬µÇÂ¼½áÊø\n");
        }
    }
    ::connection_disconnect(agent);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=38, nargs=1, insts=38
// --------------------------------------------------------------------
int get_account_matrix_status(mixed matrix_info) {
    /* ---- Enhanced Pseudocode ---- */
    if ((matrix_info == 1)) {
        return 1;
    }
    if ((matrix_info == 2)) {
        return 2;
    }
    if ((!is_mapping(matrix_info))) {
        return 1;
    }
    if ((!check_matrix_info(matrix_info))) {
        return 1;
    }
    if ((!atoi(matrix_info["enable"]))) {
        return 2;
    }
    return 3;

}

// --------------------------------------------------------------------
// @code  nc=45, nargs=0, insts=45
// --------------------------------------------------------------------
private void init_matrix() {
    /* ---- Enhanced Pseudocode ---- */
    mixed size;
    int i;
    mixed x;
    mixed y;

    size = (sizeof(vector_x) * sizeof(vector_y));
    matrix_list = allocate_array(size);
    i = 0;
    loop (x in vector_x) {
        loop (y in vector_y) {
            matrix_list[i++] = sprintf("%s%s", x, y);
        }  // end loop (verified)
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @code  nc=77, nargs=4, insts=77
// --------------------------------------------------------------------
private int pwd_protect_deny_fixed_tm(string account, mapping m, mixed param, mixed dist) {
    /* ---- Enhanced Pseudocode ---- */
    mixed tm_arr;
    mixed fixed_tm;
    mixed time_str;

    fixed_tm = m["protect_info"];
    if ((!is_string(fixed_tm))) {
        return 0;
    }
    tm_arr = explode(fixed_tm, "-");
    if (((((!is_array(tm_arr)) || (sizeof(tm_arr) != 2)) || (sizeof(tm_arr[0]) != 4)) || (sizeof(tm_arr[1]) != 4))) {
        return 0;
    }
    time_str = ctime(time(), 3)[8..11];
    if (((time_str < tm_arr[0]) || (time_str > tm_arr[1]))) {
        return 1;
    }
    return 0;

}

// --------------------------------------------------------------------
// @code  nc=28, nargs=2, insts=28
// --------------------------------------------------------------------
private int _check_client_apex_status(mapping account_info, string password) {
    /* ---- Enhanced Pseudocode ---- */
    mixed md5password;
    mixed new_md5password;

    md5password = password;
    new_md5password = calc_password_new_md5(account_info["account"], md5password);
    if ((stricmp(account_info["password"], new_md5password) == 0)) {
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=16, nargs=2, insts=16
// --------------------------------------------------------------------
string calc_password_new_md5(string account, string pwd) {
    /* ---- Enhanced Pseudocode ---- */
    return save_string(md5(((account + pwd) + "20070201")))[1..<2];

}

// --------------------------------------------------------------------
// @code  nc=85, nargs=5, insts=85
// --------------------------------------------------------------------
private void log_protect_time(string account, string branch, string protect_type, string time_desc, int cno) {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr;
    mixed sql_cmd;
    int protect_time;

    sql_cmd = mysql_bind_parameters("select * from protect_bonus where account = ? and branch = ?", account, branch);
    arr = mysql_select(cno, sql_cmd);
    if ((!is_array(arr))) {
        return;
    }
    protect_time = (protect_type + "_time");
    if ((sizeof(arr) < 1)) {
        sql_cmd = mysql_bind_parameters(((("insert into protect_bonus(account, branch, " + protect_time) + ", ") + "insert_time) values(?, ?, ?, ?)"), account, branch, time_desc, ctime(time(), 3));
    } else {
        time_desc = (((string)arr[0][protect_time] + ",") + time_desc);
        sql_cmd = mysql_bind_parameters(((("update protect_bonus set " + protect_time) + " = ?") + " where account = ? and branch = ?"), time_desc, account, branch);
    }
    (*f_execute)(cno, sql_cmd);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=31, nargs=1, insts=31
// --------------------------------------------------------------------
int get_account_controlled_safe_state(string account) {
    /* ---- Enhanced Pseudocode ---- */
    mapping switch_info;
    mixed safe_ctrl_state;

    switch_info = query_safe_ctrl(account, "total_switch");
    if (((!is_mapping(switch_info)) || (!sizeof(switch_info)))) {
        return 0;
    }
    safe_ctrl_state = atoi((string)switch_info["protect_info"]);
    return safe_ctrl_state;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=40, nargs=2, insts=40
// --------------------------------------------------------------------
void log_bad_checksum_account(string account, mapping account_info) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed sql_cmd;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 0)) {
        return;
    }
    sql_cmd = mysql_bind_parameters("insert into acc_log(ip, time,account, dist, type, msg, memo) values(?, ?, ?, ?, ?, ?, ?)", "", ctime(time(), 3)[0..13], account, "", "bad_checksum", save_string(account_info), "");
    (*f_execute)(cno, sql_cmd);

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=60, nargs=1, insts=60
// --------------------------------------------------------------------
mixed query_mp_device_info(string account) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed arr;
    mixed sql_cmd;
    mapping m;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 0)) {
        return -1;
    }
    sql_cmd = mysql_bind_parameters("select * from mp_device where account = ?", account);
    arr = mysql_select(cno, sql_cmd, 1);
    if ((!is_array(arr))) {
        return -1;
    }
    if ((sizeof(arr) < 1)) {
        return ([ ]);
    }
    m = arr[0];
    m["os"] = (int)m["os"];
    return m;

}

// --------------------------------------------------------------------
// @code  nc=113, nargs=1, insts=113
// --------------------------------------------------------------------
string calc_account_checksum(mapping account_info) {
    /* ---- Enhanced Pseudocode ---- */
    mixed account;
    mixed password;
    mixed privilege;
    mixed blocked_time;
    mixed gold_coin;
    mixed silver_coin;
    mixed coin_password;
    mixed unlock_coin_password_time;
    mixed trade_lock_time;
    mixed permit_ip;
    mixed permit_id;
    mixed str;

    account = account_info["account"];
    password = account_info["password"];
    permit_ip = account_info["permit_ip"];
    permit_id = account_info["permit_id"];
    privilege = sprintf("%08X", atoi(account_info["privilege"]));
    gold_coin = sprintf("%08X", atoi(account_info["gold_coin"]));
    silver_coin = sprintf("%08X", atoi(account_info["silver_coin"]));
    blocked_time = account_info["blocked_time"];
    coin_password = account_info["coin_password"];
    trade_lock_time = account_info["trade_lock_time"];
    unlock_coin_password_time = account_info["unlock_coin_password_time"];
    str = (((((((((((account + password) + privilege) + blocked_time) + gold_coin) + silver_coin) + coin_password) + unlock_coin_password_time) + trade_lock_time) + permit_ip) + permit_id) + "ABCDEF");
    return save_string(md5(str))[1..<2];

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=10, nargs=2, insts=10
// --------------------------------------------------------------------
private void set_protect_info_analysis_func(string type, mixed func) {
    /* ---- Enhanced Pseudocode ---- */
    set(("analysis_func/" + type), func);

}

// --------------------------------------------------------------------
// @code  nc=25, nargs=2, insts=25
// --------------------------------------------------------------------
mixed calc_local_encrypt_checksum(mapping m, string le_info) {
    /* ---- Enhanced Pseudocode ---- */
    mixed text;

    text = sprintf("%s%s%s%s", m["account"], le_info, m["time"], "20080823");
    return save_string(md5(text))[1..<2];

}

// --------------------------------------------------------------------
// @code  nc=22, nargs=4, insts=22
// --------------------------------------------------------------------
private string calc_matrix_pwd_md5(string value1, string value2, string value3, string value4) {
    /* ---- Enhanced Pseudocode ---- */
    mixed text;

    text = (((value1 + value2) + value3) + value4);
    return save_string(md5(text))[1..<2];

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=267, nargs=6, insts=267
// --------------------------------------------------------------------
int control_account(string account, int oper, string password, string permit_ip, string permit_id, mixed &prompt) {
    /* ---- Enhanced Pseudocode ---- */
    int privilege;
    int ret;
    string msg;
    mapping account_info;
    mapping m;

    ret = 0;
    do {
        msg = "";
        if ((!account_info = query_account_info(account))) {
            msg = "ÎÞ·¨»ñÈ¡ÕÊºÅÐÅÏ¢£¬¿ÉÄÜÊý¾Ý¿âÁ¬½ÓÊ§°Ü»òÕßÕÊºÅ²»´æÔÚ";
            break;
        }
        "/daemons/logd.c"->log_local(1, "control_account", "control", account_info["account"], "before_control", account_info["checksum"], save_string(account_info));
        if ((account_info["checksum"] != calc_account_checksum(account_info))) {
            msg = "ÕÊºÅÊý¾Ý±»ÆÆ»µ£¬ÔÝÊ±ÎÞ·¨½Ó¹ÜÕÊºÅ¡££¨Ô­Òò:1009£©";
            break;
        }
        if ((1 == oper)) {
            if ((is_int(privilege = atoi(account_info["privilege"])) && (privilege > 0))) {
                msg = "GMÕÊºÅÎÞ·¨±»½Ó¹Ü¡£";
                break;
            }
            if ((strlen(account_info["permit_id"]) > 0)) {
                msg = "¸ÃÕÊºÅÓÐ»úÆ÷ID°ó¶¨·þÎñ£¬²»ÄÜ±»½Ó¹Ü¡£";
                break;
            }
            if ((!strlen(permit_id))) {
                msg = "²Ù×÷²ÎÊý´íÎó";
                break;
            }
            if (((strlen(account_info["org_password"]) > 0) || (strlen(account_info["org_permit_ip"]) > 0))) {
                msg = "½Ó¹ÜÊ§°Ü£¬ÕÊºÅÒÑ´¦ÓÚ±»½Ó¹ÜÖÐ";
                break;
            }
            account_info["org_password"] = account_info["password"];
            account_info["org_permit_ip"] = account_info["permit_ip"];
            account_info["password"] = calc_password_new_md5(account_info["account"], password);
            account_info["permit_ip"] = permit_ip;
            account_info["permit_id"] = permit_id;
            msg = "½Ó¹Ü³É¹¦";
        } else {
            if (((!account_info["org_password"]) || (account_info["org_password"] == ""))) {
                msg = "ÕÊºÅÎ´±»½Ó¹Ü£¬½â³ýÊ§°Ü";
                break;
            }
            account_info["password"] = account_info["org_password"];
            account_info["permit_ip"] = account_info["org_permit_ip"];
            account_info["permit_id"] = "";
            account_info["org_password"] = "";
            account_info["org_permit_ip"] = "";
            msg = "½â³ý½Ó¹Ü³É¹¦";
        }
        if ((!update_account_info(account_info))) {
            msg = "½Ó¹ÜÊ§°Ü£¬¿ÉÄÜÊÇÊý¾Ý¿âÒì³£";
            break;
        }
        ret = 1;
    } while (0);

    // --- label L0236 ---
    prompt = msg;
    m = query_account_info(account);
    "/daemons/logd.c"->log_local(1, "control_account", "control", m["account"], "after_control", m["checksum"], save_string(m));
    return ret;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int get_matrix_status() {
    /* ---- Enhanced Pseudocode ---- */
    return matrix_active_status;

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=14, nargs=1, insts=14
// --------------------------------------------------------------------
int set_active_account_status(int status) {
    /* ---- Enhanced Pseudocode ---- */
    mixed current_status;

    current_status = get_active_account_status();
    account_active_status = status;
    return current_status;

}

// --------------------------------------------------------------------
// @code  nc=36, nargs=5, insts=36
// --------------------------------------------------------------------
mixed auth_phone_protect(string account, string ip, string dist, string auth_info, mixed &prompt) {
    /* ---- Enhanced Pseudocode ---- */
    mixed auth_result;
    string deny_prompt;

    deny_prompt = "Çë²¦´òµç»°#Y400-811-8801#n»ò#Y400-618-9132#n½â³ýËø¶¨¡£½â³ýºó£¬µã»÷#RÈ·ÈÏ#n¼ÌÐø²Ù×÷¡£";
    auth_result = deny_by_pwd_protect(account, "phone", &deny_prompt, ip, dist);
    switch (auth_result) {
        case 0:
        case 1:
        prompt = deny_prompt;
        return -1;
        case 2:
        return 1;
        default:
        return 0;
    }  // end switch

}

// --------------------------------------------------------------------
// @code  nc=66, nargs=2, insts=66
// --------------------------------------------------------------------
private int pwd_protect_fetch_phone(string account, mapping &m) {
    /* ---- Enhanced Pseudocode ---- */
    mixed protect_fields;

    if (strlen(m["protect_info"])) {
        protect_fields = explode(m["protect_info"], ",");
    } else {
        protect_fields = ({ "" });
    }
    switch (sizeof(protect_fields)) {
        case 0:
        break;
        case 1:
        m["permission_expired_time"] = trim(protect_fields[0]);
        break;
        case 2:
        m["permission_expired_time"] = trim(protect_fields[0]);
        m["login_ip"] = trim(protect_fields[1]);
        break;
        default:
        break;
    }  // end switch

    // --- label L0062 ---
    return 1;

}

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=50, nargs=2, insts=50
// --------------------------------------------------------------------
private void do_when_batch_block_account(array arr_account, string dist) {
    /* ---- Enhanced Pseudocode ---- */
    mixed mks;
    mixed agent;
    mixed server;
    mixed text;
    mixed all_conns;

    all_conns = "/aaa/daemons/gameserverd.c"->get_all_conns();
    if ((sizeof(all_conns) < 1)) {
        return;
    }
    text = save_string(arr_account);
    loop (mks in all_conns) {
        server = mks[0];
        if ((!is_object(agent = "/aaa/daemons/accountd.c"->assure_server_agent(server)))) {
            continue;
        }
        agent->send_message(9348, text);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @code  nc=66, nargs=2, insts=66
// --------------------------------------------------------------------
private int pwd_protect_fetch_sm(string account, mapping &m) {
    /* ---- Enhanced Pseudocode ---- */
    mixed protect_fields;

    if (strlen(m["protect_info"])) {
        protect_fields = explode(m["protect_info"], ",");
    } else {
        protect_fields = ({ "" });
    }
    switch (sizeof(protect_fields)) {
        case 0:
        break;
        case 1:
        m["permission_expired_time"] = trim(protect_fields[0]);
        break;
        case 2:
        m["permission_expired_time"] = trim(protect_fields[0]);
        m["login_ip"] = trim(protect_fields[1]);
        break;
        default:
        break;
    }  // end switch

    // --- label L0062 ---
    return 1;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=100, nargs=2, insts=100
// --------------------------------------------------------------------
void refresh_matrix_pic(mixed agent, int index) {
    /* ---- Enhanced Pseudocode ---- */
    mixed last_refresh;
    mixed matrix_random;
    mixed auth_pic_buf;
    mixed auth_text;

    if ((!is_object(agent))) {
        return;
    }
    if ((!is_matrix_pic_enabled())) {
        return;
    }
    if ((!agent->lite_query_temp("matrix"))) {
        return;
    }
    last_refresh = agent->query_temp(("last_refresh/" + index));
    if ((is_int(last_refresh) && ((time() - last_refresh) < 3))) {
        agent->send_message(8165, "Ë¢ÐÂÊ±¼ä¼ä¸ô²»ÄÜÐ¡ÓÚ3Ãë", 1);
        return;
    }
    matrix_random = agent->lite_query_temp("matrix_random");
    if ((!is_array(matrix_random))) {
        return;
    }
    if ((index >= sizeof(matrix_random))) {
        return;
    }
    auth_text = encode_matrix_auth_text(agent->lite_query_temp("matrix_cookie"), "", "", "");
    auth_pic_buf = get_matrix_pic(({ matrix_random[index] }));
    if ((!is_object(agent))) {
        return;
    }
    agent->send_message(4987, auth_text, auth_pic_buf, (index + 1));

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=59, nargs=1, insts=59
// --------------------------------------------------------------------
string get_place_by_ip(string ip_addr) {
    /* ---- Enhanced Pseudocode ---- */
    mixed sql_cmd;
    int cno;
    mixed ip_int;
    mapping map;

    if ((!sizeof(ip_addr))) {
        return "";
    }
    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 1)) {
        return "";
    }
    ip_int = atoinet(ip_addr);
    sql_cmd = mysql_bind_parameters("select * from ip_info where start_ip <= ? and end_ip >= ?", ip_int, ip_int);
    map = mysql_select_single_result(cno, sql_cmd);
    if ((!is_mapping(map))) {
        return "";
    }
    return sprintf("%s@%s@%s", map["country"], map["province"], map["city"]);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=218, nargs=10, insts=218
// --------------------------------------------------------------------
private void account_login(object agent, string account, string password, string mac_addr, string client_data, string passpod, string dist, int mobile_client, string version, string qr_id) {
    /* ---- Enhanced Pseudocode ---- */
    mixed account_ob;
    mixed qr_code_info;
    int i;

    if ((strlen(version) & 1)) {
        agent->send_message(8165, "Äãµ±Ç°µÄ¿Í»§¶Ë°æ±¾Óë·þÎñÆ÷°æ±¾²»Æ¥Åä£¬ÎÞ·¨µÇÂ¼ÓÎÏ·¡£\nÇë¸üÐÂµ½1.60.0905°æ±¾¡£", 1);
        agent->to_be_disconnect(2);
        return;
    }
    for (i = 0; i <= (strlen(version) - 1); i++) {
        if ((!isxdigit(version[i]))) {
            agent->send_message(8165, "Äãµ±Ç°µÄ¿Í»§¶Ë°æ±¾Óë·þÎñÆ÷°æ±¾²»Æ¥Åä£¬ÎÞ·¨µÇÂ¼ÓÎÏ·¡£\nÇë¸üÐÂµ½1.60.0905°æ±¾¡£", 1);
            agent->to_be_disconnect(2);
            return;
        }
    }  // end loop (verified)
    version = (string)des(restore_value(((":" + version) + ":")), 1, "abcdef");
    if (((!mobile_client) && (version != "1.60.0905"))) {
        agent->send_message(8165, "Äãµ±Ç°µÄ¿Í»§¶Ë°æ±¾Óë·þÎñÆ÷°æ±¾²»Æ¥Åä£¬ÎÞ·¨µÇÂ¼ÓÎÏ·¡£\nÇë¸üÐÂµ½1.60.0905°æ±¾¡£", 1);
        agent->to_be_disconnect(2);
        return;
    }
    if ((!agent->lite_query_temp("antibot_answer"))) {
        agent->to_be_disconnect(0);
        return;
    }
    account_ob = clone_object("/aaa/clone/account.c", ([ "account": account ]));
    agent->relay_agent(account_ob);
    set_dynamic_key(account_ob, "login", query_dynamic_key(agent, "login"));
    if (is_mapping(qr_code_info = agent->lite_query_temp("qr_code"))) {
        account_ob->lite_set_temp("qr_code", qr_code_info);
    }
    destruct_object(agent);
    set_client_attrib(account_ob->get_client_no(), ([ "type": "Client", "size": 4094, "peek": -1 ]));
    account_ob->lite_set_temp("mobile_client", mobile_client);
    if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
        "/daemons/logd.c"->assure_log_file(get_login_info_file(account));
    }
    if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
        "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", sprintf("------ ¿Í»§¶Ë´Ó%sµÇÂ¼\n", account_ob->get_peer_ip()));
    }
    "/daemons/threadd.c"->dispatch_service((: _account_login, account_ob, account, password, mac_addr, client_data, passpod, dist, qr_id, 1 :));

}

// --------------------------------------------------------------------
// @code  nc=107, nargs=2, insts=107
// --------------------------------------------------------------------
private int pwd_protect_fetch_matrix(string account, mapping &m) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    int num;
    mixed protect_fields;

    protect_fields = explode(m["protect_info"], ",");
    num = sizeof(protect_fields);
    if ((num < 6)) {
        return 1;
    }
    for (i = 0; i <= (num - 1); i++) {
        protect_fields[i] = trim(protect_fields[i]);
    }  // end loop (verified)
    m["matrix"] = protect_fields[0];
    m["bind_time"] = protect_fields[1];
    m["expired_time"] = protect_fields[2];
    m["claim_lost_off_time"] = protect_fields[3];
    m["enable"] = protect_fields[4];
    m["update_time"] = protect_fields[5];
    if ((num > 6)) {
        m["used_matrix"] = protect_fields[6..<1];
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=52, nargs=2, insts=52
// --------------------------------------------------------------------
int delete_protect_info(string account, mixed protect_type) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed sql_cmd;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 0)) {
        return 0;
    }
    if (is_string(protect_type)) {
        sql_cmd = mysql_bind_parameters("delete from pwd_protect where account = ? and protect_type = ?", account, protect_type);
    } else {
        sql_cmd = mysql_bind_parameters("delete from pwd_protect where account = ?", account);
    }
    if (((*f_execute)(cno, sql_cmd) < 1)) {
        printf("delete_protect_info mysql error: %s\n", (*f_get_error)(cno));
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
void enable_safe_tips() {
    /* ---- Enhanced Pseudocode ---- */
    safe_tips_enbled = 1;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=92, nargs=1, insts=92
// --------------------------------------------------------------------
int update_protect_info(mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed f;
    mixed sql_cmd;
    mixed protect_type;
    mixed account;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 0)) {
        return 0;
    }
    protect_type = m["protect_type"];
    account = m["account"];
    f = query(("pwd_protect_update_func/" + protect_type));
    if ((is_function(f) && (!(*f)(account, &m)))) {
        return 0;
    }
    m["checksum"] = calc_protect_info_checksum(m);
    sql_cmd = mysql_bind_parameters("update pwd_protect set protect_info = ?, bind_time =?, checksum = ?, memo = ? where account = ? and protect_type = ?", m["protect_info"], m["bind_time"], m["checksum"], m["memo"], account, protect_type);
    if (((*f_execute)(cno, sql_cmd) < 1)) {
        printf("update_protect_info mysql error: %s\n", (*f_get_error)(cno));
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=30, nargs=4, insts=30
// --------------------------------------------------------------------
private int pwd_protect_deny_fixed_city(string account, mapping m, mixed param, mixed dist) {
    /* ---- Enhanced Pseudocode ---- */
    mixed set_city;
    mixed place;

    if ((!get_fixed_city_status())) {
        return 0;
    }
    set_city = trim(m["protect_info"]);
    place = get_place_by_ip(trim(param));
    if ((place == set_city)) {
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=33, nargs=2, insts=33
// --------------------------------------------------------------------
private int pwd_protect_update_phone(string account, mapping &m) {
    /* ---- Enhanced Pseudocode ---- */
    mixed protect_info;

    protect_info = sprintf("%s", m["permission_expired_time"]);
    if (is_string(m["login_ip"])) {
        protect_info += sprintf(",%s", m["login_ip"]);
    }
    m["protect_info"] = protect_info;
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=57, nargs=5, insts=57
// --------------------------------------------------------------------
private int deny_by_pwd_protect(string account, string protect_type, mixed &prompt, mixed param, mixed dist) {
    /* ---- Enhanced Pseudocode ---- */
    mixed f;
    mapping m;

    m = query_protect_info(account, protect_type);
    if ((!is_mapping(m))) {
        return 2;
    }
    if ((calc_protect_info_checksum(m) != toupper(m["checksum"]))) {
        prompt = "ÑéÖ¤·þÎñÆ÷Òì³££¬µÇÂ¼Ê§°Ü¡££¨Ô­Òò£º1015£©";
        return 0;
    }
    f = query(("pwd_protect_deny_func/" + protect_type));
    if ((is_function(f) && (*f)(account, m, param, dist))) {
        return 1;
    }
    return 3;

}

// --------------------------------------------------------------------
// @code  nc=14, nargs=2, insts=14
// --------------------------------------------------------------------
int update_matrix_info(mapping m, int encode) {
    /* ---- Enhanced Pseudocode ---- */
    if (encode) {
        encode_matrix_info(&m);
    }
    return update_protect_info(m);

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=130, nargs=3, insts=130
// --------------------------------------------------------------------
int is_protect_actived(string account, int protect_flag, mixed account_ob) {
    /* ---- Enhanced Pseudocode ---- */
    mixed matrix_status;
    int protect_bits;
    mixed protect_info;

    protect_bits = 0;
    if ((4 & protect_flag)) {
        protect_info = query_protect_info(account, "passpod");
        if (is_mapping(protect_info)) {
            protect_bits |= 4;
            if (is_object(account_ob)) {
                account_ob->set_temp("protects/A", 1);
            }
        }
    }
    if ((2 & protect_flag)) {
        protect_info = query_protect_info(account, "phone");
        if (is_mapping(protect_info)) {
            protect_bits |= 2;
            if (is_object(account_ob)) {
                account_ob->set_temp("protects/B", 1);
            }
        }
    }
    if ((1 & protect_flag)) {
        protect_info = query_protect_info(account, "sm");
        if (is_mapping(protect_info)) {
            protect_bits |= 1;
            if (is_object(account_ob)) {
                account_ob->set_temp("protects/C", 1);
            }
        }
    }
    if ((get_matrix_status() && (8 & protect_flag))) {
        protect_info = query_matrix_info(account);
        matrix_status = get_account_matrix_status(protect_info);
        if ((3 == matrix_status)) {
            protect_bits |= 8;
            if (is_object(account_ob)) {
                account_ob->set_temp("protects/D", 1);
            }
        }
    }
    return protect_bits;

}

// --------------------------------------------------------------------
// @code  nc=139, nargs=5, insts=139
// --------------------------------------------------------------------
int temp_block_account(string dist, string account, int interval, string reason, mixed &prompt) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ti;
    int ret;
    string msg;
    string btime;
    mapping account_info;

    ret = 0;
    do {
        if ((interval < 0)) {
            msg = "ÁÙÊ±·âÍ£ÕÊºÅ²ÎÊýinterval±ØÐë>=0";
            break;
        }
        if ((!account_info = query_account_info(account))) {
            msg = "ÎÞ·¨»ñÈ¡ÕÊºÅÐÅÏ¢£¬¿ÉÄÜÊý¾Ý¿âÁ¬½ÓÊ§°Ü»òÕßÕÊºÅ²»´æÔÚ";
            break;
        }
        if ((account_info["checksum"] != calc_account_checksum(account_info))) {
            msg = "ÕÊºÅÊý¾Ý±»ÆÆ»µ£¬ÔÝÊ±ÎÞ·¨·â±ÕÕÊºÅ¡££¨Ô­Òò£º1009£©";
            break;
        }
        ti = time();
        interval += ti;
        if ((ti == interval)) {
            btime = "";
        } else {
            btime = ctime(interval, 3);
        }
        if (((account_info["temp_blocked_time"] != btime) || (account_info["temp_blocked_reason"] != reason))) {
            account_info["temp_blocked_time"] = btime;
            account_info["temp_blocked_reason"] = reason;
            if ((!update_account_info(account_info))) {
                if ((!sizeof(btime))) {
                    msg = "½â³ýÁÙÊ±·âÍ£ÕÊºÅ²Ù×÷Ê§°Ü£¬¿ÉÄÜÊý¾Ý¿âÒì³£";
                } else {
                    msg = "ÁÙÊ±·âÍ£ÕÊºÅ²Ù×÷Ê§°Ü£¬¿ÉÄÜÊý¾Ý¿âÒì³£";
                }
                break;
            }
        }
        if ((!sizeof(btime))) {
            msg = "½â³ýÁÙÊ±·âÍ£ÕÊºÅ²Ù×÷³É¹¦";
        } else {
            msg = "ÁÙÊ±·âÍ£ÕÊºÅ²Ù×÷³É¹¦";
        }
        ret = 1;
    } while (0);

    // --- label L0129 ---
    prompt = msg;
    return ret;

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
void disable_local_encrypt() {
    /* ---- Enhanced Pseudocode ---- */
    local_encrypt_enabled = 0;

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
void enable_apex() {
    /* ---- Enhanced Pseudocode ---- */
    apex_enabled = 1;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=130, nargs=4, insts=130
// --------------------------------------------------------------------
private int pwd_protect_deny_sm(string account, mapping m, mixed param, mixed dist) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ct;
    mixed login_ip;
    mixed auth_method;
    mapping map;

    auth_method = get_auth_method();
    switch (auth_method) {
        case 1:
        ct = ctime(time(), 3);
        if ((ct > m["permission_expired_time"])) {
            return 1;
        }
        break;
        case 2:
        map = "/aaa/daemons/pwdprotect_authd.c"->auth_protect("sm", account, dist);
        if ((map["result"] != 0)) {
            if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
                "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", sprintf("¶ÌÐÅÃÜ±£ÈÏÖ¤Ê§°Ü£¬Ô­ÒòÊÇ%s\n", map["reason"]));
            }
            return 1;
        }
        if ((map["reason"] == "AUTH_UNBIND")) {
            cancel_pwd_protect(account, "sm", param, dist);
        }
        return 0;
        default:
        break;
    }  // end switch

    // --- label L0083 ---
    if ((!is_object(param))) {
        return 0;
    }
    login_ip = sprintf("%s@%s", param->get_peer_ip(), param->lite_query_temp("mac_addr"));
    if ((!m["login_ip"])) {
        m["login_ip"] = login_ip;
        update_matrix_info(m);
        return 0;
    }
    if ((m["login_ip"] != login_ip)) {
        return 1;
    }
    return 0;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=88, nargs=1, insts=88
// --------------------------------------------------------------------
int insert_safe_ctrl(mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    string sql_cmd;
    mixed protect_type;
    mixed account;
    mixed m_info;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 0)) {
        return 0;
    }
    protect_type = m["protect_type"];
    account = m["account"];
    m_info = query_safe_ctrl(account, protect_type);
    if ((is_mapping(m_info) && sizeof(m_info))) {
        return update_protect_info_ex(m);
    }
    m["checksum"] = calc_protect_info_checksum(m);
    sql_cmd = "insert into safe_ctrl (account, protect_type, protect_info, checksum, memo) values (?, ?, ?, ?, ?)";
    sql_cmd = mysql_bind_parameters(sql_cmd, m["account"], m["protect_type"], m["protect_info"], m["checksum"], (string)m["memo"]);
    if (((*f_execute)(cno, sql_cmd) < 0)) {
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=82, nargs=2, insts=82
// --------------------------------------------------------------------
mixed query_phone_bind_info(string account, int format) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed arr;
    mixed sql_cmd;
    mapping m;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 0)) {
        return -1;
    }
    sql_cmd = mysql_bind_parameters("select * from phone_bind where account = ?", account);
    arr = mysql_select(cno, sql_cmd, 1);
    if ((!is_array(arr))) {
        return -1;
    }
    if ((sizeof(arr) < 1)) {
        return ([ ]);
    }
    m = arr[0];
    if (format) {
        map_delete(m, "account");
        map_delete(m, "update_time");
        m["bind_time"] = time(m["bind_time"]);
        m["state"] = (int)m["state"];
    }
    return m;

}

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=39, nargs=2, insts=39
// --------------------------------------------------------------------
private void do_when_block_account(string account, string dist) {
    /* ---- Enhanced Pseudocode ---- */
    mixed mks;
    mixed server;
    mixed agent;
    mixed all_conns;

    all_conns = "/aaa/daemons/gameserverd.c"->get_all_conns();
    loop (mks in all_conns) {
        server = mks[0];
        if ((!is_object(agent = "/aaa/daemons/accountd.c"->assure_server_agent(server)))) {
            continue;
        }
        agent->send_message(9261, account);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  parser: extract/transform data
// @code  nc=43, nargs=1, insts=43
// --------------------------------------------------------------------
void decode_matrix_info(mapping &m) {
    /* ---- Enhanced Pseudocode ---- */
    mixed matrix_buf;
    string new_matrix;
    mixed matrix;

    matrix = m["matrix"];
    try {
        matrix_buf = restore_value(sprintf(":%s:", matrix));
    } catch { /* error handled */ }

    // --- label L0020 ---
    new_matrix = (string)std_des(matrix_buf, 1, "20070622", "", 1, 1, 1, "XiaMenGB");
    m["matrix"] = new_matrix;

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
void disable_apex() {
    /* ---- Enhanced Pseudocode ---- */
    apex_enabled = 0;

}

// --------------------------------------------------------------------
// @code  nc=27, nargs=0, insts=27
// --------------------------------------------------------------------
private void _timer_clean_client_seedA() {
    /* ---- Enhanced Pseudocode ---- */
    mixed client_no;

    loop (client_no in keys(client_seedA)) {
        if ((!find_agent_by_client(client_no))) {
            map_delete(client_seedA, client_no);
        }
    }  // end loop (verified)
    delete_timer("t_clean_seedA");
    set_timer(60, "t_clean_seedA", (: _timer_clean_client_seedA :));

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=25, nargs=1, insts=25
// --------------------------------------------------------------------
mixed query_local_encrypt_info(string account) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;

    if (((cno = "/daemons/dbd.c"->assure_connection("adb")) == -1)) {
        return 0;
    }
    sql_cmd = mysql_bind_parameters("select * from local_encrypt where account = ?", account);
    return mysql_select_single_result(cno, sql_cmd);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=52, nargs=3, insts=52
// --------------------------------------------------------------------
void get_extra_block_reason_msg(int check_left_day, string real_reason, string &breason) {
    /* ---- Enhanced Pseudocode ---- */
    mixed reason_type;
    mixed reason_meno;

    if ((sscanf(real_reason, "%s(%s)", &reason_type, &reason_meno) != 2)) {
        return;
    }
    switch (reason_type) {
        case "Íâ¹Ò":
        breason = "ÄúµÄÕÊºÅÉæÏÓÊ¹ÓÃÍâ¹Ò±»·â´æ£¬";
        break;
        case "µÁºÅ":
        breason = "ÄúµÄÕÊºÅÉæÏÓµÁºÅ±»·â´æ£¬";
        break;
        case "ÏÞÖÆµÇÂ¼":
        breason = "ÏµÍ³ÒÑ¾­ÏÞÖÆ´ËÕÊºÅµÇÂ¼£¬";
        break;
        default:
        return;
    }  // end switch

    // --- label L0036 ---
    if ((check_left_day >= 1)) {
        breason += sprintf("½«ÓÚ#R%d#nÌìºó½â³ý·â±Õ¡£", check_left_day);
    }
    breason += "ÈçÓÐÒìÒé£¬ÇëÇ°Íù#url¿Í·þÖÐÐÄ{http://faq.gyyx.cn/Service/ResumeSubmit.aspx}#tÉêÇë½â·â¡£";

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int get_active_account_status() {
    /* ---- Enhanced Pseudocode ---- */
    return account_active_status;

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
void enable_matrix_pic() {
    /* ---- Enhanced Pseudocode ---- */
    matrix_pic_enabled = 1;

}

// --------------------------------------------------------------------
// @code  nc=23, nargs=4, insts=23
// --------------------------------------------------------------------
private string encode_matrix_auth_text(string cookie, string pos1, string pos2, string pos3) {
    /* ---- Enhanced Pseudocode ---- */
    mixed text;

    text = sprintf("%s/%s/%s/%s", cookie, pos1, pos2, pos3);
    return save_string(des(text, 0, "abcdef"))[1..<2];

}

// --------------------------------------------------------------------
// @code  nc=77, nargs=4, insts=77
// --------------------------------------------------------------------
private string des_local_encrypt_pos(array arr_pos, string replace_str, string seed1, string seed2) {
    /* ---- Enhanced Pseudocode ---- */
    mixed pos;
    int i;
    mixed seed;
    string des_str;
    mixed str_pos;

    des_str = "";
    loop (pos in arr_pos) {
        --pos;
        seed = ((pos % 2) ? seed1 : seed2);
        str_pos = sprintf("%09d", pos);
        i = 0;

        // --- label L0036 ---
        while ((i < strlen(str_pos))) {
            put_sub_data(&str_pos, i, replace_str[(str_pos[i] - 48)]);
            ++i;
        }  // end loop (verified)
        des_str += save_string(des(str_pos, 0, seed))[1..<2];
    }  // end loop (verified)
    return des_str;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=82, nargs=1, insts=82
// --------------------------------------------------------------------
int update_protect_info_ex(mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed sql_cmd;
    mixed f;
    mixed protect_type;
    mixed account;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 0)) {
        return 0;
    }
    protect_type = m["protect_type"];
    account = m["account"];
    f = query(("pwd_protect_update_func/" + protect_type));
    if ((is_function(f) && (!(*f)(account, &m)))) {
        return 0;
    }
    m["checksum"] = calc_protect_info_checksum(m);
    sql_cmd = mysql_bind_parameters("update safe_ctrl set protect_info = ?, checksum = ?, memo = ? where account = ? and protect_type = ?", m["protect_info"], m["checksum"], m["memo"], account, protect_type);
    if (((*f_execute)(cno, sql_cmd) < 0)) {
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=23, nargs=1, insts=23
// --------------------------------------------------------------------
private buffer build_local_encrypt_pos_buf(array arr_pos) {
    /* ---- Enhanced Pseudocode ---- */
    mixed pos;
    mixed buf;

    buf = make16(sizeof(arr_pos));
    loop (pos in arr_pos) {
        buf += make32(pos);
    }  // end loop (verified)
    return buf;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=106, nargs=3, insts=106
// --------------------------------------------------------------------
void _send_antibot_question(mixed agent, string md5_value, string checksum) {
    /* ---- Enhanced Pseudocode ---- */
    mixed text;
    mixed new_checksum;

    if ((!is_object(agent))) {
        return;
    }
    if (is_need_open_guard) {
        if (((sizeof(md5_value) == 0) || (sizeof(checksum) == 0))) {
            agent->to_be_disconnect(0);
            return;
        }
        new_checksum = save_string(des(md5_value, 0, "uWvgwf"))[1..<2];
        if ((new_checksum != checksum)) {
            agent->to_be_disconnect(0);
            return;
        }
    }
    if (agent->lite_query_temp("antibot_question")) {
        agent->to_be_disconnect(0);
        return;
    }
    text = "/aaa/daemons/antibotd.c"->user_login(agent->get_client_no());
    if ((!is_object(agent))) {
        return;
    }
    if ((!is_string(text))) {
        agent->to_be_disconnect(0);
        return;
    }
    agent->lite_set_temp("antibot_question", 1);
    agent->send_message(6918, text, is_need_open_guard, "/aaa/daemons/qr_coded.c"->get_qr_code_status());

}

// --------------------------------------------------------------------
// @brief  validator: verify data
// @code  nc=50, nargs=2, insts=50
// --------------------------------------------------------------------
private int check_for_protect_account_ex(string account, mixed &ret) {
    /* ---- Enhanced Pseudocode ---- */
    mapping protect_info;

    if ((!protect_info = protect_account_ex[account])) {
        return 0;
    }
    if (is_undefined(clean_protect_account_timer_ex)) {
        set_timer(600, &clean_protect_account_timer_ex, (: clean_protect_account_ex :));
    }
    if ((protect_info["times"] < 5)) {
        return 0;
    }
    if (((time() - protect_info["update_time"]) > 600)) {
        map_delete(protect_account, account);
        return 0;
    }
    ret = "ÓÉÓÚÄã¶ÌÊ±¼äÄÚ¶à´Î³¢ÊÔµÇÂ¼¸ÃÕÊºÅ¶ø¾ùÎ´³É¹¦½âËø£¬#R10#n·ÖÖÓÄÚ¸ÃÕÊºÅ»á±»Ëø¶¨£¬ÇëÉÔºó³¢ÊÔ¡£";
    return 1;

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=7, nargs=0, insts=7
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    delete_timer("t_clean_seedA");

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=12, nargs=4, insts=12
// --------------------------------------------------------------------
private void send_login_result(object agent, int result, int auth_key, string prompt) {
    /* ---- Enhanced Pseudocode ---- */
    agent->send_message(21329, result, auth_key, prompt);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=41, nargs=4, insts=41
// --------------------------------------------------------------------
private void log_antibot(string ip, string account, int bot_type, mixed bot_content) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed sql_cmd;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 0)) {
        return;
    }
    if ((!is_string(bot_content))) {
        bot_content = "";
    }
    sql_cmd = mysql_bind_parameters("insert into antibot_log(ip, account, bot_type, bot_content, time) values(?, ?, ?, ?, ?)", ip, account, bot_type, bot_content, ctime(time(), 3));
    (*f_execute)(cno, sql_cmd);

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=21, nargs=1, insts=21
// --------------------------------------------------------------------
int is_enable_matrix_info_for_gs(string account) {
    /* ---- Enhanced Pseudocode ---- */
    mapping matrix_info;

    matrix_info = query_safe_ctrl(account, "matrix");
    if ((!is_mapping(matrix_info))) {
        return 0;
    }
    return (int)matrix_info["enable"];

}

// --------------------------------------------------------------------
// @brief  parser: extract/transform data
// @code  nc=54, nargs=1, insts=54
// --------------------------------------------------------------------
private mixed decode_matrix_auth_text(string auth_text) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cookie;
    mixed pwd_md5;
    string raw_text;
    mixed text;
    mixed buf;
    mixed m;

    text = sprintf(":%s:", auth_text);
    try {
        buf = restore_value(text);
    } catch { /* error handled */ }

    // --- label L0018 ---
    if ((!is_buffer(buf))) {
        return;
    }
    raw_text = (string)des(buf, 1, "abcdef");
    if ((sscanf(raw_text, "%s/%s", &cookie, &pwd_md5) != 2)) {
        return;
    }
    m = ([ "matrix_cookie": cookie, "MD5": pwd_md5 ]);
    return m;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=126, nargs=1, insts=126
// --------------------------------------------------------------------
mapping get_coin_info(mapping account_info) {
    /* ---- Enhanced Pseudocode ---- */
    int ul_ti;
    int have_pwd;
    mapping coin_info;

    coin_info = ([ "gold_coin": atoi(account_info["gold_coin"]), "silver_coin": atoi(account_info["silver_coin"]), "trade_coin": atoi(account_info["trade_coin"]), "consum_coin": atoi(account_info["consum_coin"]), "last_trade_coin": account_info["last_trade_coin"], "last_consum_coin": account_info["last_consum_coin"], "limit_trade_coin": atoi(account_info["limit_trade_coin"]) ]);
    ul_ti = account_info["unlock_coin_password_time"];
    if ((ul_ti != "")) {
        ul_ti = time_desc_to_int(ul_ti, 3);
    } else {
        ul_ti = 0;
    }
    have_pwd = (is_string(account_info["coin_password"]) && (account_info["coin_password"] != ""));
    if (((have_pwd && (ul_ti > 0)) && ((ul_ti + 604800) < time()))) {
        account_info["unlock_coin_password_time"] = "";
        account_info["coin_password"] = "";
        if ("/aaa/daemons/accountd.c"->update_account_info(account_info)) {
            have_pwd = 0;
        }
    }
    coin_info["ul_ti"] = ul_ti;
    coin_info["have_pwd"] = have_pwd;
    return coin_info;

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
void set_auth_method(int _method) {
    /* ---- Enhanced Pseudocode ---- */
    g_auth_method = _method;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=11, nargs=2, insts=11
// --------------------------------------------------------------------
void client_auth_pic(mixed agent, string client_data) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_service((: _client_auth_pic, agent, client_data :));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=455, nargs=6, insts=455
// --------------------------------------------------------------------
int batch_block_account(string dist, array arr_account, int interval, string reason, int ignore_data, mixed &prompt) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ti;
    mixed cno;
    int count;
    int i;
    int ret;
    mixed arr;
    mixed new_arr_account;
    string msg;
    string btime;
    mixed sql_cmd;
    int account;
    mapping account_info;
    mixed info_list;
    mixed valids;
    mixed invalids;
    mixed db_fails;

    count = sizeof(arr_account);
    for (i = 0; i <= (count - 1); i++) {
        arr_account[i] = tolower(arr_account[i]);
    }  // end loop (verified)
    ret = 0;
    do {
        if ((count < 1)) {
            msg = "Ã»ÓÐ´«ÈëÈÎºÎÒª·â±ÕµÄÕÊºÅ";
            break;
        }
        if (((cno = "/daemons/dbd.c"->assure_connection("adb")) < 0)) {
            msg = "Á¬½ÓADBÊ§°Ü";
            break;
        }
        sql_cmd = sprintf("select * from account where account in (%s)", implode(allocate_array(count, "?"), ","));
        sql_cmd = mysql_bind_parameters_arr(sql_cmd, arr_account);
        arr = mysql_select(cno, sql_cmd, count);
        if ((!is_array(arr))) {
            msg = "²éÑ¯account±íÊ§°Ü";
            break;
        }
        clean_array(arr, 7);
        info_list = ([ ]);
        valids = ([ ]);
        invalids = ([ ]);
        db_fails = ([ ]);
        loop (account_info in arr) {
            account = tolower(account_info["account"]);
            info_list[account] = account_info;
            if ((account_info["checksum"] != calc_account_checksum(account_info))) {
                invalids[account] = 1;
            } else {
                valids[account] = 1;
            }
        }  // end loop (verified)
        if ((sizeof(info_list) < 1)) {
            msg = "Ëù´«ÕÊºÅ¾ù²»´æÔÚ";
            break;
        }
        new_arr_account = keys(valids);
        count = sizeof(new_arr_account);
        if ((count < 1)) {
            msg = "Ëù´«ÕÊºÅ¾ù·¢ÉúCHECKSUM´íÎó";
            break;
        }
        if ((-2 == interval)) {
            for (i = 0; i <= (count - 1); i++) {
                account_info = info_list[new_arr_account[i]];
                account_info["blocked_reason"] = reason;
                if ((!update_account_info(account_info, 0, account_info["checksum"]))) {
                    db_fails[new_arr_account[i]] = 1;
                    new_arr_account[i] = 0;
                }
            }  // end loop (verified)
            clean_array(new_arr_account, 4);
            msg = sprintf("%s||OK:%s^NOT_EXIST:%s^BAD_MD5:%s^DB_FAIL:%s", "ÅúÁ¿¸üÐÂ·âºÅÔ­Òò³É¹¦", implode(new_arr_account, ","), implode((arr_account - keys(info_list)), ","), implode(keys(invalids), ","), implode(keys(db_fails), ","));
            ret = 1;
            break;
        }
        ti = time();
        if ((interval < 0)) {
            interval = 2147483647;
        } else {
            interval += ti;
        }
        if ((ti == interval)) {
            btime = "";
        } else {
            btime = ctime(interval, 3);
        }
        for (i = 0; i <= (count - 1); i++) {
            account_info = info_list[new_arr_account[i]];
            account_info["blocked_time"] = btime;
            account_info["blocked_reason"] = reason;
            if ((!update_account_info(account_info, 0, account_info["checksum"]))) {
                db_fails[new_arr_account[i]] = 1;
                new_arr_account[i] = 0;
            }
        }  // end loop (verified)
        clean_array(new_arr_account, 4);
        if ((!sizeof(btime))) {
            msg = sprintf("%s||OK:%s^NOT_EXIST:%s^BAD_MD5:%s^DB_FAIL:%s", "ÅúÁ¿½â·âÕÊºÅ²Ù×÷³É¹¦", implode(new_arr_account, ","), implode((arr_account - keys(info_list)), ","), implode(keys(invalids), ","), implode(keys(db_fails), ","));
        } else {
            msg = sprintf("%s||OK:%s^NOT_EXIST:%s^BAD_MD5:%s^DB_FAIL:%s", "ÅúÁ¿·â±ÕÕÊºÅ²Ù×÷³É¹¦", implode(new_arr_account, ","), implode((arr_account - keys(info_list)), ","), implode(keys(invalids), ","), implode(keys(db_fails), ","));
            if ((!ignore_data)) {
                try {
                    do_when_batch_block_account(new_arr_account, dist);
                } catch { /* error handled */ }

                // --- label L0424 ---
            }
        }
        ret = 1;
        "/daemons/threadd.c"->dispatch(random(10), (: call_other, "/aaa/daemons/vip_smsd.c", "do_when_batch_block_account", dist, new_arr_account, btime :));
    } while (0);

    // --- label L0445 ---
    prompt = msg;
    return ret;

}

// --------------------------------------------------------------------
// @code  nc=36, nargs=5, insts=36
// --------------------------------------------------------------------
mixed auth_sm_protect(string account, string ip, string dist, string auth_info, mixed &prompt) {
    /* ---- Enhanced Pseudocode ---- */
    mixed auth_result;
    string deny_prompt;

    deny_prompt = "Çë·¢ËÍ#Y8801#Rµ½#Y106653661280#R½â³ýËø¶¨£¬ÈçÄãÊÇÁªÍ¨ÓÃ»§£¬Çë·¢ËÍ#Y8801#Rµ½#Y106688881280#R½â³ýËø¶¨¡£½â³ýºó£¬µã»÷#RÈ·ÈÏ#n¼ÌÐø²Ù×÷¡£";
    auth_result = deny_by_pwd_protect(account, "sm", &deny_prompt, ip, dist);
    switch (auth_result) {
        case 0:
        case 1:
        prompt = deny_prompt;
        return -1;
        case 2:
        return 1;
        default:
        return 0;
    }  // end switch

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=136, nargs=6, insts=136
// --------------------------------------------------------------------
mixed auth_passpod_protect(string account, string ip, string dist, string auth_info, mixed &prompt, mapping &extra_info) {
    /* ---- Enhanced Pseudocode ---- */
    mixed auth_result;
    mixed deny_prompt;
    mapping auth_ret;

    auth_result = deny_by_pwd_protect(account, "passpod", &deny_prompt, ip, dist);
    switch (auth_result) {
        case 0:
        case 1:
        prompt = deny_prompt;
        return -1;
        case 2:
        return 1;
        default:
        if ((sizeof(auth_info) == 0)) {
            return -2;
        }
        auth_ret = "/aaa/daemons/passpodd.c"->auth_passpod(account, auth_info);
        switch (auth_ret["result"]) {
            case 0:
            return 0;
            case 1:
            prompt = "ÄãÊäÈëµÄÇ¬À¤ËøÃÜÂë´íÎó¡£";
            extra_info["inc_fail"] = 1;
            return -1;
            case 3:
            prompt = "ÄãµÄÇ¬À¤ËøÒÑ¾­¹ýÆÚ£¬Çë¼°Ê±¸ü»»¡£";
            return 0;
            case 5:
            prompt = "ÄãµÄÃÜ±£ÒÑ±»·âÍ££¬²Ù×÷Ê§°Ü¡£";
            return -1;
            case 6:
            prompt = "ÄãµÄÇ¬À¤Ëøµ±´ÎÃÜÂëÒÑÊ§Ð§£¬ÇëÉÔºóÔÙÊÔ¡£";
            return -1;
            case 7:
            prompt = "µ±Ç°Ç¬À¤ËøÊäÈë´ÎÊý¹ý¶à£¬ÇëÉÔºóÔÙÊÔ¡£";
            return -1;
            case 2:
            switch (when_return_user_not_found) {
                case 2:
                catch(error("User not found for passpod(account=%s)\n", account));

                // --- label L0112 ---
                break;
                case 3:
                cancel_pwd_protect(account, "passpod", ip, dist);
                break;
                default:
                break;
            }  // end switch

            // --- label L0125 ---
            case 10:
            default:
            prompt = "#RÇ¬À¤Ëø#nÃÜÂëÑéÖ¤Á¬½ÓÊ§°Ü¡££¨´íÎó±àºÅ£º1016£©\n#R×¢£ºÈç¹ûÄúÒÑ½â³ýÇ¬À¤Ëø£¬Ö»ÐèÒªÔÚÇ¬À¤ËøÀ¸ÊäÈëÈÎÒâ×Ö·û¼´¿ÉÆÆ½âÑéÖ¤¡£#n";
            return -1;
        }  // end switch
    }  // end switch

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=124, nargs=2, insts=124
// --------------------------------------------------------------------
private mixed get_value_from_matrix_text(string matrix_text, string pos) {
    /* ---- Enhanced Pseudocode ---- */
    int pos_x;
    int pos_y;
    mixed index;
    mixed token_size;
    mixed arr_matrix;

    if ((strlen(pos) != 2)) {
        return "";
    }
    pos_x = ((pos[0] - 65) + 1);
    pos_y = ((pos[1] - 49) + 1);
    if (((pos_x < 1) || (pos_y < 1))) {
        return "";
    }
    index = (((pos_x - 1) * sizeof(vector_y)) + pos_y);
    arr_matrix = explode(matrix_text, ",");
    token_size = sizeof(arr_matrix);
    if ((1 == token_size)) {
        if ((strlen(matrix_text) != ((sizeof(vector_x) * sizeof(vector_y)) * 2))) {
            return "";
        }
        return matrix_text[((index * 2) - 2)..((index * 2) - 1)];
    }
    if ((((sizeof(vector_x) * sizeof(vector_y)) > token_size) || (((sizeof(vector_x) * sizeof(vector_y)) + 1) < token_size))) {
        return "";
    }
    return arr_matrix[(index - 1)];

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=153, nargs=2, insts=153
// --------------------------------------------------------------------
private void _client_auth_pic(mixed agent, string client_data) {
    /* ---- Enhanced Pseudocode ---- */
    int login_result;
    int client_no;
    mixed encrypt_key;
    string seed_A;
    string seed_B;
    mapping seed_map;

    if ((!is_object(agent))) {
        return;
    }
    client_no = agent->get_client_no();
    if ((agent->lite_query_temp("pic_auth_result") != client_data)) {
        login_result = 0;
    } else {
        login_result = 1;
    }
    agent->lite_delete_temp("pic_auth_result");
    encrypt_key = (login_result ? generate_encrypt_cookie() : "");
    if (is_apex_enabled()) {
        seed_A = "abcdefghijklmnop";
        seed_B = "zyxwvutsrqponmlkjihgfedcbaABCDEFGHIJKLMNOPQ";
        if (((!is_mapping(seed_map = get_seed(seed_A, seed_B))) || (sizeof(seed_map) < 1))) {
            return;
        }
        seed_A = seed_map["seed_A"];
        seed_B = seed_map["seed_B"];
        if (((((!is_string(seed_A)) || (strlen(seed_A) < 1)) || (!is_string(seed_B))) || (strlen(seed_B) < 1))) {
            return;
        }
        client_seedA[client_no] = seed_A;
        agent->send_message(11012, login_result, encrypt_key, seed_B);
    } else {
        agent->send_message(11012, login_result, encrypt_key, "");
    }
    if ((!login_result)) {
        agent->to_be_disconnect(2);
    } else {
        set_dynamic_key(agent, "login", encrypt_key);
        agent->lite_set_temp("antibot_answer", 1);
        agent->to_be_connect(60);
    }

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int get_guard_state() {
    /* ---- Enhanced Pseudocode ---- */
    return is_need_open_guard;

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=14, nargs=1, insts=14
// --------------------------------------------------------------------
int set_guard_state(int state) {
    /* ---- Enhanced Pseudocode ---- */
    mixed pre_state;

    pre_state = is_need_open_guard;
    is_need_open_guard = state;
    return pre_state;

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
void disable_safe_tips() {
    /* ---- Enhanced Pseudocode ---- */
    safe_tips_enbled = 0;

}

// --------------------------------------------------------------------
// @code  nc=78, nargs=0, insts=78
// --------------------------------------------------------------------
private void clean_protect_account() {
    /* ---- Enhanced Pseudocode ---- */
    mixed busy_time;
    mixed ti;
    int index;
    mapping m;

    if ((!busy_time = get_busy_time())) {
        return;
    }
    if ((sizeof(protect_account) > 500)) {
        protect_account = ([ ]);
        clean_protect_account_timer = 0;
        return;
    }
    ti = time();
    loop (index in keys(protect_account)) {
        if ((!is_mapping(m = protect_account[index]))) {
            continue;
        }
        if (((int)m["time"] < (ti - busy_time))) {
            map_delete(protect_account, index);
            continue;
        }
        if (((ti - (int)m["update_time"]) > busy_time)) {
            map_delete(protect_account, index);
        }
    }  // end loop (verified)
    clean_protect_account_timer = 0;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
mixed get_account_ob_by_auth_key(int auth_key) {
    /* ---- Enhanced Pseudocode ---- */
    return auth_key_account[auth_key];

}

// --------------------------------------------------------------------
// @code  nc=80, nargs=0, insts=80
// --------------------------------------------------------------------
private void clean_protect_account_ex() {
    /* ---- Enhanced Pseudocode ---- */
    mixed ti;
    int index;
    mapping m;

    if ((sizeof(protect_account_ex) > 500)) {
        protect_account_ex = ([ ]);
        clean_protect_account_timer_ex = 0;
        return;
    }
    ti = time();
    loop (index in keys(protect_account_ex)) {
        if ((!is_mapping(m = protect_account_ex[index]))) {
            continue;
        }
        if (((ti - (int)m["first_time"]) > 600)) {
            map_delete(protect_account, index);
        }
        if ((((int)m["update_time"] < (ti - 600)) && ((int)m["update_time"] >= 5))) {
            map_delete(protect_account_ex, index);
            continue;
        }
    }  // end loop (verified)
    clean_protect_account_timer_ex = 0;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=20, nargs=0, insts=20
// --------------------------------------------------------------------
private int generate_auth_key() {
    /* ---- Enhanced Pseudocode ---- */
    ++cur_auth_key;
    return ((cur_auth_key & 16777215) + ("/aaa/daemons/configd.c"->get_my_info()["id"] << 24));

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=38, nargs=1, insts=38
// --------------------------------------------------------------------
mixed query_password_protect_info(string account) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed arr;
    mixed sql_cmd;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 0)) {
        return 0;
    }
    sql_cmd = mysql_bind_parameters("select * from pwd_protect where account = ?", account);
    arr = mysql_select(cno, sql_cmd, 100);
    if ((!is_array(arr))) {
        return 0;
    }
    return arr;

}

// --------------------------------------------------------------------
// @brief  validator: verify data
// @code  nc=79, nargs=3, insts=79
// --------------------------------------------------------------------
private int check_for_protect_account(string account, string ip, mixed &ret) {
    /* ---- Enhanced Pseudocode ---- */
    mixed err_times;
    mixed busy_time;
    int delta;
    int key;
    mapping protect_info;

    key = get_protect_key(account, ip);
    if ((!err_times = get_error_times())) {
        return 0;
    }
    if ((!busy_time = get_busy_time())) {
        return 0;
    }
    if ((!protect_info = protect_account[key])) {
        return 0;
    }
    if (is_undefined(clean_protect_account_timer)) {
        set_timer(busy_time, &clean_protect_account_timer, (: clean_protect_account :));
    }
    if ((protect_info["bad_count"] < err_times)) {
        return 0;
    }
    delta = ((protect_info["time"] - time()) + busy_time);
    if ((delta < 0)) {
        map_delete(protect_account, key);
        return 0;
    }
    ret = sprintf("¸ÃÕÊºÅ±»Ëø¶¨£¬½«ÔÚ#R%d#nÃëºó½â³ýËø¶¨¡£", delta);
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=20, nargs=4, insts=20
// --------------------------------------------------------------------
private void _process_command(mixed func, mixed agent, string cmd, array arr) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!is_function(func))) {
        return;
    }
    if ((!is_object(agent))) {
        return;
    }
    (*func)(agent, cmd, arr);

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int is_apex_enabled() {
    /* ---- Enhanced Pseudocode ---- */
    return apex_enabled;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=184, nargs=9, insts=184
// --------------------------------------------------------------------
private void _account_login(mixed agent, string account, string password, string mac_addr, string client_data, string passpod, string dist, string qr_id, int auth_verfy_code) {
    /* ---- Enhanced Pseudocode ---- */
    int qr_code_login;
    mixed account_info;
    mixed arr_pos;
    mixed le_buf;
    mapping le_m;
    mapping le_info;

    if ("/aaa/daemons/qr_coded.c"->check_qr_code_login(agent, qr_id, account)) {
        qr_code_login = 1;
    } else {
        qr_code_login = 0;
    }
    account_info = check_account_info(agent, account, password, mac_addr, client_data, passpod, dist, qr_code_login);
    if ((!is_mapping(account_info))) {
        return;
    }
    agent->lite_set_temp("ok", 1);
    agent->lite_set_temp("mac_addr", mac_addr);
    agent->lite_set_temp("para_passpod", passpod);
    agent->lite_set_temp("para_dist", dist);
    do {
        if (qr_code_login) {
            break;
        }
        if ((!local_encrypt_enabled)) {
            break;
        }
        if (is_under_control(account_info)) {
            break;
        }
        if ((!is_mapping(le_m = query_local_encrypt_info(account)))) {
            break;
        }
        if ((!(int)le_m["state"])) {
            break;
        }
        le_info = restore_value(le_m["local_encrypt_info"]);
        if (((!is_mapping(le_info)) || (le_m["checksum"] != calc_local_encrypt_checksum(le_m, le_m["local_encrypt_info"])))) {
            send_login_result(agent, 0, 0, "±¾µØ¼ÓÃÜÏµÍ³ÑéÖ¤Ê§°Ü¡£");
            agent->to_be_disconnect(1);
            if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
                "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "Êý¾Ý¿âÖÐ´óÎÄ¼þÐÅÏ¢´íÎó\n");
            }
            return;
        }
        arr_pos = get_random_pos(le_info["file_length"]);
        le_buf = build_local_encrypt_pos_buf(arr_pos);
        agent->send_message(5025, (le_buf + pad_buffer(le_m["file_name"])));
        agent->lite_set_temp("local_encrypt_pos", arr_pos);
        agent->to_be_disconnect(timeout_for_protect_auth);
        return;
    } while (0);

    // --- label L0176 ---
    auth_all_protect(agent, account, account_info, qr_code_login);

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=25, nargs=1, insts=25
// --------------------------------------------------------------------
mixed query_account_info(string account) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;

    if (((cno = "/daemons/dbd.c"->assure_connection("adb")) == -1)) {
        return 0;
    }
    sql_cmd = mysql_bind_parameters("select * from account where account = ?", account);
    return mysql_select_single_result(cno, sql_cmd);

}

// --------------------------------------------------------------------
// @brief  validator: verify data
// @code  nc=11, nargs=2, insts=11
// --------------------------------------------------------------------
void check_client_data(mixed agent, string client_data) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_service((: _check_client_data, agent, client_data :));

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
void enable_local_encrypt() {
    /* ---- Enhanced Pseudocode ---- */
    local_encrypt_enabled = 1;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=142, nargs=6, insts=142
// --------------------------------------------------------------------
void add_coins(mapping account_info, string dist, string server, string account, int add_silver_coin, int add_gold_coin) {
    /* ---- Enhanced Pseudocode ---- */
    int cur_silver_coin;
    int cur_gold_coin;
    mixed silver_coin_left;
    mixed gold_coin_left;
    mixed task_start_time;
    mixed task_end_time;

    task_start_time = time_desc_to_int("2009-01-07-16:00");
    task_end_time = time_desc_to_int("2009-01-14-09:00");
    if (((time() < task_start_time) || (time() > task_end_time))) {
        return;
    }
    if ((add_silver_coin < 0)) {
        add_silver_coin = 0;
    }
    if ((add_gold_coin < 0)) {
        add_gold_coin = 0;
    }
    if ((!is_mapping(account_info))) {
        return;
    }
    if ((account_info["checksum"] != "/aaa/daemons/accountd.c"->calc_account_checksum(account_info))) {
        return;
    }
    if ((account_info["memo"] == "1")) {
        return;
    }
    silver_coin_left = atoi(account_info["silver_coin"]);
    if (((2147483647 - add_silver_coin) >= silver_coin_left)) {
        cur_silver_coin = (silver_coin_left + add_silver_coin);
    } else {
        cur_silver_coin = 2147483647;
    }
    gold_coin_left = atoi(account_info["gold_coin"]);
    if (((2147483647 - add_gold_coin) >= gold_coin_left)) {
        cur_gold_coin = (gold_coin_left + add_gold_coin);
    } else {
        cur_gold_coin = 2147483647;
    }
    account_info["silver_coin"] = (string)cur_silver_coin;
    account_info["gold_coin"] = (string)cur_gold_coin;
    account_info["memo"] = "1";
    account_info["checksum"] = "/aaa/daemons/accountd.c"->calc_account_checksum(account_info);
    update_account_info(account_info);

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=57, nargs=2, insts=57
// --------------------------------------------------------------------
mixed query_protect_info(string account, string protect_type) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed f;
    mixed sql_cmd;
    mixed m;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 0)) {
        return 0;
    }
    sql_cmd = mysql_bind_parameters("select * from pwd_protect where account = ? and protect_type = ?", account, protect_type);
    m = mysql_select_single_result(cno, sql_cmd);
    if ((!is_mapping(m))) {
        return 0;
    }
    f = query(("pwd_protect_fetch_func/" + protect_type));
    if ((is_function(f) && (!(*f)(account, &m)))) {
        return 0;
    }
    return m;

}

// --------------------------------------------------------------------
// @code  nc=52, nargs=4, insts=52
// --------------------------------------------------------------------
private int deny_by_pwd_protect_ex(string account, string protect_type, mixed &prompt, mixed param) {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;
    mixed f;

    m = query_safe_ctrl(account, protect_type);
    if ((!is_mapping(m))) {
        return 2;
    }
    if ((calc_protect_info_checksum(m) != toupper(m["checksum"]))) {
        prompt = "ÑéÖ¤·þÎñÆ÷Òì³££¬µÇÂ¼Ê§°Ü¡££¨Ô­Òò£º1015£©";
        return 0;
    }
    f = query(("pwd_protect_deny_func/" + protect_type));
    if ((is_function(f) && (*f)(account, m, param))) {
        return 1;
    }
    return 3;

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=46, nargs=1, insts=46
// --------------------------------------------------------------------
int query_protect_range(string account) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed arr;
    mixed sql_cmd;

    if (((cno = "/daemons/dbd.c"->assure_connection("adb")) < 0)) {
        return -1;
    }
    sql_cmd = mysql_bind_parameters("select p_range from protect_range where account=? limit 1", account);
    arr = mysql_select(cno, sql_cmd);
    if ((!is_array(arr))) {
        return -1;
    }
    if ((!sizeof(arr))) {
        return 2;
    }
    return atoi(arr[0]["p_range"]);

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
void disable_matrix_pic() {
    /* ---- Enhanced Pseudocode ---- */
    matrix_pic_enabled = 0;

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=127, nargs=2, insts=127
// --------------------------------------------------------------------
mixed query_phone_auth_info(string account, int format) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed arr;
    mixed sql_cmd;
    mapping m;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 0)) {
        return -1;
    }
    sql_cmd = mysql_bind_parameters("select * from phone_auth where account = ?", account);
    arr = mysql_select(cno, sql_cmd, 1);
    if ((!is_array(arr))) {
        return -1;
    }
    if ((sizeof(arr) < 1)) {
        return ([ ]);
    }
    m = arr[0];
    if (format) {
        map_delete(m, "account");
        map_delete(m, "update_time");
        m["bind_time"] = time(m["bind_time"]);
        m["state"] = (int)m["state"];
        if ((is_string(m["bonus_info"]) && sizeof(m["bonus_info"]))) {
            try {
                m["bonus_info"] = restore_value(m["bonus_info"]);
            } catch { /* error handled */ }

            // --- label L0102 ---
            if (is_mapping(m["bonus_info"])) {
                m["bonus_info"]["time"] = time(m["bonus_info"]["time"]);
            }
        }
    }
    return m;

}

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=100, nargs=3, insts=100
// --------------------------------------------------------------------
private void do_when_password_error(mixed agent, string account, mixed &ret) {
    /* ---- Enhanced Pseudocode ---- */
    mixed err_times;
    mixed busy_time;
    mixed ti;
    int key;

    err_times = get_error_times();
    if ((!err_times)) {
        return;
    }
    busy_time = get_busy_time();
    if ((!busy_time)) {
        return;
    }
    if ((!is_object(agent))) {
        return;
    }
    key = get_protect_key(account, agent->get_peer_ip());
    ti = time();
    if ((!is_mapping(protect_account[key]))) {
        protect_account[key] = ([ "bad_count": 1, "update_time": ti ]);
    } else {
        ++protect_account[key]["bad_count"];
        protect_account[key]["update_time"] = ti;
    }
    if ((protect_account[key]["bad_count"] >= err_times)) {
        protect_account[key]["time"] = ti;
        ret = sprintf("#RÄúÒÑ¾­Á¬Ðø%d´ÎÊäÈëÃÜÂë´íÎó£¬ÎªÁË±£»¤ÕÊºÅµÄ°²È«£¬%dÃëÖ®ÄÚ¸ÃÕÊºÅÎÞ·¨µÇÂ¼¡£#n", err_times, busy_time);
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=9, nargs=0, insts=9
// --------------------------------------------------------------------
private void timer_clean_client_seedA() {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_service((: _timer_clean_client_seedA :));

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=47, nargs=1, insts=47
// --------------------------------------------------------------------
mixed query_alipay_info(string account) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed arr;
    mixed sql_cmd;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 0)) {
        return -1;
    }
    sql_cmd = mysql_bind_parameters("select * from alipay_account where account = ?", account);
    arr = mysql_select(cno, sql_cmd, 1);
    if ((!is_array(arr))) {
        return -1;
    }
    if ((sizeof(arr) < 1)) {
        return ([ ]);
    }
    return arr[0];

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=33, nargs=3, insts=33
// --------------------------------------------------------------------
override void process_command(object agent, string cmd, array args) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!agent->lite_query_temp("ok"))) {
        agent->send_message(8165, "Á¬½Ó·þÎñÆ÷´íÎó£¬ÎÞ·¨µÇÂ¼¡££¨Ô­Òò£º1008£©", 1);
        agent->to_be_disconnect(1);
        return;
    }
    "/daemons/threadd.c"->dispatch_service((: _process_command, (: ::process_command :), agent, cmd, args :));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=248, nargs=4, insts=248
// --------------------------------------------------------------------
private void cancel_pwd_protect(string account, string protect_type, mixed param, mixed dist) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed bind_time;
    mixed start_time_last_month;
    mixed start_time_this_month;
    mixed sql_cmd;
    mixed ip;
    mixed state;
    mixed time_desc;
    mixed year;
    int month;
    mixed branch;
    mapping protect_info;
    mapping lt_m;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 0)) {
        return;
    }
    protect_info = query_protect_info(account, protect_type);
    if ((!is_mapping(protect_info))) {
        return;
    }
    if (is_object(param)) {
        ip = param->get_peer_ip();
    } else if (is_string(param)) {
        ip = param;
    }
    state = protect_index[protect_type];
    if ((!strlen(bind_time = protect_info["bind_time"]))) {
        bind_time = ctime(time(), 3);
    } else {
        start_time_this_month = get_month_zero_time(time());
        start_time_last_month = get_month_zero_time((start_time_this_month - 10));
        if ((time(bind_time) > start_time_this_month)) {
            time_desc = ((bind_time + "-") + ctime(time(), 3));
            branch = ctime(time(), 2)[0..6];
            log_protect_time(account, branch, protect_type, time_desc, cno);
        } else {
            time_desc = ((ctime(start_time_this_month, 3) + "-") + ctime(time(), 3));
            branch = ctime(time(), 2)[0..6];
            log_protect_time(account, branch, protect_type, time_desc, cno);
            time_desc = ((time(bind_time) >= start_time_last_month) ? ((bind_time + "-") + ctime(start_time_this_month, 3)) : ((ctime(start_time_last_month, 3) + "-") + ctime(start_time_this_month, 3)));
            lt_m = local_time();
            year = lt_m["year"];
            if ((!month = (lt_m["mon"] - 1))) {
                month = 12;
                --year;
            }
            branch = sprintf("%d-%02d", year, month);
            log_protect_time(account, branch, protect_type, time_desc, cno);
        }
    }
    sql_cmd = mysql_bind_parameters("insert into acc_log(ip, time,account, dist, type, msg, memo) values(?, ?, ?, ?, ?, ?, ?)", (string)ip, ctime(time(), 3)[0..13], account, (string)dist, "pwd_protect", state, bind_time);
    (*f_execute)(cno, sql_cmd);
    sql_cmd = mysql_bind_parameters("delete from pwd_protect where account=? and protect_type=?", account, protect_type);
    (*f_execute)(cno, sql_cmd);

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=136, nargs=0, insts=136
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    mixed active_config;
    mixed active_status;

    annouce_eternal_data("protect_account", &protect_account);
    annouce_eternal_data("cur_auth_key", &cur_auth_key);
    annouce_eternal_data("auth_key_account", &auth_key_account);
    set_case_check(protect_account, 0);
    load_all_handlers("/aaa/daemons/account/");
    "/daemons/connectiond.c"->register_auth("cmd_l_account", (: account_login :));
    "/daemons/connectiond.c"->register_auth("cmd_l_get_antibot_question", (: send_antibot_question :));
    "/daemons/connectiond.c"->register_auth("cmd_l_check_user_data", (: check_client_data :));
    "/daemons/connectiond.c"->register_auth("cmd_l_auth_pic", (: client_auth_pic :));
    "/daemons/connectiond.c"->register_ban((: check_for_ban :));
    active_config = atoi(read_ini("Account", "Disactive", INI_FILE));
    active_status = (active_config ? 0 : 1);
    set_active_account_status(active_status);
    init_matrix();
    set_matrix_status(1);
    set("pwd_protect_fetch_func/matrix", (: pwd_protect_fetch_matrix :));
    set("pwd_protect_fetch_func/phone", (: pwd_protect_fetch_phone :));
    set("pwd_protect_fetch_func/sm", (: pwd_protect_fetch_sm :));
    set("pwd_protect_fetch_func/passpod", (: pwd_protect_fetch_passpod :));
    set("pwd_protect_update_func/matrix", (: pwd_protect_update_matrix :));
    set("pwd_protect_update_func/phone", (: pwd_protect_update_phone :));
    set("pwd_protect_update_func/sm", (: pwd_protect_update_sm :));
    set("pwd_protect_update_func/passpod", (: pwd_protect_update_passpod :));
    set("pwd_protect_deny_func/phone", (: pwd_protect_deny_phone :));
    set("pwd_protect_deny_func/sm", (: pwd_protect_deny_sm :));
    set("pwd_protect_deny_func/passpod", (: pwd_protect_deny_passpod :));
    set("pwd_protect_deny_func/fixed_ip", (: pwd_protect_deny_fixed_ip :));
    set("pwd_protect_deny_func/fixed_tm", (: pwd_protect_deny_fixed_tm :));
    set("pwd_protect_deny_func/fixed_city", (: pwd_protect_deny_fixed_city :));
    if (apex_enabled) {
        timer_clean_client_seedA();
    }

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
void set_fixed_city_status(int _status) {
    /* ---- Enhanced Pseudocode ---- */
    fixed_city_status = _status;

}

// --------------------------------------------------------------------
// @code  nc=33, nargs=1, insts=33
// --------------------------------------------------------------------
void encode_matrix_info(mapping &m) {
    /* ---- Enhanced Pseudocode ---- */
    mixed matrix;
    mixed new_matrix;

    matrix = m["matrix"];
    new_matrix = std_des(matrix, 0, "20070622", "", 1, 1, 1, "XiaMenGB");
    m["matrix"] = save_string(new_matrix)[1..<2];

}

// --------------------------------------------------------------------
// @code  nc=20, nargs=2, insts=20
// --------------------------------------------------------------------
private int pwd_protect_update_passpod(string account, mapping &m) {
    /* ---- Enhanced Pseudocode ---- */
    mixed protect_info;

    protect_info = sprintf("%s", m["expired_lock_time"]);
    m["protect_info"] = protect_info;
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=27, nargs=1, insts=27
// --------------------------------------------------------------------
mixed calc_protect_info_checksum(mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    mixed text;

    text = sprintf("%s%s%s%s", m["account"], m["protect_type"], m["protect_info"], "20070620");
    return save_string(md5(text))[1..<2];

}

// --------------------------------------------------------------------
// @code  nc=11, nargs=1, insts=11
// --------------------------------------------------------------------
private string generate_auth_matrix_cookie(string account) {
    /* ---- Enhanced Pseudocode ---- */
    return sprintf("%d%s%d", random(2147483647), account, time());

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=113, nargs=1, insts=113
// --------------------------------------------------------------------
buffer get_matrix_pic(array matrix) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    int j;
    int k;
    int pic_count;
    mixed cur_matrix;
    mixed pic_buf;
    mixed buf;
    mixed t_buf;

    pic_count = 0;
    cur_matrix = ({});
    pic_buf = allocate_buffer(0);
    buf = allocate_buffer(0);
    t_buf = allocate_buffer(0);
    for (i = 0; i <= (sizeof(matrix) - 1); i++) {
        for (k = 0; k <= (sizeof(matrix[i]) - 1); k++) {
            cur_matrix += ({ sprintf("%c", matrix[i][k]) });
        }  // end loop (verified)
    }  // end loop (verified)
    for (j = 0; j <= (sizeof(cur_matrix) - 1); j++) {
        ++pic_count;
        t_buf = "/aaa/daemons/picd.c"->generate_pic(cur_matrix[j], 1, 32, 32);
        pic_buf += pad_buffer(sizeof(t_buf), t_buf);
    }  // end loop (verified)
    buf += (pad_buffer(pic_count) + pic_buf);
    return buf;

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=57, nargs=2, insts=57
// --------------------------------------------------------------------
mixed query_safe_ctrl(string account, string protect_type) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed sql_cmd;
    mixed m;
    mixed f;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 0)) {
        return 0;
    }
    sql_cmd = mysql_bind_parameters("select * from safe_ctrl where account = ? and protect_type = ?", account, protect_type);
    m = mysql_select_single_result(cno, sql_cmd);
    if ((!is_mapping(m))) {
        return 0;
    }
    f = query(("pwd_protect_fetch_func/" + protect_type));
    if ((is_function(f) && (!(*f)(account, &m)))) {
        return 0;
    }
    return m;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=1206, nargs=4, insts=1206
// --------------------------------------------------------------------
private void auth_all_protect(mixed agent, string account, mapping account_info, int qr_code_login) {
    /* ---- Enhanced Pseudocode ---- */
    int auth_key;
    int matrix_status;
    int is_advanced_protect_method;
    int protect_range;
    int login_auth;
    mixed matrix_random;
    string prompt;
    mixed matrix_cookie;
    mixed auth_text;
    mixed permit_ip;
    mixed permit_id;
    mixed mac_addr;
    mapping matrix_info;
    string deny_prompt;
    mapping auth_ret;
    mixed dist;
    mixed passpod;
    int spec_protect;
    int privilege;
    mapping protect_phone_info;
    mixed protect_phone_content;
    int password_protect_var;
    string auth_pic_buf;

    is_advanced_protect_method = 0;
    prompt = "";
    if ((!is_object(agent))) {
        return;
    }
    if ((!get_safe_tips_status())) {
        agent->lite_set("safe_status", 1);
    }
    if ((stricmp(agent->query_account(), account) != 0)) {
        agent->to_be_disconnect(1);
        return;
    }
    mac_addr = agent->lite_query_temp("mac_addr");
    login_auth = 1;
    protect_range = query_protect_range(account);
    if ((protect_range == 1)) {
        login_auth = 0;
    }
    spec_protect = 0;
    if ((!is_object(agent))) {
        return;
    }
    dist = agent->lite_query_temp("para_dist");
    if ((((((!qr_code_login) && is_int(privilege = atoi(account_info["privilege"]))) && (privilege >= 300)) && (member_array(dist, ignore_dist_list) < 0)) && ((!login_auth) || (!is_mapping(query_protect_info(account, "passpod")))))) {
        send_login_result(agent, 0, 0, "GMÕÊºÅÎ´°ó¶¨Ç¬À¤Ëø»òÎ´ÉèÖÃµÇÂ¼Ê±ÑéÖ¤Ç¬À¤Ëø£¬ÎÞ·¨µÇÂ¼¡£");
        agent->to_be_disconnect(2);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "GMÕÊºÅÎ´°ó¶¨Ç¬À¤Ëø»òÎ´ÉèÖÃµÇÂ¼Ê±ÑéÖ¤Ç¬À¤Ëø£¬ÎÞ·¨µÇÂ¼¡£");
        }
        return;
    }
    spec_protect = get_auth_spec_game_bits();
    if (((((((!qr_code_login) && login_auth) && (4 & (~spec_protect))) && is_mapping(query_protect_info(account, "passpod"))) && (account_info["org_password"] == "")) && (!check_skip_protected(agent, mac_addr, account_info)))) {
        if ((deny_by_pwd_protect(account, "passpod", &deny_prompt) < 2)) {
            send_login_result(agent, 6, 0, "¸ÃÕÊºÅ°ó¶¨µÄ#RÇ¬À¤Ëø#nÒÑ¾­¹ÒÊ§£¬ÎÞ·¨µÇÂ¼ÓÎÏ·¡£");
            agent->to_be_disconnect(2);
            return;
        }
        passpod = agent->lite_query_temp("para_passpod");
        auth_ret = "/aaa/daemons/passpodd.c"->auth_passpod(account, passpod);
        if ((!is_object(agent))) {
            return;
        }
        switch (auth_ret["result"]) {
            case 0:
            break;
            case 1:
            prompt = "ÄãÊäÈëµÄÇ¬À¤ËøÃÜÂë´íÎó¡£";
            do_when_password_error(agent, account, &prompt);
            send_login_result(agent, 6, 0, prompt);
            agent->to_be_disconnect(2);
            return;
            case 3:
            agent->send_message(8165, "ÄãµÄÇ¬À¤ËøÒÑ¾­¹ýÆÚ£¬Çë¼°Ê±¸ü»»¡£", 1);
            break;
            case 2:
            switch (when_return_user_not_found) {
                case 2:
                catch(error("User not found for passpod(account=%s)\n", account));

                // --- label L0281 ---
                break;
                case 3:
                cancel_pwd_protect(account, "passpod", agent, dist);
                break;
                default:
                break;
            }  // end switch

            // --- label L0294 ---
            break;
            case 5:
            send_login_result(agent, 6, 0, "ÄãµÄÃÜ±£ÒÑ±»·âÍ££¬²Ù×÷Ê§°Ü¡£");
            agent->to_be_disconnect(2);
            return;
            case 6:
            send_login_result(agent, 6, 0, "ÄãµÄÇ¬À¤Ëøµ±´ÎÃÜÂëÒÑÊ§Ð§£¬ÇëÉÔºóÔÙÊÔ¡£");
            agent->to_be_disconnect(2);
            return;
            case 7:
            send_login_result(agent, 6, 0, "µ±Ç°Ç¬À¤ËøÊäÈë´ÎÊý¹ý¶à£¬ÇëÉÔºóÔÙÊÔ¡£");
            agent->to_be_disconnect(2);
            return;
            case 10:
            default:
            send_login_result(agent, 6, 0, "#RÇ¬À¤Ëø#nÃÜÂëÑéÖ¤Á¬½ÓÊ§°Ü¡££¨´íÎó±àºÅ£º1016£©\n#R×¢£ºÈç¹ûÄúÒÑ½â³ýÇ¬À¤Ëø£¬Ö»ÐèÒªÔÚÇ¬À¤ËøÀ¸ÊäÈëÈÎÒâ×Ö·û¼´¿ÉÆÆ½âÑéÖ¤¡£#n");
            agent->to_be_disconnect(2);
            return;
        }  // end switch

        // --- label L0349 ---
        if ((!is_object(agent))) {
            return;
        }
        agent->lite_set("passpod_type", auth_ret["passpod_type"]);
        is_advanced_protect_method = 1;
    }
    permit_ip = account_info["permit_ip"];
    if (is_apex_enabled()) {
        permit_ip = trim((string)permit_ip);
    }
    if (((permit_ip != "") && (!check_for_permit(permit_ip, agent->get_peer_ip(), ",;/")))) {
        send_login_result(agent, 0, 0, "¶Ô²»Æð£¬ÓÉÓÚÄúÉêÇëÁËIP°ó¶¨·þÎñ£¬µ±Ç°µÇÂ¼ÊÜµ½ÏÞÖÆ£¡");
        agent->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "µÇÂ¼IPºÍÉèÖÃµÄ°ó¶¨IP²»Ò»ÖÂ£¬µÇÂ¼Ê§°Ü\n");
        }
        return;
    }
    permit_id = account_info["permit_id"];
    if (is_apex_enabled()) {
        permit_id = trim((string)permit_id);
    }
    if (((permit_id != "") && (!check_for_permit(permit_id, mac_addr, ",;/")))) {
        send_login_result(agent, 0, 0, "¶Ô²»Æð£¬ÓÉÓÚÄúÉêÇëÁË»úÆ÷ID°ó¶¨·þÎñ£¬µ±Ç°µÇÂ¼ÊÜµ½ÏÞÖÆ£¡");
        agent->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "µÇÂ¼»úÆ÷MACµØÖ·ºÍ°ó¶¨MACµØÖ·²»Ò»ÖÂ£¬µÇÂ¼Ê§°Ü\n");
        }
        return;
    }
    password_protect_var = 0;
    if ((((!qr_code_login) && login_auth) && (2 & (~spec_protect)))) {
        do {
            protect_phone_content = "/aaa/daemons/configd.c"->get_global_config("protect_phone");
            if (((!is_string(protect_phone_content)) || (!sizeof(protect_phone_content)))) {
                deny_prompt = "#RÄãµÄÕÊºÅÒÑ¾­°ó¶¨ÁËµç»°ÃÜ±££¬Ðè½â³ýËø¶¨ºó²ÅÄÜµÇÂ¼ÓÎÏ·£¬Çë²¦´òµç»°#n#Y400-811-0495¡¢400-811-0507#n#RÈçµç»°Ã¦Çë³¢ÊÔ²¦´ò#n#Y0531-6677-3523#n#R#R½â³ýËø¶¨£¬Ò²¿É±à¼­¶ÌÐÅ#n#Y¡°1¡±#n#R·¢ËÍµ½#n#Y18954164801#n#R½âËø¡£#n";
                break;
            }
            try {
                protect_phone_info = restore_value(protect_phone_content);
            } catch { /* error handled */ }

            // --- label L0529 ---
            if (((!is_mapping(protect_phone_info)) || (!sizeof(protect_phone_info)))) {
                deny_prompt = "#RÄãµÄÕÊºÅÒÑ¾­°ó¶¨ÁËµç»°ÃÜ±££¬Ðè½â³ýËø¶¨ºó²ÅÄÜµÇÂ¼ÓÎÏ·£¬Çë²¦´òµç»°#n#Y400-811-0495¡¢400-811-0507#n#RÈçµç»°Ã¦Çë³¢ÊÔ²¦´ò#n#Y0531-6677-3523#n#R#R½â³ýËø¶¨£¬Ò²¿É±à¼­¶ÌÐÅ#n#Y¡°1¡±#n#R·¢ËÍµ½#n#Y18954164801#n#R½âËø¡£#n";
                break;
            }
            if (((!is_string(deny_prompt = protect_phone_info["tip"])) || (!sizeof(deny_prompt)))) {
                deny_prompt = "#RÄãµÄÕÊºÅÒÑ¾­°ó¶¨ÁËµç»°ÃÜ±££¬Ðè½â³ýËø¶¨ºó²ÅÄÜµÇÂ¼ÓÎÏ·£¬Çë²¦´òµç»°#n#Y400-811-0495¡¢400-811-0507#n#RÈçµç»°Ã¦Çë³¢ÊÔ²¦´ò#n#Y0531-6677-3523#n#R#R½â³ýËø¶¨£¬Ò²¿É±à¼­¶ÌÐÅ#n#Y¡°1¡±#n#R·¢ËÍµ½#n#Y18954164801#n#R½âËø¡£#n";
                break;
            }
        } while (0);

        // --- label L0567 ---
        password_protect_var = deny_by_pwd_protect(account, "phone", &deny_prompt, agent, dist);
        if ((((password_protect_var < 2) && (account_info["org_password"] == "")) && (!check_skip_protected(agent, mac_addr, account_info)))) {
            if (is_object(agent)) {
                agent->send_message(8165, deny_prompt, 1);
                agent->to_be_disconnect(5);
            }
            if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
                "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "ÕÊºÅ°ó¶¨µç»°ÃÜ±££¬½ûÖ¹µÇÂ¼¡£");
            }
            if ((!is_mapping(protect_account_ex[account]))) {
                protect_account_ex[account] = ([ ]);
            }
            if (((time() - (int)protect_account_ex[account]["first_time"]) > 600)) {
                protect_account_ex[account]["first_time"] = time();
                protect_account_ex[account]["times"] = 1;
                protect_account_ex[account]["update_time"] = time();
            } else {
                ++protect_account_ex[account]["times"];
                protect_account_ex[account]["update_time"] = time();
            }
            return;
        }
        if ((password_protect_var == 3)) {
            is_advanced_protect_method = 1;
        }
    }
    if ((((!qr_code_login) && login_auth) && (1 & (~spec_protect)))) {
        deny_prompt = "#RÄãµÄÕÊºÅÒÑ¾­°ó¶¨ÁË¶ÌÐÅÃÜ±££¬Ðè½â³ýËø¶¨ºó²ÅÄÜµÇÂ¼ÓÎÏ·£¬ÈçÄãÊÇÒÆ¶¯ÓÃ»§£¬Çë·¢ËÍ#Y8801#Rµ½#Y106653661280#R½â³ýËø¶¨£¬ÈçÄãÊÇÁªÍ¨ÓÃ»§£¬Çë·¢ËÍ#Y8801#Rµ½#Y106688881280#R½â³ýËø¶¨¡£";
        password_protect_var = deny_by_pwd_protect(account, "sm", &deny_prompt, agent, dist);
        if ((((password_protect_var < 2) && (account_info["org_password"] == "")) && (!check_skip_protected(agent, mac_addr, account_info)))) {
            if (is_object(agent)) {
                agent->send_message(8165, deny_prompt, 1);
                agent->to_be_disconnect(5);
            }
            if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
                "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "ÕÊºÅ°ó¶¨¶ÌÐÅÃÜ±££¬½ûÖ¹µÇÂ¼¡£");
            }
            if ((!is_mapping(protect_account_ex[account]))) {
                protect_account_ex[account] = ([ ]);
            }
            if (((time() - (int)protect_account_ex[account]["first_time"]) > 600)) {
                protect_account_ex[account]["first_time"] = time();
                protect_account_ex[account]["times"] = 1;
                protect_account_ex[account]["update_time"] = time();
            } else {
                ++protect_account_ex[account]["times"];
                protect_account_ex[account]["update_time"] = time();
            }
            return;
        }
        if ((password_protect_var == 3)) {
            is_advanced_protect_method = 1;
        }
    }
    if (((((qr_code_login || (!login_auth)) || (!get_matrix_status())) || is_under_control(account_info)) || check_skip_protected(agent, mac_addr, account_info))) {
        matrix_status = 2;
    } else {
        matrix_info = query_matrix_info(account);
        matrix_status = get_account_matrix_status(matrix_info);
    }
    if ((!is_object(agent))) {
        return;
    }
    if ((matrix_status == 1)) {
        send_login_result(agent, 0, 0, "ÕÊºÅµÇÂ¼Ê§°Ü¡££¨Ô­Òò£º1014£©");
        agent->to_be_disconnect(2);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "ÎÞ·¨È·ÈÏÊÇ·ñ°ó¶¨ÃÜ±£¿¨£¬µÇÂ¼Ê§°Ü\n");
        }
        return;
    }
    agent->replace_entire_dbase((account_info << agent->query_entire_dbase()));
    agent->lite_set("protect_range", protect_range);
    if ((matrix_status == 2)) {
        do {
            if (is_under_control(account_info)) {
                break;
            }
            if (is_advanced_protect_method) {
                agent->lite_set("safe_status", 1);
                break;
            }
            if (is_account_controlled_safe(agent, account, &prompt)) {
                if ((is_string(prompt) && sizeof(prompt))) {
                    send_login_result(agent, 0, 0, prompt);
                }
                agent->to_be_disconnect(2);
                if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
                    "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", prompt);
                }
                return;
            }
        } while (0);

        // --- label L1026 ---
        if ((!is_object(agent))) {
            return;
        }
        auth_key = generate_auth_key();
        agent->lite_set("auth_key", auth_key);
        send_login_result(agent, (aa_rule_login ? 7 : 1), auth_key, "");
        clean_mapping_by_value(auth_key_account, 10);
        auth_key_account[auth_key] = agent;
        do_when_password_correct(agent, account);
        agent->to_be_disconnect(timeout_for_choose_server);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", sprintf("µÇÂ¼³É¹¦£¬·¢ËÍ AUTH_KEY:%d\n", auth_key));
        }
        return;
    }
    agent->lite_set("safe_status", 1);
    matrix_cookie = generate_auth_matrix_cookie(account);
    matrix_random = generate_random_matrix(agent, 3, matrix_info["used_matrix"]);
    agent->lite_set_temp("matrix_cookie", matrix_cookie);
    agent->lite_set_temp("matrix_random", matrix_random);
    agent->lite_set_temp("matrix", 1);
    auth_pic_buf = "";
    if ((is_matrix_pic_enabled() && (!agent->lite_query_temp("mobile_client")))) {
        auth_text = encode_matrix_auth_text(matrix_cookie, "", "", "");
        auth_pic_buf = get_matrix_pic(matrix_random);
    } else {
        auth_text = encode_matrix_auth_text(matrix_cookie, matrix_random[0], matrix_random[1], matrix_random[2]);
    }
    agent->send_message(4987, auth_text, auth_pic_buf, 0);
    agent->to_be_disconnect(timeout_for_protect_auth);
    if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
        "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "È·ÈÏ°ó¶¨ÁËÃÜ±£¿¨£¬µÈ´ýÑéÖ¤ÃÜ±£ÃÜÂë\n");
    }

}

// --------------------------------------------------------------------
// @code  nc=33, nargs=2, insts=33
// --------------------------------------------------------------------
private int pwd_protect_update_sm(string account, mapping &m) {
    /* ---- Enhanced Pseudocode ---- */
    mixed protect_info;

    protect_info = sprintf("%s", m["permission_expired_time"]);
    if (is_string(m["login_ip"])) {
        protect_info += sprintf(",%s", m["login_ip"]);
    }
    m["protect_info"] = protect_info;
    return 1;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=20, nargs=0, insts=20
// --------------------------------------------------------------------
private int get_busy_time() {
    /* ---- Enhanced Pseudocode ---- */
    mixed busy_time;

    if ((!busy_time = "/aaa/daemons/configd.c"->get_my_info()["busy_time"])) {
        return 0;
    }
    return atoi(busy_time);

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=120, nargs=1, insts=120
// --------------------------------------------------------------------
mixed query_pwd_protect_err_checksum(mixed arg) {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr;
    mixed protect_list;
    int account;
    mapping m;
    mixed result;
    mapping protect_info;

    arr = ({});
    if (is_string(arg)) {
        arr = ({ arg });
    } else if (is_array(arg)) {
        arr += arg;
    } else {
        printf("Error paras, please input a string or an array.\n");
        return 0;
    }
    result = ([ ]);
    loop (account in arr) {
        protect_list = query_password_protect_info(account);
        if ((!is_array(protect_list))) {
            continue;
        }
        loop (m in protect_list) {
            if (((!is_mapping(m)) || (!sizeof(m)))) {
                continue;
            }
            protect_info = query_protect_info(account, m["protect_type"]);
            if (((!is_mapping(protect_info)) || (!sizeof(protect_info)))) {
                continue;
            }
            if ((protect_info["checksum"] == calc_protect_info_checksum(protect_info))) {
                continue;
            }
            if ((!is_array(result[account]))) {
                result[account] = ({});
            }
            result[account] += ({ m["protect_type"] });
        }  // end loop (verified)
    }  // end loop (verified)
    printf("Qeury result:\n%O\n", result);
    return;

}

// --------------------------------------------------------------------
// @code  nc=76, nargs=4, insts=76
// --------------------------------------------------------------------
private string md5_local_encrypt_pos(array arr_pos, string replace_str, string seed1, string seed2) {
    /* ---- Enhanced Pseudocode ---- */
    mixed pos;
    int i;
    mixed seed;
    string md5_str;
    string str_pos;

    md5_str = "";
    loop (pos in arr_pos) {
        --pos;
        seed = ((pos % 2) ? seed1 : seed2);
        str_pos = (string)pos;
        i = 0;

        // --- label L0035 ---
        while ((i < strlen(str_pos))) {
            put_sub_data(&str_pos, i, replace_str[(str_pos[i] - 48)]);
            ++i;
        }  // end loop (verified)
        md5_str += save_string(md5((str_pos + seed)))[1..<2];
    }  // end loop (verified)
    return md5_str;

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=46, nargs=3, insts=46
// --------------------------------------------------------------------
private int is_account_controlled_safe(object agent, string account, string &prompt) {
    /* ---- Enhanced Pseudocode ---- */
    mapping switch_info;
    mixed safe_ctrl_state;

    switch_info = query_safe_ctrl(account, "total_switch");
    prompt = "";
    if (((!is_mapping(switch_info)) || (!sizeof(switch_info)))) {
        return 0;
    }
    safe_ctrl_state = atoi((string)switch_info["protect_info"]);
    if ((!(safe_ctrl_state & -2147483648))) {
        return 0;
    }
    prompt = "";
    return 0;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=228, nargs=2, insts=228
// --------------------------------------------------------------------
private void _check_client_data(mixed agent, string client_data) {
    /* ---- Enhanced Pseudocode ---- */
    int result;
    int bot_type;
    int client_no;
    mixed text;
    mixed pic_buf;
    mixed bot_content;
    int login_result;
    mixed encrypt_key;
    string seed_A;
    string seed_B;
    mapping seed_map;

    if ((!is_object(agent))) {
        return;
    }
    client_no = agent->get_client_no();
    if ((!agent->lite_query_temp("antibot_question"))) {
        agent->to_be_disconnect(0);
        return;
    }
    bot_type = "/aaa/daemons/antibotd.c"->user_check_data(client_no, client_data, &bot_content);
    if ((!is_object(agent))) {
        return;
    }
    result = "/aaa/daemons/antibotd.c"->get_check_result(bot_type);
    do {
        login_result = ((result != 1) ? 0 : 1);
        if (((result < 0) && "/aaa/daemons/picd.c"->get_pic(&text, &pic_buf))) {
            agent->lite_set_temp("pic_auth_result", text);
            agent->send_message(6920, pic_buf);
            agent->to_be_disconnect(timeout_for_protect_auth);
            break;
        }
        encrypt_key = (login_result ? generate_encrypt_cookie() : "");
        if (is_apex_enabled()) {
            seed_A = "abcdefghijklmnop";
            seed_B = "zyxwvutsrqponmlkjihgfedcbaABCDEFGHIJKLMNOPQ";
            if (((!is_mapping(seed_map = get_seed(seed_A, seed_B))) || (sizeof(seed_map) < 1))) {
                break;
            }
            seed_A = seed_map["seed_A"];
            seed_B = seed_map["seed_B"];
            if (((((!is_string(seed_A)) || (strlen(seed_A) < 1)) || (!is_string(seed_B))) || (strlen(seed_B) < 1))) {
                break;
            }
            client_seedA[client_no] = seed_A;
            agent->send_message(11012, login_result, encrypt_key, seed_B);
        } else {
            agent->send_message(11012, login_result, encrypt_key, "");
        }
        if ((!login_result)) {
            agent->to_be_disconnect(2);
        } else {
            set_dynamic_key(agent, "login", encrypt_key);
            agent->lite_set_temp("antibot_answer", 1);
            agent->to_be_connect(60);
        }
    } while (0);

    // --- label L0207 ---
    if ((bot_type > 0)) {
        agent->lite_set_temp("bot_type", bot_type);
        log_antibot(agent->get_peer_ip(), "", bot_type, bot_content);
    }

}

// --------------------------------------------------------------------
// @brief  validator: verify data
// @code  nc=70, nargs=3, insts=70
// --------------------------------------------------------------------
private int check_skip_protected(mixed agent, string mac_addr, mapping account_info) {
    /* ---- Enhanced Pseudocode ---- */
    mixed last_login_ip;
    mixed last_login_id;
    mixed last_login_time;

    return 0;
    last_login_ip = trim((string)account_info["last_login_ip"]);
    if ((last_login_ip != agent->get_peer_ip())) {
        return 0;
    }
    last_login_id = trim((string)account_info["last_login_id"]);
    if ((last_login_id != mac_addr)) {
        return 0;
    }
    last_login_time = account_info["last_login_time"];
    if ((!is_string(last_login_time))) {
        return 0;
    }
    last_login_time = time_desc_to_int(last_login_time, 3);
    if (((time() - last_login_time) > 300)) {
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=12, nargs=3, insts=12
// --------------------------------------------------------------------
void send_antibot_question(mixed agent, string md5_value, string checksum) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_service((: _send_antibot_question, agent, md5_value, checksum :));

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=9, nargs=1, insts=9
// --------------------------------------------------------------------
private mixed get_protect_info_analysis_func(string type) {
    /* ---- Enhanced Pseudocode ---- */
    return query(("analysis_func/" + type));

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=18, nargs=1, insts=18
// --------------------------------------------------------------------
int set_matrix_status(int status) {
    /* ---- Enhanced Pseudocode ---- */
    mixed old_status;

    old_status = matrix_active_status;
    matrix_active_status = (status ? 1 : 0);
    return old_status;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=20, nargs=0, insts=20
// --------------------------------------------------------------------
private int get_error_times() {
    /* ---- Enhanced Pseudocode ---- */
    mixed err_times;

    if ((!err_times = "/aaa/daemons/configd.c"->get_my_info()["error_password_times"])) {
        return 0;
    }
    return atoi(err_times);

}

// --------------------------------------------------------------------
// @code  nc=128, nargs=1, insts=128
// --------------------------------------------------------------------
mixed revise_pwd_protect_checksum(mixed arg) {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr;
    mixed protect_list;
    int account;
    mapping m;
    mixed result;
    mixed protect_info;

    arr = ({});
    if (is_string(arg)) {
        arr = ({ arg });
    } else if (is_array(arg)) {
        arr += arg;
    } else {
        printf("Error paras, please input a string or an array.\n");
        return 0;
    }
    clean_array(arr, 4);
    if ((!sizeof(arr))) {
        return 0;
    }
    result = ([ ]);
    loop (account in arr) {
        protect_list = query_password_protect_info(account);
        if ((!is_array(protect_list))) {
            continue;
        }
        loop (m in protect_list) {
            if (((!is_mapping(m)) || (!sizeof(m)))) {
                continue;
            }
            protect_info = query_protect_info(account, m["protect_type"]);
            if (((!is_mapping(protect_info)) || (!sizeof(protect_info)))) {
                continue;
            }
            if ((!update_protect_info(protect_info))) {
                continue;
            }
            if ((!is_mapping(result[account]))) {
                result[account] = ([ ]);
            }
            result[account][m["protect_type"]] = "revised";
        }  // end loop (verified)
    }  // end loop (verified)
    printf("Revised result:\n%O\n", result);
    return;

}

// --------------------------------------------------------------------
// @code  nc=13, nargs=0, insts=13
// --------------------------------------------------------------------
private string generate_encrypt_cookie() {
    /* ---- Enhanced Pseudocode ---- */
    return sprintf("%07d", random(134217727))[0..6];

}

// --------------------------------------------------------------------
// @code  nc=110, nargs=4, insts=110
// --------------------------------------------------------------------
private mixed _generate_random_matrix(int count, mixed filters, mixed &prompt, mixed &is_out) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    mixed matrix;
    mixed left_matrix;
    mixed str;

    matrix = allocate_array(count);
    left_matrix = ({});
    is_out = 0;
    do {
        if ((!get_safe_tips_status())) {
            break;
        }
        if ((!is_array(filters))) {
            break;
        }
        left_matrix = (matrix_list - filters);
        if ((sizeof(left_matrix) < 1)) {
            prompt = "¸Ã¾ØÕóÃÜ±£¿¨ÉÏµÄµØÖ·ÒÑ¾­È«²¿±»Ê¹ÓÃ¹ýÁË£¬¼ÌÐøÊ¹ÓÃÔòÎÞ·¨È·±£°²È«ÐÔ£¬ÇëµÇÂ¼#B#urlhttp://www.gyyx.cn/UserCenter/JZMB/JzmbBind.aspx#t#nÖØÐÂ°ó¶¨ÃÜ±£¿¨£¬ÈçÐèÒª°ó¶¨ÍêÈ«Ãâ·ÑµÄµç×Ó°æ¾ØÕóÃÜ±££¬ÇëµÇÂ¼#B#urlhttp://www.gyyx.cn/UserCenter/JZMB/EjzmbBind.aspx#t#n";
            break;
        }
        if ((sizeof(filters) < 50)) {
            break;
        }
        is_out = 1;
        prompt = "ÄãµÄÃÜ±£¿¨Ê¹ÓÃ´ÎÊý¹ý¶à£¬ÇëµÇÂ¼#B#urlhttp://security.gyyx.cn/Matrix/Bind/#t#nÖØÐÂ°ó¶¨ÃÜ±£¿¨\nÈçÐèÒª°ó¶¨ÍêÈ«Ãâ·ÑµÄµç×Ó°æ¾ØÕóÃÜ±££¬ÇëµÇÂ¼#B#urlhttp://security.gyyx.cn/Matrix/GetMatrixCard#t#n";
    } while (0);

    // --- label L0058 ---
    i = 0;
    if (sizeof(left_matrix)) {
        matrix[i++] = left_matrix[random(sizeof(left_matrix))];
    }
    do {
        left_matrix = (matrix_list - matrix);
        str = left_matrix[random(sizeof(left_matrix))];
        matrix[i++] = str;
    } while (i < count);
    return matrix;

}

// --------------------------------------------------------------------
// @code  nc=7, nargs=1, insts=7
// --------------------------------------------------------------------
mixed calc_matrix_checksum(mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    return calc_protect_info_checksum(m);

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=25, nargs=2, insts=25
// --------------------------------------------------------------------
mixed query_matrix_info(string account, int decode) {
    /* ---- Enhanced Pseudocode ---- */
    mixed m;

    m = query_protect_info(account, "matrix");
    if ((!is_mapping(m))) {
        return 2;
    }
    if (decode) {
        decode_matrix_info(&m);
    }
    return m;

}

// --------------------------------------------------------------------
// @code  nc=57, nargs=2, insts=57
// --------------------------------------------------------------------
private int pwd_protect_update_matrix(string account, mapping &m) {
    /* ---- Enhanced Pseudocode ---- */
    mixed protect_info;

    protect_info = sprintf("%s,%s,%s,%s,%s,%s", m["matrix"], m["bind_time"], m["expired_time"], m["claim_lost_off_time"], (string)m["enable"], m["update_time"]);
    if ((is_array(m["used_matrix"]) && sizeof(m["used_matrix"]))) {
        protect_info += sprintf(",%s", implode(m["used_matrix"], ","));
    }
    m["protect_info"] = protect_info;
    return 1;

}

// --------------------------------------------------------------------
// @brief  validator: verify data
// @code  nc=1112, nargs=8, insts=1112
// --------------------------------------------------------------------
private mixed check_account_info(mixed agent, string account, string password, string mac_addr, string client_data, string passpod, string dist, int qr_code_login) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed ti;
    int btime;
    int client_no;
    int activate_result;
    mixed limit_info;
    mixed md5password;
    mixed sql_cmd;
    string prompt;
    mixed new_md5password;
    string breason;
    mixed real_reason;
    mixed ip;
    int encrypt_key;
    mixed activate_prompt;
    string aa_url;
    mixed limit_str;
    mapping activate_ret;
    mapping account_info;
    int bot_type;
    int result;
    mixed bot_content;
    mixed seed_A;
    mixed new_pwd;
    int check_block_day;
    int left_deny_time;
    int left_deny_day;
    mixed need_prompt;

    prompt = "";
    if ((!is_object(agent))) {
        return 0;
    }
    client_no = agent->get_client_no();
    bot_type = "/aaa/daemons/antibotd.c"->user_check_data(agent->get_client_no(), client_data, &bot_content);
    if ((!is_object(agent))) {
        map_delete(client_seedA, client_no);
        return 0;
    }
    result = "/aaa/daemons/antibotd.c"->get_check_result(bot_type);
    if ((bot_type > 0)) {
        log_antibot(agent->get_peer_ip(), account, bot_type, bot_content);
        if ((!is_object(agent))) {
            map_delete(client_seedA, client_no);
            return 0;
        }
    }
    if ((result == 0)) {
        map_delete(client_seedA, client_no);
        agent->to_be_disconnect(1);
        return 0;
    }
    if (check_for_protect_account(account, agent->get_peer_ip(), &prompt)) {
        map_delete(client_seedA, client_no);
        send_login_result(agent, 0, 0, prompt);
        agent->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "¸ÃÕÊºÅÒÑ±»ÔÝÊ±½ûÖ¹´Ó¸ÃIPµÇÂ¼\n");
        }
        return 0;
    }
    if (check_for_protect_account_ex(account, &prompt)) {
        map_delete(client_seedA, client_no);
        send_login_result(agent, 0, 0, prompt);
        agent->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "¸ÃÕÊºÅÔÚ10·ÖÖÓÄÚÖØ¸´5´Î½«Í¬Ò»ÕÊºÅÃÜÂëÊäÈëÕýÈ·£¬µ«¶¼Ã»ÓÐ³É¹¦Íê³Éµç»°£¨¶ÌÐÅ£©ÃÜ±£½âËø£¬±»½ûÖ¹µÇÂ¼¡£");
        }
        return 0;
    }
    if (((cno = "/daemons/dbd.c"->assure_connection("adb")) == -1)) {
        map_delete(client_seedA, client_no);
        printf("[1;31mÁ¬½Ó ADB Êý¾Ý¿âÊ§°Ü\n[2;37;0m");
        send_login_result(agent, 0, 0, "ÕÊºÅµÇÂ¼Ê§°Ü¡££¨Ô­Òò£º1001£©");
        agent->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "Á¬½Ó ADB Êý¾Ý¿âÊ§°Ü£¬ÔÝÊ±ÎÞ·¨Ìá¹©µÇÂ¼·þÎñ\n");
        }
        return 0;
    }
    account_info = query_account_info(account);
    if ((!is_object(agent))) {
        map_delete(client_seedA, client_no);
        return 0;
    }
    encrypt_key = 0;
    encrypt_key = query_dynamic_key(agent, "login");
    password = get_raw_password(("!*" + password), encrypt_key);
    if (((is_apex_enabled() && is_mapping(account_info)) && (_check_client_apex_status(account_info, password) == 1))) {
        seed_A = client_seedA[client_no];
        if (((is_string(seed_A) && is_string(new_pwd = get_pwd(password, strlen(password), seed_A))) && (strlen(new_pwd) > 0))) {
            password = new_pwd;
        }
    }
    map_delete(client_seedA, client_no);
    md5password = password;
    if (((!is_mapping(account_info)) && get_active_account_status())) {
        if (qr_code_login) {
            send_login_result(agent, 0, 0, "ÄúµÄÕÊºÅÔÚ´ËÇø×éÉÐÎ´¼¤»î£¬³õ´ÎµÇÂ¼ÐèÊ¹ÓÃÔ­ÊäÈëÕÊºÅÃÜÂë·½Ê½½øÐÐµÇÂ¼¡£");
            agent->to_be_disconnect(1);
            if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
                "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "Î´¼¤»îÕÊºÅ²»ÄÜ½øÐÐ¶þÎ¬ÂëµÇÂ¼£¬µÇÂ¼Ê§°Ü\n");
            }
            return 0;
        }
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "Êý¾Ý¿â²»´æÔÚ¸ÃÕÊºÅ£¬³¢ÊÔ×Ô¶¯¼¤»î\n");
        }
        activate_ret = activate_account(account, md5password, agent->get_peer_ip());
        activate_result = activate_ret["result"];
        activate_prompt = activate_ret["prompt"];
        if ((!is_object(agent))) {
            return 0;
        }
        if ((activate_result == 2)) {
            do_when_password_error(agent, account, &activate_prompt);
        } else {
            do_when_password_correct(agent, account);
        }
        if ((activate_result == 3)) {
            activate_result = (aa_rule_activate_account ? 3 : 1);
        }
        switch (activate_result) {
            case 0:
            break;
            case 2:
            send_login_result(agent, 3, 0, activate_prompt);
            agent->to_be_disconnect(1);
            if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
                "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "ÕÊºÅÃÜÂë²»Æ¥Åä£¬µÇÂ¼Ê§°Ü\n");
            }
            return 0;
            case 3:
            if ((!is_string(aa_url = "/aaa/daemons/configd.c"->get_global_config("aa_url")))) {
                aa_url = "http://account.gyyx.cn/gameAdult/AdultValidate.aspx";
            }
            send_login_result(agent, 0, 0, sprintf("#RÄúµÄ×¢²á×ÊÁÏÎ´Í¨¹ý·À³ÁÃÔÑéÖ¤£¬½«±»ÄÉÈë·À³ÁÃÔÏµÍ³£¬ÎÞ·¨½øÈëÓÎÏ·¡£Çëµã»÷#n#Y#urlÕâÀï{%s}#t#n#R²¹ÌîÕýÈ·ÐÅÏ¢¡£#n", aa_url));
            agent->to_be_disconnect(5);
            if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
                "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "×Ô¶¯¼¤»îÃ»ÓÐÍ¨¹ý·À³ÁÃÔÑéÖ¤£¬µÇÂ¼Ê§°Ü");
            }
            return 0;
            case 1:
            send_login_result(agent, 0, 0, activate_prompt);
            agent->to_be_disconnect(1);
            if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
                "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "×Ô¶¯¼¤»î²»³É¹¦£¬µÇÂ¼Ê§°Ü\n");
            }
            return 0;
            default:
        }  // end switch

        // --- label L0535 ---
        account_info = query_account_info(account);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "×Ô¶¯¼¤»îÍê³É\n");
        }
        if ((is_mapping(account_info) && (strlen(account_info["active_path"]) < 1))) {
            account_info["active_path"] = 1;
            update_account_info(account_info);
        }
        if ((!is_object(agent))) {
            return 0;
        }
    }
    if ((!is_mapping(account_info))) {
        send_login_result(agent, 0, 0, "ÕÊºÅµÇÂ¼Ê§°Ü¡££¨Ô­Òò£º1007£©");
        agent->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "Êý¾Ý¿â²»´æÔÚ¸ÃÕÊºÅ£¬µÇÂ¼Ê§°Ü\n");
        }
        return 0;
    }
    if ((account_info["checksum"] != calc_account_checksum(account_info))) {
        send_login_result(agent, 0, 0, "ÕÊºÅµÇÂ¼Ê§°Ü¡££¨Ô­Òò£º1009£©");
        agent->to_be_disconnect(1);
        log_bad_checksum_account(account, account_info);
        return 0;
    }
    if (((!is_under_control(account_info)) && (account_info["locked"] == "1"))) {
        send_login_result(agent, 5, 0, "¸ÃÕÊºÅÒÑ±»Ëø¶¨£¬ÇëÏÈµÇÂ¼¹âÓîÓÎÏ·ÉçÇø½â³ýËø¶¨£¡£¨Ô­Òò£º1012£©");
        agent->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "ÕÊºÅ´¦ÓÚËø¶¨×´Ì¬£¬µÇÂ¼Ê§°Ü\n");
        }
        return 0;
    }
    check_block_day = 0;
    btime = account_info["blocked_time"];
    if (is_apex_enabled()) {
        btime = trim((string)btime);
    }
    real_reason = account_info["blocked_reason"];
    breason = "ÔËÓªÖÐ±»·â±ÕµÄÕÊºÅ";
    if (is_undefined(btime = time_desc_to_int(btime, 3))) {
        breason = "ÔËÓªÖÐ±»·â±ÕµÄÕÊºÅ¡££¨Ô­Òò£º1010£©";
        get_extra_block_reason_msg(check_block_day, real_reason, &breason);
        send_login_result(agent, 4, 0, breason);
        agent->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", sprintf("¸ÃÕÊºÅ±»·â£¬½ûÖ¹µÇÂ¼£¬Ô­Òò£º%O\n", "block_time×Ö¶Î¸ñÊ½´íÎó"));
        }
        return 0;
    }
    if ((btime > time())) {
        left_deny_time = (btime - time());
        if ((left_deny_time < 15552000)) {
            left_deny_day = (left_deny_time / 86400);
            left_deny_day = ((left_deny_time % 86400) ? (left_deny_day + 1) : left_deny_day);
            check_block_day = left_deny_day;
            breason = sprintf("ÔËÓªÖÐ±»·â±ÕµÄÕÊºÅ£¬½«ÓÚ#R%d#nÌìºó½â³ý·â±Õ¡£", left_deny_day);
        }
        if ((sscanf(real_reason, "need_prompt:%s", &need_prompt) == 1)) {
            breason = need_prompt;
        }
        get_extra_block_reason_msg(check_block_day, real_reason, &breason);
        send_login_result(agent, 4, 0, breason);
        agent->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", sprintf("¸ÃÕÊºÅ±»·â£¬½ûÖ¹µÇÂ¼£¬Ô­Òò£º%O\n", real_reason));
        }
        return 0;
    }
    if (((btime > 0) && strlen(btime = ctime(btime, 3)))) {
        account_info["blocked_time"] = "";
        account_info["blocked_reason"] = "";
        ip = agent->get_peer_ip();
        account = agent->query_account();
        update_account_info(account_info);
        ti = ctime(time(), 3);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "³¬¹ý·âºÅÊ±¼ä£¬¸ø¸ÃÕÊºÅ½â·â\n");
        }
        sql_cmd = mysql_bind_parameters("insert into acc_log(ip, time,account, dist, type, msg, memo) values(?, ?, ?, ?, ?, ?, ?)", ip, ti, account, btime, "auto_unlock_account", "·âºÅÊ±¼äÒÑµ½£¬×Ô¶¯½âËø", "");
        (*f_execute)(cno, sql_cmd);
        if ((!is_object(agent))) {
            return 0;
        }
    }
    if ((!qr_code_login)) {
        new_md5password = calc_password_new_md5(account_info["account"], md5password);
        printf("pass = %s \n", new_md5password);
        if ((stricmp(account_info["password"], new_md5password) != 0)) {
            prompt = "ÕÊºÅºÍÃÜÂë²»Æ¥Åä£¡";
            if ((stricmp(account_info["org_password"], new_md5password) == 0)) {
                send_login_result(agent, 3, 0, "¶Ô²»Æð£¬¹ÜÀíÈËÔ±ÕýÔÚ¶ÔÄãµÄÕÊºÅ½øÐÐ²Ù×÷£¬ÔÝÊ±ÎÞ·¨µÇÂ¼¡£");
            } else {
                do_when_password_error(agent, account, &prompt);
                send_login_result(agent, 3, 0, prompt);
            }
            agent->to_be_disconnect(1);
            if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
                "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "ÕÊºÅÃÜÂë²»Æ¥Åä£¬µÇÂ¼Ê§°Ü\n");
            }
            return 0;
        }
    }
    if (((((merge_limit_login && is_string(limit_str = "/aaa/daemons/configd.c"->get_global_config("merge_limit_login"))) && is_array(limit_info = restore_value(limit_str))) && is_string(account_info["last_login_time"])) && (time_desc_to_int(account_info["last_login_time"], 3) < (time_desc_to_int(limit_info[0], 3) - (int)limit_info[1])))) {
        send_login_result(agent, 0, 0, sprintf("ÄãµÄÕÊºÅÕýÔÚ½øÐÐÊý¾Ý»¥Í¨´¦Àí£¬ÐèÉÔºó²ÅÄÜ½øÐÐµÇÂ¼£¬Ô¤¼Æ¿ÉÒÔµÇÂ¼Ê±¼äÎª£º%s", (string)limit_info[2]));
        agent->to_be_disconnect(5);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "µ±Ç°ÕÊºÅÕýÔÚºÏ·þÖÐ£¬½ûÖ¹µÇÂ¼¡£\n");
        }
        return 0;
    }
    return account_info;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=46, nargs=1, insts=46
// --------------------------------------------------------------------
private array get_random_pos(int file_length) {
    /* ---- Enhanced Pseudocode ---- */
    mixed pos_num;
    int i;
    mixed interval;
    mixed arr_pos;

    pos_num = random(4, 8);
    interval = (file_length / pos_num);
    arr_pos = ({});
    for (i = 0; i <= (pos_num - 1); i++) {
        arr_pos += ({ (((i * interval) + 1) + random(interval)) });
    }  // end loop (verified)
    return arr_pos;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=130, nargs=4, insts=130
// --------------------------------------------------------------------
private int pwd_protect_deny_phone(string account, mapping m, mixed param, mixed dist) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ct;
    mixed login_ip;
    mixed auth_method;
    mapping map;

    auth_method = get_auth_method();
    switch (auth_method) {
        case 1:
        ct = ctime(time(), 3);
        if ((ct > m["permission_expired_time"])) {
            return 1;
        }
        break;
        case 2:
        map = "/aaa/daemons/pwdprotect_authd.c"->auth_protect("phone", account, dist);
        if ((map["result"] != 0)) {
            if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
                "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", sprintf("µç»°ÃÜ±£ÈÏÖ¤Ê§°Ü£¬Ô­ÒòÊÇ%s\n", map["reason"]));
            }
            return 1;
        }
        if ((map["reason"] == "AUTH_UNBIND")) {
            cancel_pwd_protect(account, "phone", param, dist);
        }
        return 0;
        default:
        break;
    }  // end switch

    // --- label L0083 ---
    if ((!is_object(param))) {
        return 0;
    }
    login_ip = sprintf("%s@%s", param->get_peer_ip(), param->lite_query_temp("mac_addr"));
    if ((!m["login_ip"])) {
        m["login_ip"] = login_ip;
        update_protect_info(m);
        return 0;
    }
    if ((m["login_ip"] != login_ip)) {
        return 1;
    }
    return 0;

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int is_matrix_pic_enabled() {
    /* ---- Enhanced Pseudocode ---- */
    return matrix_pic_enabled;

}

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=21, nargs=2, insts=21
// --------------------------------------------------------------------
private void do_when_password_correct(mixed agent, string account) {
    /* ---- Enhanced Pseudocode ---- */
    mixed key;

    if ((!is_object(agent))) {
        return;
    }
    key = get_protect_key(account, agent->get_peer_ip());
    map_delete(protect_account, key);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=289, nargs=3, insts=289
// --------------------------------------------------------------------
void auth_local_encrypt(mixed agent, string account, string auth_text) {
    /* ---- Enhanced Pseudocode ---- */
    int encrypt_type;
    mixed arr_pos;
    mixed seed1;
    mixed seed2;
    mixed replace_str;
    mapping le_map;
    mapping le_info;
    mixed account_info;

    if ((!is_object(agent))) {
        return;
    }
    do {
        account = trim(account);
        if (((sizeof(account) == 0) || (stricmp(agent->query_account(), account) != 0))) {
            agent->to_be_disconnect(1);
            return;
        }
        if ((!is_mapping(le_map = query_local_encrypt_info(account)))) {
            break;
        }
        if ((!le_map["state"])) {
            break;
        }
        le_info = restore_value(le_map["local_encrypt_info"]);
        arr_pos = agent->lite_query_temp("local_encrypt_pos");
        if (((!is_mapping(le_info)) || (!is_array(arr_pos)))) {
            send_login_result(agent, 0, 0, "±¾µØÔ¿³×ÎÄ¼þÑéÖ¤Ê§°Ü£¬ÇëÈ·¶¨ÃÜÔ¿ÎÄ¼þÎ»ÖÃÊÇ·ñÕýÈ·¡£");
            agent->to_be_disconnect(1);
            if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
                "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "´óÎÄ¼þ¼ÓÃÜÑéÖ¤Ê§°Ü");
            }
            return;
        }
        agent->set_temp("protects/E", 1);
        encrypt_type = le_info["encrypt_type"];
        seed1 = le_info["seed1"];
        seed2 = le_info["seed2"];
        replace_str = le_info["replace_str"];
        if ((encrypt_type == 1)) {
            if ((auth_text != des_local_encrypt_pos(arr_pos, replace_str, seed1, seed2))) {
                send_login_result(agent, 0, 0, "±¾µØÔ¿³×ÎÄ¼þÑéÖ¤Ê§°Ü£¬ÇëÈ·¶¨ÃÜÔ¿ÎÄ¼þÎ»ÖÃÊÇ·ñÕýÈ·¡£");
                agent->to_be_disconnect(1);
                if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
                    "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "´óÎÄ¼þ¼ÓÃÜÑéÖ¤Ê§°Ü");
                }
                return;
            }
            agent->lite_set("local_encrypt", 1);
            break;
        }
        if ((auth_text == md5_local_encrypt_pos(arr_pos, replace_str, seed1, seed2))) {
            agent->lite_set("local_encrypt", 1);
            break;
        }
        send_login_result(agent, 0, 0, "±¾µØÔ¿³×ÎÄ¼þÑéÖ¤Ê§°Ü£¬ÇëÈ·¶¨ÃÜÔ¿ÎÄ¼þÎ»ÖÃÊÇ·ñÕýÈ·¡£");
        agent->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "´óÎÄ¼þ¼ÓÃÜÑéÖ¤Ê§°Ü");
        }
        return;
    } while (0);

    // --- label L0231 ---
    if ((agent->lite_query("local_encrypt") == 1)) {
        agent->lite_set("safe_status", 1);
    }
    if ((!is_mapping(account_info = query_account_info(account)))) {
        send_login_result(agent, 0, 0, "ÕÊºÅµÇÂ¼Ê§°Ü¡££¨Ô­Òò£º1007£©");
        agent->to_be_disconnect(1);
        if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
            "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "Êý¾Ý¿â²»´æÔÚ¸ÃÕÊºÅ£¬µÇÂ¼Ê§°Ü\n");
        }
        return;
    }
    auth_all_protect(agent, account, account_info);

}

// --------------------------------------------------------------------
// @code  nc=20, nargs=4, insts=20
// --------------------------------------------------------------------
private int pwd_protect_deny_passpod(string account, mapping m, mixed param, mixed dist) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ct;

    ct = ctime(time(), 3);
    if ((ct < m["expired_lock_time"])) {
        return 1;
    }
    return 0;

}

// --------------------------------------------------------------------
// @code  nc=45, nargs=2, insts=45
// --------------------------------------------------------------------
private int pwd_protect_fetch_passpod(string account, mapping &m) {
    /* ---- Enhanced Pseudocode ---- */
    mixed protect_fields;

    if (strlen(m["protect_info"])) {
        protect_fields = explode(m["protect_info"], ",");
    } else {
        protect_fields = ({ "" });
    }
    switch (sizeof(protect_fields)) {
        case 0:
        break;
        case 1:
        m["expired_lock_time"] = trim(protect_fields[0]);
        break;
        default:
        break;
    }  // end switch

    // --- label L0041 ---
    return 1;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=45, nargs=3, insts=45
// --------------------------------------------------------------------
private array generate_random_matrix(mixed agent, int count, mixed filters) {
    /* ---- Enhanced Pseudocode ---- */
    mixed matrix;
    mixed prompt;
    mixed is_out;

    if ((!is_object(agent))) {
        return ({});
    }
    if ((!get_safe_tips_status())) {
        agent->lite_set("safe_status", 1);
    }
    matrix = _generate_random_matrix(count, filters, &prompt, &is_out);
    if (is_out) {
        agent->lite_set("safe_status", 2);
        agent->send_message(8165, prompt, 1);
    }
    return matrix;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int get_fixed_city_status() {
    /* ---- Enhanced Pseudocode ---- */
    return fixed_city_status;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=424, nargs=3, insts=424
// --------------------------------------------------------------------
void auth_matrix(mixed agent, string account, string auth_text) {
    /* ---- Enhanced Pseudocode ---- */
    int auth_key;
    int interval;
    int matrix_status;
    mixed matrix_random;
    mixed matrix_text;
    string reason;
    mixed value_1;
    mixed value_2;
    mixed value_3;
    mapping m;
    mapping matrix_info;

    do {
        if ((is_object(agent) != 1)) {
            return;
        }
        interval = 0;
        if ((stricmp(agent->query_account(), account) != 0)) {
            break;
        }
        if ((!agent->lite_query_temp("matrix"))) {
            if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
                "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "¿Í»§¶ËÔÚÎ´Í¨¹ýÆÕÍ¨ÃÜÂëÑéÖ¤Çé¿öÏÂ£¬Ö±½Ó·¢ËÍÃÜ±£ÃÜÂëÑéÖ¤ÇëÇó£¬·Ç·¨£¬µÇÂ¼Ê§°Ü---1\n");
            }
            break;
        }
        agent->lite_delete_temp("matrix");
        matrix_random = agent->lite_query_temp("matrix_random");
        if (((!is_array(matrix_random)) || (sizeof(matrix_random) != 3))) {
            if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
                "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "¿Í»§¶ËÔÚÎ´Í¨¹ýÆÕÍ¨ÃÜÂëÑéÖ¤Çé¿öÏÂ£¬Ö±½Ó·¢ËÍÃÜ±£ÃÜÂëÑéÖ¤ÇëÇó£¬·Ç·¨£¬µÇÂ¼Ê§°Ü---2\n");
            }
            break;
        }
        m = decode_matrix_auth_text(auth_text);
        if ((!is_mapping(m))) {
            if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
                "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "¿Í»§¶ËÊäÈëµÄÃÜ±£ÑéÖ¤×Ö·û·Ç·¨£¬µÇÂ¼Ê§°Ü\n");
            }
            break;
        }
        if ((m["matrix_cookie"] != agent->lite_query_temp("matrix_cookie"))) {
            if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
                "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "¿Í»§¶ËÊäÈëµÄÃÜ±£ÑéÖ¤COOKIE·Ç·¨£¬µÇÂ¼Ê§°Ü\n");
            }
            break;
        }
        interval = 2;
        if ((!get_matrix_status())) {
            matrix_status = 2;
        } else {
            matrix_info = query_matrix_info(account);
            matrix_status = get_account_matrix_status(matrix_info);
        }
        if ((1 == matrix_status)) {
            if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
                "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "Êý¾Ý¿âÔ­Òò£¬ÎÞ·¨¶Ô¿Í»§¶ËÊäÈë½øÐÐÑéÖ¤£¬µÇÂ¼Ê§°Ü");
            }
            reason = "ÑéÖ¤·þÎñÆ÷Òì³££¬µÇÂ¼Ê§°Ü£¨1013£©¡£";
            break;
        }
        if ((2 != matrix_status)) {
            decode_matrix_info(&matrix_info);
            matrix_text = matrix_info["matrix"];
            value_1 = get_value_from_matrix_text(matrix_text, matrix_random[0]);
            value_2 = get_value_from_matrix_text(matrix_text, matrix_random[1]);
            value_3 = get_value_from_matrix_text(matrix_text, matrix_random[2]);
            if ((calc_matrix_pwd_md5(value_1, value_2, value_3, m["matrix_cookie"]) != m["MD5"])) {
                if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
                    "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "¿Í»§¶ËÊäÈëµÄÃÜ±£ÃÜÂëºÍÊý¾Ý¿âÖÐµÄ²»Æ¥Åä£¬µÇÂ¼Ê§°Ü\n");
                }
                reason = "ÄãÊäÈëµÄÃÜ±£¿¨ÃÜÂë´íÎó£¬µÇÂ¼Ê§°Ü¡£";
                do_when_password_error(agent, account, &reason);
                break;
            }
            if (is_array(matrix_info["used_matrix"])) {
                matrix_info["used_matrix"] -= matrix_random;
                matrix_info["used_matrix"] += matrix_random;
            } else {
                matrix_info["used_matrix"] = matrix_random;
            }
            do_when_password_correct(agent, account);
            update_matrix_info(matrix_info, 1);
        }
        if ((is_object(agent) != 1)) {
            return;
        }
        auth_key = generate_auth_key();
        agent->lite_set("auth_key", auth_key);
        send_login_result(agent, (aa_rule_login ? 7 : 1), auth_key, "");
        clean_mapping_by_value(auth_key_account, 10);
        auth_key_account[auth_key] = agent;
        agent->to_be_disconnect(timeout_for_choose_server);
        if ((2 == matrix_status)) {
            if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
                "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "Î´¼¤»îÃÜ±£»úÖÆ£¬ÎÞÐèÑéÖ¤£¬µÇÂ¼³É¹¦\n");
            }
        } else {
            if ((is_string(account) || is_string(account->query_temp("login_info/account")))) {
                "/daemons/logd.c"->log_login_info(get_login_info_file(account), "login", "ÃÜ±£ÃÜÂëÑéÖ¤Í¨¹ý£¬µÇÂ¼³É¹¦\n");
            }
        }
        return;
    } while (0);

    // --- label L0398 ---
    if ((is_object(agent) == 1)) {
        if ((is_string(reason) && strlen(reason))) {
            send_login_result(agent, 0, 0, reason);
        }
        agent->to_be_disconnect(interval);
    }

}

// --------------------------------------------------------------------
// @brief  validator: verify data
// @code  nc=74, nargs=2, insts=74
// --------------------------------------------------------------------
private int check_for_ban(object unauthed_agent, string &prompt) {
    /* ---- Enhanced Pseudocode ---- */
    int client_per_ip;
    int count;
    mixed mks;
    mixed agent;
    mapping m;

    if ((!is_mapping(m = "/aaa/daemons/configd.c"->get_my_info()))) {
        return 0;
    }
    if ((!client_per_ip = m["client_per_ip"])) {
        return 0;
    }
    client_per_ip = atoi(client_per_ip);
    if ((client_per_ip <= 0)) {
        return 0;
    }
    count = 0;
    loop (mks in client_agents) {
        agent = mks[1];
        if ((agent->get_peer_ip() == unauthed_agent->get_peer_ip())) {
            ++count;
        }
        if ((count > client_per_ip)) {
            return 1;
        }
    }  // end loop (verified)
    return 0;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int get_auth_method() {
    /* ---- Enhanced Pseudocode ---- */
    return g_auth_method;

}

/* ================================================================
   DECLARED FUNCTIONS (no bytecode - likely inherited or forward decl)
   ================================================================ */
/*
   private void do_when_auth_verify_code_failed(mixed agent, string account);  // inherited from parent or forward declaration
   private void do_when_auth_verify_code_succed(mixed agent, string account, string password, string mac_addr, string client_data, string passpod, string dist);  // inherited from parent or forward declaration
*/

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        144
   Code segments found:         144
   Successfully decoded:        144
   Unresolved/minimal:          0
   Functions without code:      2
   Inherited programs:          4
      - /feature/common/dbase.c()
      - /feature/daemon/register.c()
      - /feature/daemon/handler_module.c()
      - /feature/daemon/eternal_data.c()

   Note: Functions without code are usually inherited from the
   parent object (), forward declarations, or
   simple property accessor methods that the compiler optimizes away.
   ================================================================ */