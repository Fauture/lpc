/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: serverd.o
 * Source: /aaa/daemons/serverd.c
 * Inherits: (none)
 * Inherited programs: /feature/daemon/register.c(), /feature/daemon/handler_module.c()
 * Functions: 3 names, 3 code segments found
 * Strings: 21, Variables: 0
 *********************************************************************/

inherit "/feature/daemon/register.c";
inherit "/feature/daemon/handler_module.c";

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void do_auth_new_connection(object agent, int type, string name, int cookie);
private void destruct();
private void create();

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  action handler: process request
// @code  nc=109, nargs=4, insts=109
// --------------------------------------------------------------------
private void do_auth_new_connection(object agent, int type, string name, int cookie) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ip_infos;
    mixed server_ob;
    mapping agent_info;

    agent_info = "/aaa/daemons/configd.c"->get_info_by_type(type, name);
    if ((!is_mapping(agent_info))) {
        return;
    }
    if ((type == 11)) {
        ip_infos = explode(agent_info["ip"], ",");
        if ((member_array(agent->get_peer_ip(), ip_infos) < 0)) {
            destruct_object(agent);
            return;
        }
    }
    if ((agent->get_peer_ip() != agent_info["ip"])) {
        return;
    }
    server_ob = clone_object("/aaa/clone/server.c", agent_info);
    agent->relay_agent(server_ob);
    destruct_object(agent);
    server_ob->set_name(name);
    server_ob->set("type", type);
    server_ob->set("dist", agent_info["dist"]);
    set_client_attrib(server_ob->get_client_no(), ([ "type": "internal", "size": 131000, "peek": -1 ]));
    if ((type == 11)) {
        server_ob->set_cmd_module("/daemons/ctacommandd.c");
    }

}

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=4, nargs=0, insts=4
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=13, nargs=0, insts=13
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    load_all_handlers("/aaa/daemons/server/");
    "/daemons/connectiond.c"->register_auth("cmd_internal_auth", (: do_auth_new_connection :));

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        3
   Code segments found:         3
   Successfully decoded:        3
   Unresolved/minimal:          0
   Functions without code:      0
   Inherited programs:          2
      - /feature/daemon/register.c()
      - /feature/daemon/handler_module.c()
   ================================================================ */