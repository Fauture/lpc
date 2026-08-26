/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_request_coupon.o
 * Source: /aaa/daemons/server/cmd_l_request_coupon.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 25, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void request_coupon(mixed agent, string dist, string gid, string account, string type, string para);
void main(object agent, string dist, string gid, string account, string type, string para);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=299, nargs=6, insts=299
// --------------------------------------------------------------------
private void request_coupon(mixed agent, string dist, string gid, string account, string type, string para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed request_time;
    int result;
    mixed ct;
    int can_request;
    mixed arr;
    mixed sql_cmd;
    mixed checksum;
    string coupon_type;
    string cookie;
    mixed request_time_str;
    mapping encourage;
    //mixed <loop_1>;
    mixed encourage_type;

    if ((!is_object(agent))) {
        return;
    }
    result = 3;
    cookie = "";
    coupon_type = "";
    do {
        cno = "/daemons/dbd.c"->assure_connection("adb");
        if ((-1 == cno)) {
            printf("[1;31mÁ¬½Ó ADB Êý¾Ý¿âÊ§°Ü\n[2;37;0m");
            break;
        }
        ct = time();
        sql_cmd = mysql_bind_parameters("select * from encourage where account=? and used=? and (expired_time='' or expired_time>=?) limit 1", account, 0, ctime(ct, 3));
        arr = mysql_select(cno, sql_cmd);
        if ((!is_array(arr))) {
            break;
        }
        if ((sizeof(arr) < 1)) {
            result = 2;
            break;
        }
        can_request = 0;
        loop (encourage in arr) {
            encourage_type = encourage["encourage_type"];
            if ((sizeof(type) && (strsrch(encourage_type, type) < 0))) {
                continue;
            }
            if (((encourage["dist"] == "") || (encourage["dist"] == "*"))) {
                can_request = 1;
                break;
            }
            if (check_for_permit(encourage["dist"], dist, ";")) {
                can_request = 1;
                break;
            }
            if ((!stricmp(encourage["dist"], "webservice"))) {
                can_request = 1;
                break;
            }
        }  // end loop (verified)
        if ((!can_request)) {
            result = 2;
            break;
        }
        if (atoi(encourage["locked"])) {
            request_time = time_desc_to_int(encourage["request_time"], 3);
            if (((request_time + 360) > ct)) {
                result = 1;
                break;
            }
        }
        checksum = save_string(md5(sprintf("%s$%s$%s$%s", encourage["account"], encourage["encourage_type"], encourage["used"], "201004")))[1..<2];
        if (stricmp(checksum, encourage["checksum"])) {
            sql_cmd = mysql_bind_parameters("update encourage set used = ? where id = ?", 3, encourage["id"]);
            (*f_execute)(cno, sql_cmd);
            break;
        }
        request_time_str = ctime(ct, 3);
        sql_cmd = mysql_bind_parameters("update encourage set locked = ?, request_time = ? where id = ? and used = ? and (locked = ? or\r\n            (locked = ? and request_time < ?))", 1, request_time_str, encourage["id"], 0, 0, 1, ctime((ct - 360), 3));
        if (((*f_execute)(cno, sql_cmd) <= 0)) {
            break;
        }
        cookie = sprintf("%s/%s/%s", encourage["id"], request_time_str, encourage["dist"]);
        coupon_type = encourage["encourage_type"];
        result = 0;
    } while (0);

    // --- label L0281 ---
    if ((!is_object(agent))) {
        return;
    }
    agent->send_message(21357, gid, coupon_type, result, type, para, cookie);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=23, nargs=6, insts=23
// --------------------------------------------------------------------
void main(object agent, string dist, string gid, string account, string type, string para) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: request_coupon, agent, dist, gid, account, type, para :));

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