/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: ecardd.o
 * Source: /aaa/daemons/ecardd.c
 * Inherits: (none)
 * Functions: 22 names, 22 code segments found
 * Strings: 77, Variables: 7
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   int IDseq;
   int config_ok;
   string ecard_surl;
   string ecard_url;
   string ecard_vurl;
   mapping processing;
   int web_status;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void ecard_business_done(mapping m);
private string get_entire_url(mapping info);
int filter_delete_sale_flag_ecard(mapping &m);
private void record_ecard_business(mapping m);
int does_it_resending(mapping m);
private void destruct();
int get_web_status();
private void do_init();
private void handle_result(mapping info, mixed &result);
int query_processing(string buyer_name);
private void record_ecard_for_resend(mapping m);
void test_if_have_ecard_business(object agent, string user_name, string user_gid, string ecard_id, string cookie);
void create_adb_table();
private void create();
void delete_processing(string buyer_name);
private void log_to_adb(mapping m, int dir, mixed result = 0);
private int test_web_status();
private void ecard_daemon();
void set_processing(string buyer_name);
mapping fill_ecard(mapping info);
private string choose_one_url(string url_list);
int is_config_ok();

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   IDseq = 1;  // /aaa/daemons/ecardd.c:36
   config_ok = 0;
   web_status = 0;
   processing = ([ ]);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=95, nargs=1, insts=95
// --------------------------------------------------------------------
void ecard_business_done(mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno == -1)) {
        printf("[1;31m¡¨Ω” ADB  ˝æ›ø‚ ß∞‹\n[2;37;0m");
        catch(error("ecard connection fail : m=%O\n", m));

        // --- label L0025 ---
        return;
    }
    if ((m["action"] == "sell_add_money")) {
        sql_cmd = mysql_bind_parameters("UPDATE ecard_business SET seller_done = 1, update_time = date_format(CURRENT_TIMESTAMP, '%Y%m%d%H%i%s') WHERE id = ? and seller_done <> 1", m["id"]);
        if (((*f_execute)(cno, sql_cmd) < 1)) {
            catch(error("sql cmd fail : m = %O\n", m));

            // --- label L0058 ---
        }
    } else if ((m["action"] == "buy_clear_flag")) {
        sql_cmd = mysql_bind_parameters("UPDATE ecard_business SET buyer_done = 1, update_time = date_format(CURRENT_TIMESTAMP, '%Y%m%d%H%i%s') WHERE id = ? and buyer_done <> 1", m["id"]);
        if (((*f_execute)(cno, sql_cmd) < 1)) {
            catch(error("sql cmd fail : m = %O\n", m));

            // --- label L0091 ---
        }
    }

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=160, nargs=1, insts=160
// --------------------------------------------------------------------
private string get_entire_url(mapping info) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    mixed bufMd5;
    mixed CardNO;
    mixed CardPWD;
    mixed OrderID;
    mixed UserName;
    mixed ServerName;
    mixed SalesName;
    mixed SalesIP;
    string ParValue;
    mixed SignKey;
    string strMd5;
    mixed strServ;
    mixed url;

    CardNO = info["ecard_no"];
    CardPWD = info["ecard_pwd"];
    UserName = info["ecard_buyer_account"];
    ServerName = info["ecard_dist"];
    SalesName = (is_string(info["ecard_seller_account"]) ? info["ecard_seller_account"] : "");
    SalesIP = (is_string(info["seller_ip"]) ? info["seller_ip"] : "");
    ParValue = (string)info["ecard_type"];
    OrderID = ctime(time(), 3)[2..<1];
    OrderID += sprintf("%08d", IDseq++);
    info["ecard_order_id"] = OrderID;
    strServ = convert_to_url_encode(ServerName);
    SignKey = ((((((((CardNO + CardPWD) + OrderID) + UserName) + strServ) + SalesName) + SalesIP) + ParValue) + "KfEqyUUHllKPz37EVYyUzGQvgM3ck7yFO0EVwCqQhxrlMwFG9zI9OhvYfHMwUiHHgAhl9Uk7Jv4");
    bufMd5 = md5(SignKey);
    strMd5 = "";
    i = 0;

    // --- label L0122 ---
    while ((i < sizeof(bufMd5))) {
        strMd5 += sprintf("%02X", bufMd5[i]);
        ++i;
    }  // end loop (verified)
    url = sprintf("%s?CardNO=%s&CardPWD=%s&OrderID=%s&UserName=%s&ServerName=%s&SalesName=%s&SalesIP=%s&ParValue=%s&SignKey=%s", ecard_url, CardNO, CardPWD, OrderID, UserName, strServ, SalesName, SalesIP, ParValue, strMd5);
    return url;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=263, nargs=1, insts=263
