/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_auth_all_protect.o
 * Source: /aaa/daemons/server/cmd_l_auth_all_protect.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 18, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _main(object agent, string account, int check_flag, string ip, string dist, string passpod, string matrix, int seq_no, int step);
void main(object agent, string account, int check_flag, string ip, string dist, string passpod, string matrix, int seq_no, int step);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=226, nargs=9, insts=226
// --------------------------------------------------------------------
private void _main(object agent, string account, int check_flag, string ip, string dist, string passpod, string matrix, int seq_no, int step) {
    /* ---- Enhanced Pseudocode ---- */
    mixed check_result;
    mixed passpod_text;
    mixed passpod_type;
    mixed prompt;
    mapping m;
    mapping extra_info;
    mixed build_info;

    if (("/daemons/dbd.c"->assure_connection("adb") < 0)) {
        return;
    }
    m = ([ ]);
    m["inc_fail"] = 1;
    if ((check_flag & 1)) {
        check_result = "/aaa/daemons/accountd.c"->auth_sm_protect(account, ip, dist, "", &prompt);
        if (is_string(prompt)) {
            m[1] = ([ "res": check_result, "prompt": prompt ]);
        } else {
            m[1] = ([ "res": check_result ]);
        }
    }
    if ((check_flag & 2)) {
        check_result = "/aaa/daemons/accountd.c"->auth_phone_protect(account, ip, dist, "", &prompt);
        if (is_string(prompt)) {
            m[2] = ([ "res": check_result, "prompt": prompt ]);
        } else {
            m[2] = ([ "res": check_result ]);
        }
    }
    if ((check_flag & 4)) {
        extra_info = ([ ]);
        check_result = "/aaa/daemons/accountd.c"->auth_passpod_protect(account, ip, dist, passpod, &prompt, &extra_info);
        if (is_string(prompt)) {
            m[4] = ([ "res": check_result, "prompt": prompt ]);
        } else {
            m[4] = ([ "res": check_result ]);
        }
        if ((sscanf(passpod, "%s|%d", &passpod_text, &passpod_type) == 2)) {
            m["passpod_type"] = passpod_type;
        }
        if ((!extra_info["inc_fail"])) {
            m["inc_fail"] = 0;
        }
    }
    if ((check_flag & 8)) {
        check_result = "/aaa/daemons/accountd.c"->auth_matrix_protect(account, ip, dist, matrix, &prompt, &build_info);
        if (is_string(prompt)) {
            m[8] = ([ "res": check_result, "prompt": prompt ]);
        } else {
            m[8] = ([ "res": check_result ]);
        }
        if (is_mapping(build_info)) {
            m[8]["build_info"] = build_info;
        }
    }
    agent->send_message(21763, account, step, seq_no, save_string(m));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=26, nargs=9, insts=26
// --------------------------------------------------------------------
void main(object agent, string account, int check_flag, string ip, string dist, string passpod, string matrix, int seq_no, int step) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _main, agent, account, check_flag, ip, dist, passpod, matrix, seq_no, step :));

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