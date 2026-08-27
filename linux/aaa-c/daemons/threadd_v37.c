/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: threadd.o
 * Source: /daemons/threadd.c
 * Inherits: (none)
 * Inherited programs: /feature/common/daemon.c()
 * Functions: 19 names, 19 code segments found
 * Strings: 39, Variables: 17
 *********************************************************************/

inherit "/feature/common/daemon.c";

/* ================================================================
   Global/Object Variables
   ================================================================ */
   int max_comm_for_gs_thread_num;
   int max_cs_thread_num;
   int max_ddb_service_thread_num;
   int max_http_service_thread_num;
   int max_inter_service_thread_num;
   int max_ldb_service_thread_num;
   int max_ma_service_thread_num;
   int max_rpc_exec_thread_num;
   int max_service_thread_num;
   int max_thread_num;
   int max_trade_service_thread_num;
   int max_user_thread_num;
   int max_web_service_thread_num;
   array rpc_exec_tids;
   mapping stat;
   array threadQ;
   array tids;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void dispatch_web_service(function func, int timeout = -1);
void dispatch_ddb_service(function func, int timeout = -1);
void dispatch_http_service(function func, int timeout = -1);
private void invoke_before_timeout(int time_limit, function f);
void dispatch_ldb_service(function func, int timeout = -1);
void dispatch_rpc_exec(function func, int timeout = -1);
void dispatch_inter_service(function func, int timeout = -1);
private void destruct();
void dispatch_comm_service(int i, function func, int timeout = -1);
int is_rpc_exec_thread(int tid);
void dispatch_trade_service(function func, int timeout = -1);
void dispatch_cs_service(int i, function func, int timeout = -1);
void dispatch_ma_service(function func, int timeout = -1);
mapping get_stat();
void dispatch_service(function func, int timeout = -1);
private void create();
private void daemon(int qid);
void reset_stat(string index);
void dispatch(int qno, function func, int timeout = -1);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   max_user_thread_num = 5;  // /daemons/threadd.c:48
   max_service_thread_num = 20;  // /daemons/threadd.c:73
   max_rpc_exec_thread_num = 0;  // /daemons/threadd.c:92
   max_comm_for_gs_thread_num = 20;  // /daemons/threadd.c:104
   max_cs_thread_num = 0;  // /daemons/threadd.c:108
   max_web_service_thread_num = 0;  // /daemons/threadd.c:112
   max_ddb_service_thread_num = 0;  // /daemons/threadd.c:738
   max_ldb_service_thread_num = 0;
   max_inter_service_thread_num = 0;
   max_ma_service_thread_num = 6;
   max_trade_service_thread_num = 0;
   max_http_service_thread_num = 0;
   max_thread_num = (((((((((((max_user_thread_num + max_service_thread_num) + max_rpc_exec_thread_num) + max_comm_for_gs_thread_num) + max_cs_thread_num) + max_web_service_thread_num) + max_ddb_service_thread_num) + max_ldb_service_thread_num) + max_inter_service_thread_num) + max_ma_service_thread_num) + max_trade_service_thread_num) + max_http_service_thread_num);
   tids = ({ });
   rpc_exec_tids = ({ });

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=39, nargs=2, insts=39
// --------------------------------------------------------------------
void dispatch_web_service(function func, int timeout) {
    /* ---- Enhanced Pseudocode ---- */
    ++stat["dispatch_web_service"];
    if ((timeout > 0)) {
        func = (: invoke_before_timeout, (time() + timeout), func :);
    }
    send_to_queue(threadQ[(((max_user_thread_num + max_comm_for_gs_thread_num) + max_cs_thread_num) + 2)], func);

}

// --------------------------------------------------------------------
// @code  nc=45, nargs=2, insts=45
// --------------------------------------------------------------------
void dispatch_ddb_service(function func, int timeout) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!max_ddb_service_thread_num)) {
        return;
    }
    ++stat["dispatch_ddb_service"];
    if ((timeout > 0)) {
        func = (: invoke_before_timeout, (time() + timeout), func :);
    }
    send_to_queue(threadQ[((((max_user_thread_num + max_comm_for_gs_thread_num) + max_cs_thread_num) + 2) + 1)], func);

}

