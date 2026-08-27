/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: express_recharged.o
 * Source: /aaa/daemons/express_recharged.c
 * Inherits: (none)
 * Functions: 23 names, 23 code segments found
 * Strings: 61, Variables: 7
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   mapping can_record_defined_error;
   mapping card_type_to_gy;
   int config_ok;
   int cur_seq_no;
   string inquiry_url;
   string recharge_url;
   int wrong_rsp_stat;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private string get_entire_recharge_url(mapping info);
int do_alipay_recharge(string dist, string account, int rmb);
void do_inquiry(string order_id, int need_comm, mixed cb);
private int handle_alipay_rsp(mixed ret);
private int to_over(int request_type, mapping info);
private void fill_result_info(mapping info, int result, int result_code, int gold);
private int write_order_log(mapping info);
private void _do_inquiry(string order_id, int need_comm, mixed cb);
private mixed select_order_info(string order_id);
private int to_inquiry(int request_type, mapping info);
private void destruct();
private void _do_recharge(mapping info, mixed cb);
private void do_init();
private string get_entire_alipay_url(string dist, string account, int rmb);
private void handle_http_rsp(int request_type, mixed rsp, mapping info);
private void inquiry_daemon();
private int create_order_log(mapping info);
void create_adb_table();
private void create();
private void handle_internal_rsp(int request_type, int rsp, mapping info);
private string get_entire_inquiry_url(mapping info);
void do_recharge(mapping info, mixed cb);
private int get_thread_id_by_order_id(string order_id);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   cur_seq_no = 0;  // /aaa/daemons/express_recharged.c:51
   can_record_defined_error = ([ 100 : 0, 101 : 1, 102 : 1, 103 : 0 ]);
   card_type_to_gy = ([ 1 : 0, 2 : 1, 3 : 2 ]);
   wrong_rsp_stat = 0;

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=180, nargs=1, insts=180
// --------------------------------------------------------------------
private string get_entire_recharge_url(mapping info) {
    /* ---- Enhanced Pseudocode ---- */
    int par_value;
    int card_type;
    mixed card_no;
    mixed card_pwd;
    mixed user_name;
    mixed order_id;
    mixed server_name;
    mixed sales_name;
    mixed user_ip;
    mixed pay_type;
    mixed str_serv;
    mixed str_md5;
    mixed sign_key;
    mixed addr_url;

    card_no = info["card_no"];
    card_pwd = info["card_pwd"];
    order_id = info["order_id"];
    user_name = info["user_name"];
    server_name = info["server_name"];
    sales_name = info["sales_name"];
    user_ip = info["user_ip"];
    par_value = (int)info["par_value"];
    pay_type = info["pay_type"];
    card_type = (int)info["card_type"];
    str_serv = convert_to_url_encode(server_name);
    switch (pay_type) {
        case "S19Pay":
        if ((strlen(addr_url = "/aaa/daemons/configd.c"->get_global_config("mobile_recharge_url")) < 1)) {
            return "";
        }
        sign_key = (((((((((card_no + card_pwd) + order_id) + 2) + str_serv) + user_name) + user_ip) + par_value) + card_type) + "KfEqyUUHllKPz37EVYyUzGQvgM3ck7yFO0EVwCqQhxrlMwFG9zI9OhvYfHMwUiHHgAhl9Uk7Jv4");
        str_md5 = save_string(md5(sign_key))[1..<2];
        return sprintf("%s?cardNo=%s&cardPwd=%s&orderNo=%s&game=%d&serverName=%s&userName=%s&userIp=%s&payValue=%d&cardType=%d&signKey=%s", addr_url, card_no, card_pwd, order_id, 2, str_serv, user_name, user_ip, par_value, card_type, str_md5);
        default:
        sign_key = (((((((((card_no + card_pwd) + order_id) + user_name) + str_serv) + sales_name) + user_ip) + par_value) + pay_type) + "KfEqyUUHllKPz37EVYyUzGQvgM3ck7yFO0EVwCqQhxrlMwFG9zI9OhvYfHMwUiHHgAhl9Uk7Jv4");
        str_md5 = save_string(md5(sign_key))[1..<2];
        return sprintf("%s?CardNO=%s&CardPwd=%s&OrderID=%s&UserName=%s&ServerName=%s&SalesName=%s&UserIP=%s&ParValue=%d&PayType=%s&SignKey=%s", recharge_url, card_no, card_pwd, order_id, user_name, str_serv, sales_name, user_ip, par_value, pay_type, str_md5);
    }  // end switch

}

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=28, nargs=3, insts=28
// --------------------------------------------------------------------
int do_alipay_recharge(string dist, string account, int rmb) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ret;
    mixed url;

    url = get_entire_alipay_url(dist, account, rmb);
    if ((strlen(url) < 1)) {
        return 101;
    }
    ret = "/daemons/http_clientd.c"->http_get_web(url);
    return handle_alipay_rsp(ret);

}

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=18, nargs=3, insts=18
// --------------------------------------------------------------------
void do_inquiry(string order_id, int need_comm, mixed cb) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!config_ok)) {
        return;
    }
    "/daemons/threadd.c"->dispatch(get_thread_id_by_order_id(order_id), (: _do_inquiry, order_id, need_comm, cb :));

}

