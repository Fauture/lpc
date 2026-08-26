/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_cfg_write_etc.o
 * Source: /aaa/daemons/server/cmd_cfg_write_etc.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 12, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void write_etc(mixed agent, string dist_name, string section, string key, buffer value, string version, int cookie);
void main(mixed agent, string dist_name, string section, string key, buffer value, string version, int cookie);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  data writer: persist information
// @code  nc=145, nargs=7, insts=145
// --------------------------------------------------------------------
private void write_etc(mixed agent, string dist_name, string section, string key, buffer value, string version, int cookie) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cno;
    int ret;
    mixed arr;
    mixed sql_cmd;

    ret = 0;
    if ((!is_object(agent))) {
        return;
    }
    if ((section == "global")) {
        version = "";
    }
    cno = "/daemons/dbd.c"->assure_connection("adb");
    if ((cno == -1)) {
        printf("[1;31mÁ¬½Ó ADB Êý¾Ý¿âÊ§°Ü\n[2;37;0m");
        return;
    }
    sql_cmd = mysql_bind_parameters("select * from config where version = ? and dist = ? and section = ? and name=?", version, dist_name, section, key);
    arr = mysql_select(cno, sql_cmd);
    if ((is_array(arr) && sizeof(arr))) {
        cno = "/daemons/dbd.c"->assure_connection("adb");
        if ((cno == -1)) {
            printf("[1;31mÁ¬½Ó ADB Êý¾Ý¿âÊ§°Ü\n[2;37;0m");
            return;
        }
        sql_cmd = mysql_bind_parameters("update config set value = ? where dist = ? and section = ? and name = ? and version = ?", (string)value, dist_name, section, key, version);
        ret = (*f_execute)(cno, sql_cmd);
    } else {
        cno = "/daemons/dbd.c"->assure_connection("adb");
        if ((cno == -1)) {
            printf("[1;31mÁ¬½Ó ADB Êý¾Ý¿âÊ§°Ü\n[2;37;0m");
            return;
        }
        sql_cmd = mysql_bind_parameters("insert into config (version, dist, section, name, value) values (?, ?, ?, ?, ?)", version, dist_name, section, key, (string)value);
        ret = (*f_execute)(cno, sql_cmd);
        if ((ret < 0)) {
            ret = 0;
        }
    }
    agent->send_message(9331, cookie, ret);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=18, nargs=7, insts=18
// --------------------------------------------------------------------
void main(mixed agent, string dist_name, string section, string key, buffer value, string version, int cookie) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch(random(10), (: write_etc, agent, dist_name, section, key, value, version, cookie :));

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