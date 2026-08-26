/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: save.o
 * Source: /feature/common/save.c
 * Inherits: (none)
 * Functions: 6 names, 6 code segments found
 * Strings: 13, Variables: 0
 *********************************************************************/

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private int raw_restore(int auto_recover, mixed f);
int auto_recover_restore(mixed f);
int restore_ex(mixed &restore_detail);
int can_be_saved();
int restore(mixed f);
int save(mixed as_create, mixed f);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=110, nargs=2, insts=110
// --------------------------------------------------------------------
private int raw_restore(int auto_recover, mixed f) {
    /* ---- Enhanced Pseudocode ---- */
    string path;
    string name;
    mixed old_name;
    string branch;
    mixed data;
    mixed ob;

    ob = this_object();
    path = "";
    name = "";
    old_name = ob->get_name();
    if (((!ob->get_save_path(&path, &name, &branch)) || (!is_string(name)))) {
        return 0;
    }
    if ((!branch)) {
        branch = "";
    }
    if (is_function(f)) {
        if (auto_recover) {
            error("Auto recover util not found.\n");
        } else {
            "/daemons/filed.c"->read_content_to(path, name, branch, f);
        }
        return 1;
    }
    if (auto_recover) {
        error("Auto recover util not found.\n");
    } else {
        data = "/daemons/filed.c"->read_content(path, name, branch);
    }
    if ((!is_string(data))) {
        return 0;
    }
    if ((!ob->restore_from_text(data))) {
        return 0;
    }
    if ((is_object(ob) != 1)) {
        return 0;
    }
    if (is_string(old_name)) {
        ob->set_name(old_name);
    }
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
int auto_recover_restore(mixed f) {
    /* ---- Enhanced Pseudocode ---- */
    return raw_restore(1, f);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=134, nargs=1, insts=134
// --------------------------------------------------------------------
int restore_ex(mixed &restore_detail) {
    /* ---- Enhanced Pseudocode ---- */
    string path;
    string name;
    mixed old_name;
    string branch;
    mixed data;
    mixed ob;

    ob = this_object();
    path = "";
    name = "";
    old_name = ob->get_name();
    if (((!ob->get_save_path(&path, &name, &branch)) || (!is_string(name)))) {
        restore_detail = 1;
        return 0;
    }
    if ((!branch)) {
        branch = "";
    }
    data = "/daemons/filed.c"->read_content(path, name, branch);
    if (is_int(data)) {
        switch (data) {
            case -1:
            restore_detail = 2;
            break;
            case -2:
            restore_detail = 3;
            break;
            case -3:
            restore_detail = 4;
            break;
            default:
        }  // end switch

        // --- label L0083 ---
        return 0;
    }
    if ((!is_string(data))) {
        restore_detail = 5;
        return 0;
    }
    if ((!ob->restore_from_text(data))) {
        restore_detail = 6;
        return 0;
    }
    if ((is_object(ob) != 1)) {
        restore_detail = 7;
        return 0;
    }
    if (is_string(old_name)) {
        ob->set_name(old_name);
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int can_be_saved() {
    /* ---- Enhanced Pseudocode ---- */
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
int restore(mixed f) {
    /* ---- Enhanced Pseudocode ---- */
    return raw_restore(0, f);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=86, nargs=2, insts=86
// --------------------------------------------------------------------
int save(mixed as_create, mixed f) {
    /* ---- Enhanced Pseudocode ---- */
    mixed data;
    string path;
    string name;
    string branch;

    path = "";
    name = "";
    data = this_object()->save_to_text();
    if ((!is_string(data))) {
        return 0;
    }
    if (((!this_object()->get_save_path(&path, &name, &branch)) || (!is_string(name)))) {
        return 0;
    }
    if ((!branch)) {
        branch = "";
    }
    if (as_create) {
        if (is_function(f)) {
            "/daemons/filed.c"->create_content_to(path, name, branch, data, f);
            return 1;
        }
        return (0 == "/daemons/filed.c"->create_content(path, name, branch, data));

        // --- label L0072 ---
    }
    "/daemons/filed.c"->write_content_to(path, name, branch, data, f);
    return 1;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        6
   Code segments found:         6
   Successfully decoded:        6
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */