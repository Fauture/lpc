/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: coin_traded.o
 * Source: /aaa/daemons/coin_traded.c
 * Inherits: (none)
 * Functions: 5 names, 5 code segments found
 * Strings: 17, Variables: 2
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   int _status;
   string coin_trade_server;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

mapping cointrad_success_notice(string username, string exchangetime, int coins, string orderid, string server);
int get_status();
private void do_init();
private void create();
void set_status(int s);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   coin_trade_server = "";  // /aaa/daemons/coin_traded.c:118
   _status = 0;

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=112, nargs=5, insts=112
// --------------------------------------------------------------------
mapping cointrad_success_notice(string username, string exchangetime, int coins, string orderid, string server) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ret;
    mixed url;
    mixed checkvalue;
    mixed strServ;
    mapping m;

    m = ([ "result": 0, "reason": "" ]);
    if ((!get_status())) {
        return m;
    }
    strServ = convert_to_url_encode(server);
    checkvalue = save_string(md5(sprintf("%s%s%s%s%s%s%s", username, exchangetime, (string)coins, orderid, "2", strServ, "6wI#=vp|rNTu^$")))[1..<2];
    url = sprintf("http://%s/WenDaoServiceV2/exchange.ashx?username=%s&exchangetime=%s&coins=%d&orderid=%s&gameid=%s&servername=%s&checkvalue=%s", coin_trade_server, username, exchangetime, coins, orderid, "2", strServ, checkvalue);
    ret = "/daemons/http_clientd.c"->http_get_web(url);
    switch (ret) {
        case "SUCESSED":
        break;
        case "FAILED":
        m["result"] = 1;
        m["reason"] = "NOTICE_FAILED";
        break;
        case "SYSTEM_ERROR":
        m["result"] = 2;
        m["reason"] = "SYSTEM_ERROR";
        break;
        default:
        m["result"] = 2;
        m["reason"] = ret;
        break;
    }  // end switch

    // --- label L0108 ---
    return m;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int get_status() {
    /* ---- Enhanced Pseudocode ---- */
    return _status;

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=55, nargs=0, insts=55
// --------------------------------------------------------------------
private void do_init() {
    /* ---- Enhanced Pseudocode ---- */
    if (((!is_object(find_object("/aaa/daemons/configd.c"))) || (!is_mapping("/aaa/daemons/configd.c"->get_my_info())))) {
        register_post_init((: do_init :));
        return;
    }
    coin_trade_server = "/aaa/daemons/configd.c"->get_my_info()["coin_trade_server"];
    if (is_string(coin_trade_server)) {
        coin_trade_server = trim(coin_trade_server);
        set_status(1);
        return;
    }
    catch(error("Please config ADB.aaa field: coin_trade_server.\nFormat: IP:port ---> localhost:8080\n"));

    // --- label L0049 ---
    set_status(0);

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
// @brief  mutator: set property
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
void set_status(int s) {
    /* ---- Enhanced Pseudocode ---- */
    _status = s;

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        5
   Code segments found:         5
   Successfully decoded:        5
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */