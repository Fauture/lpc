/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_ecard_business.o
 * Source: /aaa/daemons/server/cmd_l_ecard_business.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 30, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _main(mixed agent, string action, string map_para, string cookie);
void main(mixed agent, string action, string map_para, string cookie);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=149, nargs=4, insts=149
// --------------------------------------------------------------------
private void _main(mixed agent, string action, string map_para, string cookie) {
    /* ---- Enhanced Pseudocode ---- */
    mixed map_ret;
    mapping m;
    mixed ret;

    if (((!is_object(find_object("/aaa/daemons/ecardd.c"))) || (!"/aaa/daemons/ecardd.c"->is_config_ok()))) {
        printf("[1;31mECARD_DÄ£¿éÎ´¼ÓÔØ³É¹¦£¬»òÉÐÎ´ÅäÖÃÍ×µ±¡£ÔÝ²»´¦Àí±¾´ÎÇëÇó¡£\n[2;37;0m");
        return;
    }
    if (((!is_object(agent)) || (!is_mapping(m = restore_value(map_para))))) {
        return;
    }
    switch (action) {
        case "gs_to_aaa_req_buy_ecard":
        ret = "/aaa/daemons/ecardd.c"->fill_ecard(m);
        map_ret = save_string(ret);
        agent->send_message(17287, "aaa_to_gs_ack_buy_ecard", map_ret, cookie);
        "/aaa/daemons/ecardd.c"->delete_processing(m["ecard_buyer"]);
        break;
        case "gs_to_aaa_req_has_business":
        "/aaa/daemons/ecardd.c"->test_if_have_ecard_business(agent, m["user_name"], m["user_gid"], (string)m["ecard_id"], cookie);
        break;
        case "gs_to_aaa_req_business_done":
        "/aaa/daemons/ecardd.c"->ecard_business_done(m);
        break;
        case "gs_to_aaa_req_can_clear":
        if ((!"/aaa/daemons/ecardd.c"->does_it_resending(m))) {
            agent->send_message(17287, "aaa_to_gs_ack_can_clear", map_para, cookie);
        }
        break;
        case "gs_to_aaa_req_web_status":
        m["web_status"] = "/aaa/daemons/ecardd.c"->get_web_status();
        agent->send_message(17287, "aaa_to_gs_ack_web_status", save_string(m), cookie);
        break;
        case "gs_to_aaa_req_can_delete":
        if ((!"/aaa/daemons/ecardd.c"->filter_delete_sale_flag_ecard(&m))) {
            return;
        }
        agent->send_message(17287, "gs_to_aaa_ack_can_delete", save_string(m), cookie);
        break;
        default:
    }  // end switch

    // --- label L0147 ---

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=48, nargs=4, insts=48
// --------------------------------------------------------------------
void main(mixed agent, string action, string map_para, string cookie) {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;

    if ((!is_mapping(m = restore_value(map_para)))) {
        return;
    }
    switch (action) {
        case "gs_to_aaa_req_buy_ecard":
        "/aaa/daemons/ecardd.c"->set_processing(m["ecard_buyer"]);
        break;
        case "gs_to_aaa_req_has_business":
        if (("/aaa/daemons/ecardd.c"->query_processing(m["user_gid"]) > 0)) {
            return;
        }
        default:
    }  // end switch

    // --- label L0035 ---
    "/daemons/threadd.c"->dispatch(random(10), (: _main, agent, action, map_para, cookie :));

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