// --------------------------------------------------------------------
// @code  nc=41, nargs=2, insts=41
// --------------------------------------------------------------------
void dispatch_http_service(function func, int timeout) {
    /* ---- Enhanced Pseudocode ---- */
    ++stat["dispatch_http_service"];
    if ((timeout > 0)) {
        func = (: invoke_before_timeout, (time() + timeout), func :);
    }
    send_to_queue(threadQ[((((max_user_thread_num + max_comm_for_gs_thread_num) + max_cs_thread_num) + 2) + 6)], func);

}

// --------------------------------------------------------------------
// @code  nc=27, nargs=2, insts=27
// --------------------------------------------------------------------
private void invoke_before_timeout(int time_limit, function f) {
    /* ---- Enhanced Pseudocode ---- */
    if (((time() > time_limit) || (!is_function(f)))) {
        ++stat["drop"];
        return;
    }
    (*f)();

}

// --------------------------------------------------------------------
// @code  nc=45, nargs=2, insts=45
// --------------------------------------------------------------------
void dispatch_ldb_service(function func, int timeout) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!max_ldb_service_thread_num)) {
        return;
    }
    ++stat["dispatch_ldb_service"];
    if ((timeout > 0)) {
        func = (: invoke_before_timeout, (time() + timeout), func :);
    }
    send_to_queue(threadQ[((((max_user_thread_num + max_comm_for_gs_thread_num) + max_cs_thread_num) + 2) + 2)], func);

}

// --------------------------------------------------------------------
// @code  nc=35, nargs=2, insts=35
// --------------------------------------------------------------------
void dispatch_rpc_exec(function func, int timeout) {
    /* ---- Enhanced Pseudocode ---- */
    ++stat["dispatch_rpc_exec"];
    if ((timeout > 0)) {
        func = (: invoke_before_timeout, (time() + timeout), func :);
    }
    send_to_queue(threadQ[(max_user_thread_num + 1)], func);

}

// --------------------------------------------------------------------
// @code  nc=45, nargs=2, insts=45
// --------------------------------------------------------------------
void dispatch_inter_service(function func, int timeout) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!max_inter_service_thread_num)) {
        return;
    }
    ++stat["dispatch_inter_service"];
    if ((timeout > 0)) {
        func = (: invoke_before_timeout, (time() + timeout), func :);
    }
    send_to_queue(threadQ[((((max_user_thread_num + max_comm_for_gs_thread_num) + max_cs_thread_num) + 2) + 3)], func);

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=366, nargs=0, insts=366
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    int i;

    printf("Stop THREAD_D.all services.\n");
    i = 0;

    // --- label L0009 ---
    while (i < max_user_thread_num) {
        send_to_queue(threadQ[i], 0);
        ++i;
    }  // end loop (verified)
    i = 0;

    // --- label L0029 ---
    while (i < max_service_thread_num) {
        send_to_queue(threadQ[max_user_thread_num], 0);
        ++i;
    }  // end loop (verified)
    i = 0;

    // --- label L0049 ---
    while (i < max_rpc_exec_thread_num) {
        send_to_queue(threadQ[(max_user_thread_num + 1)], 0);
        ++i;
    }  // end loop (verified)
    i = 0;

    // --- label L0071 ---
    while (i < max_comm_for_gs_thread_num) {
        send_to_queue(threadQ[((max_user_thread_num + 2) + i)], 0);
        ++i;
    }  // end loop (verified)
    i = 0;

    // --- label L0095 ---
    while (i < max_cs_thread_num) {
        send_to_queue(threadQ[(((max_user_thread_num + max_comm_for_gs_thread_num) + 2) + i)], 0);
        ++i;
    }  // end loop (verified)
    i = 0;

    // --- label L0121 ---
    while (i < max_web_service_thread_num) {
        send_to_queue(threadQ[(((max_user_thread_num + max_comm_for_gs_thread_num) + max_cs_thread_num) + 2)], 0);
        ++i;
    }  // end loop (verified)
    i = 0;

    // --- label L0147 ---
    while (i < max_ddb_service_thread_num) {
        send_to_queue(threadQ[((((max_user_thread_num + max_comm_for_gs_thread_num) + max_cs_thread_num) + 2) + 1)], 0);
        ++i;
    }  // end loop (verified)
    i = 0;

    // --- label L0175 ---
    while (i < max_ldb_service_thread_num) {
        send_to_queue(threadQ[((((max_user_thread_num + max_comm_for_gs_thread_num) + max_cs_thread_num) + 2) + 2)], 0);
        ++i;
    }  // end loop (verified)
    i = 0;

    // --- label L0203 ---
    while (i < max_inter_service_thread_num) {
        send_to_queue(threadQ[((((max_user_thread_num + max_comm_for_gs_thread_num) + max_cs_thread_num) + 2) + 3)], 0);
        ++i;
    }  // end loop (verified)
    i = 0;

    // --- label L0231 ---
    while (i < max_ma_service_thread_num) {
        send_to_queue(threadQ[((((max_user_thread_num + max_comm_for_gs_thread_num) + max_cs_thread_num) + 2) + 4)], 0);
        ++i;
    }  // end loop (verified)
    i = 0;

    // --- label L0259 ---
    while (i < max_trade_service_thread_num) {
        send_to_queue(threadQ[((((max_user_thread_num + max_comm_for_gs_thread_num) + max_cs_thread_num) + 2) + 5)], 0);
        ++i;
    }  // end loop (verified)
    i = 0;

    // --- label L0287 ---
    while (i < max_http_service_thread_num) {
        send_to_queue(threadQ[((((max_user_thread_num + max_comm_for_gs_thread_num) + max_cs_thread_num) + 2) + 6)], 0);
        ++i;
    }  // end loop (verified)
    i = 0;

    // --- label L0315 ---
    while (i < max_thread_num) {

        // --- label L0319 ---
        if ((get_thread_id(tids[i]) != -1)) {
            sleep(0.1);
        }
    }  // end loop (verified)
    ++i;
}  // end loop (verified)
i = 0;