// --------------------------------------------------------------------
// @brief  event handler: respond to event
// @code  nc=43, nargs=1, insts=43
// --------------------------------------------------------------------
private int handle_alipay_rsp(mixed ret) {
    /* ---- Enhanced Pseudocode ---- */
    if (is_int(ret)) {
        switch (ret) {
            case -3:
            return 102;
            case -2:
            case -1:
            default:
            return 103;
        }  // end switch
    }
    if ((!is_string(ret))) {
        return 104;
    }
    if ((ret == "0")) {
        return 0;
    }
    if ((atoi(ret) <= 0)) {
        return 104;
    }
    return atoi(ret);

}

// --------------------------------------------------------------------
// @brief  converter: transform data
// @code  nc=49, nargs=2, insts=49
// --------------------------------------------------------------------
private int to_over(int request_type, mapping info) {
    /* ---- Enhanced Pseudocode ---- */
    int op_res;

    if ((0 == request_type)) {
        op_res = create_order_log(info);
    } else {
        op_res = write_order_log(info);
    }
    if ((op_res != 2)) {
        "/daemons/logd.c"->log_local(1, "rechg", "over", (string)(request_type | (op_res << 8)), info["order_id"], info["card_no"], save_string(info));
        return op_res;
    }
    return 2;

}

// --------------------------------------------------------------------
// @brief  data mapper: populate structure
// @code  nc=22, nargs=4, insts=22
// --------------------------------------------------------------------
private void fill_result_info(mapping info, int result, int result_code, int gold) {
    /* ---- Enhanced Pseudocode ---- */
    info["result"] = result;
    info["result_code"] = result_code;
    info["gold"] = gold;

}

// --------------------------------------------------------------------
// @brief  data writer: persist information
// @code  nc=85, nargs=1, insts=85
// --------------------------------------------------------------------
private int write_order_log(mapping info) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    int result;
    mixed sql_cmd;

    if ((!is_string(info["order_id"]))) {
        return 1;
    }
    if (((info["result"] == 9) && can_record_defined_error[info["result_code"]])) {
        info["result"] = 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 1)) {
        return 0;
    }
    info["update_time"] = ctime(time(), 3);
    sql_cmd = convert_mapping_to_sql_cmd("update", "express_recharge", info, "order_id");
    result = (*f_execute)(cno, sql_cmd);
    if ((result < 0)) {
        return 0;
    }
    if ((result < 1)) {
        return 1;
    }
    return 2;

}