// --------------------------------------------------------------------
int filter_delete_sale_flag_ecard(mapping &m) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed ecard_ids;
    mixed del_ids;
    mixed ecard_ids_temp;
    mixed arr;
    mixed arr_temp;
    mixed sql_cmd;
    mapping mks;

    del_ids = ({});
    if (((!is_array(ecard_ids = m["sale_ecard_ids"])) || (!sizeof(ecard_ids)))) {
        return 0;
    }
    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno == -1)) {
        printf("[1;31m¡¨Ω” ADB  ˝æ›ø‚ ß∞‹\n[2;37;0m");
        return 0;
    }
    sql_cmd = sprintf("SELECT distinct ecard_id FROM ecard_business WHERE ecard_id in ('%s') AND (ecard_seller = ? OR ecard_seller = ?) AND seller_done = 1 ", implode(ecard_ids, "','"));
    sql_cmd = mysql_bind_parameters(sql_cmd, m["user_name"], m["user_gid"]);
    ecard_ids_temp = ({});
    if ((is_array(arr = mysql_select(cno, sql_cmd, 100)) && sizeof(arr))) {
        loop (mks in arr) {
            ecard_ids_temp += ({ mks["ecard_id"] });
        }  // end loop (verified)
    }
    del_ids += ecard_ids_temp;
    ecard_ids -= ecard_ids_temp;
    sql_cmd = sprintf("SELECT distinct ecard_id FROM ecard_log WHERE ecard_id in ('%s') AND (ecard_seller = ? OR ecard_seller = ?) AND result in (1, 15, 17) ", implode(ecard_ids, "','"));
    sql_cmd = mysql_bind_parameters(sql_cmd, m["user_name"], m["user_gid"]);
    ecard_ids_temp = ({});
    if ((is_array(arr = mysql_select(cno, sql_cmd, 100)) && sizeof(arr))) {
        loop (mks in arr) {
            ecard_ids_temp += ({ mks["ecard_id"] });
        }  // end loop (verified)
    }
    del_ids += ecard_ids_temp;
    ecard_ids -= ecard_ids_temp;
    sql_cmd = sprintf("SELECT distinct ecard_id FROM ecard_log WHERE ecard_id in ('%s') AND (ecard_seller = ? OR ecard_seller = ?) AND result = 2 ", implode(ecard_ids, "','"));
    sql_cmd = mysql_bind_parameters(sql_cmd, m["user_name"], m["user_gid"]);
    ecard_ids_temp = ({});
    if ((is_array(arr = mysql_select(cno, sql_cmd, 100)) && sizeof(arr))) {
        loop (mks in arr) {
            ecard_ids_temp += ({ mks["ecard_id"] });
        }  // end loop (verified)
        sql_cmd = sprintf("SELECT distinct ecard_id FROM ecard_need_to_resend WHERE ecard_id in ('%s') ", implode(ecard_ids_temp, "','"));
        sql_cmd = mysql_bind_parameters(sql_cmd);
        if (is_array(arr_temp = mysql_select(cno, sql_cmd, 100))) {
            loop (mks in arr_temp) {
                ecard_ids_temp -= ({ mks["ecard_id"] });
            }  // end loop (verified)
        } else {
            ecard_ids_temp = ({});
        }
    }
    del_ids += ecard_ids_temp;
    ecard_ids -= ecard_ids_temp;
    if ((!sizeof(del_ids))) {
        return 0;
    }
    m["sale_ecard_ids"] = del_ids;
    return 1;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=90, nargs=1, insts=90
