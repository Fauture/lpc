// Local reproduction of the character-name rule chain, run with:
//   build\Release\lpcvm.exe run _nametest.c
// It mirrors /gs/start_gs.c::_valid_name + valid_name + valid_char_name and the
// 1-argument call logind::check_char_name makes (missing args are undefined).
mapping ban_words = ([ ]);

int valid_ban_words_by_group(string name, mixed group) { return 0; }

mixed _valid_name(string name, int min_len, int max_len, int may_chinese,
                  mixed include_group, mixed exclude_group) {
    string str;
    mixed group;
    mixed sz;
    int hasnt_need_word;
    mapping m;
    int i;

    m = ([ ]);
    if ((sz = strlen(name)) < min_len) {
        m["msg"] = "名字不能少于" + min_len + "个字符";
        m["type"] = -1;
        return m;
    }
    if (sz > max_len) {
        m["msg"] = "名字不能超过" + max_len + "个字符";
        m["type"] = -2;
        return m;
    }
    str = "";
    i = 0;
    while (i < sz) {
        if (isdigit(name[i]) || isalpha(name[i])) {
        } else {
            if (!may_chinese) {
                m["msg"] = "名字必须由英文字母或阿拉伯数字组成";
                m["type"] = -4;
                return m;
            }
            str += name[i..(i + 1)];
            ++i;
        }
        ++i;
    }
    printf("DBG not_include_group=%d sizeof_include=%d\n",
           !include_group, sizeof(include_group));
    if (!include_group) include_group = ({ });
    if (!exclude_group) exclude_group = keys(ban_words);
    hasnt_need_word = 1;
    if (!sizeof(include_group)) hasnt_need_word = 0;
    loop (group in include_group) {
        if (valid_ban_words_by_group(name, group)) { hasnt_need_word = 0; break; }
    }
    if (hasnt_need_word) {
        m["msg"] = "你输入的名字中缺少必含字符，请重新选择名字";
        m["type"] = -5;
        return m;
    }
    m["msg"] = 0;
    m["type"] = 0;
    return m;
}

mixed valid_name(string name, int min_len, int max_len, int may_chinese,
                 mixed include_group = 0, mixed exclude_group = 0) {
    return _valid_name(name, min_len, max_len, may_chinese, include_group, exclude_group)["msg"];
}

mixed valid_char_name(string char_name, mixed include_group, mixed exclude_group) {
    return valid_name(char_name, 3, 12, 1, include_group, exclude_group);
}

int create() {
    printf("测试    -> %O\n", valid_char_name("测试"));
    printf("abcdef  -> %O\n", valid_char_name("abcdef"));
    printf("GM form -> %O\n", valid_char_name("测试", ({ "GM必用字符" }), ({ })));
    return 1;
}
