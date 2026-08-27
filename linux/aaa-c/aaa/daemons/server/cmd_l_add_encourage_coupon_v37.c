/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_add_encourage_coupon.o
 * Source: /aaa/daemons/server/cmd_l_add_encourage_coupon.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 14, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

int add_coupon(mixed agent, string dist, string account, string gid, string type, int cost_once, string para, string expired_time = "");
void main(mixed agent, string dist, string account, string gid, string type, int cost_once, string para);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=123, nargs=8, insts=123
// --------------------------------------------------------------------
int add_coupon(mixed agent, string dist, string account, string gid, string type, int cost_once, string para, string expired_time) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;
    mixed checksum;
    mixed cur_ti;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((-1 == cno)) {
        catch(error("Failed to connect adb! %s, %s, %s, %s, %s\n", account, gid, type, para, expired_time));

        // --- label L0028 ---
        return 0;
    }
    if ((!strlen(type))) {
        catch(error("Type is lost! %s, %s, %s, %s, %s\n", account, gid, type, para, expired_time));

        // --- label L0049 ---
        return 0;
    }
    checksum = save_string(md5(iconv(sprintf("%s$%s$%d$%s", account, type, 0, "201004"), "utf-8", "GBK")))[1..<2];
    cur_ti = ctime(time(), 3);
    sql_cmd = mysql_bind_parameters("insert into encourage  (account,  encourage_type, order_id, dist, used, checksum, insert_time, expired_time, memo)  select ?, ?, concat(?, lpad(floor(100000 * rand()), 5, '0')), ?, ?, ?, ?, ?, ? ", account, type, cur_ti, dist, 0, checksum, cur_ti, expired_time, (cost_once ? (string)cost_once : ""));
    if (((*f_execute)(cno, sql_cmd) < 0)) {
        catch(error("Failed to insert into encourage! %s, %s, %s, %s\n", account, gid, type, para));

        // --- label L0115 ---
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=24, nargs=7, insts=24
// --------------------------------------------------------------------
void main(mixed agent, string dist, string account, string gid, string type, int cost_once, string para) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: add_coupon, agent, dist, account, gid, type, cost_once, para :));

}

/* ================================================================
   DECLARED FUNCTIONS (no bytecode - likely inherited or forward decl)
   ================================================================ */
/*
   private void do_encourage_log(string dist, string account, string gid, string type, string id, string cookie);  // inherited from parent or forward declaration
*/

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        2
   Code segments found:         2
   Successfully decoded:        2
   Unresolved/minimal:          0
   Functions without code:      1
   Inherited programs:          1
      - /inherit/misc/handler.c()

   Note: Functions without code are usually inherited from the
   parent object (), forward declarations, or
   simple property accessor methods that the compiler optimizes away.
   ================================================================ */