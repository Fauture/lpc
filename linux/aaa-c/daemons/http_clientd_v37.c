/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: http_clientd.o
 * Source: /daemons/http_clientd.c
 * Inherits: (none)
 * Functions: 7 names, 7 code segments found
 * Strings: 23, Variables: 2
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   mapping domain_list;
   int resolve_interval;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private mixed get_ip_by_domain(string domain_name);
private int analyse_url(string url, mixed host, mixed port);
private void destruct();
mixed http_post_web(string url, int timeout, int size, string body);
private void create();
mixed http_get_web(string url, int timeout = 15, int size = 4096);
private void daemon();

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   domain_list = ([ ]);  // /daemons/http_clientd.c:297
   resolve_interval = 60;

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=74, nargs=1, insts=74
// --------------------------------------------------------------------
private mixed get_ip_by_domain(string domain_name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed inets;
    mapping m;

    if (((!is_mapping(m = domain_list[domain_name])) || (!sizeof((inets = m["inets"]))))) {
        if (((!is_array(inets = resolve(domain_name))) || (sizeof(inets) < 1))) {
            return;
        }
        domain_list[domain_name] = ([ "time": time(), "inets": inets ]);
        return;
    }
    if (((time() - (int)m["time"]) > resolve_interval)) {
        m["time"] = time();
        send_to_queue("qHttpClient", domain_name);
    }
    return inettoa(inets[random(sizeof(inets))]);

}

// --------------------------------------------------------------------
// @code  nc=57, nargs=3, insts=57
// --------------------------------------------------------------------
private int analyse_url(string url, mixed host, mixed port) {
    /* ---- Enhanced Pseudocode ---- */
    if ((url[0..6] == "http://")) {
        url = url[7..<1];
    }
    if ((sscanf(url, "%s/%*s", &host) < 2)) {
        host = url;
    }
    if ((sscanf(host, "%s:%d", &host, &port) < 2)) {
        port = 80;
    }
    if ((!is_valid_ip(host))) {
        if ((!is_string(host = get_ip_by_domain(host)))) {
            return 0;
        }
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=20, nargs=0, insts=20
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    send_to_queue("qHttpClient", "QUIT");

    // --- label L0006 ---
    while ((get_thread_id("tHttpClient") != -1)) {
        sleep(0.1);
    }  // end loop (verified)
    delete_queue("qHttpClient");

}

// --------------------------------------------------------------------
// @code  nc=178, nargs=4, insts=178
// --------------------------------------------------------------------
mixed http_post_web(string url, int timeout, int size, string body) {
    /* ---- Enhanced Pseudocode ---- */
    mixed fd;
    mixed port;
    mixed host;
    mixed http_message;
    mixed request_line;
    mixed buf;
    string data;
    int recv_size;
    int n;
    mixed fd_read;
    mixed fd_write;
    mixed fd_except;

    if ((!analyse_url(url, &host, &port))) {
        return -2;
    }
    fd = socket(2);
    if ((connect(fd, host, port) != 0)) {
        close_socket(fd);
        return -1;
    }
    request_line = sprintf("POST %s %s%s", url, "HTTP/1.1", "\r\n");
    http_message = (((((((((((((((((((request_line + "Cache-Control: no-cache, must-revalidate") + "\r\nConnection: close") + "\r\nData: ") + ctime()[0..<2]) + "\r\n") + "Accept: text/html") + "\r\nAccept-Charset: GBK") + "\r\nAccept-Encoding: identity") + "\r\nAccept-Language: zh-cn,zh") + "\r\nHost: ") + host) + "\r\n") + "Content-Type: text/xml; charset=utf-8") + "\r\n") + "Content-Length: ") + sizeof(body)) + "\r\n") + "\r\n") + body);
    if ((send(fd, (buffer)http_message) != sizeof(http_message))) {
        close_socket(fd);
        return -1;
    }
    data = allocate_buffer(0);
    while (1) {
        fd_read = ({ fd });
        fd_write = ({});
        fd_except = ({});
        n = select(&fd_read, &fd_write, &fd_except, timeout);
        if ((n <= 0)) {
            close_socket(fd);
            fd = -1;
            return -3;
        }
        recv_size = recv(fd, &buf, size);
        if ((recv_size < 0)) {
            break;
        }
        data += buf;
        if ((recv_size == 0)) {
            break;
        }
    }  // end loop (verified)

    // --- label L0161 ---
    close_socket(fd);
    data = (string)data;
    sscanf(data, "%*s\r\n\r\n%s", &data);
    return data;

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=11, nargs=0, insts=11
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    create_thread("tHttpClient", (: daemon :));
    create_queue("qHttpClient");

}

// --------------------------------------------------------------------
// @code  nc=168, nargs=3, insts=168
// --------------------------------------------------------------------
mixed http_get_web(string url, int timeout, int size) {
    /* ---- Enhanced Pseudocode ---- */
    mixed fd;
    mixed port;
    mixed host;
    mixed http_message;
    mixed request_line;
    mixed buf;
    string data;
    int recv_size;
    int n;
    mixed fd_read;
    mixed fd_write;
    mixed fd_except;

    if ((!analyse_url(url, &host, &port))) {
        return -2;
    }
    fd = socket(2);
    if ((connect(fd, host, port) != 0)) {
        close_socket(fd);
        return -1;
    }
    request_line = sprintf("GET %s %s%s", url, "HTTP/1.1", "\r\n");
    http_message = (((((((((((((request_line + "Cache-Control: no-cache, must-revalidate") + "\r\nConnection: close") + "\r\nData: ") + ctime()[0..<2]) + "\r\n") + "Accept: text/html") + "\r\nAccept-Charset: GBK") + "\r\nAccept-Encoding: identity") + "\r\nAccept-Language: zh-cn,zh") + "\r\nHost: ") + host) + "\r\n") + "\r\n");
    if ((send(fd, (buffer)http_message) != sizeof(http_message))) {
        close_socket(fd);
        return -1;
    }
    data = allocate_buffer(0);
    while (1) {
        fd_read = ({ fd });
        fd_write = ({});
        fd_except = ({});
        n = select(&fd_read, &fd_write, &fd_except, timeout);
        if ((n <= 0)) {
            close_socket(fd);
            fd = -1;
            return -3;
        }
        recv_size = recv(fd, &buf, size);
        if ((recv_size < 0)) {
            break;
        }
        data += buf;
        if ((recv_size == 0)) {
            break;
        }
    }  // end loop (verified)

    // --- label L0151 ---
    close_socket(fd);
    data = (string)data;
    sscanf(data, "%*s\r\n\r\n%s", &data);
    return data;

}

// --------------------------------------------------------------------
// @code  nc=48, nargs=0, insts=48
// --------------------------------------------------------------------
private void daemon() {
    /* ---- Enhanced Pseudocode ---- */
    mixed inets;
    int msg;

    set_as_daemon();

    // --- label L0004 ---
    while (1) {
        if ((receive_from_queue("qHttpClient", &msg) != 0)) {
            continue;
        }
        if (((!is_string(msg)) || (!strlen(msg)))) {
            continue;
        }
        if (("QUIT" == msg)) {
            return;
        }
        if (sizeof((inets = resolve(msg)))) {
            domain_list[msg] = ([ "time": time(), "inets": inets ]);
        }
    }  // end loop (verified)

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        7
   Code segments found:         7
   Successfully decoded:        7
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */