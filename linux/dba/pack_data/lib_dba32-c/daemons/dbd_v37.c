/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: dbd.o
 * Source: /daemons/dbd.c
 * Inherits: (none)
 * Functions: 8 names, 8 code segments found
 * Strings: 25, Variables: 4
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   mapping db_config;
   int is_shutdown_now;
   mapping thread_dbc;
   int use_db;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

int remove_db_config(string name);
mixed query_db_config(mixed db_type);
private void destruct();
private void load_db_config(string name, string section);
private void create();
int add_db_config(string name, string host, string user, string password, string database, mixed port);
int assure_connection(string name);
void notice_db_failed(string name);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   thread_dbc = ([ ]);  // /daemons/dbd.c:14
   is_shutdown_now = 0;
   db_config = ([ ]);
   use_db = 0;

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=17, nargs=1, insts=17
// --------------------------------------------------------------------
int remove_db_config(string name) {
    /* ---- Enhanced Pseudocode ---- */
    if (is_undefined(db_config[name])) {
        return 0;
    }
    map_delete(db_config, name);
    return 1;

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=13, nargs=1, insts=13
// --------------------------------------------------------------------
mixed query_db_config(mixed db_type) {
    /* ---- Enhanced Pseudocode ---- */
    if (is_string(db_type)) {
        return db_config[db_type];
    }
    return db_config;

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=28, nargs=0, insts=28
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    mixed tdi;
    mapping m;

    is_shutdown_now = 1;
    loop (tdi in values(thread_dbc)) {
        loop (m in values(tdi)) {
            (*f_disconnect)(m["cno"]);
        }  // end loop (verified)
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @code  nc=110, nargs=2, insts=110
// --------------------------------------------------------------------
private void load_db_config(string name, string section) {
    /* ---- Enhanced Pseudocode ---- */
    mixed host;
    mixed user;
    mixed password;
    mixed database;
    mixed port;

    host = trim(read_ini(section, "Host", INI_FILE));
    user = trim(read_ini(section, "User", INI_FILE));
    password = trim(read_ini(section, "Password", INI_FILE));
    database = trim(read_ini(section, "Database", INI_FILE));
    port = trim(read_ini(section, "Port", INI_FILE));
    if ((!strlen(host))) {
        printf((("Miss or bad value of field \"Host\" in section \"%s\", expected database host address.\n[1;31mBad configuration, please edit your " + INI_FILE) + "\n[2;37;0mFor example:\n[1;32m[%s]\nHost = 127.0.0.1\n[2;37;0m"), section, section);
        suspend_thread();
    }
    if ((!strlen(database))) {
        printf((("Miss or bad value of field \"Database\" in section \"%s\", expected database name.\n[1;31mBad configuration, please edit your " + INI_FILE) + "\n[2;37;0mFor example:\n[1;32m[%s]\nDatabase = DDB\n[2;37;0m"), section, section);
        suspend_thread();
    }
    password = get_raw_password(password);
    if ((strlen(port) && (port != "3306"))) {
        host = sprintf("%s : %s", host, port);
    }
    db_config[name] = ([ "host": host, "user": user, "password": password, "database": database ]);

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=30, nargs=0, insts=30
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    load_db_config("ddb", "DDB");
    load_db_config("ldb", "LDB");
    switch (tolower(read_ini("File", "USE_DB", INI_FILE))) {
        case "disabled":
        printf("Database is disabled.\n");
        break;
        default:
        use_db = 1;
        break;
    }  // end switch

    // --- label L0028 ---

}

// --------------------------------------------------------------------
// @code  nc=47, nargs=6, insts=47
// --------------------------------------------------------------------
int add_db_config(string name, string host, string user, string password, string database, mixed port) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!is_undefined(db_config[name]))) {
        return 0;
    }
    if (((is_string(port) && strlen(port)) && (port != "3306"))) {
        host = sprintf("%s : %s", host, port);
    }
    db_config[name] = ([ "host": host, "user": user, "password": password, "database": database ]);
    return 1;

}

// --------------------------------------------------------------------
// @brief  verification: confirm/guarantee state
// @code  nc=151, nargs=1, insts=151
// --------------------------------------------------------------------
int assure_connection(string name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed tdi;
    mapping dbi;
    mapping m;
    mixed cno;

    if (is_shutdown_now) {
        return -1;
    }
    if ((!use_db)) {
        return -1;
    }
    if ((!is_mapping(dbi = db_config[name]))) {
        return -1;
    }
    if ((!is_mapping(tdi = thread_dbc[get_thread_id()]))) {
        thread_dbc[get_thread_id()] = (tdi = ([ ]));
    }
    if ((!is_mapping(m = tdi[name]))) {
        cno = (*f_connect)(dbi["host"], dbi["user"], dbi["password"], dbi["database"]);
        if ((cno == -1)) {
            return -1;
        }
        tdi[name] = (m = ([ "cno": cno, "ping": time() ]));
    }
    if (((time() - m["ping"]) > 60)) {
        if ((!(*f_is_connected)(m["cno"]))) {
            (*f_disconnect)(m["cno"]);
            cno = (*f_connect)(dbi["host"], dbi["user"], dbi["password"], dbi["database"]);
            if ((cno == -1)) {
                map_delete(tdi, name);
                return -1;
            }
            m["cno"] = cno;
        }
        m["ping"] = time();
    }
    return m["cno"];

}

// --------------------------------------------------------------------
// @brief  notification: send message
// @code  nc=28, nargs=1, insts=28
// --------------------------------------------------------------------
void notice_db_failed(string name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed tdi;
    mapping m;

    if (((!is_mapping(tdi = thread_dbc[get_thread_id()])) || (!is_mapping(m = tdi[name])))) {
        return;
    }
    m["ping"] = 0;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        8
   Code segments found:         8
   Successfully decoded:        8
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */