/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: picd.o
 * Source: /aaa/daemons/picd.c
 * Inherits: (none)
 * Functions: 9 names, 9 code segments found
 * Strings: 17, Variables: 4
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   array chinese_list;
   array chinese_pool_list;
   mapping font_files;
   mapping pic_pool_map;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void set_pic_pool_map(mapping m);
int get_pic(mixed &key, mixed &pic_buf);
private void destruct();
buffer generate_pic(string str, int type, int w = 96, int h = 96);
private void refresh_pic_pool();
void random_generate_pic(mixed &text, mixed &pic_buf);
private void create();
private void daemon();
private string get_random_chinese();

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   font_files = ([ "gbsn00lp.ttf" : 0, "gkai00mp.ttf" : 0 ]);  // /aaa/daemons/picd.c:225
   pic_pool_map = ([ ]);
   chinese_pool_list = ({ });
   chinese_list = ({ });

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=13, nargs=1, insts=13
// --------------------------------------------------------------------
private void set_pic_pool_map(mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    pic_pool_map = m;
    chinese_pool_list = keys(pic_pool_map);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=32, nargs=2, insts=32
// --------------------------------------------------------------------
int get_pic(mixed &key, mixed &pic_buf) {
    /* ---- Enhanced Pseudocode ---- */
    key = chinese_pool_list[random(sizeof(chinese_pool_list))];
    if ((!is_string(key))) {
        return 0;
    }
    pic_buf = pic_pool_map[key];
    if ((!is_string(pic_buf))) {
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=24, nargs=0, insts=24
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    int file;

    delete_thread("tPicPool");
    loop (file in keys(font_files)) {
        if ((!font_files[file])) {
            continue;
        }
        verigen_remove_font(file);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @code  nc=157, nargs=4, insts=157
// --------------------------------------------------------------------
buffer generate_pic(string str, int type, int w, int h) {
    /* ---- Enhanced Pseudocode ---- */
    int zip;
    mixed org_buf_len;
    mixed buf;
    mixed buf2;
    float buf3;
    mixed str2;
    mixed utf8_str;
    mixed utf8_str2;
    mapping pic_header;

    utf8_str = iconv(str, "utf-8", "gb2312");
    str2 = get_random_chinese();
    utf8_str2 = iconv(str2, "utf-8", "gb2312");
    buf = verigen_render(utf8_str, 65535, 0, w, h);
    buf2 = verigen_render(utf8_str2, 65535, 0, w, h);
    buf3 = verigen_merge(buf, buf2, (w * 0.45), (h * 0.45), 1, 65535, w, h);
    if ((type == 1)) {
        buf3 = buf;
    }
    org_buf_len = sizeof(buf3);
    buf3 = lzss_compress(buf3);
    if ((int)buf3[0..3]) {
        zip = 1;
    } else {
        zip = 0;
    }
    buf3 = buf3[4..<1];
    pic_header = ([ "cbsize": 32, "type": 1, "zip": zip, "bpp": 16, "alpha": 0, "width": w, "height": h, "len": sizeof(buf3) ]);
    return pad_buffer(make32(pic_header["cbsize"]), make32(pic_header["type"]), make32(pic_header["zip"]), make32(pic_header["bpp"]), make32(pic_header["alpha"]), make32(pic_header["width"]), make32(pic_header["height"]), make32(pic_header["len"]), make32(org_buf_len), buf3);

}

// --------------------------------------------------------------------
// @code  nc=33, nargs=0, insts=33
// --------------------------------------------------------------------
private void refresh_pic_pool() {
    /* ---- Enhanced Pseudocode ---- */
    mixed m;
    int text;
    mixed buf;

    m = ([ ]);

    // --- label L0006 ---
    while ((sizeof(m) < 10)) {
        random_generate_pic(&text, &buf);
        m[text] = save_string(buf)[1..<2];
    }  // end loop (verified)
    set_pic_pool_map(m);

}

// --------------------------------------------------------------------
// @code  nc=14, nargs=2, insts=14
// --------------------------------------------------------------------
void random_generate_pic(mixed &text, mixed &pic_buf) {
    /* ---- Enhanced Pseudocode ---- */
    text = get_random_chinese();
    pic_buf = generate_pic(text, 2);

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=43, nargs=0, insts=43
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    int file;
    mixed buf;

    loop (file in keys(font_files)) {
        buf = read_etc_file(file, "b");
        if ((!verigen_load_font(file, buf))) {
            error("Load font file %s failed.\n", file);
        }
        font_files[file] = 1;
    }  // end loop (verified)
    chinese_list = read_etc_file("hanzi_table.list", "b");
    create_thread("tPicPool", "daemon");

}

// --------------------------------------------------------------------
// @code  nc=21, nargs=0, insts=21
// --------------------------------------------------------------------
private void daemon() {
    /* ---- Enhanced Pseudocode ---- */
    set_as_daemon();

    // --- label L0004 ---
    while (1) {
        try {
            refresh_pic_pool();
        } catch { /* error handled */ }

        // --- label L0013 ---
        sleep(600);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=24, nargs=0, insts=24
// --------------------------------------------------------------------
private string get_random_chinese() {
    /* ---- Enhanced Pseudocode ---- */
    mixed index;

    index = random((sizeof(chinese_list) / 2));
    return (string)chinese_list[(index * 2)..((index * 2) + 1)];

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        9
   Code segments found:         9
   Successfully decoded:        9
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */