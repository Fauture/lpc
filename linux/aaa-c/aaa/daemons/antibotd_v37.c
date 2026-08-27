/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: antibotd.o
 * Source: /aaa/daemons/antibotd.c
 * Inherits: (none)
 * Functions: 20 names, 20 code segments found
 * Strings: 24, Variables: 5
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   int ab_socket;
   int broken;
   int cookie;
   mapping oper_map;
   array status_list;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mixed ab_user_logout(int user_id);
mixed user_check_data(int user_id, string client_data, mixed &bot_content = "");
int get_check_result(int bot_type);
mixed user_login(int user_id);
private void destruct();
mixed send_to_abserver(int msg_id, buffer buf);
private void msg_ab_user_check_data(buffer buf);
string get_status();
private void do_init();
private void do_when_user_logout(mixed agent);
void ab_server_daemon();
mixed ab_user_check_data(int user_id, buffer buf, mixed &bot_content);
private void create();
private void msg_ab_user_login(buffer buf);
void set_status(mixed status);
private void _ab_server_daemon();
private void do_when_user_disconnected(int user_id);
mixed user_logout(mixed user_id);
mixed ab_user_login(int user_id);
private mixed read_ab_data(int size);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   ab_socket = 65535;  // /aaa/daemons/antibotd.c:29
   broken = 0;  // /aaa/daemons/antibotd.c:34
   cookie = 10000;
   oper_map = ([ ]);
   status_list = ({ "ignore", "deny", "disable", "ignore_but_check" });

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=17, nargs=1, insts=17
// --------------------------------------------------------------------
mixed ab_user_logout(int user_id) {
    /* ---- Enhanced Pseudocode ---- */
    mixed my_cookie;

    ++cookie;
    my_cookie = cookie;
    send_to_abserver(2053, make32(user_id, my_cookie));
    return;

}