// --------------------------------------------------------------------
private void record_ecard_business(mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno == -1)) {
        printf("[1;31m¡¨Ω” ADB  ˝æ›ø‚ ß∞‹\n[2;37;0m");
        catch(error("ecard connection fail : m=%O\n", m));

        // --- label L0025 ---
        return;
    }
    sql_cmd = mysql_bind_parameters("INSERT INTO ecard_business (ecard_no, ecard_id, ecard_server, ecard_seller, ecard_buyer, ecard_buyer_account, ecard_price, ecard_type, ecard_time, action, buyer_done, seller_done, insert_time, update_time)VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, date_format(CURRENT_TIMESTAMP, '%Y%m%d%H%i%s'),date_format(CURRENT_TIMESTAMP, '%Y%m%d%H%i%s'))", m["ecard_no"], m["ecard_id"], m["ecard_server"], m["ecard_seller"], m["ecard_buyer"], m["ecard_buyer_account"], m["ecard_price"], m["ecard_type"], m["ecard_time"], 0, 0, 0);
    if (((*f_execute)(cno, sql_cmd) < 1)) {
        catch(error("sql cmd fail : m = %O\n", m));

        // --- label L0079 ---
    }
    m["id"] = (*f_get_insert_id)(cno);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=53, nargs=1, insts=53
