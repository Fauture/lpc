/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_l_set_account_info.o
 * Source: /aaa/daemons/server/cmd_l_set_account_info.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 2 names, 2 code segments found
 * Strings: 10, Variables: 2
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   array forbid_fields;
   /* mixed global_0; */

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _main(string account, string text);
void main(object agent, string account, string text);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   forbid_fields = ({ "account", "gold_coin", "silver_coin" });

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=82, nargs=2, insts=82
// --------------------------------------------------------------------
private void _main(string account, string text) {
    /* ---- Enhanced Pseudocode ---- */
    mixed fields;
    int field;
    mixed para;
    mixed account_info;

    try {
        para = restore_value(text);
    } catch { /* error handled */ }

    // --- label L0012 ---
    if ((0 || (!is_mapping(para)))) {
        return;
    }
    fields = keys(para);
    if ((sizeof((forbid_fields - fields)) != sizeof(forbid_fields))) {
        printf("Including forbid field to update.\n");
        return;
    }
    if ((!is_mapping(account_info = "/aaa/daemons/accountd.c"->query_account_info(account)))) {
        return;
    }
    if ((sizeof((fields - keys(account_info))) > 0)) {
        printf("Including invalid field to update.\n");
        return;
    }
    loop (field in fields) {
        account_info[field] = para[field];
    }  // end loop (verified)
    "/aaa/daemons/accountd.c"->update_account_info(account_info, fields);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=19, nargs=3, insts=19
// --------------------------------------------------------------------
void main(object agent, string account, string text) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _main, account, text :));

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