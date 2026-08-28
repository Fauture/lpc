/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: reboot_server.o
 * Source: /cmds/cta/reboot_server.c
 * Inherits: (none)
 * Functions: 4 names, 4 code segments found
 * Strings: 25, Variables: 1
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   int reboot_time;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _main(int client_no, mapping paras);
private void create();
int main(int client_no, mapping paras);
private void reboot_server(int client_no, mapping paras);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   reboot_time = 65535;

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=20, nargs=2, insts=20
// --------------------------------------------------------------------
private void _main(int client_no, mapping paras) {
    /* ---- Enhanced Pseudocode ---- */
    reboot_server(client_no, paras);
    paras["msg"] = "reboot_server";
    "/daemons/ctad.c"->send_to_cta(client_no, paras);

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=9, nargs=0, insts=9
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    register_daemon_data("/daemons/ctacommandd.c", "reboot_server", "/cmds/cta/reboot_server.c");

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=13, nargs=2, insts=13
// --------------------------------------------------------------------
int main(int client_no, mapping paras) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_service((: _main, client_no, paras :));
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=212, nargs=2, insts=212
// --------------------------------------------------------------------
private void reboot_server(int client_no, mapping paras) {
    /* ---- Enhanced Pseudocode ---- */
    int delay;
    int interval;
    int times;
    int map;
    mixed t;
    mixed h;
    mixed m;
    mixed s;

    delay = -1;
    interval = 0;
    times = 0;
    switch (paras["reboot_type"]) {
        case "query_time":
        if ((reboot_time < 0)) {
            paras["prompt"] = "服务器没有设置定时重启";
        } else {
            t = (reboot_time - time());
            h = (t / 3600);
            t ?= 3600;
            m = (t / 60);
            s = (t % 60);
            paras["prompt"] = sprintf("服务器还有(%d小时%d分钟%d秒)重启", h, m, s);
        }
        break;
        case "settime":
        reboot_time = time(paras["time"]);
        if ((reboot_time < time())) {
            paras["prompt"] = "无效的设置时间，设置重启时刻比当前时刻早。";
        } else {
            paras["prompt"] = "服务器设置重启时刻成功。";
            delay = (reboot_time - time());
        }
        break;
        case "setdelay":
        delay = atoi(paras["time"]);
        reboot_time = (time() + delay);
        paras["prompt"] = "服务器设置延时重启成功。";
        break;
        case "rebootnow":
        default:
        paras["prompt"] = "服务器立刻准备重启。";
        delay = 2;
        break;
    }  // end switch

    // --- label L0142 ---
    if ((delay >= 0)) {
        delete_timer("rservertimer");
        if (1) {
            set_timer(delay, "rservertimer", "reboot");
            return;
        }
        map = ([ ]);
        if (is_string(paras["broadcast_interval"])) {
            interval = atoi(paras["broadcast_interval"]);
        }
        if (is_string(paras["broadcast_times"])) {
            times = atoi(paras["broadcast_times"]);
        }
        if (((interval > 0) && (times > 0))) {
            map = ([ "broadcast_interval": interval, "broadcast_times": times ]);
        }
        set_timer(delay, "rservertimer", "reboot", map);
    }

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        4
   Code segments found:         4
   Successfully decoded:        4
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */