/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: pwdprotect_authd.o
 * Source: /aaa/daemons/pwdprotect_authd.c
 * Inherits: (none)
 * Functions: 7 names, 7 code segments found
 * Strings: 36, Variables: 7
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   int _status;
   int http_timeout;
   string md5_desc;
   mapping protect_log;
   mapping protect_log_status;
   mapping protect_type;
   string url_desc;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mapping auth_protect(string type, string account, string dist);
int get_status();
private void do_init();
private void create();
void set_status(int s);
void log_auth_result(string type, string account, string dist, mixed auth_ret);
private mapping to_auth_protect(string type, string account, string dist, string url);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   http_timeout = 6;  // /aaa/daemons/pwdprotect_authd.c:32
   protect_type = ([ "sm" : "1", "phone" : "2" ]);  // /aaa/daemons/pwdprotect_authd.c:44
   protect_log = ([ "sm" : "sm_auth_result", "phone" : "phone_auth_result" ]);
   protect_log_status = ([ "sm" : 0, "phone" : 0 ]);
   _status = 0;

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=91, nargs=3, insts=91
// --------------------------------------------------------------------
mapping auth_protect(string type, string account, string dist) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    mixed sign_key;
    mixed strServ;
    mixed url;
    mapping m;

    m = ([ "result": 0, "reason": "AUTH_OK" ]);
    if ((!get_status())) {
        return m;
    }
    strServ = convert_to_url_encode(dist);
    sign_key = save_string(md5(sprintf(md5_desc, account, protect_type[type], strServ)))[1..<2];
    url = sprintf(url_desc, account, protect_type[type], strServ, sign_key);
    for (i = 1; i <= 3; i++) {
        m = to_auth_protect(type, account, dist, url);
        if (((m["result"] == 0) || (m["result"] == 1))) {
            return m;
        }
    }  // end loop (verified)
    log_auth_result(type, account, dist, "auth_exception");
    return m;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int get_status() {
    /* ---- Enhanced Pseudocode ---- */
    return _status;

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=80, nargs=0, insts=80
// --------------------------------------------------------------------
private void do_init() {
    /* ---- Enhanced Pseudocode ---- */
    mixed my_domain_name;

    if (((!is_object(find_object("/aaa/daemons/configd.c"))) || (!is_mapping("/aaa/daemons/configd.c"->get_my_info())))) {
        register_post_init((: do_init :));
        return;
    }
    my_domain_name = "/aaa/daemons/configd.c"->get_my_info()["auth_server"];
    if ((!is_string(my_domain_name))) {
        return;
    }
    my_domain_name = trim(my_domain_name);
    if ((is_string(my_domain_name) && sizeof(my_domain_name))) {
        md5_desc = "2%s%s%s2YSgkc7zC)$Cii";
        url_desc = my_domain_name;
        url_desc += "?Game=2";
        url_desc += "&Account=%s";
        url_desc += "&Type=%s";
        url_desc += "&ServerName=%s";
        url_desc += "&SignKey=%s";
        set_status(1);
        return;
    }

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
// @brief  mutator: set property
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
void set_status(int s) {
    /* ---- Enhanced Pseudocode ---- */
    _status = s;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=60, nargs=4, insts=60
// --------------------------------------------------------------------
void log_auth_result(string type, string account, string dist, mixed auth_ret) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    mixed sql_cmd;
    mixed log_type;

    if ((!protect_log_status[type])) {
        return;
    }
    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno < 0)) {
        return;
    }
    sql_cmd = mysql_bind_parameters("insert into acc_log(ip, time,account, dist, type, msg, memo) values(?, ?, ?, ?, ?, ?, ?)", "", ctime(time(), 3)[0..13], account, dist, (is_string(log_type = protect_log[type]) ? log_type : "other_auth_result"), (is_string(auth_ret) ? auth_ret : save_string(auth_ret)), "");
    (*f_execute)(cno, sql_cmd);

}

// --------------------------------------------------------------------
// @brief  converter: transform data
// @code  nc=69, nargs=4, insts=69
// --------------------------------------------------------------------
private mapping to_auth_protect(string type, string account, string dist, string url) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ret;

    ret = "/daemons/http_clientd.c"->http_get_web(url, http_timeout);
    switch (ret) {
        case "succ":
        return ([ "result": 0, "reason": "AUTH_OK" ]);
        case "unbind":
        return ([ "result": 0, "reason": "AUTH_UNBIND" ]);
        case "error":
        return ([ "result": 1, "reason": "AUTH_ERROR" ]);
        case -2:
        return ([ "result": 2, "reason": "URL_ERROR" ]);
        case -1:
        return ([ "result": 2, "reason": "INET_ERROR" ]);
        case -3:
        return ([ "result": 2, "reason": "TIMEOUT_ERROR" ]);
        default:
        return ([ "result": 1, "reason": "OTHER_ERROR" ]);
    }  // end switch

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