// --------------------------------------------------------------------
// @code  nc=45, nargs=3, insts=45
// --------------------------------------------------------------------
mixed user_check_data(int user_id, string client_data, mixed &bot_content) {
    /* ---- Enhanced Pseudocode ---- */
    mixed bot_type;
    mixed buf;
    mixed text;

    if ((get_status() == "disable")) {
        return 0;
    }
    text = sprintf(":%s:", client_data);
    try {
        buf = restore_value(text);
    } catch { /* error handled */ }

    // --- label L0026 ---
    if ((0 || (!is_buffer(buf)))) {
        return -1;
    }
    bot_type = ab_user_check_data(user_id, buf, &bot_content);
    return bot_type;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=39, nargs=1, insts=39
// --------------------------------------------------------------------
int get_check_result(int bot_type) {
    /* ---- Enhanced Pseudocode ---- */
    if ((bot_type < 0)) {
        return 1;
    }
    if ((bot_type == 0)) {
        return 1;
    }
    if ((bot_type <= 100)) {
        return 1;
    }
    switch (get_status()) {
        case "deny":
        return 0;
        case "ignore":
        return 1;
        case "ignore_but_check":
        return -1;
        default:
        return 0;
    }  // end switch

}

// --------------------------------------------------------------------
// @code  nc=32, nargs=1, insts=32
// --------------------------------------------------------------------
mixed user_login(int user_id) {
    /* ---- Enhanced Pseudocode ---- */
    mixed buf;
    mixed text;

    if ((get_status() == "disable")) {
        return "";
    }
    buf = ab_user_login(user_id);
    if ((!is_buffer(buf))) {
        return "";
    }
    text = save_string(buf)[1..<2];
    return text;

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=17, nargs=0, insts=17
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    if ((ab_socket != -1)) {
        close_socket(ab_socket);
    }
    delete_thread("tAntibot");
    delete_mailbox("mbAntibox");

}

// --------------------------------------------------------------------
// @code  nc=45, nargs=2, insts=45
// --------------------------------------------------------------------
mixed send_to_abserver(int msg_id, buffer buf) {
    /* ---- Enhanced Pseudocode ---- */
    mixed buf_send;

    if (((ab_socket == -1) || broken)) {
        return 0;
    }
    buf_send = ((make32(((sizeof(buf) + 4) + 2)) + make16(msg_id)) + buf);
    if ((send(ab_socket, buf_send, 0) != sizeof(buf_send))) {
        broken = 0;
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=86, nargs=1, insts=86
// --------------------------------------------------------------------
private void msg_ab_user_check_data(buffer buf) {
    /* ---- Enhanced Pseudocode ---- */
    int my_cookie;
    mixed tid;
    int name_len;
    int bot_type;
    string content;
    mapping m;

    my_cookie = (int)make32i((int)buf[4..7]);
    m = oper_map[cookie];
    if ((!is_mapping(m))) {
        return;
    }
    bot_type = (int)make32i((int)buf[8..11]);
    name_len = (int)make32i((int)buf[12..15]);
    content = (string)buf[16..((name_len + 16) - 1)];
    m["result"] = 1;
    m["bot_type"] = bot_type;
    m["content"] = content;
    tid = m["tid"];
    post_mailbox("mbAntibox", tid, my_cookie);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=27, nargs=0, insts=27
// --------------------------------------------------------------------
string get_status() {
    /* ---- Enhanced Pseudocode ---- */
    mixed status;
    mapping m;

    if (is_mapping(m = "/aaa/daemons/configd.c"->get_my_info())) {
        status = m["antibot_action"];
    }
    return ((member_array(status, status_list) >= 0) ? status : "deny");

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=18, nargs=0, insts=18
// --------------------------------------------------------------------
private void do_init() {
    /* ---- Enhanced Pseudocode ---- */
    if ((!is_object(find_object("/aaa/daemons/configd.c")))) {
        register_post_init((: do_init :));
        return;
    }
    create_thread("tAntibot", "ab_server_daemon");

}

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=14, nargs=1, insts=14
// --------------------------------------------------------------------
private void do_when_user_logout(mixed agent) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!is_object(agent))) {
        return;
    }
    user_logout(agent->get_client_no());

}

// --------------------------------------------------------------------
// @code  nc=21, nargs=0, insts=21
// --------------------------------------------------------------------
void ab_server_daemon() {
    /* ---- Enhanced Pseudocode ---- */
    set_as_daemon();

    // --- label L0004 ---
    while (1) {
        try {
            _ab_server_daemon();
        } catch { /* error handled */ }

        // --- label L0013 ---
        if (0) {
            sleep(1);
        }
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @code  nc=102, nargs=3, insts=102
// --------------------------------------------------------------------
mixed ab_user_check_data(int user_id, buffer buf, mixed &bot_content) {
    /* ---- Enhanced Pseudocode ---- */
    mixed thread_id;
    int my_cookie;
    mixed received;
    mapping m;

    thread_id = get_thread_id();
    ++cookie;
    my_cookie = cookie;
    m = ([ ]);
    m["result"] = 0;
    m["tid"] = thread_id;
    oper_map[my_cookie] = m;
    if ((!send_to_abserver(2051, ((make32(user_id, my_cookie) + make32(sizeof(buf))) + buf)))) {
        map_delete(oper_map, my_cookie);
        return -1;
    }
    peek_mailbox("mbAntibox", thread_id, &received, 3);
    m = oper_map[my_cookie];
    map_delete(oper_map, my_cookie);
    if ((!m["result"])) {
        return -1;
    }
    if ((!is_int(m["bot_type"]))) {
        return -1;
    }
    if ((argc >= 3)) {
        bot_content = m["content"];
    }
    return m["bot_type"];

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=22, nargs=0, insts=22
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/connectiond.c"->register_disconnect((: do_when_user_logout :));
    register_post_disconnected((: do_when_user_disconnected :));
    create_mailbox("mbAntibox", get_system_info()["max_threads"]);
    register_post_init((: do_init :));

}

// --------------------------------------------------------------------
// @code  nc=69, nargs=1, insts=69
// --------------------------------------------------------------------
private void msg_ab_user_login(buffer buf) {
    /* ---- Enhanced Pseudocode ---- */
    int my_cookie;
    int buff_len;
    mixed tid;
    mixed content;
    mapping m;

    my_cookie = (int)make32i((int)buf[4..7]);
    m = oper_map[my_cookie];
    if ((!is_mapping(m))) {
        return;
    }
    buff_len = (int)make32i((int)buf[8..11]);
    content = buf[12..((buff_len + 12) - 1)];
    m["result"] = 1;
    m["content"] = content;
    tid = m["tid"];
    post_mailbox("mbAntibox", tid, my_cookie);

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=7, nargs=1, insts=7
// --------------------------------------------------------------------
void set_status(mixed status) {
    /* ---- Enhanced Pseudocode ---- */
    printf("Please edit 'antibot_action' field of ADB aaa table .\n");

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=180, nargs=0, insts=180
// --------------------------------------------------------------------
private void _ab_server_daemon() {
    /* ---- Enhanced Pseudocode ---- */
    int ab_server_port;
    int msg_id;
    int pack_len;
    mixed ab_server_addr;
    mixed buf;
    mixed ab_server_config;
    mapping m;

    if ((!is_mapping(m = "/aaa/daemons/configd.c"->get_my_info()))) {
        sleep(1);
        return;
    }
    ab_server_config = m["antibot_server"];
    if ((is_string(ab_server_config) && strlen(ab_server_config))) {
        sscanf(ab_server_config, "%s:%d", &ab_server_addr, &ab_server_port);
    }
    if (((((!is_string(ab_server_addr)) || (strlen(ab_server_addr) <= 0)) || (!is_int(ab_server_port))) || (ab_server_port <= 0))) {
        sleep(1);
        return;
    }
    ab_socket = socket(2);
    set_sock_opt(ab_socket, 1, 7, 262144);
    set_sock_opt(ab_socket, 1, 8, 262144);
    if ((connect(ab_socket, ab_server_addr, ab_server_port) != 0)) {
        close_socket(ab_socket);
        ab_socket = -1;
        sleep(1);
        return;
    }
    broken = 0;

    // --- label L0099 ---
    while (((ab_socket != -1) && (!broken))) {
        if ((!is_buffer(buf = read_ab_data(4)))) {
            break;
        }
        pack_len = (int)make32i((int)buf[0..3]);
        if ((!is_buffer(buf = read_ab_data((pack_len - 4))))) {
            break;
        }
        msg_id = (int)make16i((int)buf[0..1]);
        switch (msg_id) {
            case 2050:
            msg_ab_user_login(buf[2..<1]);
            break;
            case 2052:
            msg_ab_user_check_data(buf[2..<1]);
            break;
            default:
        }  // end switch

        // --- label L0166 ---
    }  // end loop (verified)
    close_socket(ab_socket);
    ab_socket = -1;
    sleep(1);

}

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=7, nargs=1, insts=7
// --------------------------------------------------------------------
private void do_when_user_disconnected(int user_id) {
    /* ---- Enhanced Pseudocode ---- */
    user_logout(user_id);

}

// --------------------------------------------------------------------
// @code  nc=12, nargs=1, insts=12
// --------------------------------------------------------------------
mixed user_logout(mixed user_id) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!is_int(user_id))) {
        return;
    }
    ab_user_logout(user_id);
    return;

}

// --------------------------------------------------------------------
// @code  nc=86, nargs=1, insts=86
// --------------------------------------------------------------------
mixed ab_user_login(int user_id) {
    /* ---- Enhanced Pseudocode ---- */
    mixed thread_id;
    int my_cookie;
    mixed received;
    mapping m;

    thread_id = get_thread_id();
    ++cookie;
    my_cookie = cookie;
    m = ([ ]);
    m["result"] = 0;
    m["tid"] = thread_id;
    oper_map[my_cookie] = m;
    if ((!send_to_abserver(2049, make32(user_id, my_cookie)))) {
        map_delete(oper_map, my_cookie);
        return -1;
    }
    peek_mailbox("mbAntibox", thread_id, &received, 3);
    m = oper_map[my_cookie];
    map_delete(oper_map, my_cookie);
    if ((!m["result"])) {
        return -1;
    }
    if ((!is_buffer(m["content"]))) {
        return -1;
    }
    return m["content"];

}

// --------------------------------------------------------------------
// @code  nc=94, nargs=1, insts=94
// --------------------------------------------------------------------
private mixed read_ab_data(int size) {
    /* ---- Enhanced Pseudocode ---- */
    int count;
    mixed left_size;
    mixed buf;
    mixed recv_buf;

    left_size = size;
    recv_buf = allocate_buffer(0);

    // --- label L0011 ---
    while (left_size) {
        if (((ab_socket == -1) || broken)) {
            return 0;
        }
        count = recv(ab_socket, &buf, left_size);
        if ((count < 0)) {
            if ((count == -101)) {
                sleep(0.1);
                continue;
            }
            return 0;
        }
        if ((count == 0)) {
            return 0;
        }
        if (((count != sizeof(buf)) || (count > left_size))) {
            catch(error("Bad buffer to receive, buf = %O, left_size = %d, count = %d\n", buf, left_size, count));

            // --- label L0072 ---
            broken = 1;
            return 0;
        }
        left_size -= count;
        recv_buf += buf;
    }  // end loop (verified)
    return recv_buf;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        20
   Code segments found:         20
   Successfully decoded:        20
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */