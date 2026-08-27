/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_cost_coupon.o
 * Source: /aaa/daemons/server/cmd_l_cost_coupon.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 19, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void cost_coupon(mixed agent, string dist, string gid, string account, string encourage_type, string type, string cookie, string para, int clean_lock, int coupon_result);
void main(object agent, string dist, string gid, string account, string encourage_type, string type, string cookie, string para, int clean_lock, int is_expired);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=212, nargs=10, insts=212
// --------------------------------------------------------------------
private void cost_coupon(mixed agent, string dist, string gid, string account, string encourage_type, string type, string cookie, string para, int clean_lock, int coupon_result) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    int used_flag;
    mixed arr;
    mixed cookie_arr;
    mixed sql_cmd;
    mixed id;
    string checksum;
    mixed request_time;
    mapping encourage;

    checksum = "";
    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((-1 == cno)) {
        printf("[1;31mÁ¬½Ó ADB Êý¾Ý¿âÊ§°Ü\n[2;37;0m");
        return;
    }
    cookie_arr = explode(cookie, "/");
    if ((sizeof(cookie_arr) <= 1)) {
        return;
    }
    id = cookie_arr[0];
    request_time = cookie_arr[1];
    if (clean_lock) {
        sql_cmd = mysql_bind_parameters("update encourage set locked = ? where id = ? ", 0, id);
        (*f_execute)(cno, sql_cmd);
        return;
    }
    sql_cmd = mysql_bind_parameters("select * from encourage where id = ? ", id);
    arr = mysql_select(cno, sql_cmd);
    encourage = arr[0];
    switch (coupon_result) {
        case 1:
        used_flag = 2;
        break;
        case 2:
        used_flag = 4;
        break;
        default:
        used_flag = 1;
        break;
    }  // end switch

    // --- label L0103 ---
    if ((2 == used_flag)) {
        checksum = save_string(md5(iconv(sprintf("%s$%s$%s$%s", encourage["account"], encourage["encourage_type"], (string)used_flag, "201004"), "utf-8", "GBK")))[1..<2];
        sql_cmd = mysql_bind_parameters("update encourage set used=?, locked=?, checksum=? where id = ?", used_flag, 0, checksum, id);
        if (((*f_execute)(cno, sql_cmd) <= 0)) {
            return;
        }
        return;
    }
    checksum = save_string(md5(iconv(sprintf("%s$%s$%s$%s", encourage["account"], encourage["encourage_type"], (string)used_flag, "201004"), "utf-8", "GBK")))[1..<2];
    sql_cmd = mysql_bind_parameters("update encourage set used=?, checksum=?  where id=? ", used_flag, checksum, id);
    if (((*f_execute)(cno, sql_cmd) <= 0)) {
        return;
    }
    if ((used_flag != 1)) {
        return;
    }
    sql_cmd = mysql_bind_parameters("insert into encourage_log(gid, account, encourage_type, request_time, dist, encourage_id, memo) values(?, ?, ?, ?, ?, ?, ?)", gid, account, encourage_type, request_time, dist, id, "");
    (*f_execute)(cno, sql_cmd);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=27, nargs=10, insts=27
// --------------------------------------------------------------------
void main(object agent, string dist, string gid, string account, string encourage_type, string type, string cookie, string para, int clean_lock, int is_expired) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: cost_coupon, agent, dist, gid, account, encourage_type, type, cookie, para, clean_lock, is_expired :));

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        2
   Code segments found:         2
   Successfully decoded:        2
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          1
      - /inherit/misc/handler.c()
   ================================================================ */