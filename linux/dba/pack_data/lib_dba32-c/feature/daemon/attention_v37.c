/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: attention.o
 * Source: /feature/daemon/attention.c
 * Inherits: (none)
 * Functions: 2 names, 2 code segments found
 * Strings: 0, Variables: 1
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   mapping attention_info;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void register_attention(string key, function f);
void post_attention(string key, mixed para);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   attention_info = ([ ]);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=30, nargs=2, insts=30
// --------------------------------------------------------------------
void register_attention(string key, function f) {
    /* ---- Enhanced Pseudocode ---- */
    mixed funcs;

    if ((!is_array(funcs = attention_info[key]))) {
        attention_info[key] = ({ f });
        return;
    }
    attention_info[key] = (funcs + ({ f }));

}

// --------------------------------------------------------------------
// @code  nc=48, nargs=2, insts=48
// --------------------------------------------------------------------
void post_attention(string key, mixed para) {
    /* ---- Enhanced Pseudocode ---- */
    int update_flag;
    mixed funcs;
    mixed f;

    if ((!is_array(funcs = attention_info[key]))) {
        return;
    }
    loop (f in funcs) {
        if ((!is_function(f))) {
            update_flag = 1;
            continue;
        }
        try {
            (*f)(para);
        } catch { /* error handled */ }

        // --- label L0035 ---
    }  // end loop (verified)
    if (update_flag) {
        clean_array(attention_info[key], 9);
    }

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        2
   Code segments found:         2
   Successfully decoded:        2
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */