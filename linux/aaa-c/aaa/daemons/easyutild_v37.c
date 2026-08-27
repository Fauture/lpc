/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: easyutild.o
 * Source: /aaa/daemons/easyutild.c
 * Inherits: (none)
 * Functions: 6 names, 6 code segments found
 * Strings: 26, Variables: 2
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   int notice_level_up_status;
   int notice_login_status;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

int get_notice_level_up_status();
void notice_level_up(string account, int channel, int action, int ti, int did);
private mixed get_notice_level_up_url(string account, int channel, int action, int ti, int did);
int get_notice_login_status();
void notice_login(string account, mapping para);
private mixed get_notice_login_url(string account, mapping para);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   notice_level_up_status = 1;  // /aaa/daemons/easyutild.c:92
   notice_login_status = 1;

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int get_notice_level_up_status() {
    /* ---- Enhanced Pseudocode ---- */
    return notice_level_up_status;

}

// --------------------------------------------------------------------
// @brief  notification: send message
// @code  nc=28, nargs=5, insts=28
// --------------------------------------------------------------------
void notice_level_up(string account, int channel, int action, int ti, int did) {
    /* ---- Enhanced Pseudocode ---- */
    mixed url;

    if ((!notice_level_up_status)) {
        return;
    }
    if ((!is_string(url = get_notice_level_up_url(account, channel, action, ti, did)))) {
        return;
    }
    "/daemons/http_clientd.c"->http_post_web(url, 15, 65535, "");

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=53, nargs=5, insts=53
// --------------------------------------------------------------------
private mixed get_notice_level_up_url(string account, int channel, int action, int ti, int did) {
    /* ---- Enhanced Pseudocode ---- */
    mixed url;
    mixed path;
    mixed sign;
    mixed sign_md5;

    if ((!is_string(url = "/aaa/daemons/configd.c"->get_global_config("notice_level_up_url")))) {
        return;
    }
    sscanf(url, "http://%*s/%s", &path);
    sign = sprintf("/%saccount=%s&action=%d&channel=%d&distid=%d&timestamp=%d", path, account, action, channel, did, ti);
    sign_md5 = save_string(md5((sign + "sicent@gywd")))[1..<2];
    return sprintf((url + "account=%s&action=%d&channel=%d&distid=%d&timestamp=%d&sign=%s&sign_type=md5"), account, action, channel, did, ti, sign_md5);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int get_notice_login_status() {
    /* ---- Enhanced Pseudocode ---- */
    return notice_login_status;

}

// --------------------------------------------------------------------
// @brief  notification: send message
// @code  nc=25, nargs=2, insts=25
// --------------------------------------------------------------------
void notice_login(string account, mapping para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed url;

    if ((!notice_login_status)) {
        return;
    }
    if ((!is_string(url = get_notice_login_url(account, para)))) {
        return;
    }
    "/daemons/http_clientd.c"->http_post_web(url, 15, 65535, "");

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=77, nargs=2, insts=77
// --------------------------------------------------------------------
private mixed get_notice_login_url(string account, mapping para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed url;
    mixed str_dist;
    mixed str_name;
    mixed str_family;

    if ((!is_string(url = "/aaa/daemons/configd.c"->get_global_config("notice_login_url")))) {
        return;
    }
    str_dist = convert_to_url_encode(para["dist"]);
    str_name = convert_to_url_encode(para["name"]);
    str_family = convert_to_url_encode(para["family"]);
    return sprintf((url + "?account=%s&serverName=%s&roleId=%s&roleName=%s&level=%d&tao=%d&family=%s&goldCoin=%d&silverCoin=%d&firstLoginIp=%s&loginIp=%s&loginTime=%s&roleCreateTime=%s&securityType=%s&timeUsed=%d"), account, str_dist, para["gid"], str_name, para["level"], para["tao"], str_family, para["gold_coin"], para["silver_coin"], para["first_login_ip"], para["last_login_ip"], para["login_ti"], para["create_ti"], para["protect_type"], para["online_time"]);

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        6
   Code segments found:         6
   Successfully decoded:        6
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */