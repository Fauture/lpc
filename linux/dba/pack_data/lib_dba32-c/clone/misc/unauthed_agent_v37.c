/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: unauthed_agent.o
 * Source: /clone/misc/unauthed_agent.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/agent.c()
 * Functions: 5 names, 5 code segments found
 * Strings: 12, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/agent.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

override void connection_lost();
int is_unauthed_agent();
private void destruct();
override void process_command(string cmd, array args);
private void create();

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=9, nargs=0, insts=9
// --------------------------------------------------------------------
override void connection_lost() {
    /* ---- Enhanced Pseudocode ---- */
    ::connection_lost();
    destruct_object(this_object());

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int is_unauthed_agent() {
    /* ---- Enhanced Pseudocode ---- */
    return 1;

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=29, nargs=0, insts=29
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    mixed mks;
    mixed callback_list;

    if ((!is_mapping(callback_list = query_temp("destruct_callback")))) {
        return;
    }
    loop (mks in callback_list) {
        if (is_function(mks[1])) {
            (*mks[1])();
        }
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @code  nc=34, nargs=2, insts=34
// --------------------------------------------------------------------
override void process_command(string cmd, array args) {
    /* ---- Enhanced Pseudocode ---- */
    switch (cmd) {
        case "cmd_internal_auth":
        case "cmd_ma_l_account":
        case "cmd_l_account":
        case "cmd_internal_auth_ex":
        break;
        case "cmd_l_get_antibot_question":
        case "cmd_l_check_user_data":
        case "cmd_l_auth_pic":
        case "cmd_l_auth_local_encrypt":
        case "cmd_l_get_qr_code_info":
        case "cmd_l_login_by_qr_code":
        break;
        default:
        return;
    }  // end switch

    // --- label L0028 ---
    ::process_command(cmd, args);

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=12, nargs=0, insts=12
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    set_name("unauthed_agent");
    if (is_clone()) {
        to_be_disconnect(60);
    }

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        5
   Code segments found:         5
   Successfully decoded:        5
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          1
      - /inherit/misc/agent.c()
   ================================================================ */