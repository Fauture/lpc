/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_add_encourage_coupon.o
 * Source: /aaa/daemons/server/cmd_l_add_encourage_coupon.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 11, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

int add_coupon(mixed agent, string dist, string account, string gid, string type, string para);
void main(mixed agent, string dist, string account, string gid, string type, string para);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=104, nargs=6, insts=104
// --------------------------------------------------------------------
int add_coupon(mixed agent, string dist, string account, string gid, string type, string para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    mixed sql_cmd;
    mixed checksum;

    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((-1 == cno)) {
        catch(error("Failed to connect adb! %s, %s, %s, %s\n", account, gid, type, para));

        // --- label L0025 ---
        return 0;
    }
    if ((!strlen(type))) {
        catch(error("Type is lost! %s, %s, %s, %s\n", account, gid, type, para));

        // --- label L0045 ---
        return 0;
    }
    checksum = save_string(md5(sprintf("%s$%s$%d$%s", account, type, 0, "201004")))[1..<2];
    sql_cmd = mysql_bind_parameters("insert into encourage  (account,  encourage_type, dist, used, checksum, insert_time )  values(?, ?, ?, ?, ?, ?) ", account, type, dist, 0, checksum, ctime(time(), 3));
    if (((*f_execute)(cno, sql_cmd) < 0)) {
        catch(error("Failed to insert into encourage! %s, %s, %s, %s\n", account, gid, type, para));

        // --- label L0096 ---
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=23, nargs=6, insts=23
// --------------------------------------------------------------------
void main(mixed agent, string dist, string account, string gid, string type, string para) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: add_coupon, agent, dist, account, gid, type, para :));

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