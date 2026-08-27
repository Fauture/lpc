/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_query_coupon.o
 * Source: /aaa/daemons/server/cmd_l_query_coupon.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 25, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void main(object agent, string dist, string account, string gid, string para);
private void query_coupon(mixed agent, string dist, string account, string gid, string para);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=22, nargs=5, insts=22
// --------------------------------------------------------------------
void main(object agent, string dist, string account, string gid, string para) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: query_coupon, agent, dist, account, gid, para :));

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=274, nargs=5, insts=274
// --------------------------------------------------------------------
private void query_coupon(mixed agent, string dist, string account, string gid, string para) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    mixed cno;
    mixed request_time;
    mixed arr;
    mixed sql_cmd;
    string result;
    string type;
    mixed checksum;
    mapping encourage;
    mixed result_temp;
    int can_query;
    int flag;
    mixed ct;

    if ((!is_object(agent))) {
        return;
    }
    result = "";
    result_temp = ([ ]);
    type = "";
    do {
        cno = "/daemons/dbd.c"->assure_connection("adb");
        if ((cno == -1)) {
            printf("[1;31mÁ¬½Ó ADB Êý¾Ý¿âÊ§°Ü\n[2;37;0m");
            break;
        }
        ct = time();
        sql_cmd = mysql_bind_parameters("select * from encourage where used = ? and account = ? and (expired_time = '' or expired_time >= ?)", 0, account, ctime(ct, 3));
        arr = mysql_select(cno, sql_cmd, 50);
        if ((!is_array(arr))) {
            break;
        }
        if ((sizeof(arr) < 1)) {
            break;
        }
        for (i = 0; i <= (sizeof(arr) - 1); i++) {
            encourage = arr[i];
            flag = 0;
            can_query = 0;
            if ((((encourage["dist"] == "") || (encourage["dist"] == "*")) || check_for_permit(encourage["dist"], dist, ";"))) {
                can_query = 1;
            }
            if ((!stricmp(encourage["dist"], "webservice"))) {
                can_query = 1;
            }
            if ((!can_query)) {
                continue;
            }
            if (atoi(encourage["locked"])) {
                request_time = time_desc_to_int(encourage["request_time"], 3);
                if (((request_time + 360) > ct)) {
                    continue;
                }
            }
            loop (type in keys(result_temp)) {
                if ((type == encourage["encourage_type"])) {
                    flag = 1;
                    break;
                }
            }  // end loop (verified)
            checksum = save_string(md5(iconv(sprintf("%s$%s$%s$%s", encourage["account"], encourage["encourage_type"], encourage["used"], "201004"), "utf-8", "GBK")))[1..<2];
            if (stricmp(checksum, encourage["checksum"])) {
                sql_cmd = mysql_bind_parameters("update encourage set used = ? where id = ?", 3, encourage["id"]);
                (*f_execute)(cno, sql_cmd);
                continue;
            }
            if (flag) {
                ++result_temp[encourage["encourage_type"]];
            } else {
                result_temp[encourage["encourage_type"]] = 1;
            }
        }  // end loop (verified)
        if ((([ ]) == result_temp)) {
            break;
        }
        result = save_string(result_temp);
    } while (0);

    // --- label L0264 ---
    agent->send_message(9217, gid, para, result);

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