// --------------------------------------------------------------------
int does_it_resending(mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed set;
    mixed sql_cmd;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno == -1)) {
        printf("[1;31m¡¨Ω” ADB  ˝æ›ø‚ ß∞‹\n[2;37;0m");
        catch(error("ecard connection fail : m=%O\n", m));

        // --- label L0025 ---
        return 1;
    }
    sql_cmd = mysql_bind_parameters("SELECT * FROM ecard_need_to_resend WHERE ecard_id = ? AND send_result = ?", m["ecard_id"], -1);
    set = mysql_select(cno, sql_cmd);
    return (sizeof(set) ? 1 : 0);

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=7, nargs=0, insts=7
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    delete_thread("ecard_thread");

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int get_web_status() {
    /* ---- Enhanced Pseudocode ---- */
    return web_status;

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=68, nargs=0, insts=68
// --------------------------------------------------------------------
private void do_init() {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;


    // --- label L0002 ---
    while (is_undefined(m = "/aaa/daemons/configd.c"->get_my_info())) {
        sleep(5);
    }  // end loop (verified)
    ecard_url = choose_one_url(m["ecard_fill_url"]);
    ecard_vurl = choose_one_url(m["ecard_resend_url"]);
    ecard_surl = choose_one_url(m["ecard_web_status_url"]);
    if ((((ecard_url == "") || (ecard_vurl == "")) || (ecard_surl == ""))) {
        broadcast("»±…Ÿµ„ø®Ωª“◊œµÕ≥µƒweb“≥√Ê≈‰÷√\n");
        config_ok = 0;
    } else {
        config_ok = 1;
    }
    create_thread("ecard_thread", "ecard_daemon");

}

// --------------------------------------------------------------------
// @brief  event handler: respond to event
// @code  nc=53, nargs=2, insts=53
// --------------------------------------------------------------------
private void handle_result(mapping info, mixed &result) {
    /* ---- Enhanced Pseudocode ---- */
    if ((is_string(result) && (strlen(result) > 2))) {
        result = "99";
    } else if ((result == "0")) {
        record_ecard_business(info);
    } else if ((result == -3)) {
        record_ecard_for_resend(info);
        result = "98";
    } else {
        result = atoi(result);
        if ((result <= 0)) {
            result = "99";
        }
    }

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=9, nargs=1, insts=9
// --------------------------------------------------------------------
int query_processing(string buyer_name) {
    /* ---- Enhanced Pseudocode ---- */
    return (int)processing[buyer_name];

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=90, nargs=1, insts=90
// --------------------------------------------------------------------
private void record_ecard_for_resend(mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno == -1)) {
        printf("[1;31m¡¨Ω” ADB  ˝æ›ø‚ ß∞‹\n[2;37;0m");
        catch(error("ecard connection fail : m=%O\n", m));

        // --- label L0025 ---
        return;
    }
    sql_cmd = mysql_bind_parameters("INSERT INTO ecard_need_to_resend (ecard_no, ecard_pwd, ecard_id, ecard_server, ecard_seller, ecard_buyer, ecard_buyer_account, ecard_price, ecard_type, ecard_dist, ecard_time, ecard_order_id, send_result, memo, insert_time, update_time)VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, date_format(CURRENT_TIMESTAMP, '%Y%m%d%H%i%s'),date_format(CURRENT_TIMESTAMP, '%Y%m%d%H%i%s'))", m["ecard_no"], m["ecard_pwd"], m["ecard_id"], m["ecard_server"], m["ecard_seller"], m["ecard_buyer"], m["ecard_buyer_account"], m["ecard_price"], m["ecard_type"], m["ecard_dist"], m["ecard_time"], m["ecard_order_id"], -1, "");
    if (((*f_execute)(cno, sql_cmd) < 1)) {
        catch(error("sql cmd fail : m = %O\n", m));

        // --- label L0087 ---
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=220, nargs=5, insts=220
// --------------------------------------------------------------------
void test_if_have_ecard_business(object agent, string user_name, string user_gid, string ecard_id, string cookie) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed set;
    mixed sql_cmd;
    mixed map_ret;
    mapping mks;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno == -1)) {
        printf("[1;31m¡¨Ω” ADB  ˝æ›ø‚ ß∞‹\n[2;37;0m");
        return;
    }
    sql_cmd = mysql_bind_parameters("SELECT * FROM ecard_business WHERE (ecard_seller = ? OR ecard_seller = ?) AND seller_done = 0", user_name, user_gid);
    set = mysql_select(cno, sql_cmd, 9999);
    if ((is_array(set) && (sizeof(set) > 0))) {
        loop (mks in set) {
            mks["action"] = "sell_add_money";
            map_ret = save_string(mks);
            agent->send_message(17287, "aaa_to_gs_ack_has_business", map_ret, cookie);
        }  // end loop (verified)
    }
    if (((int)processing[user_gid] > 0)) {
        return;
    }
    sql_cmd = mysql_bind_parameters("SELECT * FROM ecard_business WHERE (ecard_buyer = ? OR ecard_buyer = ?) AND buyer_done = 0", user_name, user_gid);
    set = mysql_select(cno, sql_cmd, 9999);
    if ((!is_array(set))) {
        return;
    }
    if ((is_array(set) && (sizeof(set) > 0))) {
        loop (mks in set) {
            mks["action"] = "buy_clear_flag";
            map_ret = save_string(mks);
            agent->send_message(17287, "aaa_to_gs_ack_has_business", map_ret, cookie);
        }  // end loop (verified)
    }
    sql_cmd = mysql_bind_parameters("SELECT * FROM ecard_need_to_resend WHERE (ecard_buyer = ? OR ecard_buyer = ?) and send_result = ?", user_name, user_gid, -1);
    set = mysql_select(cno, sql_cmd, 9999);
    if ((!is_array(set))) {
        return;
    }
    if ((is_array(set) && (sizeof(set) > 0))) {
        loop (mks in set) {
            mks["action"] = "buy_timeout_wait";
            map_ret = save_string(mks);
            agent->send_message(17287, "aaa_to_gs_ack_has_business", map_ret, cookie);
        }  // end loop (verified)
    } else {
        mks = ([ ]);
        mks["action"] = "buy_return_money";
        mks["ecard_id"] = ecard_id;
        map_ret = save_string(mks);
        agent->send_message(17287, "aaa_to_gs_ack_has_business", map_ret, cookie);
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=35, nargs=0, insts=35
// --------------------------------------------------------------------
void create_adb_table() {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno == -1)) {
        printf("[1;31m¡¨Ω” ADB  ˝æ›ø‚ ß∞‹\n[2;37;0m");
        return;
    }
    (*f_execute)(cno, "CREATE TABLE ecard_log (id             MEDIUMINT NOT NULL AUTO_INCREMENT,ecard_no              VARCHAR(30),ecard_pwd             VARCHAR(20),ecard_id              VARCHAR(100),ecard_server          VARCHAR(30),ecard_seller          VARCHAR(128),ecard_buyer           VARCHAR(128),ecard_buyer_account   VARCHAR(128),ecard_price           INT,ecard_type            INT,ecard_time            VARCHAR(30),direction             VARCHAR(10),result                VARCHAR(10),memo                  TEXT,insert_time           VARCHAR(14),update_time           VARCHAR(14),PRIMARY KEY           (id))");
    (*f_execute)(cno, "CREATE TABLE ecard_business (id             MEDIUMINT NOT NULL AUTO_INCREMENT,ecard_no             VARCHAR(30),ecard_id             VARCHAR(100),ecard_server         VARCHAR(30),ecard_seller         VARCHAR(128),ecard_buyer          VARCHAR(128),ecard_buyer_account  VARCHAR(128),ecard_price          INT,ecard_type           INT,ecard_time           VARCHAR(30),action               VARCHAR(10),buyer_done           INT,seller_done          INT,insert_time          VARCHAR(14),update_time          VARCHAR(14),PRIMARY KEY          (id))");
    (*f_execute)(cno, "CREATE TABLE ecard_need_to_resend (id             MEDIUMINT NOT NULL AUTO_INCREMENT,ecard_no             VARCHAR(30),ecard_pwd            VARCHAR(20),ecard_id             VARCHAR(100),ecard_server         VARCHAR(30),ecard_seller         VARCHAR(128),ecard_buyer          VARCHAR(128),ecard_buyer_account  VARCHAR(128),ecard_price          INT,ecard_type           INT,ecard_dist           VARCHAR(30),ecard_time           VARCHAR(30),ecard_order_id       TEXT,send_result          INT,memo                 TEXT,insert_time          VARCHAR(14),update_time          VARCHAR(14),PRIMARY KEY          (id))");

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
// @code  nc=20, nargs=1, insts=20
// --------------------------------------------------------------------
void delete_processing(string buyer_name) {
    /* ---- Enhanced Pseudocode ---- */
    processing[buyer_name] -= 1;
    if ((processing[buyer_name] == 0)) {
        map_delete(processing, buyer_name);
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=100, nargs=3, insts=100
// --------------------------------------------------------------------
private void log_to_adb(mapping m, int dir, mixed result) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;
    string direction;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno == -1)) {
        printf("[1;31m¡¨Ω” ADB  ˝æ›ø‚ ß∞‹\n[2;37;0m");
        catch(error("ecard connection fail : m=%O\n", m));

        // --- label L0027 ---
        return;
    }
    if ((dir == 0)) {
        direction = "∑¢ÀÕ∏¯π‚”Ó";
    } else {
        direction = "Ω” ’◊‘π‚”Ó";
    }
    sql_cmd = mysql_bind_parameters("insert into ecard_log(ecard_no, ecard_pwd, ecard_id, ecard_server, ecard_seller, ecard_buyer, ecard_buyer_account, ecard_price, ecard_type, ecard_time, direction, result, memo, insert_time, update_time) values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, date_format(CURRENT_TIMESTAMP, '%Y%m%d%H%i%s'),date_format(CURRENT_TIMESTAMP, '%Y%m%d%H%i%s'))", m["ecard_no"], m["ecard_pwd"], m["ecard_id"], m["ecard_server"], m["ecard_seller"], m["ecard_buyer"], m["ecard_buyer_account"], m["ecard_price"], m["ecard_type"], ctime(time(), 3), direction, result, "");
    if (((*f_execute)(cno, sql_cmd) < 1)) {
        catch(error("sql cmd fail : m = %O\n", m));

        // --- label L0097 ---
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=19, nargs=0, insts=19
// --------------------------------------------------------------------
private int test_web_status() {
    /* ---- Enhanced Pseudocode ---- */
    mixed result;

    result = "/daemons/http_clientd.c"->http_get_web(ecard_surl);
    web_status = (result == "0");
    return web_status;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=147, nargs=0, insts=147
// --------------------------------------------------------------------
private void ecard_daemon() {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed result;
    int to_update;
    mixed set;
    string sql_cmd;
    mixed url;
    mapping mks;

    set_as_daemon();

    // --- label L0004 ---
    while (1) {
        if ((!test_web_status())) {
            sleep(120);
            continue;
        }
        cno = "/daemons/dbd.c"->assure_connection("adb");
        if ((cno == -1)) {
            printf("[1;31m¡¨Ω” ADB  ˝æ›ø‚ ß∞‹\n[2;37;0m");
            sleep(120);
            continue;
        }
        sql_cmd = mysql_bind_parameters("SELECT * FROM ecard_need_to_resend WHERE send_result = ?", -1);
        set = mysql_select(cno, sql_cmd, 10);
        if ((is_array(set) && (sizeof(set) > 0))) {
            loop (mks in set) {
                url = sprintf("%s?OrderId=%s", ecard_vurl, mks["ecard_order_id"]);
                result = "/daemons/http_clientd.c"->http_get_web(url);
                if ((result == -3)) {
                    continue;
                }
                switch (result) {
                    case "0":
                    record_ecard_business(mks);
                    case "1":
                    case "2":
                    to_update = 1;
                    break;
                    default:
                    to_update = 0;
                    break;
                }  // end switch

                // --- label L0103 ---
                if ((!to_update)) {
                    continue;
                }
                sql_cmd = "UPDATE ecard_need_to_resend SET send_result = ?, update_time = date_format(CURRENT_TIMESTAMP, '%Y%m%d%H%i%s') WHERE ecard_order_id = ? ";
                sql_cmd = mysql_bind_parameters(sql_cmd, atoi(result), mks["ecard_order_id"]);
                if (((*f_execute)(cno, sql_cmd) < 1)) {
                    catch(error("sql cmd fail \n"));

                    // --- label L0136 ---
                }
            }  // end loop (verified)
        }
        sleep(120);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=15, nargs=1, insts=15
// --------------------------------------------------------------------
void set_processing(string buyer_name) {
    /* ---- Enhanced Pseudocode ---- */
    processing[buyer_name] = ((int)processing[buyer_name] + 1);

}

// --------------------------------------------------------------------
// @brief  data mapper: populate structure
// @code  nc=60, nargs=1, insts=60
// --------------------------------------------------------------------
mapping fill_ecard(mapping info) {
    /* ---- Enhanced Pseudocode ---- */
    mixed url;
    mixed result;
    mapping ret;

    ret = info;
    if (does_it_resending(info)) {
        ret["result"] = "97";
        return ret;
    }
    url = get_entire_url(info);
    log_to_adb(info, 0);
    result = "/daemons/http_clientd.c"->http_get_web(url);
    log_to_adb(info, 1, result);
    try {
        handle_result(info, &result);
    } catch { /* error handled */ }

    // --- label L0048 ---
    ret["result"] = (string)result;
    return ret;

}

// --------------------------------------------------------------------
// @code  nc=37, nargs=1, insts=37
// --------------------------------------------------------------------
private string choose_one_url(string url_list) {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr;
    string ret;

    ret = "";
    arr = explode(url_list, ",");
    arr -= ({ "" });
    if ((sizeof(arr) == 0)) {
        return ret;
    }
    ret = trim(arr[random(sizeof(arr))]);
    return ret;

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int is_config_ok() {
    /* ---- Enhanced Pseudocode ---- */
    return config_ok;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        22
   Code segments found:         22
   Successfully decoded:        22
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */