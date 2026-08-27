/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_ma_oper.o
 * Source: /aaa/daemons/server/cmd_ma_oper.c
 * Inherits: (none)
 * Inherited programs: /inherit/misc/handler.c()
 * Functions: 8 names, 8 code segments found
 * Strings: 75, Variables: 0
 *********************************************************************/

inherit "/inherit/misc/handler.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _zero_coin(mixed server, int cookie, string admintool, array args);
private void _control_account(mixed server, int cookie, string admintool, array args);
private void _block_account(mixed server, int cookie, string admintool, array args);
private void _temp_block_account(mixed server, int cookie, string admintool, array args);
private void _oper_local_encrypt(mixed server, int cookie, string admintool, array args);
private void _batch_block_account(mixed server, int cookie, string admintool, array args);
void main(object server, int cookie, string command, string admintool, string target, int num, array args);
private void _block_gm_account(mixed server, int cookie, string admintool, array args);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=200, nargs=4, insts=200
// --------------------------------------------------------------------
private void _zero_coin(mixed server, int cookie, string admintool, array args) {
    /* ---- Enhanced Pseudocode ---- */
    int ret;
    mixed cno;
    int gold_coin;
    int silver_coin;
    mixed sql_cmd;
    mixed account;
    string prompt;
    mixed ip;
    string msg;
    string memo;
    mapping account_info;

    ip = (is_object(server) ? server->get_peer_ip() : "");
    msg = "";
    memo = "";
    account = args[0];
    do {
        account_info = "/aaa/daemons/accountd.c"->query_account_info(account);
        if ((!is_mapping(account_info))) {
            prompt = "AAA无法获取帐号信息，操作失败";
            ret = 0;
            break;
        }
        if ((account_info["checksum"] != "/aaa/daemons/accountd.c"->calc_account_checksum(account_info))) {
            prompt = "帐号数据被破坏(校验和不匹配)，操作失败。";
            ret = 0;
            break;
        }
        gold_coin = atoi(account_info["gold_coin"]);
        silver_coin = atoi(account_info["silver_coin"]);
        if (((gold_coin == 0) && (silver_coin == 0))) {
            prompt = "清除帐号元宝操作成功（帐号的金、银元宝数目清除前已经为空）";
            ret = 0;
            break;
        }
        account_info["gold_coin"] = 0;
        account_info["silver_coin"] = 0;
        if ((!"/aaa/daemons/accountd.c"->update_account_info(account_info))) {
            prompt = "清除帐号元宝操作失败(更新帐号数据失败)，可能数据库异常";
            ret = 0;
            break;
        }
        prompt = "清除帐号元宝操作成功";
        ret = 1;
        msg = sprintf("金元宝：%d,银元宝：%d", gold_coin, silver_coin);
        memo = sprintf("%d", (gold_coin + silver_coin));
    } while (0);

    // --- label L0154 ---
    sql_cmd = mysql_bind_parameters("insert into acc_log(ip, time,account, dist, type, msg, memo) values(?, ?, ?, ?, ?, ?, ?)", ip, ctime(time(), 3)[0..13], account, "", "zero_coin", msg, memo);
    if (((cno = "/daemons/dbd.c"->assure_connection("adb")) >= 0)) {
        (*f_execute)(cno, sql_cmd);
    }
    if (is_object(server)) {
        server->send_message(16881, cookie, ret, prompt, 0);
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=167, nargs=4, insts=167
// --------------------------------------------------------------------
private void _control_account(mixed server, int cookie, string admintool, array args) {
    /* ---- Enhanced Pseudocode ---- */
    int oper;
    int ret;
    mixed cno;
    string password;
    string permit_ip;
    string permit_id;
    mixed account;
    string prompt;
    mixed sql_cmd;
    mixed msg;

    account = args[0];
    oper = atoi(args[1]);
    if ((1 == oper)) {
        password = args[2];
        sscanf(password, "des:%s", &password);
        password = save_string(md5(get_raw_password(("!*" + password))))[1..<2];
        permit_ip = args[3];
        permit_id = (string)args[4];
    } else {
        password = "";
        permit_ip = "";
        permit_id = "";
    }
    try {
        ret = "/aaa/daemons/accountd.c"->control_account(account, oper, password, permit_ip, permit_id, &prompt);
    } catch { /* error handled */ }

    // --- label L0088 ---
    if (0) {
        prompt = "AAA 执行出错";
        ret = 0;
    } else {
        msg = sprintf("离线帐号管理器%s%s帐号", admintool, ((oper != 1) ? "解除接管" : "接管"));
        sql_cmd = mysql_bind_parameters("insert into acc_log(ip, time,account, dist, type, msg, memo) values(?, ?, ?, ?, ?, ?, ?)", server->get_peer_ip(), ctime(time(), 3)[0..13], account, "", "control_account", (msg + (ret ? "成功" : "失败")), "");
        if (((cno = "/daemons/dbd.c"->assure_connection("adb")) >= 0)) {
            (*f_execute)(cno, sql_cmd);
        }
    }
    if (is_object(server)) {
        server->send_message(16881, cookie, ret, prompt, 0);
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=253, nargs=4, insts=253
// --------------------------------------------------------------------
private void _block_account(mixed server, int cookie, string admintool, array args) {
    /* ---- Enhanced Pseudocode ---- */
    int interval;
    int size;
    mixed to_gy;
    mixed ignore_data;
    int ret;
    mixed cno;
    mixed sql_cmd;
    mixed msg;
    mixed reason_memo;
    mixed reason_memo_cpy;
    mixed account;
    mixed reason;
    string prompt;
    string oper_type;
    string dist;

    size = sizeof(args);
    if (((size < 3) || (size > 6))) {
        return;
    }
    dist = (string)server->query("dist");
    account = args[0];
    interval = atoi(args[1]);
    reason = args[2];
    reason_memo = ((size >= 4) ? (string)args[3] : "");
    reason_memo_cpy = ((sizeof(reason_memo) < 1) ? "" : (("(" + reason_memo) + ")"));
    to_gy = ((size >= 5) ? atoi(args[4]) : 0);
    ignore_data = ((size >= 6) ? atoi(args[5]) : 0);
    try {
        ret = "/aaa/daemons/accountd.c"->block_account(dist, account, interval, (reason + reason_memo_cpy), ignore_data, &prompt);
    } catch { /* error handled */ }

    // --- label L0117 ---
    if (0) {
        prompt = "AAA 执行出错";
        ret = 0;
    } else {
        if ((to_gy && (-2 != interval))) {
            "/aaa/daemons/admintoold.c"->add_notice_gy(account, admintool, interval, reason);
        }
        cno = "/daemons/dbd.c"->assure_connection("adb");
        if ((-2 != interval)) {
            oper_type = "block_account";
            }
            msg = sprintf("离线帐号管理器%s%s帐号%s，原因:%s =>", admintool, ((interval == 0) ? "解封" : "封闭"), ((interval < 0) ? "永久" : (("" + interval) + "秒")), reason);
        } else {
            oper_type = "edit_reason";
            msg = sprintf("离线帐号管理器%s%s帐号%s，原因:%s =>", admintool, "修改", "封闭原因", reason);
        }
        sql_cmd = mysql_bind_parameters("insert into acc_log(ip, time,account, dist, type, msg, memo) values(?, ?, ?, ?, ?, ?, ?)", server->get_peer_ip(), ctime(time(), 3)[0..13], account, "", oper_type, (msg + (ret ? "成功" : "失败")), sprintf("to_gy=%d", to_gy));
        (*f_execute)(cno, sql_cmd);
    }
    if (is_object(server)) {
        server->send_message(16881, cookie, ret, prompt, 0);
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=203, nargs=4, insts=203
// --------------------------------------------------------------------
private void _temp_block_account(mixed server, int cookie, string admintool, array args) {
    /* ---- Enhanced Pseudocode ---- */
    int interval;
    int size;
    int ret;
    mixed cno;
    mixed sql_cmd;
    mixed msg;
    mixed reason_memo;
    mixed reason_memo_cpy;
    mixed account;
    mixed reason;
    string prompt;
    string oper_type;
    string dist;

    if ((!is_object(server))) {
        return;
    }
    size = sizeof(args);
    if (((size < 3) || (size > 4))) {
        return;
    }
    dist = (string)server->query("dist");
    account = args[0];
    interval = atoi(args[1]);
    reason = args[2];
    reason_memo = ((size == 4) ? (string)args[3] : "");
    reason_memo_cpy = ((sizeof(reason_memo) < 1) ? "" : (("(" + reason_memo) + ")"));
    if ((interval < 0)) {
        server->send_message(16881, cookie, 0, "不能永久封停帐号！", 0);
        return;
    }
    try {
        ret = "/aaa/daemons/accountd.c"->temp_block_account(dist, account, interval, (reason + reason_memo_cpy), &prompt);
    } catch { /* error handled */ }

    // --- label L0110 ---
    if (0) {
        prompt = "AAA 执行出错";
        ret = 0;
    } else {
        cno = "/daemons/dbd.c"->assure_connection("adb");
        oper_type = "temp_block_account";
        msg = sprintf("离线帐号管理器%s%s帐号%s，原因:%s =>", admintool, ((interval == 0) ? "解除临时停封" : "临时停封"), ((interval == 0) ? "" : (("" + interval) + "秒")), reason);
        sql_cmd = mysql_bind_parameters("insert into acc_log(ip, time,account, dist, type, msg, memo) values(?, ?, ?, ?, ?, ?, ?)", server->get_peer_ip(), ctime(time(), 3)[0..13], account, "", oper_type, (msg + (ret ? "成功" : "失败")), "");
        (*f_execute)(cno, sql_cmd);
    }
    if (is_object(server)) {
        server->send_message(16881, cookie, ret, prompt, 0);
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=207, nargs=4, insts=207
// --------------------------------------------------------------------
private void _oper_local_encrypt(mixed server, int cookie, string admintool, array args) {
    /* ---- Enhanced Pseudocode ---- */
    int cno;
    int ret;
    mixed arr;
    mixed account;
    mixed oper;
    mixed sql_cmd;
    string prompt;
    mapping m;

    ret = 1;
    account = args[0];
    oper = atoi(args[1]);
    sql_cmd = mysql_bind_parameters("select * from local_encrypt where account = ?", account);
    do {
        cno = "/daemons/dbd.c"->assure_connection("adb");
        if ((cno < 1)) {
            ret = 0;
            prompt = "数据库连接失败";
            break;
        }
        arr = mysql_select(cno, sql_cmd);
        if ((!is_array(arr))) {
            ret = 0;
            prompt = "数据库操作异常";
            break;
        }
        if (((!sizeof(arr)) || (!is_mapping(m = arr[0])))) {
            prompt = "非大文件加密帐号";
            break;
        }
        if ((!oper)) {
            if (atoi(m["state"])) {
                prompt = "大文件加密帐号，已启用";
            } else {
                prompt = "大文件加密帐号，未启用";
            }
            break;
        }
        sql_cmd = mysql_bind_parameters("delete from local_encrypt where account = ?", account);
        cno = "/daemons/dbd.c"->assure_connection("adb");
        if ((cno < 1)) {
            ret = 0;
            prompt = "数据库连接失败";
            break;
        }
        if (((*f_execute)(cno, sql_cmd) < 1)) {
            ret = 0;
            prompt = "数据库操作异常";
            break;
        }
        prompt = "清除大文件加密标记成功";
        cno = "/daemons/dbd.c"->assure_connection("adb");
        if ((cno < 1)) {
            break;
        }
        sql_cmd = mysql_bind_parameters("insert into acc_log(ip, time,account, dist, type, msg, memo) values(?, ?, ?, ?, ?, ?, ?)", "", ctime(time(), 3)[0..13], account, "", "oper_local_encrypt", sprintf("被管理员%s成功清除了大文件加密标记", admintool), save_string(m));
        (*f_execute)(cno, sql_cmd);
    } while (0);

    // --- label L0193 ---
    if (is_object(server)) {
        server->send_message(16881, cookie, ret, prompt, 0);
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=404, nargs=4, insts=404
// --------------------------------------------------------------------
private void _batch_block_account(mixed server, int cookie, string admintool, array args) {
    /* ---- Enhanced Pseudocode ---- */
    int interval;
    int size;
    mixed to_gy;
    mixed ignore_data;
    mixed count;
    int i;
    int ret;
    mixed cno;
    mixed arr_account;
    mixed paras;
    mixed vals;
    mixed paras_input;
    mixed vals_input;
    mixed sql_cmd;
    mixed msg;
    mixed reason_memo;
    mixed reason_memo_cpy;
    mixed reason;
    string prompt;
    string oper_type;
    string dist;
    mixed account;

    size = sizeof(args);
    if (((size < 3) || (size > 6))) {
        return;
    }
    dist = (string)server->query("dist");
    arr_account = (explode(args[0], ",") - ({ "" }));
    interval = atoi(args[1]);
    reason = args[2];
    reason_memo = ((size >= 4) ? (string)args[3] : "");
    reason_memo_cpy = ((sizeof(reason_memo) < 1) ? "" : (("(" + reason_memo) + ")"));
    to_gy = ((size >= 5) ? atoi(args[4]) : 0);
    ignore_data = ((size >= 6) ? atoi(args[5]) : 0);
    try {
        ret = "/aaa/daemons/accountd.c"->batch_block_account(dist, arr_account, interval, (reason + reason_memo_cpy), ignore_data, &prompt);
    } catch { /* error handled */ }

    // --- label L0122 ---
    if (0) {
        prompt = "AAA 执行出错";
        ret = 0;
    } else if (ret) {
        if ((to_gy && (-2 != interval))) {
            loop (account in arr_account) {
                "/aaa/daemons/admintoold.c"->add_notice_gy(account, admintool, interval, reason);
            }  // end loop (verified)
        }
        if ((-2 != interval)) {
            oper_type = "block_account";
            }
            msg = sprintf("离线帐号管理器%s%s帐号%s，原因:%s =>", admintool, ((interval == 0) ? "解封" : "封闭"), ((interval < 0) ? "永久" : (("" + interval) + "秒")), reason);
        } else {
            oper_type = "edit_reason";
            msg = sprintf("离线帐号管理器%s%s帐号%s，原因:%s =>", admintool, "修改", "封闭原因", reason);
        }
        if (((cno = "/daemons/dbd.c"->assure_connection("adb")) > -1)) {
            count = sizeof(arr_account);
            vals_input = sprintf("(%s)", implode(allocate_array(7, "?"), ","));
            paras_input = allocate_array(count, vals_input);
            sql_cmd = sprintf("insert into acc_log(ip, time, account, dist, type, msg, memo) values %s", implode(paras_input, ","));
            vals = ({ server->get_peer_ip(), ctime(time(), 3), "", "", oper_type, (msg + (ret ? "成功" : "失败")), sprintf("to_gy=%d", to_gy) });
            paras = allocate_array((7 * count));
            for (i = 0; i <= (count - 1); i++) {
                paras[((7 * i) + 0)] = vals[0];
                paras[((7 * i) + 1)] = vals[1];
                paras[((7 * i) + 2)] = arr_account[i];
                paras[((7 * i) + 3)] = vals[3];
                paras[((7 * i) + 4)] = vals[4];
                paras[((7 * i) + 5)] = vals[5];
                paras[((7 * i) + 6)] = vals[6];
            }  // end loop (verified)
            sql_cmd = mysql_bind_parameters_arr(sql_cmd, paras);
            (*f_execute)(cno, sql_cmd);
        }
    }
    if (is_object(server)) {
        server->send_message(16881, cookie, ret, prompt, 0);
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=180, nargs=7, insts=180
// --------------------------------------------------------------------
void main(object server, int cookie, string command, string admintool, string target, int num, array args) {
    /* ---- Enhanced Pseudocode ---- */
    mixed account;

    if ((((sizeof(args) < 1) || (!is_string(account = args[0]))) || (!sizeof(account)))) {
        server->send_message(16881, cookie, 0, "Bad args", 0);
        return;
    }
    switch (command) {
        case "block_gm_account":
        "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _block_gm_account, server, cookie, admintool, args :));
        return;
        case "block_account":
        "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _block_account, server, cookie, admintool, args :));
        return;
        case "batch_block_account":
        "/daemons/threadd.c"->dispatch_ma_service((: _batch_block_account, server, cookie, admintool, args :));
        return;
        case "control_account":
        "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _control_account, server, cookie, admintool, args :));
        return;
        case "zero_coin":
        "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _zero_coin, server, cookie, admintool, args :));
        return;
        case "oper_local_encrypt":
        "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _oper_local_encrypt, server, cookie, admintool, args :));
        return;
        case "temp_block_account":
        "/daemons/threadd.c"->dispatch_comm_service((sizeof(account) ? account[0] : 0), (: _temp_block_account, server, cookie, admintool, args :));
        return;
        default:
        server->send_message(16881, cookie, 0, ("AAA目前不支持" + command), 0);
        return;
    }  // end switch

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=118, nargs=4, insts=118
// --------------------------------------------------------------------
private void _block_gm_account(mixed server, int cookie, string admintool, array args) {
    /* ---- Enhanced Pseudocode ---- */
    int ret;
    mixed cno;
    mixed account;
    string prompt;
    mixed msg;
    mixed sql_cmd;
    string dist;

    if ((!is_object(server))) {
        return;
    }
    if ((sizeof(args) != 1)) {
        return;
    }
    dist = (string)server->query("dist");
    account = args[0];
    try {
        ret = "/aaa/daemons/accountd.c"->block_account(dist, account, -1, "回收GM号", 0, &prompt, 1);
    } catch { /* error handled */ }

    // --- label L0045 ---
    if (0) {
        prompt = "AAA 执行出错";
        ret = 0;
    } else {
        cno = "/daemons/dbd.c"->assure_connection("adb");
        msg = sprintf("离线帐号管理器%s%s帐号%s，原因:%s =>", admintool, "封闭", "永久", "回收GM号");
        sql_cmd = mysql_bind_parameters("insert into acc_log(ip, time,account, dist, type, msg, memo) values(?, ?, ?, ?, ?, ?, ?)", server->get_peer_ip(), ctime(time(), 3)[0..13], account, "", "block_account", (msg + (ret ? "成功" : "失败")), "");
        (*f_execute)(cno, sql_cmd);
    }
    if (is_object(server)) {
        server->send_message(16881, cookie, ret, prompt, 0);
    }

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        8
   Code segments found:         8
   Successfully decoded:        8
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          1
      - /inherit/misc/handler.c()
   ================================================================ */