// --- label L0340 ---
while ((i < (((max_user_thread_num + 9) + max_comm_for_gs_thread_num) + max_cs_thread_num))) {
    delete_queue(threadQ[i]);
    ++i;
}  // end loop (verified)
printf("THREAD_D.all services done.\n");

}

// --------------------------------------------------------------------
// @code  nc=43, nargs=3, insts=43
// --------------------------------------------------------------------
void dispatch_comm_service(int i, function func, int timeout) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!max_comm_for_gs_thread_num)) {
        return;
    }
    ++stat["dispatch_comm_service"];
    if ((timeout > 0)) {
        func = (: invoke_before_timeout, (time() + timeout), func :);
    }
    send_to_queue(threadQ[((max_user_thread_num + 2) + (i % max_comm_for_gs_thread_num))], func);

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=10, nargs=1, insts=10
// --------------------------------------------------------------------
int is_rpc_exec_thread(int tid) {
    /* ---- Enhanced Pseudocode ---- */
    return (member_array(tid, rpc_exec_tids) >= 0);

}

// --------------------------------------------------------------------
// @code  nc=41, nargs=2, insts=41
// --------------------------------------------------------------------
void dispatch_trade_service(function func, int timeout) {
    /* ---- Enhanced Pseudocode ---- */
    ++stat["dispatch_trade_service"];
    if ((timeout > 0)) {
        func = (: invoke_before_timeout, (time() + timeout), func :);
    }
    send_to_queue(threadQ[((((max_user_thread_num + max_comm_for_gs_thread_num) + max_cs_thread_num) + 2) + 5)], func);

}

// --------------------------------------------------------------------
// @code  nc=43, nargs=3, insts=43
// --------------------------------------------------------------------
void dispatch_cs_service(int i, function func, int timeout) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!max_cs_thread_num)) {
        return;
    }
    ++stat["dispatch_cs_service"];
    if ((timeout > 0)) {
        func = (: invoke_before_timeout, (time() + timeout), func :);
    }
    send_to_queue(threadQ[((max_user_thread_num + 2) + (i % max_cs_thread_num))], func);

}