// --------------------------------------------------------------------
// @brief  internal handler: sub-action processor
// @code  nc=130, nargs=3, insts=130
// --------------------------------------------------------------------
private void _do_inquiry(string order_id, int need_comm, mixed cb) {
    /* ---- Enhanced Pseudocode ---- */
    mixed result;
    mixed arr;
    mixed url;
    mapping info;

    arr = select_order_info(order_id);
    if ((!is_array(arr))) {
        return;
    }
    if ((sizeof(arr) == 0)) {
        if (is_function(cb)) {
            (*cb)(order_id, "", 9, 103, 0);
        }
        return;
    }
    info = arr[0];
    if ((info["result"] != 0)) {
        if (is_function(cb)) {
            (*cb)(order_id, info["pay_type"], info["result"], info["result_code"], info["gold"]);
        }
        return;
    }
    if ((need_comm == 0)) {
        if (is_function(cb)) {
            (*cb)(info["order_id"], info["pay_type"], info["result"], info["result_code"], info["gold"]);
        }
        return;
    }
    url = get_entire_inquiry_url(info);
    result = "/daemons/http_clientd.c"->http_get_web(url);
    handle_http_rsp(1, result, info);
    if (is_function(cb)) {
        (*cb)(info["order_id"], info["pay_type"], info["result"], info["result_code"], info["gold"]);
    }

}

// --------------------------------------------------------------------
// @brief  query: database retrieval
// @code  nc=89, nargs=1, insts=89
// --------------------------------------------------------------------
private mixed select_order_info(string order_id) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed arr;
    mapping m;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 1)) {
        return 0;
    }
    arr = mysql_select(cno, mysql_bind_parameters("select * from express_recharge where order_id=? limit 1", order_id));
    if ((!arr)) {
        return 0;
    }
    if (sizeof(arr)) {
        m = arr[0];
        m["result"] = atoi(m["result"]);
        m["result_code"] = atoi(m["result_code"]);
        m["gold"] = atoi(m["gold"]);
        m["par_value"] = atoi(m["par_value"]);
        m["inquiry_times"] = atoi(m["inquiry_times"]);
    }
    return arr;

}

// --------------------------------------------------------------------
// @brief  converter: transform data
// @code  nc=63, nargs=2, insts=63
// --------------------------------------------------------------------
private int to_inquiry(int request_type, mapping info) {
    /* ---- Enhanced Pseudocode ---- */
    int op_res;

    if ((0 == request_type)) {
        info["inquiry_times"] = 0;
        op_res = create_order_log(info);
    } else {
        info["inquiry_times"] += 1;
        op_res = write_order_log(info);
    }
    if ((op_res != 2)) {
        "/daemons/logd.c"->log_local(1, "rechg", "inquiry", (string)(request_type | (op_res << 8)), info["order_id"], info["card_no"], save_string(info));
        return op_res;
    }
    return 2;

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=10, nargs=0, insts=10
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    if (get_thread_info("inquiry_thread")) {
        delete_thread("inquiry_thread");
    }

}

