/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_cfg_read_etc.o
 * Source: /aaa/daemons/server/cmd_cfg_read_etc.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 16, Variables: 2
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   /* mixed global_0; */
   array must_check_md5_sections;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void read_etc(mixed agent, string dist_name, string section, string key, string version, int cookie);
void main(mixed agent, string dist_name, string section, string key, string version, int cookie);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   must_check_md5_sections = ({ "etc_file" });

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=160, nargs=6, insts=160
// --------------------------------------------------------------------
private void read_etc(mixed agent, string dist_name, string section, string key, string version, int cookie) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    mixed cno;
    mixed etc_len;
    mixed sql_cmd;
    mixed etc;
    mixed check_sum;
    mapping etc_data;

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
    sql_cmd = mysql_bind_parameters("select * from config where dist = ? and section = ? and name = ? and version = ?", dist_name, section, key, version);
    etc_data = mysql_select_single_result(cno, sql_cmd);
    if ((!is_object(agent))) {
        return;
    }
    if (((!is_mapping(etc_data)) || (etc_data["value"] == ""))) {
        agent->send_message(9331, cookie, 0);
        return;
    }
    etc = etc_data["value"];
    if ((member_array(section, must_check_md5_sections) >= 0)) {
        check_sum = save_string(md5(sprintf("version=%ssection=%skey=%svalue=%s", version, section, key, replace_string(etc, " ", "/"))))[1..<2];
        if ((check_sum != etc_data["checksum"])) {
            agent->send_message(9331, cookie, 3);
            return;
        }
    }
    etc_len = strlen(etc);
    i = 0;

    // --- label L0126 ---
    while (i < etc_len) {
        agent->send_message(9329, cookie, lzss_compress(etc[i..((i + 2048) - 1)]));
        i += 2048;
    }  // end loop (verified)
    agent->send_message(9331, cookie, 1);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=17, nargs=6, insts=17
// --------------------------------------------------------------------
void main(mixed agent, string dist_name, string section, string key, string version, int cookie) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch(random(10), (: read_etc, agent, dist_name, section, key, version, cookie :));

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