// --------------------------------------------------------------------
// @code  nc=41, nargs=2, insts=41
// --------------------------------------------------------------------
void dispatch_ma_service(function func, int timeout) {
    /* ---- Enhanced Pseudocode ---- */
    ++stat["dispatch_ma_service"];
    if ((timeout > 0)) {
        func = (: invoke_before_timeout, (time() + timeout), func :);
    }
    send_to_queue(threadQ[((((max_user_thread_num + max_comm_for_gs_thread_num) + max_cs_thread_num) + 2) + 4)], func);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
mapping get_stat() {
    /* ---- Enhanced Pseudocode ---- */
    return stat;

}

// --------------------------------------------------------------------
// @code  nc=33, nargs=2, insts=33
// --------------------------------------------------------------------
void dispatch_service(function func, int timeout) {
    /* ---- Enhanced Pseudocode ---- */
    ++stat["dispatch_service"];
    if ((timeout > 0)) {
        func = (: invoke_before_timeout, (time() + timeout), func :);
    }
    send_to_queue(threadQ[max_user_thread_num], func);

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=498, nargs=0, insts=498
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    int para;

    require_uid();
    set_euid(0);
    reset_stat("all");
    printf("Start THREAD_D.all services.\n");
    threadQ = allocate_array((((max_user_thread_num + 9) + max_comm_for_gs_thread_num) + max_cs_thread_num));
    i = 0;

    // --- label L0028 ---
    while ((i < (((max_user_thread_num + 9) + max_comm_for_gs_thread_num) + max_cs_thread_num))) {
        threadQ[i] = create_queue("qthread");
        ++i;
    }  // end loop (verified)
    if (is_string(para = expand_macro("MAX_SERVICE_THREAD_NUM"))) {
        para = atoi(para);
        if ((para < 1)) {
            para = 1;
        }
        max_thread_num -= max_service_thread_num;
        max_service_thread_num = para;
        max_thread_num += max_service_thread_num;
        printf("THREAD_D.max_service_thread_num is updated to %d.\n", para);
    }
    i = 0;

    // --- label L0091 ---
    while (i < max_user_thread_num) {
        tids += ({ create_thread(("tthreaduser" + (i + 1)), "daemon", threadQ[i]) });
        ++i;
    }  // end loop (verified)
    i = 0;

    // --- label L0120 ---
    while (i < max_service_thread_num) {
        tids += ({ create_thread(("tthreadsrv" + (i + 1)), "daemon", threadQ[max_user_thread_num]) });
        ++i;
    }  // end loop (verified)
    i = 0;

    // --- label L0149 ---
    while (i < max_rpc_exec_thread_num) {
        tids += ({ create_thread(("tthreadrpc" + (i + 1)), "daemon", threadQ[(max_user_thread_num + 1)]) });
        ++i;
    }  // end loop (verified)
    i = 0;

    // --- label L0180 ---
    while (i < max_comm_for_gs_thread_num) {
        tids += ({ create_thread(("tthreadcomm" + (i + 1)), "daemon", threadQ[((max_user_thread_num + 2) + i)]) });
        ++i;
    }  // end loop (verified)
    i = 0;

    // --- label L0212 ---
    while (i < max_cs_thread_num) {
        tids += ({ create_thread(("tthreadcs" + (i + 1)), "daemon", threadQ[(((max_user_thread_num + max_comm_for_gs_thread_num) + 2) + i)]) });
        ++i;
    }  // end loop (verified)
    i = 0;

    // --- label L0246 ---
    while (i < max_web_service_thread_num) {
        tids += ({ create_thread(("tthreadweb" + (i + 1)), "daemon", threadQ[(((max_user_thread_num + max_comm_for_gs_thread_num) + max_cs_thread_num) + 2)]) });
        ++i;
    }  // end loop (verified)
    i = 0;

    // --- label L0280 ---
    while (i < max_ddb_service_thread_num) {
        tids += ({ create_thread(("tthreadddbsrv" + (i + 1)), "daemon", threadQ[((((max_user_thread_num + max_comm_for_gs_thread_num) + max_cs_thread_num) + 2) + 1)]) });
        ++i;
    }  // end loop (verified)
    i = 0;

    // --- label L0316 ---
    while (i < max_ldb_service_thread_num) {
        tids += ({ create_thread(("tthreadldbsrv" + (i + 1)), "daemon", threadQ[((((max_user_thread_num + max_comm_for_gs_thread_num) + max_cs_thread_num) + 2) + 2)]) });
        ++i;
    }  // end loop (verified)
    i = 0;

    // --- label L0352 ---
    while (i < max_inter_service_thread_num) {
        tids += ({ create_thread(([ "name": ("tthreadintersrv" + (i + 1)), "stack_size": 512 ]), "daemon", threadQ[((((max_user_thread_num + max_comm_for_gs_thread_num) + max_cs_thread_num) + 2) + 3)]) });
        ++i;
    }  // end loop (verified)
    i = 0;

    // --- label L0392 ---
    while (i < max_ma_service_thread_num) {
        tids += ({ create_thread(("tthreadmasrv" + (i + 1)), "daemon", threadQ[((((max_user_thread_num + max_comm_for_gs_thread_num) + max_cs_thread_num) + 2) + 4)]) });
        ++i;
    }  // end loop (verified)
    i = 0;

    // --- label L0428 ---
    while (i < max_trade_service_thread_num) {
        tids += ({ create_thread(("tthreadtradesrv" + (i + 1)), "daemon", threadQ[((((max_user_thread_num + max_comm_for_gs_thread_num) + max_cs_thread_num) + 2) + 5)]) });
        ++i;
    }  // end loop (verified)
    i = 0;

    // --- label L0464 ---
    while (i < max_http_service_thread_num) {
        tids += ({ create_thread(("tthreadhttpsrv" + (i + 1)), "daemon", threadQ[((((max_user_thread_num + max_comm_for_gs_thread_num) + max_cs_thread_num) + 2) + 6)]) });
        ++i;
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @code  nc=59, nargs=1, insts=59
// --------------------------------------------------------------------
private void daemon(int qid) {
    /* ---- Enhanced Pseudocode ---- */
    int msg;

    set_as_daemon();
    printf("THREAD_D.service(%d) daemon start...\n", qid);
    if ((qid == threadQ[(max_user_thread_num + 1)])) {
        rpc_exec_tids += ({ get_thread_id() });
    }
    while ((receive_from_queue(qid, &msg) == 0)) {
        if (is_function(msg)) {
            try {
                (*msg)();
            } catch { /* error handled */ }

            // --- label L0038 ---
            continue;
        }
        if ((msg != 0)) {
            printf("Bad message in THREAD_D, expected function got %O.\n", msg);
        }
    }  // end loop (verified)
    printf("THREAD_D.service daemon done.\n");
    return;

}

// --------------------------------------------------------------------
// @code  nc=64, nargs=1, insts=64
// --------------------------------------------------------------------
void reset_stat(string index) {
    /* ---- Enhanced Pseudocode ---- */
    if ((index == "all")) {
        stat = ([ "dispatch": 0, "dispatch_service": 0, "dispatch_comm_service": 0, "dispatch_cs_service": 0, "dispatch_rpc_exec": 0, "dispatch_web_service": 0, "dispatch_ddb_service": 0, "dispatch_ldb_service": 0, "dispatch_inter_service": 0, "dispatch_ma_service": 0, "dispatch_trade_service": 0, "dispatch_http_service": 0, "drop": 0, "start_time": time() ]);
        return;
    }
    if ((index == "start_time")) {
        stat[index] = time();
        return;
    }
    if (stat[index]) {
        stat[index] = 0;
    }

}

// --------------------------------------------------------------------
// @code  nc=35, nargs=3, insts=35
// --------------------------------------------------------------------
void dispatch(int qno, function func, int timeout) {
    /* ---- Enhanced Pseudocode ---- */
    ++stat["dispatch"];
    if ((timeout > 0)) {
        func = (: invoke_before_timeout, (time() + timeout), func :);
    }
    send_to_queue(threadQ[(qno % max_user_thread_num)], func);

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        19
   Code segments found:         19
   Successfully decoded:        19
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          1
      - /feature/common/daemon.c()
   ================================================================ */