// --------------------------------------------------------------------
// @brief  internal handler: sub-action processor
// @code  nc=44, nargs=2, insts=44
// --------------------------------------------------------------------
private void _do_recharge(mapping info, mixed cb) {
    /* ---- Enhanced Pseudocode ---- */
    mixed result;
    mixed url;

    url = get_entire_recharge_url(info);
    result = "/daemons/http_clientd.c"->http_get_web(url);
    handle_http_rsp(0, result, info);
    if (is_function(cb)) {
        (*cb)(info["order_id"], info["pay_type"], info["result"], (int)info["result_code"], (int)info["gold"]);
    }

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=67, nargs=0, insts=67
// --------------------------------------------------------------------
private void do_init() {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;


    // --- label L0002 ---
    while (is_undefined(m = "/aaa/daemons/configd.c"->get_my_info())) {
        sleep(5);
    }  // end loop (verified)
    recharge_url = m["recharge_url"];
    inquiry_url = m["inquiry_url"];
    if (((((!is_string(recharge_url)) || (!is_string(inquiry_url))) || (sizeof(recharge_url) == 0)) || (sizeof(inquiry_url) == 0))) {
        broadcast("Miss recharge_url or inquiry_url config.\n");
        config_ok = 0;
    } else {
        config_ok = 1;
    }
    if (config_ok) {
        create_thread("inquiry_thread", "inquiry_daemon");
    }

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=56, nargs=3, insts=56
// --------------------------------------------------------------------
private string get_entire_alipay_url(string dist, string account, int rmb) {
    /* ---- Enhanced Pseudocode ---- */
    mixed alipay_url;
    mixed str_serv;
    mixed sign_key;
    mixed sign_key_md5;

    if (((!is_string(alipay_url = "/aaa/daemons/configd.c"->get_global_config("alipay_recharge_url"))) || (strlen(alipay_url) < 1))) {
        return "";
    }
    str_serv = convert_to_url_encode(dist);
    sign_key = (((account + str_serv) + rmb) + "KfEqyUUHllKPz37EVYyUzGQvgM3ck7yFO0EVwCqQhxrlMwFG9zI9OhvYfHMwUiHHgAhl9Uk7Jv4");
    sign_key_md5 = save_string(md5(sign_key))[1..<2];
    return sprintf("%s?gameId=%d&serverName=%s&userId=%d&account=%s&rmb=%d&sign=%s", alipay_url, 2, str_serv, 0, account, rmb, sign_key_md5);

}

// --------------------------------------------------------------------
// @brief  event handler: respond to event
// @code  nc=65, nargs=3, insts=65
// --------------------------------------------------------------------
private void handle_http_rsp(int request_type, mixed rsp, mapping info) {
    /* ---- Enhanced Pseudocode ---- */
    mixed result;
    mixed result_code;
    mixed gold;

    if (is_int(rsp)) {
        handle_internal_rsp(request_type, rsp, info);
        return;
    }
    if ((sscanf(rsp, "%d|%d|%d", &result, &result_code, &gold) != 3)) {
        ++wrong_rsp_stat;
        fill_result_info(info, 9, 101, 0);
        to_inquiry(request_type, info);
        return;
    }
    fill_result_info(info, result, result_code, gold);
    switch (result) {
        case 2:
        case 3:
        to_over(request_type, info);
        return;
        case 0:
        default:
        to_inquiry(request_type, info);
        return;
    }  // end switch

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=64, nargs=0, insts=64
// --------------------------------------------------------------------
private void inquiry_daemon() {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed arr;
    mixed sql_cmd;
    mapping m;

    set_as_daemon();
    sql_cmd = mysql_bind_parameters("select order_id from express_recharge where result=? and inquiry_times < ? limit ?", 0, 6, 100);
    while (1) {
        do {
            if ((!config_ok)) {
                break;
            }
            cno = "/daemons/dbd.c"->assure_connection("adb");
            if ((cno == -1)) {
                break;
            }
            arr = mysql_select(cno, sql_cmd, 100);
            if ((!is_array(arr))) {
                break;
            }
            loop (m in arr) {
                do_inquiry(m["order_id"], 1);
            }  // end loop (verified)
        } while (0);

        // --- label L0057 ---
        sleep(120);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=109, nargs=1, insts=109
// --------------------------------------------------------------------
private int create_order_log(mapping info) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    int result;
    mixed sql_cmd;

    if ((!is_string(info["order_id"]))) {
        return 1;
    }
    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 1)) {
        return 0;
    }
    if (((info["result"] == 9) && can_record_defined_error[info["result_code"]])) {
        info["result"] = 0;
    }
    info["update_time"] = (info["insert_time"] = ctime(time(), 3));
    sql_cmd = convert_mapping_to_sql_cmd("insert", "express_recharge", info, "order_id");
    result = (*f_execute)(cno, sql_cmd);
    if ((result < 0)) {
        catch(error("Execute %s failed1.\n", sql_cmd));

        // --- label L0084 ---
        return 0;
    }
    if ((result < 1)) {
        catch(error("Execute %s failed12.\n", sql_cmd));

        // --- label L0101 ---
        return 1;
    }
    return 2;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=36, nargs=0, insts=36
// --------------------------------------------------------------------
void create_adb_table() {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno == -1)) {
        printf("[1;31mÁ¬½Ó ADB Êý¾Ý¿âÊ§°Ü\n[2;37;0m");
        return;
    }
    (*f_execute)(cno, "drop table express_recharge");
    (*f_execute)(cno, "CREATE TABLE express_recharge (order_id              VARCHAR(16),result                TINYINT(2),result_code           INT,gold                  INT,inquiry_times         INT,card_no               VARCHAR(30),card_pwd              VARCHAR(20),user_name             VARCHAR(128),char_name             VARCHAR(32),server_name           VARCHAR(30),sales_name            VARCHAR(128),user_ip               VARCHAR(15),par_value             INT,pay_type              VARCHAR(30),update_time           VARCHAR(14),insert_time           VARCHAR(14),memo                  TEXT,PRIMARY KEY           (order_id))");
    printf("%s", (*f_get_error)(cno));

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=7, nargs=0, insts=7
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    register_post_init((: do_init :));

}

// --------------------------------------------------------------------
// @brief  event handler: respond to event
// @code  nc=32, nargs=3, insts=32
// --------------------------------------------------------------------
private void handle_internal_rsp(int request_type, int rsp, mapping info) {
    /* ---- Enhanced Pseudocode ---- */
    switch (rsp) {
        case -3:
        fill_result_info(info, 9, 102, 0);
        to_inquiry(request_type, info);
        return;
        case -2:
        case -1:
        fill_result_info(info, 9, 100, 0);
        return;
        default:
        return;
    }  // end switch

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=100, nargs=1, insts=100
// --------------------------------------------------------------------
private string get_entire_inquiry_url(mapping info) {
    /* ---- Enhanced Pseudocode ---- */
    mixed order_id;
    mixed server_name;
    mixed pay_type;
    mixed str_serv;
    mixed str_md5;
    mixed sign_key;
    mixed addr_url;

    order_id = info["order_id"];
    server_name = info["server_name"];
    pay_type = info["pay_type"];
    str_serv = convert_to_url_encode(server_name);
    switch (pay_type) {
        case "S19Pay":
        if ((strlen(addr_url = "/aaa/daemons/configd.c"->get_global_config("mobile_inquiry_url")) < 1)) {
            return "";
        }
        sign_key = (((order_id + 2) + str_serv) + "KfEqyUUHllKPz37EVYyUzGQvgM3ck7yFO0EVwCqQhxrlMwFG9zI9OhvYfHMwUiHHgAhl9Uk7Jv4");
        str_md5 = save_string(md5(sign_key))[1..<2];
        return sprintf("%s?orderNo=%s&game=%d&serverName=%s&signKey=%s", addr_url, order_id, 2, str_serv, str_md5);
        default:
        sign_key = (((order_id + str_serv) + pay_type) + "KfEqyUUHllKPz37EVYyUzGQvgM3ck7yFO0EVwCqQhxrlMwFG9zI9OhvYfHMwUiHHgAhl9Uk7Jv4");
        str_md5 = save_string(md5(sign_key))[1..<2];
        return sprintf("%s?OrderId=%s&ServerName=%s&PayType=%s&SignKey=%s", inquiry_url, order_id, str_serv, pay_type, str_md5);
    }  // end switch

}

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=36, nargs=2, insts=36
// --------------------------------------------------------------------
void do_recharge(mapping info, mixed cb) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!config_ok)) {
        return;
    }
    if (("S19Pay" == info["pay_type"])) {
        info["card_type"] = (int)card_type_to_gy[info["card_type"]];
    }
    "/daemons/threadd.c"->dispatch(get_thread_id_by_order_id(info["order_id"]), (: _do_recharge, info, cb :));

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=14, nargs=1, insts=14
// --------------------------------------------------------------------
private int get_thread_id_by_order_id(string order_id) {
    /* ---- Enhanced Pseudocode ---- */
    mixed thread_id;

    sscanf(order_id[<6..<1], "%x", &thread_id);
    return thread_id;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        23
   Code segments found:         23
   Successfully decoded:        23
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */