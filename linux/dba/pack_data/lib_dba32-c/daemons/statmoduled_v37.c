/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: statmoduled.o
 * Source: /daemons/statmoduled.c
 * Inherits: (none)
 * Inherited programs: /feature/daemon/register.c()
 * Functions: 6 names, 6 code segments found
 * Strings: 25, Variables: 0
 *********************************************************************/

inherit "/feature/daemon/register.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void size_module(mixed ob);
private void destruct();
private void create();
mixed get_variables(mixed ob);
void size_variables(mixed ob);
void size_objects(mixed module);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=10, nargs=1, insts=10
// --------------------------------------------------------------------
void size_module(mixed ob) {
    /* ---- Enhanced Pseudocode ---- */
    size_variables(ob);
    size_objects(ob);

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=12, nargs=0, insts=12
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    register_daemon_data("/daemons/statmoduled.c", "modules", dup(query_all_entries()));
    empty_entries();

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=37, nargs=0, insts=37
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    mapping saved_map;
    mixed m;
    mixed index;

    saved_map = get_daemon_data("/daemons/statmoduled.c");
    if ((is_mapping(saved_map) && is_mapping(m = saved_map["modules"]))) {
        loop (index in keys(m)) {
            register_entry(index, find_object(index));
        }  // end loop (verified)
        unregister_daemon_data("/daemons/statmoduled.c", "modules");
    }

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=124, nargs=1, insts=124
// --------------------------------------------------------------------
mixed get_variables(mixed ob) {
    /* ---- Enhanced Pseudocode ---- */
    mapping info_map;
    mixed vars_map;
    mixed output_map;
    int var_desc;
    int str1;
    int str2;
    int index;
    mixed var_value;

    info_map = info_object(ob);
    vars_map = info_map["variables"];
    output_map = ([ ]);
    loop (var_desc in keys(vars_map)) {
        index = strsrch(var_desc, "::");
        if ((index < 0)) {
            continue;
        }
        str1 = var_desc[0..(index - 1)];
        str2 = var_desc[(index + 2)..<1];
        index = strsrch(str2, "*");
        if ((index < 0)) {
            index = strsrch(str2, "(");
            if ((index >= 0)) {
                str2 = str2[0..(index - 1)];
            }
        } else {
            str2 = str2[0..(index - 1)];
        }
        if ((!output_map[str1])) {
            output_map[str1] = ([ ]);
        }
        var_value = vars_map[var_desc];
        output_map[str1][str2] = var_value;
    }  // end loop (verified)
    return output_map;

}

// --------------------------------------------------------------------
// @code  nc=148, nargs=1, insts=148
// --------------------------------------------------------------------
void size_variables(mixed ob) {
    /* ---- Enhanced Pseudocode ---- */
    mixed m;
    int total;
    int base_name;
    int var_name;
    mixed type;
    mixed var_map;
    mapping s;
    mapping detail;
    mapping summary;

    if ((!is_mapping(m = get_variables(ob)))) {
        return;
    }
    printf("%-40s %-8s - mapping  array  string  buffer  value  memory\n", "Var name<Object name>", "type");
    total = 0;
    loop (base_name in keys(m)) {
        var_map = m[base_name];
        loop (var_name in keys(var_map)) {
            type = var_type(var_map[var_name]);
            if ((type != "object")) {
                s = size_value(var_map[var_name]);
            } else {
                s = size_object(var_map[var_name]);
            }
            detail = s["detail"];
            summary = s["summary"];
            if (((!is_mapping(detail)) || (!is_mapping(summary)))) {
                continue;
            }
            printf("%-40s %-8s - %7d  %5d  %6d  %6d  %5d  %5dK\n", sprintf("%s<%s>", var_name, base_name), var_type(var_map[var_name]), detail["counter"]["mapping"], detail["counter"]["array"], detail["counter"]["string"], detail["counter"]["buffer"], detail["counter"]["value"], ((summary["memory_cost"] + 1023) / 1024));
            total += summary["memory_cost"];
        }  // end loop (verified)
    }  // end loop (verified)
    printf("Total: %dK\n", ((total + 1023) / 1024));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=114, nargs=1, insts=114
// --------------------------------------------------------------------
void size_objects(mixed module) {
    /* ---- Enhanced Pseudocode ---- */
    mixed obs;
    mixed ob;
    int total;
    mapping s;
    mapping detail;
    mapping summary;

    printf("%-40s %-8s - mapping  array  string  buffer  value  memory\n", "Object name<Var name>", "type");
    total = 0;
    if ((!is_array(obs = module->get_objects()))) {
        return;
    }
    loop (ob in obs) {
        s = size_object(ob);
        detail = s["detail"];
        summary = s["summary"];
        if (((!is_mapping(detail)) || (!is_mapping(summary)))) {
            continue;
        }
        printf("%-40s %-8s - %7d  %5d  %6d  %6d  %5d  %5dK\n", object_name(ob)[0..39], "object", detail["counter"]["mapping"], detail["counter"]["array"], detail["counter"]["string"], detail["counter"]["buffer"], detail["counter"]["value"], ((summary["memory_cost"] + 1023) / 1024));
        total += summary["memory_cost"];
    }  // end loop (verified)
    printf("Total %d objects: %dK\n", sizeof(obs), ((total + 1023) / 1024));

}

/* ================================================================
   DECLARED FUNCTIONS (no bytecode - likely inherited or forward decl)
   ================================================================ */
/*
   mixed get_objects(mixed ob);  // inherited from parent or forward declaration
*/

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        6
   Code segments found:         6
   Successfully decoded:        6
   Unresolved/minimal:          0
   Functions without code:      1
   Inherited programs:          1
      - /feature/daemon/register.c()

   Note: Functions without code are usually inherited from the
   parent object (), forward declarations, or
   simple property accessor methods that the compiler optimizes away.
   ================================================================ */