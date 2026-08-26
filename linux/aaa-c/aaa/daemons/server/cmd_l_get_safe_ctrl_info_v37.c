/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_get_safe_ctrl_info.o
 * Source: /aaa/daemons/server/cmd_l_get_safe_ctrl_info.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 11, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _main(object agent, string module, string account, string dist_name, string attrib);
void main(object agent, string module, string account, string dist_name, string attrib);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=154, nargs=5, insts=154
// --------------------------------------------------------------------
private void _main(object agent, string module, string account, string dist_name, string attrib) {
    /* ---- Enhanced Pseudocode ---- */
    mapping ans_info;

    ans_info = ([ ]);
    if (((("all" == attrib) || ("setting_switch" == attrib)) || ("total_switch" == attrib))) {
        ans_info["total_switch"] = "/aaa/daemons/accountd.c"->query_safe_ctrl(account, "total_switch");
        if ((!is_mapping(ans_info["total_switch"]))) {
            ans_info["total_switch"] = ([ ]);
        }
        ans_info["fixed_tm"] = "/aaa/daemons/accountd.c"->query_safe_ctrl(account, "fixed_tm");
        ans_info["fixed_ip"] = "/aaa/daemons/accountd.c"->query_safe_ctrl(account, "fixed_ip");
        ans_info["fixed_city"] = "/aaa/daemons/accountd.c"->query_safe_ctrl(account, "fixed_city");
        if ((!is_mapping(ans_info["fixed_tm"]))) {
            ans_info["fixed_tm"] = ([ ]);
        }
        if ((!is_mapping(ans_info["fixed_ip"]))) {
            ans_info["fixed_ip"] = ([ ]);
        }
        if ((!is_mapping(ans_info["fixed_city"]))) {
            ans_info["fixed_city"] = ([ ]);
        }
        agent->send_message(9104, module, account, attrib, save_string(ans_info));
        return;
    }
    ans_info[attrib] = "/aaa/daemons/accountd.c"->query_safe_ctrl(account, attrib);
    if ((!is_mapping(ans_info[attrib]))) {
        ans_info[attrib] = ([ ]);
    }
    agent->send_message(9104, module, account, attrib, save_string(ans_info));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=22, nargs=5, insts=22
// --------------------------------------------------------------------
void main(object agent, string module, string account, string dist_name, string attrib) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _main, agent, module, account, dist_name, attrib :));

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