/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: cmd_cta_get_dist_info.o
 * Source: /aaa/daemons/cta/cmd_cta_get_dist_info.c
 * Inherits: (none)
 * Functions: 3 names, 3 code segments found
 * Strings: 33, Variables: 0
 *********************************************************************/

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void _main(int client_no, mapping paras);
private void create();
int main(int client_no, mapping paras);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=293, nargs=2, insts=293
// --------------------------------------------------------------------
private void _main(int client_no, mapping paras) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    mapping ccs_config;
    mixed dba_config;
    mapping spa_config;
    mixed gs_config;
    mapping tts_config;
    mixed dist;
    int server_name;
    mapping server_info;
    mapping csa_info;

    do {
        if ((!is_string(dist = paras["dist"]))) {
            paras["result"] = 1;
            paras["prompt"] = "区组配置不对\n";
            break;
        }
        tts_config = "/aaa/daemons/configd.c"->get_tts_info_by_dist(dist);
        if (is_mapping(tts_config)) {
            paras["tts"] = sprintf("%s:%d", tts_config["ip"], tts_config["port"]);
        }
        csa_info = "/aaa/daemons/configd.c"->get_csa_info_by_dist(dist);
        if (((((is_mapping(csa_info) && is_string(csa_info["ip"])) && (csa_info["ip"] != "")) && is_int(csa_info["port"])) && (csa_info["port"] != 0))) {
            paras["csa"] = sprintf("%s:%d", csa_info["ip"], csa_info["port"]);
        }
        ccs_config = "/aaa/daemons/configd.c"->get_ccs_info_by_dist(dist);
        if (is_mapping(ccs_config)) {
            paras["ccs"] = sprintf("%s:%d", ccs_config["ip"], ccs_config["port"]);
        }
        dba_config = "/aaa/daemons/configd.c"->get_dba_list_by_dist(dist);
        paras["dba"] = "";
        if (is_array(dba_config)) {
            for (i = 0; i <= (sizeof(dba_config) - 1); i++) {
                paras["dba"] += sprintf("%s:%d;", dba_config[i]["ip"], dba_config[i]["port"]);
            }  // end loop (verified)
        }
        if (paras["dba"]) {
            paras["dba"] = paras["dba"][0..<2];
        }
        spa_config = "/aaa/daemons/configd.c"->get_spa_info_by_dist(dist);
        if (is_mapping(spa_config)) {
            paras["spa"] = sprintf("%s:%d", spa_config["ip"], spa_config["port"]);
        }
        gs_config = "/aaa/daemons/configd.c"->get_server_list_by_dist(dist);
        paras["gs"] = "";
        if (is_mapping(gs_config)) {
            loop (server_name in keys(gs_config)) {
                server_info = gs_config[server_name];
                paras["gs"] += sprintf("%s:%d:%d;", server_info["ip"], server_info["port"], server_info["id"]);
            }  // end loop (verified)
        }
        if (paras["gs"]) {
            paras["gs"] = paras["gs"][0..<2];
        }
        paras["result"] = 0;
        paras["prompt"] = "获取区组资讯成功";
    } while (0);

    // --- label L0279 ---
    paras["msg"] = "get_dist_info";
    "/daemons/ctad.c"->send_to_cta_ex(client_no, paras);

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=9, nargs=0, insts=9
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    register_daemon_data("/daemons/ctacommandd.c", "get_dist_info", "/aaa/daemons/cta/cmd_cta_get_dist_info.c");

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=15, nargs=2, insts=15
// --------------------------------------------------------------------
int main(int client_no, mapping paras) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch(random(10), (: _main, client_no, paras :));
    return 1;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        3
   Code segments found:         3
   Successfully decoded:        3
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */