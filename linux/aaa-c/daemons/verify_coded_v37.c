/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: verify_coded.o
 * Source: /daemons/verify_coded.c
 * Inherits: (none)
 * Functions: 16 names, 16 code segments found
 * Strings: 75, Variables: 11
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   int background;
   array detect_font_arr;
   mapping detect_pic_pool;
   array font_list;
   int font_list_size;
   int foreground;
   int pic_height;
   mapping pic_pool;
   array pic_pool_index;
   int pic_width;
   mapping prompt_msg;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mapping generate_verify_code_pic();
void reselect_verify_code(mixed ob);
private void _auth_verify_code(mixed ob, string auth_key);
private void destruct();
string generate_random_detect_str();
void show_verify_code(mixed ob, int type, mixed func_succ, mixed func_fail);
private void do_init();
void get_verify_code_pic(string &key, buffer &pic_buf);
void get_one_detect_pic(string &key, buffer &pic_buf);
void auth_verify_code(mixed ob, string auth_key);
private void create();
private void replace_source(mapping resource);
private void daemon();
private buffer generate_pic_by_str(string verigen_str);
private void fill_pic_pool();
private void fill_detect_pic_pool();

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   background = 16777215;  // /daemons/verify_coded.c:20
   foreground = 0;  // /daemons/verify_coded.c:33
   pic_width = 144;  // /daemons/verify_coded.c:39
   pic_height = 48;  // /daemons/verify_coded.c:49
   font_list = "abcdefghjkmnpqrtuvwxyABCDEFGHJKMNPQRTUVWXY346789";  // /daemons/verify_coded.c:53
   font_list_size = sizeof(font_list);
   pic_pool = ([ ]);
   pic_pool_index = ({ });
   detect_font_arr = ({ "a", "b", "c", "d", "e", "f", "g", "h", "j", "k", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "A", "B", "C", "D", "E", "F", "G", "H", "J", "K", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z" });
   detect_pic_pool = ([ ]);
   prompt_msg = ([ 1 : "要进行答题，请输入字符验证", 2 : "要进行抽奖，请输入字符验证", 3 : "为了保护您的隐私，请输入字符验证", 4 : "为了保护您的帐号安全，需要进行字符验证", 5 : "为了保护您的帐号安全，需要进行字符验证" ]);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=32, nargs=0, insts=32
// --------------------------------------------------------------------
mapping generate_verify_code_pic() {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    string verigen_str;

    verigen_str = "";
    for (i = 1; i <= 4; i++) {
        verigen_str += sprintf("%c", font_list[random(font_list_size)]);
    }  // end loop (verified)
    return ([ verigen_str: generate_pic_by_str(verigen_str) ]);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=31, nargs=1, insts=31
// --------------------------------------------------------------------
void reselect_verify_code(mixed ob) {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;

    if ((!is_object(ob))) {
        return;
    }
    if ((!is_mapping(m = ob->query_temp("verify_code")))) {
        return;
    }
    show_verify_code(ob, m["type"], m["func_succ"], m["func_fail"]);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=58, nargs=2, insts=58
// --------------------------------------------------------------------
private void _auth_verify_code(mixed ob, string auth_key) {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;

    if ((!is_object(ob))) {
        return;
    }
    if ((!is_mapping(m = ob->query_temp("verify_code")))) {
        return;
    }
    auth_key = tolower(auth_key);
    if ((m["key"] != auth_key)) {
        if (is_function(m["func_fail"])) {
            (*m["func_fail"])();
        }
        return;
    }
    ob->set_temp("verify_code/answer", auth_key);
    if (is_function(m["func_succ"])) {
        (*m["func_succ"])();
    }

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=7, nargs=0, insts=7
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    debug_delete_thread("vcode_thread");

}

// --------------------------------------------------------------------
// @code  nc=31, nargs=0, insts=31
// --------------------------------------------------------------------
string generate_random_detect_str() {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    string str;

    str = "";
    for (i = 1; i <= random(3, 6); i++) {
        str += detect_font_arr[random(sizeof(detect_font_arr))];
    }  // end loop (verified)
    return str;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=61, nargs=4, insts=61
// --------------------------------------------------------------------
void show_verify_code(mixed ob, int type, mixed func_succ, mixed func_fail) {
    /* ---- Enhanced Pseudocode ---- */
    string key;
    string msg;
    mixed buf;

    if ((!is_object(ob))) {
        return;
    }
    if ((!sizeof(pic_pool))) {
        return;
    }
    key = "";
    buf = allocate_buffer(0);
    get_verify_code_pic(&key, &buf);
    ob->set_temp("verify_code", ([ "key": tolower(key), "func_succ": func_succ, "func_fail": func_fail, "type": type ]));
    if ((!is_string(msg = prompt_msg[type]))) {
        msg = "请输入字符验证";
    }
    ob->send_message(10003, buf, msg);

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
private void do_init() {
    /* ---- Enhanced Pseudocode ---- */
    create_thread("vcode_thread", "daemon");

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=18, nargs=2, insts=18
// --------------------------------------------------------------------
void get_verify_code_pic(string &key, buffer &pic_buf) {
    /* ---- Enhanced Pseudocode ---- */
    key = pic_pool_index[random(sizeof(pic_pool_index))];
    pic_buf = pic_pool[key];

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=24, nargs=2, insts=24
// --------------------------------------------------------------------
void get_one_detect_pic(string &key, buffer &pic_buf) {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr;

    arr = keys(detect_pic_pool);
    key = arr[random(sizeof(arr))];
    pic_buf = detect_pic_pool[key];

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=16, nargs=2, insts=16
// --------------------------------------------------------------------
void auth_verify_code(mixed ob, string auth_key) {
    /* ---- Enhanced Pseudocode ---- */
    _auth_verify_code(ob, auth_key);
    if (is_object(ob)) {
        ob->delete_temp("verify_code");
    }

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=7, nargs=0, insts=7
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    register_post_init((: do_init :));

}

// --------------------------------------------------------------------
// @code  nc=13, nargs=1, insts=13
// --------------------------------------------------------------------
private void replace_source(mapping resource) {
    /* ---- Enhanced Pseudocode ---- */
    pic_pool = resource;
    pic_pool_index = keys(resource);

}

// --------------------------------------------------------------------
// @code  nc=29, nargs=0, insts=29
// --------------------------------------------------------------------
private void daemon() {
    /* ---- Enhanced Pseudocode ---- */
    set_as_daemon();

    // --- label L0004 ---
    while (1) {
        try {
            fill_pic_pool();
        } catch { /* error handled */ }

        // --- label L0013 ---
        try {
            fill_detect_pic_pool();
        } catch { /* error handled */ }

        // --- label L0021 ---
        sleep(300);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @code  nc=139, nargs=1, insts=139
// --------------------------------------------------------------------
private buffer generate_pic_by_str(string verigen_str) {
    /* ---- Enhanced Pseudocode ---- */
    mixed before_compress_len;
    mixed selected_font;
    float buf;
    mixed buf_r;
    mapping PicHeader;

    selected_font = explode(verigen_str, "");
    buf = verigen_render_chars(selected_font, background, foreground, pic_width, pic_height, 0.8);
    before_compress_len = sizeof(buf);
    buf = lzss_compress(buf);
    buf = buf[4..<1];
    PicHeader = ([ ]);
    PicHeader["cbsize"] = 32;
    PicHeader["type"] = 1;
    PicHeader["zip"] = ((int)buf[0..3] ? 1 : 0);
    PicHeader["bpp"] = 16;
    PicHeader["alpha"] = 0;
    PicHeader["width"] = pic_width;
    PicHeader["height"] = pic_height;
    PicHeader["len"] = sizeof(buf);
    buf_r = pad_buffer(make32(PicHeader["cbsize"]), make32(PicHeader["type"]), make32(PicHeader["zip"]), make32(PicHeader["bpp"]), make32(PicHeader["alpha"]), make32(PicHeader["width"]), make32(PicHeader["height"]), make32(PicHeader["len"]), make32(before_compress_len), buf);
    return buf_r;

}

// --------------------------------------------------------------------
// @brief  data mapper: populate structure
// @code  nc=25, nargs=0, insts=25
// --------------------------------------------------------------------
private void fill_pic_pool() {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    mixed m;

    m = ([ ]);
    for (i = 1; i <= 50; i++) {
        m << generate_verify_code_pic();
    }  // end loop (verified)
    replace_source(m);

}

// --------------------------------------------------------------------
// @brief  data mapper: populate structure
// @code  nc=46, nargs=0, insts=46
// --------------------------------------------------------------------
private void fill_detect_pic_pool() {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    int str;
    mixed buf;
    mixed m;

    m = ([ ]);
    str = generate_random_detect_str();
    for (i = 1; i <= 50; i++) {
        str = generate_random_detect_str();
        buf = generate_pic_by_str(str);
        m[str] = (make32(sizeof(buf)) + buf);
    }  // end loop (verified)
    detect_pic_pool = m;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        16
   Code segments found:         16
   Successfully decoded:        16
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */