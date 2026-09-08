// Game client service - faithful port of vm_efun_magic.c / vm_clientSocketCallback
// from the original magic_Linux32x driver (port 8101).
//
// Wire protocol (all multi-byte fields BIG-ENDIAN on the wire):
//   header: 'M' 'Z'  checkSum(be:u16)  tickCount(be:u32)  msgLen(be:u16)  cmdNo(be:u16)
//   body:   msgLen - 12 bytes of command-specific data
//
// The receiver keeps a per-client state machine (LOST_SYNC -> SYNCED_1 ->
// WAIT_HEADER -> WAIT_BODY).  Once a full packet is assembled the command is
// looked up in cmdIndex[] and, after the attrib permission check, dispatched
// to the matching C command function, which parses the body and applies the
// corresponding LPC command on the master program (vm_apply).

#include "game_service.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "builtins.h"
#include "original_vm.h"
#include "pkt_protocol.h"
#include "value.h"
#include "value.h"

namespace lpc {

namespace {

constexpr uint16_t kSyncMagic = 0x5A4D;  // 'M''Z' read as little-endian word

void ensureWsaGame() {
    static bool done = [] {
        WSADATA wsa{};
        WSAStartup(MAKEWORD(2, 2), &wsa);
        return true;
    }();
    (void)done;
}

// attrib bits (vm_magicInit predefines)
constexpr uint32_t ATTRIB_CLIENT_USER = 1;     // CLIENT_USER
constexpr uint32_t ATTRIB_INTERNAL_COMM = 2;   // INTERNAL_COMM
constexpr uint32_t ATTRIB_FROM_SERVER = 4;     // FROM_SERVER

std::mutex& gameMutex() {
    static std::mutex m;
    return m;
}
std::unordered_map<int, GameClient>& gameClients() {
    static std::unordered_map<int, GameClient> clients;
    return clients;
}
std::atomic<int>& nextGameClientNo() {
    static std::atomic<int> n{1};
    return n;
}

uint16_t rd16be(const uint8_t* p) {
    return static_cast<uint16_t>((p[0] << 8) | p[1]);
}
uint32_t rd32be(const uint8_t* p) {
    return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
}
// Offset convenience overloads used by the generated command handlers.
uint16_t rd16be(const uint8_t* p, size_t off) { return rd16be(p + off); }
uint32_t rd32be(const uint8_t* p, size_t off) { return rd32be(p + off); }
Value make_buffer(const uint8_t* p, size_t len) {
    return lpc::make_buffer(std::string(reinterpret_cast<const char*>(p), len));
}
using lpc::make_buffer;
Value make_string(const uint8_t* p, size_t len) {
    return lpc::make_string(std::string(reinterpret_cast<const char*>(p), len));
}
Value make_string(const char* p, size_t len) { return lpc::make_string(std::string(p, len)); }
using lpc::make_string;
uint8_t rd8(const uint8_t* p) { return p[0]; }
uint8_t rd8(const uint8_t* p, size_t off) { return p[off]; }
void wr16be(uint8_t* p, uint16_t v) { p[0] = static_cast<uint8_t>(v >> 8); p[1] = static_cast<uint8_t>(v & 0xFF); }
void wr32be(uint8_t* p, uint32_t v) {
    p[0] = static_cast<uint8_t>(v >> 24); p[1] = static_cast<uint8_t>((v >> 16) & 0xFF);
    p[2] = static_cast<uint8_t>((v >> 8) & 0xFF); p[3] = static_cast<uint8_t>(v & 0xFF);
}

// vm_calculateCheckSum (0x804cfb4): v += byte * ((v & 0x7F) + 1)
uint16_t calculateCheckSum(const uint8_t* data, uint32_t len, uint16_t init = 0) {
    uint16_t v = init;
    for (uint32_t i = 0; i < len; ++i)
        v = static_cast<uint16_t>(v + data[i] * ((v & 0x7F) + 1));
    return v;
}

// Dispatch entry lookup table built from the original cmdList.  Only the
// commands whose C handler is implemented below are listed; unknown commands
// are logged and dropped (matching the original "*** Bad command" path).
struct CmdEntry {
    uint16_t cmdNo;
    uint16_t type;
    const char* lpcFun;      // LPC function applied with (clientNo, ...args)
    int argSpec;             // argument encoding, see handlers
};
const CmdEntry kCmdList[] = {
    // serverCmdList entries (Vm_Command_List_T, .data:083C8E40):
    //   <5351h, 4, vm_msgLAuth>      - MSG_L_AUTH
    //   <0F9D3h, 4, vm_msgLoginDone>
    { 0x5351, ATTRIB_CLIENT_USER | ATTRIB_INTERNAL_COMM | ATTRIB_FROM_SERVER, "msg_l_auth", 0 }, // vm_msgLAuth
    { 0xF9D3, ATTRIB_CLIENT_USER | ATTRIB_INTERNAL_COMM | ATTRIB_FROM_SERVER, "msg_login_done", 0 }, // vm_msgLoginDone
    { 0x20D2, ATTRIB_CLIENT_USER | ATTRIB_INTERNAL_COMM | ATTRIB_FROM_SERVER, "cmd_echo", 0 }, // vm_cmdEcho
    // 0xF9C3 MSG_REPLY_ECHO - inbound echo reply (cmdList entry at
    // .data:083CA158: cmd=0xF9C3 type=3 vm_msgReplyEcho).  Registered so the
    // dba/aaa/ccs accept the gs's echo replies instead of logging "bad
    // command" (which does not matter for liveness, but keeps stats clean).
    { 0xF9C3, ATTRIB_CLIENT_USER | ATTRIB_INTERNAL_COMM | ATTRIB_FROM_SERVER, "msg_reply_echo", 0 }, // vm_msgReplyEcho
    // cmdList entries (Vm_Command_List_T, .data:083C8E60).  These are the
    // DRIVER-level config messages that gs/ccs/dba (which ship without a
    // comm_desc.txt) rely on: the C handler parses the value-typed body and
    // vm_apply's the LPC msg_cfg_* function directly.
    { 0x3365, ATTRIB_INTERNAL_COMM, "msg_cfg_my_info", 0 },    // vm_msgCfgMyInfo
    { 0x3361, ATTRIB_INTERNAL_COMM, "msg_cfg_dba_list", 0 },   // vm_msgCfgDbaList
    { 0x3363, ATTRIB_INTERNAL_COMM, "msg_cfg_ldba_list", 0 },  // vm_msgCfgLdbaList
    { 0x435F, ATTRIB_INTERNAL_COMM, "msg_cfg_server_list", 0 },// vm_msgCfgGameServerList
    { 0x40F5, ATTRIB_INTERNAL_COMM, "msg_cfg_spa_info", 0 },   // vm_msgCfgSpaInfo
    { 0x236F, ATTRIB_INTERNAL_COMM, "msg_cfg_cta_list", 0 },   // vm_msgCfgCtaList
    // filed<->dba data protocol (second batch, type=2):
    { 0x2405, ATTRIB_INTERNAL_COMM, "msg_deleted_content", 0 },    // vm_msgDeletedContent
    { 0x2407, ATTRIB_INTERNAL_COMM, "msg_created_content", 0 },    // vm_msgCreatedContent
    { 0x54A1, ATTRIB_INTERNAL_COMM, "msg_moved_content", 0 },      // vm_msgMovedContent
    { 0x2409, ATTRIB_INTERNAL_COMM, "msg_got_catalog", 0 },        // vm_msgGotCatalog
    { 0x240F, ATTRIB_INTERNAL_COMM, "msg_got_catalog_ex", 0 },     // vm_msgGotCatalogEx
    { 0x241B, ATTRIB_INTERNAL_COMM, "msg_read_log", 0 },           // vm_msgReadLog
    { 0x0F51F, ATTRIB_INTERNAL_COMM, "msg_update_log", 0 },        // vm_msgUpdateLog
    { 0x240B, ATTRIB_INTERNAL_COMM, "msg_got_page_data", 0 },      // vm_msgGotPageData
    { 0x240D, ATTRIB_INTERNAL_COMM, "msg_got_page_data_ex", 0 },   // vm_msgGotPageDataEx
    { 0x2411, ATTRIB_INTERNAL_COMM, "msg_deleted_gid", 0 },        // vm_msgDeletedGid
    { 0x2413, ATTRIB_INTERNAL_COMM, "msg_updated_gid", 0 },        // vm_msgUpdatedGid
    { 0x5409, ATTRIB_INTERNAL_COMM, "msg_create_pet_deposit", 0 }, // vm_msgCreatePetDeposit
    { 0x5401, ATTRIB_INTERNAL_COMM, "msg_read_pet_deposit", 0 },   // vm_msgReadPetDeposit
    { 0x5405, ATTRIB_INTERNAL_COMM, "msg_write_pet_deposit", 0 },  // vm_msgWritePetDeposit
    { 0x4402, ATTRIB_INTERNAL_COMM, "msg_get_counts", 0 },         // vm_msgGetCounts
{ 0x7008, ATTRIB_CLIENT_USER, "cmd_apprentice_list", 0 },
{ 0x5012, ATTRIB_CLIENT_USER, "cmd_recruit_apprentice", 0 },
{ 0x21BE, ATTRIB_CLIENT_USER, "cmd_start_award", 0 },
{ 0x70A6, ATTRIB_CLIENT_USER, "cmd_start_multi_bonus", 0 },
{ 0x2242, ATTRIB_CLIENT_USER, "cmd_accelerate_move_to", 0 },
{ 0xF6, ATTRIB_CLIENT_USER, "cmd_accept", 0 },
{ 0x2164, ATTRIB_CLIENT_USER, "cmd_accept_chat_group_invite", 0 },
{ 0x2158, ATTRIB_CLIENT_USER, "cmd_accept_chat_group_join", 0 },
{ 0x21C6, ATTRIB_CLIENT_USER, "cmd_accept_common_convenant", 0 },
{ 0x6400, ATTRIB_CLIENT_USER, "cmd_accept_convenant", 0 },
{ 0x0A0C, ATTRIB_CLIENT_USER, "cmd_add_award_times", 0 },
{ 0x50A2, ATTRIB_CLIENT_USER, "cmd_add_bureau_page", 0 },
{ 0xF0C2, ATTRIB_CLIENT_USER, "cmd_add_dunwu_nimbus", 0 },
{ 0x3058, ATTRIB_CLIENT_USER, "cmd_add_friend", 0 },
{ 0xFA49, ATTRIB_CLIENT_USER, "cmd_add_godbook_direct", 0 },
{ 0x2284, ATTRIB_CLIENT_USER, "cmd_add_material", 0 },
{ 0x2154, ATTRIB_CLIENT_USER, "cmd_add_mount_item", 0 },
{ 0x125E, ATTRIB_CLIENT_USER, "cmd_add_skill_essence", 0 },
{ 0x2344, ATTRIB_CLIENT_USER, "cmd_admin_broadcast", 0 },
{ 0x2314, ATTRIB_CLIENT_USER, "cmd_admin_common_command", 0 },
{ 0x217E, ATTRIB_CLIENT_USER, "cmd_broadcast_combat_list", 0 },
{ 0xF908, ATTRIB_CLIENT_USER, "cmd_button_double_bonus", 0 },
{ 0x506E, ATTRIB_CLIENT_USER, "cmd_buy_card", 0 },
{ 0x2312, ATTRIB_CLIENT_USER, "cmd_buy_customize_package", 0 },
{ 0x4114, ATTRIB_CLIENT_USER, "cmd_buy_ecard", 0 },
{ 0x21C8, ATTRIB_CLIENT_USER, "cmd_buy_from_asktao_show_mall", 0 },
{ 0x1166, ATTRIB_CLIENT_USER, "cmd_buy_from_online_mall", 0 },
{ 0x350C, ATTRIB_CLIENT_USER, "cmd_buy_from_stall", 0 },
{ 0x281C, ATTRIB_CLIENT_USER, "cmd_buy_service", 0 },
{ 0x218A, ATTRIB_CLIENT_USER, "cmd_calc_mount_attrib", 0 },
{ 0x2216, ATTRIB_CLIENT_USER, "cmd_calc_pet_grow", 0 },
{ 0x2816, ATTRIB_CLIENT_USER, "cmd_calc_user_attribs", 0 },
{ 0x1262, ATTRIB_CLIENT_USER, "vm_cmdCancelAuthVipSms", 0 },
{ 0x21BC, ATTRIB_CLIENT_USER, "cmd_cancel_release_safe_login", 0 },
{ 0x1098, ATTRIB_CLIENT_USER, "cmd_care_item", 0 },
{ 0x1156, ATTRIB_CLIENT_USER, "cmd_cast", 0 },
{ 0x1114, ATTRIB_CLIENT_USER, "cmd_casual_game_do_action", 0 },
{ 0x50A4, ATTRIB_CLIENT_USER, "cmd_casual_oper_game", 0 },
{ 0x1A06, ATTRIB_CLIENT_USER, "cmd_catch_pker", 0 },
{ 0x226A, ATTRIB_CLIENT_USER, "cmd_c_catch_pet", 0 },
{ 0x3E10, ATTRIB_CLIENT_USER, "cmd_cc_do_action", 0 },
{ 0x2240, ATTRIB_CLIENT_USER, "cmd_cc_finish_round", 0 },
{ 0xF0C6, ATTRIB_CLIENT_USER, "cmd_cc_surrender", 0 },
{ 0xCF02, ATTRIB_CLIENT_USER, "cmd_cc_switch_card", 0 },
{ 0x217A, ATTRIB_CLIENT_USER, "cmd_c_do_action", 0 },
{ 0x1138, ATTRIB_CLIENT_USER, "cmd_c_do_action", 0 },
{ 0x1104, ATTRIB_CLIENT_USER, "cmd_c_do_action", 0 },
{ 0x2094, ATTRIB_CLIENT_USER, "cmd_cd_wishing_goods", 0 },
{ 0x70AA, ATTRIB_CLIENT_USER, "cmd_cd_wishing_rate", 0 },
{ 0x5106, ATTRIB_CLIENT_USER, "cmd_cd_wishing_start", 0 },
{ 0x4104, ATTRIB_CLIENT_USER, "cmd_c_end_animate", 0 },
{ 0x1108, ATTRIB_CLIENT_USER, "cmd_c_flee", 0 },
{ 0x2178, ATTRIB_CLIENT_USER, "cmd_challenge_answer", 0 },
{ 0x10B6, ATTRIB_CLIENT_USER, "cmd_arena_room_change_seat", 0 },
{ 0xFDA6, ATTRIB_CLIENT_USER, "cmd_change_attrib_plan", 0 },
{ 0x101A, ATTRIB_CLIENT_USER, "cmd_change_chat_group_setting", 0 },
{ 0x2336, ATTRIB_CLIENT_USER, "cmd_change_furniture_direction", 0 },
{ 0x211C, ATTRIB_CLIENT_USER, "cmd_change_item_attrib", 0 },
{ 0x21B4, ATTRIB_CLIENT_USER, "cmd_change_team_leader", 0 },
{ 0x2AE8, ATTRIB_CLIENT_USER, "cmd_change_title", 0 },
{ 0x1102, ATTRIB_CLIENT_USER, "cmd_change_title_effect", 0 },
{ 0x407C, ATTRIB_CLIENT_USER, "cmd_change_upgrade_state", 0 },
{ 0xFD24, ATTRIB_CLIENT_USER, "cmd_change_user_state", 0 },
{ 0x3062, ATTRIB_CLIENT_USER, "cmd_change_zhenfa", 0 },
{ 0xF0DD, ATTRIB_CLIENT_USER, "cmd_charge_vendue_fetch", 0 },
{ 0x23B3, ATTRIB_CLIENT_USER, "cmd_charge_vendue_offer_price", 0 },
{ 0x3501, ATTRIB_CLIENT_USER, "cmd_charge_vendue_refresh", 0 },
{ 0x10DF, ATTRIB_CLIENT_USER, "cmd_charge_vendue_store_closed", 0 },
{ 0x13AA, ATTRIB_CLIENT_USER, "cmd_char_practice", 0 },
{ 0x2276, ATTRIB_CLIENT_USER, "cmd_chat", 0 },
{ 0x3AE4, ATTRIB_CLIENT_USER, "cmd_chat_ex", 0 },
{ 0x10F4, ATTRIB_CLIENT_USER, "cmd_request_chat_group_logs", 0 },
{ 0x2290, ATTRIB_CLIENT_USER, "cmd_check_safe_answer", 0 },
{ 0x5108, ATTRIB_CLIENT_USER, "cmd_check_server", 0 },
{ 0x4122, ATTRIB_CLIENT_USER, "cmd_child_enter_hall", 0 },
{ 0x23B6, ATTRIB_CLIENT_USER, "cmd_child_enter_map", 0 },
{ 0x5008, ATTRIB_CLIENT_USER, "cmd_child_skill_level_up", 0 },
{ 0x1AFA, ATTRIB_CLIENT_USER, "cmd_child_use_combat", 0 },
{ 0x2800, ATTRIB_CLIENT_USER, "cmd_child_use_skill", 0 },
{ 0x3408, ATTRIB_CLIENT_USER, "cmd_citywar_give_orders", 0 },
{ 0x1A0C, ATTRIB_CLIENT_USER, "cmd_clean_request", 0 },
{ 0x1FB4, ATTRIB_CLIENT_USER, "cmd_clear_bag", 0 },
{ 0x2233, ATTRIB_CLIENT_USER, "cmd_clicked_linker", 0 },
{ 0x1136, ATTRIB_CLIENT_USER, "cmd_client_behavior", 0 },
{ 0x1A, ATTRIB_CLIENT_USER, "cmd_clone_special_equip", 0 },
{ 0x61, ATTRIB_CLIENT_USER, "cmd_clone_upgrade_stone", 0 },
{ 0x113E, ATTRIB_CLIENT_USER, "cmd_close_bureau", 0 },
{ 0xFDA0, ATTRIB_CLIENT_USER, "cmd_close_menu", 0 },
{ 0x4108, ATTRIB_CLIENT_USER, "cmd_close_remote_store", 0 },
{ 0x2308, ATTRIB_CLIENT_USER, "cmd_close_talk_drama", 0 },
{ 0x2080, ATTRIB_CLIENT_USER, "cmd_close_taotaotao", 0 },
{ 0x2A02, ATTRIB_CLIENT_USER, "cmd_coin_cancel_order", 0 },
{ 0x7E06, ATTRIB_CLIENT_USER, "cmd_coin_deal_order", 0 },
{ 0x3806, ATTRIB_CLIENT_USER, "cmd_coin_fetch_store", 0 },
{ 0x280A, ATTRIB_CLIENT_USER, "cmd_coin_open_mall", 0 },
{ 0xC4, ATTRIB_CLIENT_USER, "cmd_coin_refresh_mall", 0 },
{ 0xFD18, ATTRIB_CLIENT_USER, "cmd_coin_submit_order", 0 },
{ 0x2184, ATTRIB_CLIENT_USER, "cmd_collect_data", 0 },
{ 0x54, ATTRIB_CLIENT_USER, "cmd_common_guess_draw_bonus", 0 },
{ 0x1028, ATTRIB_CLIENT_USER, "cmd_common_guess_info", 0 },
{ 0x2222, ATTRIB_CLIENT_USER, "cmd_common_guess_list", 0 },
{ 0x3206, ATTRIB_CLIENT_USER, "cmd_common_guess_start_guess", 0 },
{ 0x3E30, ATTRIB_CLIENT_USER, "cmd_confirm_take_property", 0 },
{ 0x2EF0, ATTRIB_CLIENT_USER, "cmd_control_party_channel", 0 },
{ 0x306E, ATTRIB_CLIENT_USER, "cmd_create_chat_group", 0 },
{ 0x205C, ATTRIB_CLIENT_USER, "cmd_create_new_char", 0 },
{ 0x2130, ATTRIB_CLIENT_USER, "cmd_creat_new_raid", 0 },
{ 0x24, ATTRIB_CLIENT_USER, "cmd_csc_add_sign_fee", 0 },
{ 0xFA4A, ATTRIB_CLIENT_USER, "cmd_csc_view_compete_sort", 0 },
{ 0xA, ATTRIB_CLIENT_USER, "cmd_c_select_menu_item", 0 },
{ 0x120C, ATTRIB_CLIENT_USER, "cmd_c_set_pos", 0 },
{ 0x272C, ATTRIB_CLIENT_USER, "cmd_cs_exchange", 0 },
{ 0x211A, ATTRIB_CLIENT_USER, "cmd_c_use_stun_child_book", 0 },
{ 0x125A, ATTRIB_CLIENT_USER, "cmd_delete_existed_char", 0 },
{ 0x281E, ATTRIB_CLIENT_USER, "cmd_delete_music", 0 },
{ 0x2066, ATTRIB_CLIENT_USER, "cmd_deposit", 0 },
{ 0xFD60, ATTRIB_CLIENT_USER, "cmd_develop_skill", 0 },
{ 0x111C, ATTRIB_CLIENT_USER, "cmd_dig_party_fragment", 0 },
{ 0x2DCA, ATTRIB_CLIENT_USER, "cmd_dismiss_chat_group", 0 },
{ 0x3044, ATTRIB_CLIENT_USER, "cmd_distribute_recruitment", 0 },
{ 0x2EEE, ATTRIB_CLIENT_USER, "cmd_admin_fetch_pics", 0 },
{ 0xCF06, ATTRIB_CLIENT_USER, "cmd_admin_kickoff", 0 },
{ 0x2176, ATTRIB_CLIENT_USER, "cmd_admin_oper_sys_broadcast", 0 },
{ 0x3A12, ATTRIB_CLIENT_USER, "cmd_admin_print_screen", 0 },
{ 0x1AFC, ATTRIB_CLIENT_USER, "cmd_admin_query_map", 0 },
{ 0x20F6, ATTRIB_CLIENT_USER, "cmd_admin_query_net_info", 0 },
{ 0x301C, ATTRIB_CLIENT_USER, "cmd_admin_query_npc", 0 },
{ 0xFA, ATTRIB_CLIENT_USER, "cmd_admin_query_player", 0 },
{ 0x272E, ATTRIB_CLIENT_USER, "cmd_admin_query_skill", 0 },
{ 0x2442, ATTRIB_CLIENT_USER, "cmd_admin_reboot", 0 },
{ 0x700A, ATTRIB_CLIENT_USER, "cmd_admin_request_map_list", 0 },
{ 0x1364, ATTRIB_CLIENT_USER, "cmd_admin_set_attrib", 0 },
{ 0x2818, ATTRIB_CLIENT_USER, "cmd_admin_set_skill", 0 },
{ 0xFD58, ATTRIB_CLIENT_USER, "cmd_admin_shut_channel", 0 },
{ 0xFD2C, ATTRIB_CLIENT_USER, "cmd_admin_sniff_at", 0 },
{ 0x2248, ATTRIB_CLIENT_USER, "cmd_admin_srmouse_click", 0 },
{ 0x2278, ATTRIB_CLIENT_USER, "cmd_admin_summon", 0 },
{ 0x5A, ATTRIB_CLIENT_USER, "cmd_admin_teleport", 0 },
{ 0x1096, ATTRIB_CLIENT_USER, "cmd_admin_throw_in_jail", 0 },
{ 0x20BE, ATTRIB_CLIENT_USER, "cmd_admin_throw_in_jail_ex", 0 },
{ 0x6002, ATTRIB_CLIENT_USER, "cmd_admin_time_broadcast", 0 },
{ 0x2204, ATTRIB_CLIENT_USER, "cmd_admin_transport", 0 },
{ 0x10BE, ATTRIB_CLIENT_USER, "cmd_answer_result", 0 },
{ 0x110E, ATTRIB_CLIENT_USER, "cmd_answer_survey", 0 },
{ 0xFD2E, ATTRIB_CLIENT_USER, "cmd_anti_hack_collect", 0 },
{ 0x1B01, ATTRIB_CLIENT_USER, "cmd_apex", 0 },
{ 0x220C, ATTRIB_CLIENT_USER, "cmd_apply", 0 },
{ 0xFAF8, ATTRIB_CLIENT_USER, "cmd_apply_card", 0 },
{ 0xFD36, ATTRIB_CLIENT_USER, "cmd_apply_ex", 0 },
{ 0x0B22, ATTRIB_CLIENT_USER, "cmd_apply_exquisite", 0 },
{ 0x10D6, ATTRIB_CLIENT_USER, "cmd_apply_offline_stall", 0 },
{ 0x2802, ATTRIB_CLIENT_USER, "cmd_apply_on_friend", 0 },
{ 0x206C, ATTRIB_CLIENT_USER, "cmd_apply_on_item", 0 },
{ 0x2606, ATTRIB_CLIENT_USER, "cmd_apply_other", 0 },
{ 0x3A10, ATTRIB_CLIENT_USER, "cmd_apply_question_item", 0 },
{ 0x4100, ATTRIB_CLIENT_USER, "cmd_arena_enter", 0 },
{ 0x1FC8, ATTRIB_CLIENT_USER, "cmd_arena_goods_buy", 0 },
{ 0x113C, ATTRIB_CLIENT_USER, "cmd_arena_goods_list", 0 },
{ 0x0A12, ATTRIB_CLIENT_USER, "cmd_arena_goods_sell", 0 },
{ 0x1126, ATTRIB_CLIENT_USER, "cmd_arena_hall_exit", 0 },
{ 0x40D6, ATTRIB_CLIENT_USER, "cmd_arena_hall_fight", 0 },
{ 0x2428, ATTRIB_CLIENT_USER, "cmd_arena_hall_join_room", 0 },
{ 0x280E, ATTRIB_CLIENT_USER, "cmd_arena_hall_request_ci", 0 },
{ 0x2114, ATTRIB_CLIENT_USER, "cmd_arena_hall_request_pl", 0 },
{ 0xF0A8, ATTRIB_CLIENT_USER, "cmd_arena_hall_request_rd", 0 },
{ 0x202C, ATTRIB_CLIENT_USER, "cmd_arena_hall_request_rl", 0 },
{ 0xF0D4, ATTRIB_CLIENT_USER, "cmd_arena_look_on", 0 },
{ 0x2022, ATTRIB_CLIENT_USER, "cmd_arena_match_fixtures", 0 },
{ 0x2028, ATTRIB_CLIENT_USER, "cmd_arena_match_join_room", 0 },
{ 0x2038, ATTRIB_CLIENT_USER, "cmd_arena_match_list", 0 },
{ 0x2026, ATTRIB_CLIENT_USER, "cmd_arena_match_room_detail", 0 },
{ 0x2024, ATTRIB_CLIENT_USER, "cmd_arena_match_rooms", 0 },
{ 0x2342, ATTRIB_CLIENT_USER, "cmd_arena_open_rank", 0 },
{ 0x20DC, ATTRIB_CLIENT_USER, "cmd_arena_query_rank_by_name", 0 },
{ 0x2260, ATTRIB_CLIENT_USER, "cmd_arena_rank_list", 0 },
{ 0x1086, ATTRIB_CLIENT_USER, "cmd_arena_room", 0 },
{ 0x202A, ATTRIB_CLIENT_USER, "cmd_arena_room_action", 0 },
{ 0x2722, ATTRIB_CLIENT_USER, "cmd_arena_room_kick_out", 0 },
{ 0x3204, ATTRIB_CLIENT_USER, "cmd_arena_server_list", 0 },
{ 0x20A4, ATTRIB_CLIENT_USER, "cmd_arena_switch_server", 0 },
{ 0x700E, ATTRIB_CLIENT_USER, "cmd_ask_fields", 0 },
{ 0xB0C, ATTRIB_CLIENT_USER, "cmd_ask_pill", 0 },
{ 0x1366, ATTRIB_CLIENT_USER, "cmd_ask_skill_desc", 0 },
{ 0x1171, ATTRIB_CLIENT_USER, "cmd_asktao_sound", 0 },
{ 0x40E2, ATTRIB_CLIENT_USER, "cmd_assign_attrib", 0 },
{ 0xFD74, ATTRIB_CLIENT_USER, "cmd_assign_child_attrib", 0 },
{ 0x23A2, ATTRIB_CLIENT_USER, "cmd_assign_polar", 0 },
{ 0x2252, ATTRIB_CLIENT_USER, "cmd_assign_resist", 0 },
{ 0xF0CE, ATTRIB_CLIENT_USER, "cmd_assign_upgrade", 0 },
{ 0xFD97, ATTRIB_CLIENT_USER, "cmd_auth_matrix_for_gs", 0 },
{ 0x3018, ATTRIB_CLIENT_USER, "cmd_auth_refresh_pic_for_gs", 0 },
{ 0xF8, ATTRIB_CLIENT_USER, "cmd_auth_verify_code", 0 },
{ 0xF374, ATTRIB_CLIENT_USER, "cmd_auth_vip_sms", 0 },
{ 0xF0A1, ATTRIB_CLIENT_USER, "cmd_auto_combat_time", 0 },
{ 0xFD80, ATTRIB_CLIENT_USER, "cmd_bottle_dig", 0 },
{ 0xFD32, ATTRIB_CLIENT_USER, "cmd_bottle_open", 0 },
{ 0x2190, ATTRIB_CLIENT_USER, "cmd_bottle_open_interface", 0 },
{ 0x4008, ATTRIB_CLIENT_USER, "cmd_bottle_reply", 0 },
{ 0x1380, ATTRIB_CLIENT_USER, "cmd_bottle_throw", 0 },
{ 0x2048, ATTRIB_CLIENT_USER, "cmd_bottle_throw_back", 0 },

{ 0x1140, ATTRIB_CLIENT_USER, "cmd_divide", 0 },
{ 0x22, ATTRIB_CLIENT_USER, "cmd_divide_in_store", 0 },
{ 0x225E, ATTRIB_CLIENT_USER, "cmd_doucdh_challenge", 0 },
{ 0x1145, ATTRIB_CLIENT_USER, "cmd_draft_modify_my_info", 0 },
{ 0x3012, ATTRIB_CLIENT_USER, "cmd_draft_present_flower", 0 },
{ 0x3E12, ATTRIB_CLIENT_USER, "cmd_draft_report", 0 },
{ 0x1206, ATTRIB_CLIENT_USER, "cmd_draft_request_my_info", 0 },
{ 0x225C, ATTRIB_CLIENT_USER, "cmd_draft_request_stage_info", 0 },
{ 0x23AC, ATTRIB_CLIENT_USER, "cmd_draft_request_stage_rank", 0 },
{ 0x2220, ATTRIB_CLIENT_USER, "cmd_draft_roll", 0 },
{ 0x701C, ATTRIB_CLIENT_USER, "cmd_draft_vote", 0 },
{ 0x2244, ATTRIB_CLIENT_USER, "cmd_draw_card_rank_bonus", 0 },
{ 0x2236, ATTRIB_CLIENT_USER, "cmd_drop", 0 },
{ 0x4006, ATTRIB_CLIENT_USER, "cmd_drop_child", 0 },
{ 0x3300, ATTRIB_CLIENT_USER, "cmd_drop_pet", 0 },
{ 0xFD34, ATTRIB_CLIENT_USER, "cmd_drop_random_box_award", 0 },
{ 0x4078, ATTRIB_CLIENT_USER, "cmd_drop_task", 0 },
{ 0x2123, ATTRIB_CLIENT_USER, "cmd_dungeon_view_tasks", 0 },
{ 0x1C08, ATTRIB_CLIENT_USER, "cmd_emote", 0 },
{ 0xCF04, ATTRIB_CLIENT_USER, "cmd_end_challenge", 0 },
{ 0x121E, ATTRIB_CLIENT_USER, "cmd_guess_bonus", 0 },
{ 0x21DB, ATTRIB_CLIENT_USER, "cmd_guess_check_locked", 0 },
{ 0x2329, ATTRIB_CLIENT_USER, "cmd_guess_end", 0 },
{ 0x20E0, ATTRIB_CLIENT_USER, "cmd_guess_start", 0 },
{ 0x40C6, ATTRIB_CLIENT_USER, "cmd_hell_dealer", 0 },
{ 0x3E22, ATTRIB_CLIENT_USER, "cmd_hssl_request_info", 0 },
{ 0x3302, ATTRIB_CLIENT_USER, "cmd_hssl_start_challeng", 0 },
{ 0x2304, ATTRIB_CLIENT_USER, "cmd_identify_equip", 0 },
{ 0x2380, ATTRIB_CLIENT_USER, "cmd_ignore_convenant", 0 },
{ 0x2310, ATTRIB_CLIENT_USER, "cmd_invite", 0 },
{ 0x2E3A, ATTRIB_CLIENT_USER, "cmd_invite_chat_group_member", 0 },
{ 0x2268, ATTRIB_CLIENT_USER, "cmd_join_chat_group", 0 },
{ 0x1AEE, ATTRIB_CLIENT_USER, "cmd_join_system_group", 0 },
{ 0x3702, ATTRIB_CLIENT_USER, "cmd_kickout", 0 },
{ 0xFD42, ATTRIB_CLIENT_USER, "cmd_kill", 0 },
{ 0x112D, ATTRIB_CLIENT_USER, "cmd_kill_ex", 0 },
{ 0x2020, ATTRIB_CLIENT_USER, "cmd_launch_convenant", 0 },
{ 0x6384, ATTRIB_CLIENT_USER, "cmd_lc_do_action", 0 },
{ 0x1FD0, ATTRIB_CLIENT_USER, "cmd_learn_cooking", 0 },
{ 0x2162, ATTRIB_CLIENT_USER, "cmd_learn_skill", 0 },
{ 0x4226, ATTRIB_CLIENT_USER, "cmd_learn_zhenfa", 0 },
{ 0xFA46, ATTRIB_CLIENT_USER, "cmd_arena_room_leave", 0 },
{ 0x3056, ATTRIB_CLIENT_USER, "cmd_leave_magictower", 0 },
{ 0x104A, ATTRIB_CLIENT_USER, "cmd_leave_temp_team", 0 },
{ 0xFFF1, ATTRIB_CLIENT_USER, "cmd_get_safe_ctrl_content", 0 },
{ 0xFAFA, ATTRIB_CLIENT_USER, "cmd_lgxy_challenge", 0 },
{ 0x2100, ATTRIB_CLIENT_USER, "cmd_is_convert_botton", 0 },
{ 0x3708, ATTRIB_CLIENT_USER, "cmd_listen_music", 0 },
{ 0x1060, ATTRIB_CLIENT_USER, "cmd_load_existed_char", 0 },
{ 0x2316, ATTRIB_CLIENT_USER, "cmd_local_encrypt_borrow", 0 },
{ 0x4132, ATTRIB_CLIENT_USER, "cmd_local_encrypt_locked", 0 },
{ 0x2422, ATTRIB_CLIENT_USER, "cmd_local_encrypt_lock_failed", 0 },
{ 0x2170, ATTRIB_CLIENT_USER, "cmd_local_encrypt_request", 0 },
{ 0x2426, ATTRIB_CLIENT_USER, "cmd_local_encrypt_unlock", 0 },
{ 0x100E, ATTRIB_CLIENT_USER, "cmd_lock_account", 0 },
{ 0x4124, ATTRIB_CLIENT_USER, "cmd_login", 0 },
{ 0x3E2A, ATTRIB_CLIENT_USER, "cmd_logout", 0 },
{ 0x1C06, ATTRIB_CLIENT_USER, "cmd_look", 0 },
{ 0x2714, ATTRIB_CLIENT_USER, "cmd_look_on", 0 },
{ 0x3038, ATTRIB_CLIENT_USER, "cmd_lookon_broadcast_combat", 0 },
{ 0x20EE, ATTRIB_CLIENT_USER, "cmd_look_player", 0 },
{ 0x2720, ATTRIB_CLIENT_USER, "cmd_lottery_select", 0 },
{ 0x2FD3, ATTRIB_CLIENT_USER, "cmd_set_safe_ctrl_content", 0 },
{ 0xFD82, ATTRIB_CLIENT_USER, "cmd_lucky_sign", 0 },
{ 0x1036, ATTRIB_CLIENT_USER, "cmd_make_decoration", 0 },
{ 0x5506, ATTRIB_CLIENT_USER, "cmd_make_pill", 0 },
{ 0x40BA, ATTRIB_CLIENT_USER, "cmd_manage_city", 0 },
{ 0x115E, ATTRIB_CLIENT_USER, "cmd_manage_house", 0 },
{ 0xFD12, ATTRIB_CLIENT_USER, "cmd_manage_mp_device", 0 },
{ 0x207E, ATTRIB_CLIENT_USER, "cmd_merge_magic_picture", 0 },
{ 0xF0CC, ATTRIB_CLIENT_USER, "cmd_mobile_oper_task", 0 },
{ 0x1416, ATTRIB_CLIENT_USER, "cmd_mobile_request_npc_list", 0 },
{ 0x3E28, ATTRIB_CLIENT_USER, "cmd_modify_custom_npc_list", 0 },
{ 0xFD14, ATTRIB_CLIENT_USER, "cmd_modify_item_remark", 0 },
{ 0x2EF7, ATTRIB_CLIENT_USER, "cmd_modify_schedule", 0 },
{ 0xEEEE, ATTRIB_CLIENT_USER, "cmd_open_inborn_stone_list", 0 },
{ 0xFD1C, ATTRIB_CLIENT_USER, "cmd_move_child_item", 0 },
{ 0x2186, ATTRIB_CLIENT_USER, "cmd_move_on_carpet", 0 },
{ 0x4130, ATTRIB_CLIENT_USER, "cmd_move_to", 0 },
{ 0x1252, ATTRIB_CLIENT_USER, "cmd_mov_from_qiankundai", 0 },
{ 0x3015, ATTRIB_CLIENT_USER, "cmd_mov_to_qiankundai", 0 },
{ 0x1018, ATTRIB_CLIENT_USER, "cmd_mul_assign_attrib", 0 },
{ 0x1FB8, ATTRIB_CLIENT_USER, "cmd_mul_goods_buy", 0 },
{ 0xFD72, ATTRIB_CLIENT_USER, "cmd_multi_move_to", 0 },
{ 0x2502, ATTRIB_CLIENT_USER, "cmd_music_list", 0 },
{ 0x2608, ATTRIB_CLIENT_USER, "cmd_newbie_first_action", 0 },
{ 0x2156, ATTRIB_CLIENT_USER, "cmd_newbie_played_drama", 0 },
{ 0x2182, ATTRIB_CLIENT_USER, "cmd_next_question", 0 },
{ 0x4AF4, ATTRIB_CLIENT_USER, "cmd_notify_send_weibo", 0 },
{ 0x111A, ATTRIB_CLIENT_USER, "cmd_open_adventrue_box", 0 },
{ 0x3202, ATTRIB_CLIENT_USER, "cmd_arena_room_open_seat", 0 },
{ 0xEC, ATTRIB_CLIENT_USER, "cmd_open_asktao_show_gift_box", 0 },
{ 0x2258, ATTRIB_CLIENT_USER, "cmd_open_asktao_show_mall", 0 },
{ 0x4420, ATTRIB_CLIENT_USER, "cmd_open_auth_all_protect", 0 },
{ 0x500E, ATTRIB_CLIENT_USER, "cmd_open_box_adventrue_interface", 0 },
{ 0x40D4, ATTRIB_CLIENT_USER, "cmd_open_bureau", 0 },
{ 0x2152, ATTRIB_CLIENT_USER, "cmd_open_confirm_dlg", 0 },
{ 0x2064, ATTRIB_CLIENT_USER, "cmd_open_menu", 0 },
{ 0x2050, ATTRIB_CLIENT_USER, "cmd_open_menu", 0 },
{ 0x7012, ATTRIB_CLIENT_USER, "cmd_open_menu", 0 },
{ 0x226C, ATTRIB_CLIENT_USER, "cmd_open_inborn_skill_list", 0 },
{ 0x500A, ATTRIB_CLIENT_USER, "cmd_open_music", 0 },
{ 0x3E24, ATTRIB_CLIENT_USER, "cmd_open_online_mall", 0 },
{ 0x21B0, ATTRIB_CLIENT_USER, "cmd_open_safe_setting", 0 },
{ 0x5000, ATTRIB_CLIENT_USER, "cmd_operate_pet_store", 0 },
{ 0x2302, ATTRIB_CLIENT_USER, "cmd_oper_casual_game", 0 },
{ 0x1FBC, ATTRIB_CLIENT_USER, "cmd_oper_discover_event", 0 },
{ 0x122A, ATTRIB_CLIENT_USER, "cmd_oper_elixir", 0 },
{ 0x225A, ATTRIB_CLIENT_USER, "cmd_oper_envelope", 0 },
{ 0x2510, ATTRIB_CLIENT_USER, "cmd_oper_lock_exp", 0 },
{ 0x10DC, ATTRIB_CLIENT_USER, "cmd_oper_longteng_shengshi", 0 },
{ 0x2726, ATTRIB_CLIENT_USER, "cmd_oper_lottery_ticket", 0 },
{ 0xFD16, ATTRIB_CLIENT_USER, "cmd_oper_magic_picture", 0 },
{ 0x2444, ATTRIB_CLIENT_USER, "cmd_oper_master", 0 },
{ 0x400A, ATTRIB_CLIENT_USER, "cmd_oper_packed_item", 0 },
{ 0x3E26, ATTRIB_CLIENT_USER, "cmd_oper_security", 0 },
{ 0x401C, ATTRIB_CLIENT_USER, "cmd_oper_six_anni_info", 0 },
{ 0x2254, ATTRIB_CLIENT_USER, "cmd_oper_sort_task", 0 },
{ 0x2172, ATTRIB_CLIENT_USER, "cmd_oper_strategy_war", 0 },
{ 0x1162, ATTRIB_CLIENT_USER, "cmd_oper_teleport_item", 0 },
{ 0xFDAA, ATTRIB_CLIENT_USER, "cmd_end_raise", 0 },
{ 0xFD28, ATTRIB_CLIENT_USER, "cmd_end_recruitment", 0 },
{ 0x2712, ATTRIB_CLIENT_USER, "cmd_enter_arena_room", 0 },
{ 0x1026, ATTRIB_CLIENT_USER, "cmd_enter_room", 0 },
{ 0x234A, ATTRIB_CLIENT_USER, "cmd_enter_room", 0 },
{ 0x2212, ATTRIB_CLIENT_USER, "cmd_enter_room", 0 },
{ 0x1228, ATTRIB_CLIENT_USER, "cmd_equip", 0 },
{ 0x2082, ATTRIB_CLIENT_USER, "cmd_equip_inborn_stone", 0 },
{ 0x20B0, ATTRIB_CLIENT_USER, "cmd_equip_pet_dress", 0 },
{ 0xFD86, ATTRIB_CLIENT_USER, "cmd_exchange_appraise", 0 },
{ 0x4058, ATTRIB_CLIENT_USER, "cmd_exchange_cancel", 0 },
{ 0x21C2, ATTRIB_CLIENT_USER, "cmd_exchange_child_appellation", 0 },
{ 0x5004, ATTRIB_CLIENT_USER, "cmd_exchange_ok", 0 },
{ 0x3706, ATTRIB_CLIENT_USER, "cmd_exchange_open", 0 },
{ 0xFA11, ATTRIB_CLIENT_USER, "cmd_exchange_open_ex", 0 },
{ 0x10F2, ATTRIB_CLIENT_USER, "cmd_exchange_select", 0 },
{ 0x2132, ATTRIB_CLIENT_USER, "cmd_feed_child", 0 },
{ 0x203E, ATTRIB_CLIENT_USER, "cmd_feed_pet", 0 },
{ 0x116A, ATTRIB_CLIENT_USER, "cmd_feed_pet_xinfa_exp", 0 },
{ 0x2280, ATTRIB_CLIENT_USER, "cmd_fetch_gbuying_coin", 0 },
{ 0x1FD2, ATTRIB_CLIENT_USER, "cmd_fetch_other_store", 0 },
{ 0x1258, ATTRIB_CLIENT_USER, "cmd_fight", 0 },
{ 0x1AF6, ATTRIB_CLIENT_USER, "cmd_finger", 0 },
{ 0xF370, ATTRIB_CLIENT_USER, "cmd_finish_combat_guide", 0 },
{ 0xCF0C, ATTRIB_CLIENT_USER, "cmd_finish_interface_guide", 0 },
{ 0xFD6A, ATTRIB_CLIENT_USER, "cmd_finish_interface_status", 0 },
{ 0xFD4E, ATTRIB_CLIENT_USER, "cmd_finish_question", 0 },
{ 0x13AE, ATTRIB_CLIENT_USER, "cmd_finish_stall", 0 },
{ 0x2118, ATTRIB_CLIENT_USER, "cmd_fishing_apply_multi_card", 0 },
{ 0x1124, ATTRIB_CLIENT_USER, "cmd_fishing_auto_buy_weapon", 0 },
{ 0x2326, ATTRIB_CLIENT_USER, "cmd_fishing_buy_weapon", 0 },
{ 0x4230, ATTRIB_CLIENT_USER, "cmd_fishing_change_weapon", 0 },
{ 0x2346, ATTRIB_CLIENT_USER, "cmd_fishing_fire_bullet", 0 },
{ 0x234C, ATTRIB_CLIENT_USER, "cmd_fishing_open_remote_store", 0 },
{ 0x1360, ATTRIB_CLIENT_USER, "cmd_fishing_quit", 0 },
{ 0x1168, ATTRIB_CLIENT_USER, "cmd_fish_swallow_die", 0 },
{ 0x40D2, ATTRIB_CLIENT_USER, "cmd_fish_swallow_finish", 0 },
{ 0x218C, ATTRIB_CLIENT_USER, "cmd_friend_drink_invite", 0 },
{ 0x1218, ATTRIB_CLIENT_USER, "cmd_friend_drink_refresh", 0 },
{ 0x2306, ATTRIB_CLIENT_USER, "cmd_friend_tell", 0 },
{ 0x20BC, ATTRIB_CLIENT_USER, "cmd_friend_tell_auth", 0 },
{ 0x2A00, ATTRIB_CLIENT_USER, "cmd_friend_tell_ex", 0 },
{ 0x2150, ATTRIB_CLIENT_USER, "cmd_gather", 0 },
{ 0x3014, ATTRIB_CLIENT_USER, "cmd_gbuying_buy_goods", 0 },
{ 0x2210, ATTRIB_CLIENT_USER, "cmd_gbuying_fetch_coin", 0 },
{ 0x1386, ATTRIB_CLIENT_USER, "cmd_gbuying_fetch_goods", 0 },
{ 0x1392, ATTRIB_CLIENT_USER, "cmd_gbuying_inject_coin", 0 },
{ 0x2EFC, ATTRIB_CLIENT_USER, "cmd_gbuying_open_page", 0 },
{ 0xFD64, ATTRIB_CLIENT_USER, "cmd_general_notify", 0 },
{ 0x3802, ATTRIB_CLIENT_USER, "cmd_general_request", 0 },
{ 0x2328, ATTRIB_CLIENT_USER, "cmd_get", 0 },
{ 0x4012, ATTRIB_CLIENT_USER, "cmd_get_achieve_bonus", 0 },
{ 0x2728, ATTRIB_CLIENT_USER, "cmd_get_award_info", 0 },
{ 0x1119, ATTRIB_CLIENT_USER, "cmd_get_client_action", 0 },
{ 0xFA0D, ATTRIB_CLIENT_USER, "cmd_get_client_performance", 0 },
{ 0x290F, ATTRIB_CLIENT_USER, "cmd_get_client_phy_config", 0 },
{ 0x2180, ATTRIB_CLIENT_USER, "cmd_get_daily_loop", 0 },
{ 0x2320, ATTRIB_CLIENT_USER, "cmd_get_image_buffer", 0 },
{ 0xFDA2, ATTRIB_CLIENT_USER, "cmd_get_name_by_gid", 0 },
{ 0x5005, ATTRIB_CLIENT_USER, "cmd_get_order_rank", 0 },
{ 0x3506, ATTRIB_CLIENT_USER, "cmd_get_party_channel_deny_list", 0 },
{ 0x20B8, ATTRIB_CLIENT_USER, "cmd_get_party_log", 0 },
{ 0x10C2, ATTRIB_CLIENT_USER, "cmd_get_pet_evolve_list", 0 },
{ 0xB2, ATTRIB_CLIENT_USER, "cmd_get_phone_auth_bind_time", 0 },
{ 0xFD30, ATTRIB_CLIENT_USER, "cmd_get_raise_bonus", 0 },
{ 0xF0D2, ATTRIB_CLIENT_USER, "cmd_get_random_box_award", 0 },
{ 0x10AC, ATTRIB_CLIENT_USER, "cmd_get_safe_question", 0 },
{ 0x13A0, ATTRIB_CLIENT_USER, "cmd_get_safe_state", 0 },
{ 0x21C0, ATTRIB_CLIENT_USER, "cmd_get_task_item", 0 },
{ 0x2032, ATTRIB_CLIENT_USER, "cmd_get_today_stat", 0 },
{ 0x120E, ATTRIB_CLIENT_USER, "cmd_get_update_notice", 0 },
{ 0x1AEC, ATTRIB_CLIENT_USER, "cmd_goods_buy", 0 },
{ 0x508C, ATTRIB_CLIENT_USER, "cmd_goods_sell", 0 },
{ 0x0A0A, ATTRIB_CLIENT_USER, "cmd_group_channel", 0 },
{ 0xFD29, ATTRIB_CLIENT_USER, "cmd_guards_change_name", 0 },
{ 0x1B0D, ATTRIB_CLIENT_USER, "cmd_guards_cheer", 0 },
{ 0x3E05, ATTRIB_CLIENT_USER, "cmd_guards_drop_guard", 0 },
{ 0x20D3, ATTRIB_CLIENT_USER, "cmd_guards_recover", 0 },
{ 0x301B, ATTRIB_CLIENT_USER, "cmd_guards_refresh", 0 },
{ 0x10F9, ATTRIB_CLIENT_USER, "cmd_guards_set_tactics", 0 },
{ 0x1C04, ATTRIB_CLIENT_USER, "cmd_oper_mood_info", 0 },
{ 0x1AF8, ATTRIB_CLIENT_USER, "cmd_oper_visual_packs", 0 },
{ 0xF4, ATTRIB_CLIENT_USER, "cmd_oper_xiaobao_songshenbing", 0 },
{ 0x2168, ATTRIB_CLIENT_USER, "cmd_other_move_to", 0 },
{ 0x1A04, ATTRIB_CLIENT_USER, "cmd_party_black_list", 0 },
{ 0x20DA, ATTRIB_CLIENT_USER, "cmd_party_info", 0 },
{ 0x1FB6, ATTRIB_CLIENT_USER, "cmd_party_job_privilege_list", 0 },
{ 0xCC, ATTRIB_CLIENT_USER, "cmd_party_members", 0 },
{ 0x26, ATTRIB_CLIENT_USER, "cmd_party_modify_annouce", 0 },
{ 0x2340, ATTRIB_CLIENT_USER, "cmd_party_modify_member", 0 },
{ 0x2068, ATTRIB_CLIENT_USER, "cmd_party_relation", 0 },
{ 0x70A8, ATTRIB_CLIENT_USER, "cmd_party_request_list", 0 },
{ 0xCF08, ATTRIB_CLIENT_USER, "cmd_party_send_message", 0 },
{ 0x271A, ATTRIB_CLIENT_USER, "cmd_password_dlg", 0 },
{ 0x5016, ATTRIB_CLIENT_USER, "cmd_place_box", 0 },
{ 0x2806, ATTRIB_CLIENT_USER, "cmd_place_furniture", 0 },
{ 0x2264, ATTRIB_CLIENT_USER, "cmd_pre_assign_attrib", 0 },
{ 0x20F0, ATTRIB_CLIENT_USER, "cmd_pre_assign_child_attrib", 0 },
{ 0x10EA, ATTRIB_CLIENT_USER, "cmd_present_asktao_show", 0 },
{ 0x20F2, ATTRIB_CLIENT_USER, "cmd_present_music", 0 },
{ 0x3E20, ATTRIB_CLIENT_USER, "cmd_request_deck_info", 0 },
{ 0x1384, ATTRIB_CLIENT_USER, "cmd_request_deck_list", 0 },
{ 0x10AA, ATTRIB_CLIENT_USER, "cmd_request_decompose_card", 0 },
{ 0x1398, ATTRIB_CLIENT_USER, "cmd_request_delete_deck", 0 },
{ 0x7004, ATTRIB_CLIENT_USER, "cmd_request_draw_card_compete_bonus", 0 },
{ 0xFA4C, ATTRIB_CLIENT_USER, "cmd_request_drop_card_task", 0 },
{ 0x2036, ATTRIB_CLIENT_USER, "cmd_request_enter_citywar_room", 0 },
{ 0xF512, ATTRIB_CLIENT_USER, "cmd_request_enter_x_game", 0 },
{ 0x23A8, ATTRIB_CLIENT_USER, "cmd_request_finish_card_compete", 0 },
{ 0x2140, ATTRIB_CLIENT_USER, "cmd_request_friend_verify", 0 },
{ 0x1265, ATTRIB_CLIENT_USER, "cmd_request_icon", 0 },
{ 0x1148, ATTRIB_CLIENT_USER, "cmd_request_info", 0 },
{ 0x2179, ATTRIB_CLIENT_USER, "cmd_request_item_info", 0 },
{ 0x1020, ATTRIB_CLIENT_USER, "cmd_request_join", 0 },
{ 0x3A14, ATTRIB_CLIENT_USER, "cmd_request_join_recruitment", 0 },
{ 0x3070, ATTRIB_CLIENT_USER, "cmd_request_levelup", 0 },
{ 0x3704, ATTRIB_CLIENT_USER, "cmd_request_minerals_in_map", 0 },
{ 0x21B6, ATTRIB_CLIENT_USER, "cmd_request_my_card_packages", 0 },
{ 0x236C, ATTRIB_CLIENT_USER, "cmd_request_new_deck", 0 },
{ 0x58, ATTRIB_CLIENT_USER, "cmd_request_open_card_package", 0 },
{ 0x201C, ATTRIB_CLIENT_USER, "cmd_request_open_system", 0 },
{ 0x4106, ATTRIB_CLIENT_USER, "cmd_request_property_bind", 0 },
{ 0x2814, ATTRIB_CLIENT_USER, "cmd_request_receive_city_task", 0 },
{ 0x215A, ATTRIB_CLIENT_USER, "cmd_request_recharge", 0 },
{ 0x107A, ATTRIB_CLIENT_USER, "cmd_request_refresh_city_task", 0 },
{ 0x350A, ATTRIB_CLIENT_USER, "cmd_request_rename_deck", 0 },
{ 0xDE, ATTRIB_CLIENT_USER, "cmd_request_server_status", 0 },
{ 0x1142, ATTRIB_CLIENT_USER, "cmd_request_shiling", 0 },
{ 0x3E2C, ATTRIB_CLIENT_USER, "cmd_request_stall_list", 0 },
{ 0x272A, ATTRIB_CLIENT_USER, "cmd_request_start_card_compete", 0 },
{ 0x121A, ATTRIB_CLIENT_USER, "cmd_request_start_match_card_game", 0 },
{ 0x7018, ATTRIB_CLIENT_USER, "cmd_request_take_property", 0 },
{ 0x2296, ATTRIB_CLIENT_USER, "cmd_request_to_be_team_leader", 0 },
{ 0x1260, ATTRIB_CLIENT_USER, "cmd_request_top_tao_page", 0 },
{ 0x4004, ATTRIB_CLIENT_USER, "cmd_reselect_char", 0 },
{ 0x5006, ATTRIB_CLIENT_USER, "cmd_reselect_verify_code", 0 },
{ 0x10C8, ATTRIB_CLIENT_USER, "cmd_resend_vip_sms", 0 },
{ 0x1132, ATTRIB_CLIENT_USER, "cmd_reset_appellation_task_due", 0 },
{ 0x7016, ATTRIB_CLIENT_USER, "cmd_reset_appellation_task_mode", 0 },
{ 0xC, ATTRIB_CLIENT_USER, "cmd_return_team", 0 },
{ 0xFD6C, ATTRIB_CLIENT_USER, "cmd_revoke_convenant", 0 },
{ 0x20B4, ATTRIB_CLIENT_USER, "cmd_rewrite_child_book", 0 },
{ 0x1072, ATTRIB_CLIENT_USER, "cmd_search_chat_group", 0 },
{ 0xFD38, ATTRIB_CLIENT_USER, "cmd_search_stall_goods", 0 },
{ 0x1052, ATTRIB_CLIENT_USER, "cmd_select_current_child", 0 },
{ 0x56, ATTRIB_CLIENT_USER, "cmd_select_current_mount", 0 },
{ 0x206A, ATTRIB_CLIENT_USER, "cmd_select_current_pet", 0 },
{ 0x1042, ATTRIB_CLIENT_USER, "cmd_select_menu_item", 0 },
{ 0x30CA, ATTRIB_CLIENT_USER, "cmd_select_menu_item", 0 },
{ 0x215E, ATTRIB_CLIENT_USER, "cmd_select_menu_item", 0 },
{ 0x1418, ATTRIB_CLIENT_USER, "cmd_select_specified_combat_deck", 0 },
{ 0x227A, ATTRIB_CLIENT_USER, "cmd_select_visible_child", 0 },
{ 0x1AEA, ATTRIB_CLIENT_USER, "cmd_select_visible_pet", 0 },
{ 0xFA42, ATTRIB_CLIENT_USER, "cmd_send_envelope", 0 },
{ 0x110A, ATTRIB_CLIENT_USER, "cmd_send_friend_card", 0 },
{ 0x219A, ATTRIB_CLIENT_USER, "cmd_set_achieve_trace", 0 },
{ 0x1012, ATTRIB_CLIENT_USER, "cmd_set_attention", 0 },
{ 0xF077, ATTRIB_CLIENT_USER, "cmd_set_automatic_reply_msg", 0 },
{ 0x10F0, ATTRIB_CLIENT_USER, "cmd_set_birthday", 0 },
{ 0x1130, ATTRIB_CLIENT_USER, "cmd_set_card_topmost", 0 },
{ 0x7006, ATTRIB_CLIENT_USER, "cmd_set_child_name", 0 },
{ 0x701E, ATTRIB_CLIENT_USER, "cmd_set_decoration", 0 },
{ 0x3AEE, ATTRIB_CLIENT_USER, "cmd_set_doucdh_pet_pos", 0 },
{ 0x115C, ATTRIB_CLIENT_USER, "cmd_set_fireworks_release_time", 0 },
{ 0x2334, ATTRIB_CLIENT_USER, "cmd_set_friend_remark", 0 },
{ 0x1208, ATTRIB_CLIENT_USER, "cmd_set_gcsl_attribs", 0 },
{ 0x2266, ATTRIB_CLIENT_USER, "cmd_set_hotkeys", 0 },
{ 0xE001, ATTRIB_CLIENT_USER, "cmd_set_individuality_image", 0 },
{ 0x6010, ATTRIB_CLIENT_USER, "cmd_setpass", 0 },
{ 0x3402, ATTRIB_CLIENT_USER, "cmd_set_password", 0 },
{ 0x10D2, ATTRIB_CLIENT_USER, "cmd_set_pennant_skill", 0 },
{ 0xFD56, ATTRIB_CLIENT_USER, "cmd_set_pet_name", 0 },
{ 0x7014, ATTRIB_CLIENT_USER, "cmd_set_recommend_attrib", 0 },
{ 0xF0DB, ATTRIB_CLIENT_USER, "cmd_set_rec_person", 0 },
{ 0xFDAC, ATTRIB_CLIENT_USER, "cmd_set_region", 0 },
{ 0x215C, ATTRIB_CLIENT_USER, "cmd_set_safe_login_time", 0 },
{ 0x510A, ATTRIB_CLIENT_USER, "cmd_set_safe_mechanism", 0 },
{ 0x4AF2, ATTRIB_CLIENT_USER, "cmd_set_safe_state", 0 },
{ 0x2348, ATTRIB_CLIENT_USER, "cmd_set_setting", 0 },
{ 0xFAFD, ATTRIB_CLIENT_USER, "cmd_set_individuality_signature", 0 },
{ 0x2188, ATTRIB_CLIENT_USER, "cmd_set_stall_goods", 0 },
{ 0x2238, ATTRIB_CLIENT_USER, "cmd_set_task_trace", 0 },
{ 0x2116, ATTRIB_CLIENT_USER, "cmd_set_user_mood", 0 },
{ 0x29C7, ATTRIB_CLIENT_USER, "cmd_set_user_state", 0 },
{ 0x1B0E, ATTRIB_CLIENT_USER, "cmd_set_wedding_list", 0 },
{ 0x21A0, ATTRIB_CLIENT_USER, "cmd_set_wedding_packet", 0 },
{ 0x2518, ATTRIB_CLIENT_USER, "cmd_set_weibo_info", 0 },
{ 0x40CE, ATTRIB_CLIENT_USER, "cmd_set_xinfa_skill_status", 0 },
{ 0x7E08, ATTRIB_CLIENT_USER, "cmd_shift", 0 },
{ 0x1C09, ATTRIB_CLIENT_USER, "cmd_shop_ad_enter", 0 },
{ 0x5505, ATTRIB_CLIENT_USER, "cmd_shop_ad_fill", 0 },
{ 0x2353, ATTRIB_CLIENT_USER, "cmd_shop_buy_product", 0 },
{ 0x3391, ATTRIB_CLIENT_USER, "cmd_shop_buy_shop", 0 },
{ 0xFA43, ATTRIB_CLIENT_USER, "cmd_shop_change_info", 0 },
{ 0x5E21, ATTRIB_CLIENT_USER, "cmd_shop_do_business", 0 },
{ 0x235D, ATTRIB_CLIENT_USER, "cmd_shop_enter_shop", 0 },
{ 0x107B, ATTRIB_CLIENT_USER, "cmd_shop_enter_shop_type", 0 },
{ 0xFDDF, ATTRIB_CLIENT_USER, "cmd_shop_favorite", 0 },
{ 0x2271, ATTRIB_CLIENT_USER, "cmd_shop_manage_counter", 0 },
{ 0x2283, ATTRIB_CLIENT_USER, "cmd_shop_manage_money", 0 },
{ 0x260C, ATTRIB_CLIENT_USER, "cmd_preview_pet_practice_bonus", 0 },
{ 0xFD84, ATTRIB_CLIENT_USER, "cmd_prompt", 0 },
{ 0x4120, ATTRIB_CLIENT_USER, "cmd_prompt_before_leave", 0 },
{ 0x2EF2, ATTRIB_CLIENT_USER, "cmd_purchase_add_order", 0 },
{ 0x2282, ATTRIB_CLIENT_USER, "cmd_purchase_cancle_order", 0 },
{ 0x23A4, ATTRIB_CLIENT_USER, "cmd_purchase_fetch_cash", 0 },
{ 0x5010, ATTRIB_CLIENT_USER, "cmd_purchase_fetch_item", 0 },
{ 0x20F8, ATTRIB_CLIENT_USER, "cmd_purchase_get_obj_prop", 0 },
{ 0x3046, ATTRIB_CLIENT_USER, "cmd_purchase_get_order_list", 0 },
{ 0xF510, ATTRIB_CLIENT_USER, "cmd_purchase_match_order", 0 },
{ 0x13B0, ATTRIB_CLIENT_USER, "cmd_purchase_multi_match", 0 },
{ 0xFD62, ATTRIB_CLIENT_USER, "cmd_put_child_item_down", 0 },
{ 0x1226, ATTRIB_CLIENT_USER, "cmd_put_down", 0 },
{ 0xFA3C, ATTRIB_CLIENT_USER, "cmd_putin_item", 0 },
{ 0xFD40, ATTRIB_CLIENT_USER, "cmd_query_activity", 0 },
{ 0x2030, ATTRIB_CLIENT_USER, "cmd_query_practice_children", 0 },
{ 0x2300, ATTRIB_CLIENT_USER, "cmd_query_practice_pets", 0 },
{ 0x1A02, ATTRIB_CLIENT_USER, "cmd_query_recruit_info", 0 },
{ 0x3E00, ATTRIB_CLIENT_USER, "cmd_admin_query_report_info", 0 },
{ 0x2208, ATTRIB_CLIENT_USER, "cmd_query_upgrade_cash", 0 },
{ 0x4126, ATTRIB_CLIENT_USER, "cmd_query_upgrade_cash", 0 },
{ 0x1212, ATTRIB_CLIENT_USER, "cmd_query_upgrade_coin", 0 },
{ 0x408A, ATTRIB_CLIENT_USER, "cmd_quit_chat_group", 0 },
{ 0x3016, ATTRIB_CLIENT_USER, "cmd_quit_look_on", 0 },
{ 0x103C, ATTRIB_CLIENT_USER, "cmd_quit_lookon_combat", 0 },
{ 0x1010, ATTRIB_CLIENT_USER, "cmd_quit_team", 0 },
{ 0x1E, ATTRIB_CLIENT_USER, "cmd_race_feed_npc", 0 },
{ 0x1256, ATTRIB_CLIENT_USER, "cmd_race_hit_npc", 0 },
{ 0x2324, ATTRIB_CLIENT_USER, "cmd_race_request_bonus", 0 },
{ 0x2EF1, ATTRIB_CLIENT_USER, "cmd_raid_kickout_member", 0 },
{ 0x1347, ATTRIB_CLIENT_USER, "cmd_raid_leave_raid", 0 },
{ 0x3392, ATTRIB_CLIENT_USER, "cmd_build_random_user_name", 0 },
{ 0x2196, ATTRIB_CLIENT_USER, "cmd_receive_asktao_show_gift", 0 },
{ 0x125C, ATTRIB_CLIENT_USER, "cmd_recharge", 0 },
{ 0x1C, ATTRIB_CLIENT_USER, "cmd_recive_instruction_task", 0 },
{ 0x1362, ATTRIB_CLIENT_USER, "cmd_recover_asktao_show", 0 },
{ 0x2292, ATTRIB_CLIENT_USER, "cmd_recover_child_book", 0 },
{ 0x3804, ATTRIB_CLIENT_USER, "cmd_recover_magic_pennant", 0 },
{ 0x2224, ATTRIB_CLIENT_USER, "cmd_refresh", 0 },
{ 0x1146, ATTRIB_CLIENT_USER, "cmd_refresh_friend", 0 },
{ 0xF0C4, ATTRIB_CLIENT_USER, "cmd_refresh_mount_attrib", 0 },
{ 0x220A, ATTRIB_CLIENT_USER, "cmd_refresh_party_fragment", 0 },
{ 0x3064, ATTRIB_CLIENT_USER, "cmd_refresh_pet_godbook_skills", 0 },
{ 0xFD1A, ATTRIB_CLIENT_USER, "cmd_refresh_recruitment_info", 0 },
{ 0x123E, ATTRIB_CLIENT_USER, "cmd_refresh_safe_login", 0 },
{ 0xFAE0, ATTRIB_CLIENT_USER, "cmd_refresh_service_log", 0 },
{ 0x227C, ATTRIB_CLIENT_USER, "cmd_refresh_task_log", 0 },
{ 0x4076, ATTRIB_CLIENT_USER, "cmd_reject", 0 },
{ 0x1090, ATTRIB_CLIENT_USER, "cmd_remove_asktao_show_gift", 0 },
{ 0x121F, ATTRIB_CLIENT_USER, "cmd_remove_bureau_item", 0 },
{ 0x1134, ATTRIB_CLIENT_USER, "cmd_remove_chat_group_member", 0 },
{ 0x1044, ATTRIB_CLIENT_USER, "cmd_remove_friend", 0 },
{ 0xFD4A, ATTRIB_CLIENT_USER, "cmd_rename_stall", 0 },
{ 0x350E, ATTRIB_CLIENT_USER, "cmd_repair", 0 },
{ 0x402E, ATTRIB_CLIENT_USER, "cmd_repair_furniture", 0 },
{ 0x21C4, ATTRIB_CLIENT_USER, "cmd_replace_login", 0 },
{ 0x1FD6, ATTRIB_CLIENT_USER, "cmd_reply_client_doubt_info", 0 },
{ 0x3E14, ATTRIB_CLIENT_USER, "cmd_reply_client_process_info", 0 },
{ 0xCF0A, ATTRIB_CLIENT_USER, "cmd_reply_friend_verify", 0 },
{ 0x1115, ATTRIB_CLIENT_USER, "cmd_reply_request_asktao_show", 0 },
{ 0x1084, ATTRIB_CLIENT_USER, "cmd_report_cheater", 0 },
{ 0x121C, ATTRIB_CLIENT_USER, "cmd_report_illegal", 0 },
{ 0x30D0, ATTRIB_CLIENT_USER, "cmd_report_illegal_tool", 0 },
{ 0x1202, ATTRIB_CLIENT_USER, "cmd_repurchase", 0 },
{ 0x2198, ATTRIB_CLIENT_USER, "cmd_request_add_stall_page", 0 },
{ 0x23B0, ATTRIB_CLIENT_USER, "cmd_request_asktao_show", 0 },
{ 0x40E4, ATTRIB_CLIENT_USER, "cmd_request_build_deck", 0 },
{ 0x20D4, ATTRIB_CLIENT_USER, "cmd_request_buy_card_package", 0 },
{ 0x3406, ATTRIB_CLIENT_USER, "cmd_request_cancel_match_card_game", 0 },
{ 0x2166, ATTRIB_CLIENT_USER, "cmd_request_card_activity_list", 0 },
{ 0x2040, ATTRIB_CLIENT_USER, "cmd_request_card_game_sort_deck", 0 },
{ 0x111E, ATTRIB_CLIENT_USER, "cmd_request_card_game_sort_list", 0 },
{ 0x4010, ATTRIB_CLIENT_USER, "cmd_request_card_list", 0 },
{ 0x2440, ATTRIB_CLIENT_USER, "cmd_request_card_today_stat", 0 },
{ 0xF0CA, ATTRIB_CLIENT_USER, "cmd_request_chat_group_info", 0 },
{ 0xE, ATTRIB_CLIENT_USER, "cmd_request_city_building_info", 0 },
{ 0x306C, ATTRIB_CLIENT_USER, "cmd_request_city_info", 0 },
{ 0x1230, ATTRIB_CLIENT_USER, "cmd_request_city_list", 0 },
{ 0x7E02, ATTRIB_CLIENT_USER, "cmd_request_combat_deck_list", 0 },
{ 0x70AC, ATTRIB_CLIENT_USER, "cmd_request_compose_card", 0 },
{ 0xFA21, ATTRIB_CLIENT_USER, "cmd_shop_move_merch", 0 },
{ 0x6004, ATTRIB_CLIENT_USER, "cmd_shop_query_counter", 0 },
{ 0xFA13, ATTRIB_CLIENT_USER, "cmd_shop_req_runnings", 0 },
{ 0x2415, ATTRIB_CLIENT_USER, "cmd_shop_req_vendues", 0 },
{ 0x1247, ATTRIB_CLIENT_USER, "cmd_shop_sale", 0 },
{ 0x2195, ATTRIB_CLIENT_USER, "cmd_shop_search", 0 },
{ 0x2384, ATTRIB_CLIENT_USER, "cmd_shop_submit", 0 },
{ 0x20DB, ATTRIB_CLIENT_USER, "cmd_shop_vendue_shop", 0 },
{ 0x2803, ATTRIB_CLIENT_USER, "cmd_shop_view_own_shop", 0 },
{ 0x10B3, ATTRIB_CLIENT_USER, "cmd_shop_view_sale_log", 0 },
{ 0x1204, ATTRIB_CLIENT_USER, "cmd_shout", 0 },
{ 0x2230, ATTRIB_CLIENT_USER, "cmd_shouwu_zudao", 0 },
{ 0x7002, ATTRIB_CLIENT_USER, "cmd_show_sale_log", 0 },
{ 0xE004, ATTRIB_CLIENT_USER, "cmd_show_upgrade_equip_items", 0 },
{ 0x2724, ATTRIB_CLIENT_USER, "cmd_sign_in", 0 },
{ 0x2088, ATTRIB_CLIENT_USER, "cmd_sign_in_draw", 0 },
{ 0x2256, ATTRIB_CLIENT_USER, "cmd_sign_in_open", 0 },
{ 0x13AC, ATTRIB_CLIENT_USER, "cmd_skill_level_down", 0 },
{ 0x1106, ATTRIB_CLIENT_USER, "cmd_sort_pack", 0 },
{ 0x141A, ATTRIB_CLIENT_USER, "cmd_special_move_event", 0 },
{ 0x2DD5, ATTRIB_CLIENT_USER, "cmd_vendue_refresh", 0 },
{ 0x2903, ATTRIB_CLIENT_USER, "cmd_vendue_store_closed", 0 },
{ 0xFA09, ATTRIB_CLIENT_USER, "cmd_vendue_try_vendue", 0 },
{ 0x10BD, ATTRIB_CLIENT_USER, "cmd_vendue_vendue_merch", 0 },
{ 0x1164, ATTRIB_CLIENT_USER, "cmd_verify_char_name", 0 },
{ 0x2226, ATTRIB_CLIENT_USER, "cmd_view_achieve", 0 },
{ 0x141C, ATTRIB_CLIENT_USER, "cmd_view_achieve_overriew", 0 },
{ 0xE002, ATTRIB_CLIENT_USER, "cmd_view_coupon_show", 0 },
{ 0x4014, ATTRIB_CLIENT_USER, "cmd_view_cs_fixtures", 0 },
{ 0x10EC, ATTRIB_CLIENT_USER, "cmd_view_cs_match_detail", 0 },
{ 0x508A, ATTRIB_CLIENT_USER, "cmd_view_cs_matches", 0 },
{ 0x2234, ATTRIB_CLIENT_USER, "cmd_view_cs_matches_summary", 0 },
{ 0x230A, ATTRIB_CLIENT_USER, "cmd_view_cs_pw_fixtures", 0 },
{ 0x21BA, ATTRIB_CLIENT_USER, "cmd_view_party_war_history", 0 },
{ 0x5002, ATTRIB_CLIENT_USER, "cmd_view_stall", 0 },
{ 0x2074, ATTRIB_CLIENT_USER, "cmd_view_top", 0 },
{ 0x2446, ATTRIB_CLIENT_USER, "cmd_warcraft_cancel", 0 },
{ 0x1FD4, ATTRIB_CLIENT_USER, "cmd_warcraft_refresh", 0 },
{ 0x3502, ATTRIB_CLIENT_USER, "cmd_warcraft_start", 0 },
{ 0x1014, ATTRIB_CLIENT_USER, "cmd_view_war_current_info", 0 },
{ 0x1A08, ATTRIB_CLIENT_USER, "cmd_view_war_history_info", 0 },
{ 0x13A4, ATTRIB_CLIENT_USER, "cmd_wear_asktao_show", 0 },
{ 0x2808, ATTRIB_CLIENT_USER, "cmd_who", 0 },
{ 0x6382, ATTRIB_CLIENT_USER, "cmd_window_size_change", 0 },
{ 0x4062, ATTRIB_CLIENT_USER, "cmd_wishing", 0 },
{ 0x304E, ATTRIB_CLIENT_USER, "cmd_wishing_buy_item", 0 },
{ 0x2214, ATTRIB_CLIENT_USER, "cmd_wishing_open", 0 },
{ 0x1AFE, ATTRIB_CLIENT_USER, "cmd_wishing_sale_item", 0 },
{ 0x21CA, ATTRIB_CLIENT_USER, "cmd_withdraw", 0 },
{ 0x3E32, ATTRIB_CLIENT_USER, "cmd_world_level_reset_seal", 0 },
{ 0x3E34, ATTRIB_CLIENT_USER, "cmd_world_level_seal_info", 0 },
{ 0xF0C8, ATTRIB_CLIENT_USER, "cmd_workshop_buy_order", 0 },
{ 0x306A, ATTRIB_CLIENT_USER, "cmd_workshop_query_order_cash", 0 },
{ 0x21B2, ATTRIB_CLIENT_USER, "cmd_workshop_query_order_rate", 0 },
{ 0x1160, ATTRIB_CLIENT_USER, "cmd_workshop_release_order", 0 },
{ 0x10B2, ATTRIB_CLIENT_USER, "cmd_wudao_add_attrib_auto", 0 },
{ 0x5202, ATTRIB_CLIENT_USER, "cmd_wudao_add_attrib_rep", 0 },
{ 0x1254, ATTRIB_CLIENT_USER, "cmd_wudao_batch_add_rep", 0 },
{ 0x2202, ATTRIB_CLIENT_USER, "cmd_wudao_buy_rep_card", 0 },
{ 0x2112, ATTRIB_CLIENT_USER, "cmd_wudao_convert_extra_attrib", 0 },
{ 0x20A6, ATTRIB_CLIENT_USER, "cmd_wudao_draw_rep_card", 0 },
{ 0x21B8, ATTRIB_CLIENT_USER, "cmd_wudao_pre_add_attrib_rep", 0 },
{ 0x3390, ATTRIB_CLIENT_USER, "cmd_wudao_set_attrib_status", 0 },
{ 0x2602, ATTRIB_CLIENT_USER, "cmd_wudao_stage_info", 0 },
{ 0x2232, ATTRIB_CLIENT_USER, "cmd_x_set_setting", 0 },
{ 0xFD1E, ATTRIB_CLIENT_USER, "cmd_double_bonus_time", 0 },
{ 0x5018, ATTRIB_CLIENT_USER, "cmd_get_instruction_bonus", 0 },
{ 0x3508, ATTRIB_CLIENT_USER, "cmd_oper_garden_instruction", 0 },
{ 0x3328, ATTRIB_CLIENT_USER, "cmd_oper_hell_instruction", 0 },
{ 0xF372, ATTRIB_CLIENT_USER, "cmd_stall_input_price", 0 },
{ 0x10AE, ATTRIB_CLIENT_USER, "cmd_start_automatism_stall", 0 },
{ 0x260A, ATTRIB_CLIENT_USER, "cmd_start_box_adventrue", 0 },
{ 0x21CC, ATTRIB_CLIENT_USER, "cmd_start_card_activity", 0 },
{ 0x1024, ATTRIB_CLIENT_USER, "cmd_start_card_duel", 0 },
{ 0xA10, ATTRIB_CLIENT_USER, "cmd_start_card_friend_pk", 0 },
{ 0xFFDE, ATTRIB_CLIENT_USER, "cmd_start_card_instruction", 0 },
{ 0xFD26, ATTRIB_CLIENT_USER, "cmd_start_pet_practice", 0 },
{ 0x1216, ATTRIB_CLIENT_USER, "cmd_start_raise", 0 },
{ 0x10C0, ATTRIB_CLIENT_USER, "cmd_start_stall", 0 },
{ 0x500C, ATTRIB_CLIENT_USER, "cmd_start_taotaotao_run", 0 },
{ 0xF376, ATTRIB_CLIENT_USER, "cmd_store", 0 },
{ 0x4116, ATTRIB_CLIENT_USER, "cmd_submit_confirm_dlg", 0 },
{ 0xF9C1, ATTRIB_CLIENT_USER, "cmd_submit_icon", 0 },
{ 0x1367, ATTRIB_CLIENT_USER, "cmd_submit_individuality_image", 0 },
{ 0x2262, ATTRIB_CLIENT_USER, "cmd_submit_passpod_order", 0 },
{ 0x2110, ATTRIB_CLIENT_USER, "cmd_switch_back_equip", 0 },
{ 0x3052, ATTRIB_CLIENT_USER, "cmd_switch_child_item", 0 },
{ 0x9E, ATTRIB_CLIENT_USER, "cmd_switch_group", 0 },
{ 0x4228, ATTRIB_CLIENT_USER, "cmd_switch_group_ex", 0 },
{ 0x3060, ATTRIB_CLIENT_USER, "cmd_switch_in_store", 0 },
{ 0x1154, ATTRIB_CLIENT_USER, "cmd_switch_item", 0 },
{ 0x10D4, ATTRIB_CLIENT_USER, "cmd_switch_server", 0 },
{ 0xF906, ATTRIB_CLIENT_USER, "cmd_sys_purchase_buy", 0 },
{ 0xF0D0, ATTRIB_CLIENT_USER, "cmd_sys_purchase_sell", 0 },
{ 0x50A6, ATTRIB_CLIENT_USER, "cmd_sys_purchase_view_history", 0 },
{ 0x7E04, ATTRIB_CLIENT_USER, "cmd_sys_purchase_view_orders", 0 },
{ 0x3504, ATTRIB_CLIENT_USER, "cmd_take", 0 },
{ 0x700C, ATTRIB_CLIENT_USER, "cmd_take_child_item_up", 0 },
{ 0x30CC, ATTRIB_CLIENT_USER, "cmd_takeout_card", 0 },
{ 0x4102, ATTRIB_CLIENT_USER, "cmd_takeout_godbook", 0 },
{ 0x2EFE, ATTRIB_CLIENT_USER, "cmd_takeout_item", 0 },
{ 0x1144, ATTRIB_CLIENT_USER, "cmd_take_stall_cash", 0 },
{ 0x3068, ATTRIB_CLIENT_USER, "cmd_take_storing_cash", 0 },
{ 0x251A, ATTRIB_CLIENT_USER, "cmd_take_up", 0 },
{ 0x332A, ATTRIB_CLIENT_USER, "cmd_task_score_exchange_bonus", 0 },
{ 0x1AE6, ATTRIB_CLIENT_USER, "cmd_team_cancel_seek", 0 },
{ 0x7E0A, ATTRIB_CLIENT_USER, "cmd_team_match_seek", 0 },
{ 0x3054, ATTRIB_CLIENT_USER, "cmd_team_open_platform", 0 },
{ 0x70AE, ATTRIB_CLIENT_USER, "cmd_team_request_task", 0 },
{ 0x2246, ATTRIB_CLIENT_USER, "cmd_team_seek_leader", 0 },
{ 0x2160, ATTRIB_CLIENT_USER, "cmd_team_seek_member", 0 },
{ 0xFD4B, ATTRIB_CLIENT_USER, "cmd_teleport_ex", 0 },
{ 0x410A, ATTRIB_CLIENT_USER, "cmd_tell", 0 },
{ 0x2192, ATTRIB_CLIENT_USER, "cmd_tell_ex", 0 },
{ 0x307A, ATTRIB_CLIENT_USER, "cmd_temp_npc_event", 0 },
{ 0xFD68, ATTRIB_CLIENT_USER, "cmd_test_cs_login", 0 },
{ 0x1220, ATTRIB_CLIENT_USER, "cmd_tower_clicked", 0 },
{ 0x3208, ATTRIB_CLIENT_USER, "cmd_tower_selected", 0 },
{ 0x5112, ATTRIB_CLIENT_USER, "cmd_trade_add_goods", 0 },
{ 0x1158, ATTRIB_CLIENT_USER, "cmd_trade_buy_service", 0 },
{ 0x4002, ATTRIB_CLIENT_USER, "cmd_trade_cancel_sale", 0 },
{ 0x2812, ATTRIB_CLIENT_USER, "cmd_trade_change_price", 0 },
{ 0x2424, ATTRIB_CLIENT_USER, "cmd_trade_continue_sale", 0 },
{ 0x4118, ATTRIB_CLIENT_USER, "cmd_trade_favorite", 0 },
{ 0x370A, ATTRIB_CLIENT_USER, "cmd_trade_get_data", 0 },
{ 0x108A, ATTRIB_CLIENT_USER, "cmd_trade_get_obj", 0 },
{ 0x0A04, ATTRIB_CLIENT_USER, "cmd_trade_get_obj_prop", 0 },
{ 0x20F4, ATTRIB_CLIENT_USER, "cmd_trade_get_record", 0 },
{ 0x2AF2, ATTRIB_CLIENT_USER, "cmd_trade_introduce", 0 },
{ 0x1054, ATTRIB_CLIENT_USER, "cmd_trade_open_url", 0 },
{ 0xFAFC, ATTRIB_CLIENT_USER, "cmd_trade_sale", 0 },
{ 0xFA48, ATTRIB_CLIENT_USER, "cmd_trade_view_log", 0 },
{ 0x2228, ATTRIB_CLIENT_USER, "cmd_train", 0 },
{ 0x2294, ATTRIB_CLIENT_USER, "cmd_try_place_pattern", 0 },
{ 0x1070, ATTRIB_CLIENT_USER, "cmd_turn_dir", 0 },
{ 0x122E, ATTRIB_CLIENT_USER, "cmd_txss_reset_bingo", 0 },
{ 0x122C, ATTRIB_CLIENT_USER, "cmd_txss_start_award", 0 },
{ 0xFD0A, ATTRIB_CLIENT_USER, "cmd_unequip", 0 },
{ 0x1112, ATTRIB_CLIENT_USER, "cmd_unequip_inborn_stone", 0 },
{ 0x23AA, ATTRIB_CLIENT_USER, "cmd_unequip_pet_dress", 0 },
{ 0x204E, ATTRIB_CLIENT_USER, "cmd_upgrade", 0 },
{ 0xFD70, ATTRIB_CLIENT_USER, "cmd_upgrade_by_amount", 0 },
{ 0x3C, ATTRIB_CLIENT_USER, "cmd_upgrade", 0 },
{ 0x20AE, ATTRIB_CLIENT_USER, "cmd_upgrade_magic_picture", 0 },
{ 0xFD09, ATTRIB_CLIENT_USER, "cmd_vendue_cancel", 0 },
{ 0x6005, ATTRIB_CLIENT_USER, "cmd_vendue_can_vendue", 0 },
{ 0xFD43, ATTRIB_CLIENT_USER, "cmd_vendue_fetch", 0 },
{ 0x2181, ATTRIB_CLIENT_USER, "cmd_vendue_manage", 0 },
{ 0x21BD, ATTRIB_CLIENT_USER, "cmd_vendue_offer", 0 },
{ 0x5501, ATTRIB_INTERNAL_COMM, "msg_l_oper_protect_range", 0 },
{ 0x5503, ATTRIB_INTERNAL_COMM, "msg_l_auth_all_protect", 0 },
{ 0x3A, ATTRIB_INTERNAL_COMM, "cmd_intersvr_rpc_call", 0 },
{ 0x2358, ATTRIB_INTERNAL_COMM, "cmd_l_agent_connect_server", 0 },
{ 0x2400, ATTRIB_INTERNAL_COMM, "cmd_l_pend_shop", 0 },
{ 0x2484, ATTRIB_INTERNAL_COMM, "cmd_l_when_batch_block_account", 0 },
{ 0x242D, ATTRIB_INTERNAL_COMM, "cmd_l_when_block_account", 0 },
{ 0x2480, ATTRIB_INTERNAL_COMM, "cmd_l_recycle_stall", 0 },
{ 0x2482, ATTRIB_INTERNAL_COMM, "cmd_l_block_share_store", 0 },
{ 0x3A02, ATTRIB_INTERNAL_COMM, "msg_internal_auth", 0 },
{ 0x4385, ATTRIB_INTERNAL_COMM, "msg_l_add_fill_info", 0 },
{ 0x237D, ATTRIB_INTERNAL_COMM, "msg_l_buy_card", 0 },
{ 0x5361, ATTRIB_INTERNAL_COMM, "msg_l_charge_result", 0 },
{ 0x6375, ATTRIB_INTERNAL_COMM, "msg_l_cost_coin", 0 },
{ 0x6379, ATTRIB_INTERNAL_COMM, "msg_l_add_coin", 0 },
{ 0x5383, ATTRIB_INTERNAL_COMM, "msg_l_cost_fill_info", 0 },
{ 0x338D, ATTRIB_INTERNAL_COMM, "msg_l_enable_auto_lock", 0 },
{ 0x3389, ATTRIB_INTERNAL_COMM, "msg_l_lock_account", 0 },
{ 0x1043, ATTRIB_INTERNAL_COMM, "msg_ccs_friend_status", 0 },
{ 0x2347, ATTRIB_INTERNAL_COMM, "msg_ccs_online_friend", 0 },
{ 0x1FB7, ATTRIB_INTERNAL_COMM, "msg_ccs_world_channel", 0 },
{ 0xFDD1, ATTRIB_INTERNAL_COMM, "msg_ccs_system_channel", 0 },
{ 0x2C04, ATTRIB_INTERNAL_COMM, "msg_got_server_time", 0 },
{ 0x226B, ATTRIB_INTERNAL_COMM, "msg_gs_emote_data", 0 },
{ 0x10D9, ATTRIB_INTERNAL_COMM, "msg_gs_finger", 0 },
{ 0x1DF3, ATTRIB_INTERNAL_COMM, "msg_gs_friend_list", 0 },
{ 0x3801, ATTRIB_INTERNAL_COMM, "msg_gs_friend_tell", 0 },
{ 0x1249, ATTRIB_INTERNAL_COMM, "msg_gs_login", 0 },
{ 0x1E15, ATTRIB_INTERNAL_COMM, "msg_gs_logout", 0 },
{ 0xF0B5, ATTRIB_INTERNAL_COMM, "msg_gs_world_channel", 0 },
{ 0xFD15, ATTRIB_INTERNAL_COMM, "msg_gs_system_channel", 0 },
{ 0x1417, ATTRIB_INTERNAL_COMM, "msg_rpc_server_name", 0 },
{ 0x2307, ATTRIB_INTERNAL_COMM, "msg_intersvr_channel_msg", 0 },
{ 0x30D1, ATTRIB_INTERNAL_COMM, "msg_data_mining", 0 },
{ 0x3102, ATTRIB_INTERNAL_COMM, "cmd_league_oper", 0 },
{ 0x3101, ATTRIB_INTERNAL_COMM, "msg_league_oper", 0 },
{ 0x3104, ATTRIB_INTERNAL_COMM, "cmd_bottle_oper", 0 },
{ 0x3103, ATTRIB_INTERNAL_COMM, "msg_bottle_oper", 0 },
{ 0x30AC, ATTRIB_INTERNAL_COMM, "cmd_xgame_oper", 0 },
{ 0x23AB, ATTRIB_INTERNAL_COMM, "msg_xgame_oper", 0 },
{ 0x3327, ATTRIB_INTERNAL_COMM, "msg_l_do_inquiry", 0 },
{ 0x3325, ATTRIB_INTERNAL_COMM, "msg_l_do_recharge", 0 },
{ 0x2086, ATTRIB_INTERNAL_COMM, "cmd_partywar_oper", 0 },
{ 0xFD1B, ATTRIB_INTERNAL_COMM, "msg_partywar_oper", 0 },
{ 0x304C, ATTRIB_INTERNAL_COMM, "cmd_arena_oper", 0 },
{ 0x2159, ATTRIB_INTERNAL_COMM, "msg_arena_oper", 0 },
{ 0x2DD1, ATTRIB_INTERNAL_COMM, "cmd_draft_oper", 0 },
{ 0xFD33, ATTRIB_INTERNAL_COMM, "msg_draft_oper", 0 },
{ 0x2354, ATTRIB_INTERNAL_COMM, "cmd_sddh_oper", 0 },
{ 0x4105, ATTRIB_INTERNAL_COMM, "msg_sddh_oper", 0 },
{ 0x115A, ATTRIB_INTERNAL_COMM, "cmd_celebrity_oper", 0 },
{ 0x110F, ATTRIB_INTERNAL_COMM, "msg_celebrity_oper", 0 },
{ 0x2194, ATTRIB_INTERNAL_COMM, "cmd_samsung_oper", 0 },
{ 0xF3, ATTRIB_INTERNAL_COMM, "msg_samsung_oper", 0 },
{ 0x329, ATTRIB_INTERNAL_COMM, "msg_everyone_pk_oper", 0 },
{ 0x4423, ATTRIB_INTERNAL_COMM, "msg_treasure_trading_oper", 0 },
{ 0x2139, ATTRIB_INTERNAL_COMM, "msg_l_convert_silver_coin", 0 },
{ 0x2804, ATTRIB_INTERNAL_COMM, "cmd_channel_oper", 0 },
{ 0x2161, ATTRIB_INTERNAL_COMM, "msg_channel_oper", 0 },
{ 0x109A, ATTRIB_INTERNAL_COMM, "cmd_csa_common_oper", 0 },
{ 0x39D1, ATTRIB_INTERNAL_COMM, "msg_csa_common_oper", 0 },
{ 0x1B0A, ATTRIB_INTERNAL_COMM, "cmd_citywar_oper", 0 },
{ 0xFA15, ATTRIB_INTERNAL_COMM, "msg_citywar_oper", 0 },
{ 0x50A8, ATTRIB_INTERNAL_COMM, "cmd_activity_oper", 0 },
{ 0xFA37, ATTRIB_INTERNAL_COMM, "msg_activity_oper", 0 },
{ 0x3379, ATTRIB_INTERNAL_COMM, "msg_l_lock_card", 0 },
{ 0x637B, ATTRIB_INTERNAL_COMM, "msg_l_manage_vip_sms", 0 },
{ 0x6387, ATTRIB_INTERNAL_COMM, "msg_l_manage_mp_device", 0 },
{ 0x6377, ATTRIB_INTERNAL_COMM, "msg_l_precost_coin", 0 },
{ 0x238B, ATTRIB_INTERNAL_COMM, "msg_l_query_account_state", 0 },
{ 0x537F, ATTRIB_INTERNAL_COMM, "msg_l_query_card", 0 },
{ 0x6373, ATTRIB_INTERNAL_COMM, "msg_l_query_coin", 0 },
{ 0x4381, ATTRIB_INTERNAL_COMM, "msg_l_query_fill_info", 0 },
{ 0x238F, ATTRIB_INTERNAL_COMM, "msg_l_query_addiction", 0 },
{ 0x637D, ATTRIB_INTERNAL_COMM, "msg_l_send_vip_sms", 0 },
{ 0x6381, ATTRIB_INTERNAL_COMM, "msg_l_do_alipay_recharge", 0 },
{ 0x3382, ATTRIB_INTERNAL_COMM, "msg_l_ssb_copy_account", 0 },
{ 0x3381, ATTRIB_INTERNAL_COMM, "msg_l_ssb_update_account", 0 },
{ 0x3383, ATTRIB_INTERNAL_COMM, "msg_l_get_ssb_combatd_conf", 0 },
{ 0x3384, ATTRIB_INTERNAL_COMM, "msg_l_get_ssb_finish_flag", 0 },
{ 0x3385, ATTRIB_INTERNAL_COMM, "msg_l_get_ssb_prompt_msg", 0 },
{ 0x3380, ATTRIB_INTERNAL_COMM, "msg_l_get_ssb_combatd_res", 0 },
{ 0x536B, ATTRIB_INTERNAL_COMM, "msg_l_request_ip_card", 0 },
{ 0x536D, ATTRIB_INTERNAL_COMM, "msg_l_request_coupon", 0 },
{ 0x3317, ATTRIB_INTERNAL_COMM, "msg_l_change_coupon", 0 },
{ 0x3319, ATTRIB_INTERNAL_COMM, "msg_l_query_random_coupon", 0 },
{ 0xFD99, ATTRIB_INTERNAL_COMM, "msg_l_update_first_credit_bonus", 0 },
{ 0xFD79, ATTRIB_INTERNAL_COMM, "msg_l_update_recharge_return_credit", 0 },
{ 0x2391, ATTRIB_INTERNAL_COMM, "msg_l_update_addiction", 0 },
{ 0x337B, ATTRIB_INTERNAL_COMM, "msg_l_unlock_card", 0 },
{ 0x3369, ATTRIB_INTERNAL_COMM, "msg_l_verify_password", 0 },
{ 0x4367, ATTRIB_INTERNAL_COMM, "msg_verify_code_result", 0 },
{ 0x2479, ATTRIB_INTERNAL_COMM, "msg_cfg_aaa_list", 0 },
{ 0x2367, ATTRIB_INTERNAL_COMM, "msg_cfg_ccs_info", 0 },
{ 0x2363, ATTRIB_INTERNAL_COMM, "msg_cfg_ddb_list", 0 },
{ 0x235B, ATTRIB_INTERNAL_COMM, "msg_cfg_dist_list", 0 },
{ 0x2471, ATTRIB_INTERNAL_COMM, "msg_cfg_etc_page_data", 0 },
{ 0x135D, ATTRIB_INTERNAL_COMM, "msg_cfg_my_dist", 0 },
{ 0x2473, ATTRIB_INTERNAL_COMM, "msg_cfg_read_etc", 0 },
{ 0x247B, ATTRIB_INTERNAL_COMM, "msg_cfg_write_etc", 0 },
{ 0x247D, ATTRIB_INTERNAL_COMM, "msg_cfg_csa_info", 0 },
{ 0x247F, ATTRIB_INTERNAL_COMM, "msg_cfg_tts_info", 0 },
{ 0x2475, ATTRIB_INTERNAL_COMM, "msg_cfg_ah_server_info", 0 },
{ 0xFD51, ATTRIB_INTERNAL_COMM, "msg_get_goods_from_tts", 0 },
{ 0xFDA5, ATTRIB_INTERNAL_COMM, "msg_check_role_on_sale", 0 },
{ 0xFD57, ATTRIB_INTERNAL_COMM, "msg_sell_treasure", 0 },
{ 0x2E37, ATTRIB_INTERNAL_COMM, "msg_get_sys_envelope", 0 },
{ 0xF0AF, ATTRIB_INTERNAL_COMM, "msg_intersvr_rpc_ret", 0 },
{ 0x3421, ATTRIB_INTERNAL_COMM, "msg_xg_create_name", 0 },
{ 0x3423, ATTRIB_INTERNAL_COMM, "msg_xg_get_account", 0 },
{ 0x3425, ATTRIB_INTERNAL_COMM, "msg_xg_get_name", 0 },
{ 0x4387, ATTRIB_INTERNAL_COMM, "msg_l_ecard_business", 0 },
{ 0x2393, ATTRIB_INTERNAL_COMM, "msg_l_query_ip_area", 0 },
{ 0xF504, ATTRIB_INTERNAL_COMM, "msg_l_get_vote_result", 0 },
{ 0x3386, ATTRIB_INTERNAL_COMM, "msg_l_clone_account_result", 0 },
{ 0x239C, ATTRIB_INTERNAL_COMM, "msg_l_set_safe_ctrl_str", 0 },
{ 0x239D, ATTRIB_INTERNAL_COMM, "msg_l_set_safe_ctrl_var", 0 },
{ 0x2390, ATTRIB_INTERNAL_COMM, "msg_l_get_safe_ctrl_info", 0 },
{ 0x638F, ATTRIB_INTERNAL_COMM, "msg_l_get_credit_coins", 0 },
{ 0x1221, ATTRIB_INTERNAL_COMM, "msg_l_get_protect_info", 0 },
{ 0x2401, ATTRIB_INTERNAL_COMM, "msg_l_query_coupon", 0 },
{ 0x1395, ATTRIB_INTERNAL_COMM, "msg_l_get_local_encrypt_info", 0 },
{ 0x1397, ATTRIB_INTERNAL_COMM, "msg_l_set_local_encrypt_info", 0 },
{ 0x13B1, ATTRIB_INTERNAL_COMM, "msg_l_borrow_local_encrypt", 0 },
{ 0xFD9B, ATTRIB_INTERNAL_COMM, "msg_l_set_phah_bonus_info", 0 },
{ 0x23A1, ATTRIB_INTERNAL_COMM, "msg_l_query_citys", 0 },
{ 0x13A3, ATTRIB_INTERNAL_COMM, "msg_l_get_acc_comp_char", 0 },
{ 0xF903, ATTRIB_INTERNAL_COMM, "cmd_cta_to_server", 0 },
{ 0x40F6, ATTRIB_INTERNAL_COMM, "cmd_ma_oper", 0 },
{ 0x10A0, ATTRIB_INTERNAL_COMM, "cmd_ccs_shutdown_by_spa", 0 },
{ 0x40F8, ATTRIB_INTERNAL_COMM, "cmd_ma_set", 0 },
{ 0x604, ATTRIB_INTERNAL_COMM, "msg_spa_busy", 0 },
{ 0xFD17, ATTRIB_INTERNAL_COMM, "msg_ccs_reboot_result", 0 },
{ 0x1C03, ATTRIB_INTERNAL_COMM, "cmd_get_server_time", 0 },
{ 0x2383, ATTRIB_INTERNAL_COMM, "msg_ccs_emote_data", 0 },
{ 0x2147, ATTRIB_INTERNAL_COMM, "msg_ccs_finger_res", 0 },
{ 0x2805, ATTRIB_INTERNAL_COMM, "msg_ccs_friend_tell", 0 },
{ 0x4401, ATTRIB_INTERNAL_COMM, "msg_read_content", 0 },
{ 0x2403, ATTRIB_INTERNAL_COMM, "msg_written_content", 0 },
{ 0x3411, ATTRIB_INTERNAL_COMM, "msg_created_gid", 0 },
{ 0x3415, ATTRIB_INTERNAL_COMM, "msg_got_gid", 0 },
{ 0x4419, ATTRIB_INTERNAL_COMM, "msg_got_gid_arr", 0 },
{ 0x4417, ATTRIB_INTERNAL_COMM, "msg_got_gid_info", 0 },
};
constexpr size_t kCmdCount = sizeof(kCmdList) / sizeof(kCmdList[0]);

const CmdEntry* findCmd(uint16_t cmdNo) {
    for (size_t i = 0; i < kCmdCount; ++i)
        if (kCmdList[i].cmdNo == cmdNo) return &kCmdList[i];
    return nullptr;
}

// Apply an LPC function on the master program with a variable argument list.
Value applyLpc(const char* fun, const std::vector<Value>& args) {
    // The master VM is registered under "<main>" with the pak's search root
    // (e.g. "linux/aaa - bak/<main>"); find it regardless of the exact root.
    // callMasterProgramFunction is exposed by original_vm.h; pass the root of
    // whichever <main> entry exists.
    return callMasterProgramFunction(fun, args, lpc::masterSearchRoot());
}

// ---- command handlers -----------------------------------------------------

// vm_msgLAuth (0x804ca10): body = result(be:u32) authKey(be:u32) nameLen(be:u16) name
// -> vm_apply("msg_l_auth", clientNo, result, authKey, message)
//   (original pushes: funName, nArgs=4, 0x58,clientNo, 0x58,result, 0x58,authKey, 0x165,message)
void handleMsgLAuth(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 4) return;
    int32_t result = static_cast<int32_t>(rd32be(body));
    int32_t authKey = 0;
    std::string name;
    if (len >= 8) authKey = static_cast<int32_t>(rd32be(body + 4));
    if (len >= 10) {
        uint16_t nlen = rd16be(body + 8);
        if (nlen <= len - 10) name.assign(reinterpret_cast<const char*>(body + 10), nlen);
    }
    if (std::getenv("LPCVM_TRACE"))
        std::cerr << "[game] msg_l_auth client=" << cl.clientNo << " result=" << result
                  << " authKey=" << authKey << " name='" << name << "'\n";
    applyLpc("msg_l_auth", {make_int(cl.clientNo), make_int(result), make_int(authKey), make_string(name)});
}

// vm_cmdEcho (0x8053178): echo/heartbeat (CMD_ECHO 0x20D2).  body = client
// tick (be:u32).  Replies MSG_REPLY_ECHO (0xF9C3) carrying the server's
// current tick so the remote end (dba/gs/cta) can verify the link is alive.
// Mirrors the original: refreshes timeRecvEcho on a valid echo (so the gs's
// scansocketd daemon does not drop the connection as dead) and reports the
// heartbeat to LPC cmd_echo() at most once per 180s.
void handleCmdEcho(GameClient& cl, const uint8_t* body, size_t len) {
    (void)body;
    if (len < 4) return;
    // Original reads the client tick (v9) and an optional server tick (v5);
    // tickLastRecvLastEcho guards against replayed/duplicate echoes.
    uint32_t v9 = rd32be(body);
    uint32_t v5 = 0;
    if (len >= 8) v5 = rd32be(body + 4);
    if (cl.tickLastRecvLastEcho != 0) {
        uint32_t period = v9 - cl.tickLastRecvLastEcho;
        if (period <= 0x270F) {  // <= 9999ms: "Got bad ECHO" - drop it
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[game] bad echo client=" << cl.clientNo
                          << " period=" << period << "ms\n";
            return;
        }
    }
    cl.tickLastRecvLastEcho = v9;
    time_t now = time(nullptr);
    if (!v5 || cl.tickServer == v5) cl.timeRecvEcho = now;
    uint64_t nowMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    uint32_t tick = static_cast<uint32_t>(nowMs);
    cl.tickServer = tick;
    std::string frame;
    frame += "MZ";
    frame += std::string(2, '\0');     // checksum
    frame += std::string(4, '\0');     // tick field (0)
    uint16_t msgLen = 6;               // cmdNo(2) + body(4)
    frame.push_back(static_cast<char>((msgLen >> 8) & 0xFF));
    frame.push_back(static_cast<char>(msgLen & 0xFF));
    uint16_t cmd = 0xF9C3;             // MSG_REPLY_ECHO
    frame.push_back(static_cast<char>((cmd >> 8) & 0xFF));
    frame.push_back(static_cast<char>(cmd & 0xFF));
    frame.push_back(static_cast<char>((tick >> 24) & 0xFF));
    frame.push_back(static_cast<char>((tick >> 16) & 0xFF));
    frame.push_back(static_cast<char>((tick >> 8) & 0xFF));
    frame.push_back(static_cast<char>(tick & 0xFF));
    if (std::getenv("LPCVM_TRACE"))
        std::cerr << "[game] echo reply client=" << cl.clientNo << " tick=" << tick << "\n";
    gameServiceSend(cl.clientNo, frame);
    if (now - cl.timeLastReportHeartBeat > 179) {
        cl.timeLastReportHeartBeat = now;
        applyLpc("cmd_echo", {make_int(cl.clientNo)});
    }
}

// vm_msgReplyEcho (0x8066958): inbound MSG_REPLY_ECHO (0xF9C3).  body =
// server tick (be:u32).  The driver applies LPC "msg_reply_echo" with
// (clientNo, tick); gs's start_gs.c forwards reply_echo to the user object.
void handleMsgReplyEcho(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 4) return;
    uint32_t tick = rd32be(body);
    if (std::getenv("LPCVM_TRACE"))
        std::cerr << "[game] reply_echo client=" << cl.clientNo << " tick=" << tick << "\n";
    applyLpc("msg_reply_echo", {make_int(cl.clientNo), make_int(tick)});
}

// value-type body reader (vm_padVariable / vm_pktPadVariable wire form):
//   string = u8 length + bytes; int16 = u16 BE; mapping = u16 count + pairs.
struct VtReader {
    const uint8_t* p;
    size_t n;
    size_t off = 0;
    VtReader(const uint8_t* body, size_t len) : p(body), n(len) {}
    std::string str() {
        if (off >= n) return "";
        uint8_t k = p[off++];
        if (k > n - off) k = static_cast<uint8_t>(n - off);
        std::string s(reinterpret_cast<const char*>(p + off), k);
        off += k;
        return s;
    }
    int u16() {
        if (off + 2 > n) return 0;
        int v = (static_cast<int>(p[off]) << 8) | static_cast<int>(p[off + 1]);
        off += 2;
        return v;
    }
};

// vm_msgCfgMyInfo (0x8065b30): msg_cfg_my_info value-typed body ->
// LPC msg_cfg_my_info(clientNo, dist, name, port, id, distId, settings).
void handleMsgCfgMyInfo(GameClient& cl, const uint8_t* body, size_t len) {
    VtReader r(body, len);
    std::string name = r.str();
    int port = r.u16();
    std::string dist = r.str();
    int id = r.u16();
    int distId = r.u16();
    int count = r.u16();
    auto m = std::make_shared<MappingValue>();
    for (int i = 0; i < count; ++i) {
        std::string k = r.str();
        std::string v = r.str();
        m->insert(make_string(k), make_string(v));
    }
    if (std::getenv("LPCVM_TRACE"))
        std::cerr << "[game] msg_cfg_my_info client=" << cl.clientNo << " name='" << name
                  << "' port=" << port << " dist='" << dist << "' id=" << id
                  << " distId=" << distId << " settings=" << m->size();
    if (std::getenv("LPCVM_TRACE")) {
        auto items = m->items();
        std::string ks;
        for (auto& kv : items) {
            if (!ks.empty()) ks += ",";
            ks += to_string(kv.first, false) + "=" + to_string(kv.second, false).substr(0, 20);
        }
        std::cerr << " keys=[" << ks << "]\n";
    } else if (std::getenv("LPCVM_TRACE"))
        std::cerr << "\n";
    applyLpc("msg_cfg_my_info",
             {make_int(cl.clientNo), make_string(dist), make_string(name),
              make_int(port), make_int(id), make_int(distId), m});
}

// vm_msgCfgSpaInfo (0x8065ea0): body = [u16 count][name u8len+str][ip u8len+str]
// [port u16] (first SPA host) -> msg_cfg_spa_info(clientNo, ip, port, name).
void handleMsgCfgSpaInfo(GameClient& cl, const uint8_t* body, size_t len) {
    VtReader r(body, len);
    r.u16();                     // list count
    std::string name = r.str();
    std::string ip = r.str();
    int port = r.u16();
    if (std::getenv("LPCVM_TRACE"))
        std::cerr << "[game] msg_cfg_spa_info client=" << cl.clientNo << " name='" << name
                  << "' ip='" << ip << "' port=" << port << "\n";
    applyLpc("msg_cfg_spa_info",
             {make_int(cl.clientNo), make_string(ip), make_int(port), make_string(name)});
}

// vm_msgCfgGameServerList (0x8064b74): GAME_HOST list[] in column-major form:
//   [u16 total] total*(name u8len+str, ip u8len+str, port u16)
//   then total*(id u16) then total*(enable u16)
// -> msg_cfg_game_server_list(clientNo, array of [name, ip, port, id, enable]).
void handleMsgCfgGameServerList(GameClient& cl, const uint8_t* body, size_t len) {
    VtReader r(body, len);
    int total = r.u16();
    if (total < 0 || total > 100) return;
    std::vector<std::string> names, ips;
    std::vector<int> ports, ids, enables;
    for (int i = 0; i < total; ++i) { names.push_back(r.str()); ips.push_back(r.str()); ports.push_back(r.u16()); }
    for (int i = 0; i < total; ++i) ids.push_back(r.u16());
    for (int i = 0; i < total; ++i) enables.push_back(r.u16());
    // Flat array: [name, ip, port, id, enable] * total (vm_apply pushes a
    // single flat array; msg_cfg_game_server_list reads list[i], list[i+1]...).
    auto arr = std::make_shared<ArrayValue>();
    for (int i = 0; i < total; ++i) {
        arr->items.push_back(make_string(names[i]));
        arr->items.push_back(make_string(ips[i]));
        arr->items.push_back(make_int(ports[i]));
        arr->items.push_back(make_int(ids[i]));
        arr->items.push_back(make_int(enables[i]));
    }
    if (std::getenv("LPCVM_TRACE"))
        std::cerr << "[game] msg_cfg_game_server_list client=" << cl.clientNo
                  << " total=" << total << "\n";
    applyLpc("msg_cfg_game_server_list", {make_int(cl.clientNo), arr});
}

// vm_msgCfgDbaList (0x806567c) / vm_msgCfgLdbaList (0x8065824):
//   DBA_HOST/LDBA_HOST = string ip, int16 port; list[] row-major.
void handleMsgCfgDbaList(GameClient& cl, const uint8_t* body, size_t len, const char* fun) {
    VtReader r(body, len);
    int count = r.u16();
    if (count < 0 || count > 100) return;
    auto arr = std::make_shared<ArrayValue>();
    for (int i = 0; i < count; ++i) {
        arr->items.push_back(make_string(r.str()));   // ip
        arr->items.push_back(make_int(r.u16()));      // port
    }
    if (std::getenv("LPCVM_TRACE"))
        std::cerr << "[game] " << fun << " client=" << cl.clientNo << " count=" << count << "\n";
    applyLpc(fun, {make_int(cl.clientNo), arr});
}

// vm_msgCfgCtaList (0x8064454): CTA_HOST = name, ip, port, user, pass, privilege.
void handleMsgCfgCtaList(GameClient& cl, const uint8_t* body, size_t len) {
    VtReader r(body, len);
    int count = r.u16();
    if (count < 0 || count > 100) return;
    auto arr = std::make_shared<ArrayValue>();
    for (int i = 0; i < count; ++i) {
        arr->items.push_back(make_string(r.str()));   // name
        arr->items.push_back(make_string(r.str()));   // ip
        arr->items.push_back(make_int(r.u16()));      // port
        arr->items.push_back(make_string(r.str()));   // user
        arr->items.push_back(make_string(r.str()));   // pass
        arr->items.push_back(make_string(r.str()));   // privilege
    }
    if (std::getenv("LPCVM_TRACE"))
        std::cerr << "[game] msg_cfg_cta_list client=" << cl.clientNo << " count=" << count << "\n";
    applyLpc("msg_cfg_cta_list", {make_int(cl.clientNo), arr});
}

// ---- filed<->dba data protocol (second batch) -----------------------------
//
// Raw big-endian body handlers (NOT VtReader value-type encoding).  All are
// type=2 (ATTRIB_INTERNAL_COMM); the LPC function takes clientNo first.

// Two-u32 body: [u32 BE a][u32 BE b] -> fun(clientNo, a, b).  Confirmed from
// decompiles (each checks len >= 8 and pushes 88,int x 2):
//   vm_msgDeletedContent   0x806769c -> msg_deleted_content
//   vm_msgCreatedContent   0x8067704 -> msg_created_content
//   vm_msgMovedContent     0x8068000 -> msg_moved_content
//   vm_msgGotCatalog       0x80678e0 -> msg_got_catalog
//   vm_msgGotCatalogEx     0x8067948 -> msg_got_catalog_ex
//   vm_msgReadLog          0x80679b0 -> msg_read_log
//   vm_msgUpdateLog        0x8067a18 -> msg_update_log
//   vm_msgDeletedGid       0x8067c68 -> msg_deleted_gid
//   vm_msgUpdatedGid       0x8067cd0 -> msg_updated_gid
//   vm_msgCreatePetDeposit 0x806776c -> msg_create_pet_deposit
//   vm_msgWritePetDeposit  0x80677d4 -> msg_write_pet_deposit
void handleMsgIntInt(GameClient& cl, const uint8_t* body, size_t len, const char* fun) {
    if (len < 8) return;
    // a/b are SIGNED int32 in the original wire protocol (results can be
    // negative, e.g. -1 / -2 failures); pass them signed so filed's
    // do_operate sees -1 instead of 4294967295.
    int32_t a = static_cast<int32_t>(rd32be(body));
    int32_t b = static_cast<int32_t>(rd32be(body + 4));
    if (std::getenv("LPCVM_SVC_TRACE"))
        std::cerr << "[game] " << fun << " client=" << cl.clientNo
                  << " a=" << a << " b=" << b << "\n";
    applyLpc(fun, {make_int(cl.clientNo), make_int(a), make_int(b)});
}

// vm_msgReadPetDeposit (0x806783c): body = [u32 BE a][u32 BE b][u32 BE strLen][str]
// -> msg_read_pet_deposit(clientNo, a, b, str)  (88,88,88,357)
void handleMsgReadPetDeposit(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 12) return;
    uint32_t a = rd32be(body);
    int32_t b = static_cast<int32_t>(rd32be(body + 4));   // result (signed)
    uint32_t slen = rd32be(body + 8);
    if (slen > len - 12) return;
    std::string s(reinterpret_cast<const char*>(body + 12), slen);
    applyLpc("msg_read_pet_deposit",
             {make_int(cl.clientNo), make_int(a), make_int(b), make_string(s)});
}

// vm_msgGotPageData (0x8067a80): body = [u32 BE gid][u32 BE strLen][str]
// -> msg_got_page_data(clientNo, gid, str)  (88,88,357)
void handleMsgGotPageData(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 8) return;
    uint32_t gid = rd32be(body);
    uint32_t slen = rd32be(body + 4);
    if (slen > len - 8) return;
    std::string s(reinterpret_cast<const char*>(body + 8), slen);
    applyLpc("msg_got_page_data",
             {make_int(cl.clientNo), make_int(gid), make_string(s)});
}

// vm_msgGotPageDataEx (0x8067b00): body = [u32 BE gid][u32 BE blen][buffer]
// -> msg_got_page_data_ex(clientNo, gid, buffer)  (88,88,228 buffer)
void handleMsgGotPageDataEx(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 8) return;
    uint32_t gid = rd32be(body);
    uint32_t blen = rd32be(body + 4);
    if (blen > len - 8) return;
    std::string b(reinterpret_cast<const char*>(body + 8), blen);
    applyLpc("msg_got_page_data_ex",
             {make_int(cl.clientNo), make_int(gid), make_buffer(b)});
}

// vm_msgGetCounts (0x8066ed4): body = [u32 BE cookie][u32 BE result][u8 count][count bytes]
// -> msg_get_counts(clientNo, cookie, result, counts)  (88,88,88,357)
void handleMsgGetCounts(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 9) return;
    uint32_t cookie = rd32be(body);
    int32_t result = static_cast<int32_t>(rd32be(body + 4));   // result (signed)
    uint32_t count = body[8];
    if (count > len - 9) return;
    std::string s(reinterpret_cast<const char*>(body + 9), count);
    applyLpc("msg_get_counts",
             {make_int(cl.clientNo), make_int(cookie), make_int(result), make_string(s)});
}

// ---- game commands (type=1) generated handlers, chunk 1 ----

void handle_vm_cmd_Apprentice_List(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    int v1 = body[0];
    applyLpc("cmd_apprentice_list", {make_int(cl.clientNo), make_int(v1)});
}

void handle_vm_cmd_Recruit_Apprentice(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    int v1 = body[0];
    applyLpc("cmd_recruit_apprentice", {make_int(cl.clientNo), make_int(v1)});
}

void handle_vm_cmd_Start_Award(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    int v0 = body[0];
    int v1 = body[1];
    applyLpc("cmd_start_award", {make_int(cl.clientNo), make_int(v0), make_int(v1)});
}

void handle_vm_cmd_StartMultibonus(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_start_multi_bonus", {make_int(cl.clientNo)});
}

void handle_vm_cmdAccelerateMoveTo(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 10) return;
    uint32_t time = rd32be(body);
    uint32_t mapId = rd32be(body + 4);
    uint16_t n = rd16be(body + 8);
    if (n == 0 || n > 16) return;
    if (len < 10u + 4u * n + 2u) return;
    auto arr = std::make_shared<ArrayValue>();
    const uint8_t* p = body + 10;
    for (uint16_t i = 0; i < n; ++i)
    {
        arr->items.push_back(make_int(rd16be(p)));
        arr->items.push_back(make_int(rd16be(p + 2)));
        p += 4;
    }
    int dir = rd16be(p);
    applyLpc("cmd_accelerate_move_to", {make_int(cl.clientNo), make_int((int)time), make_int((int)mapId), arr, make_int(dir)});
}

void handle_vm_cmdAccept(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t off = 0;
    size_t n1 = body[off++];
    if (off + n1 > len) return;
    std::string name((const char*)body + off, n1);
    off += n1;
    if (off + 1 > len) return;
    size_t n2 = body[off++];
    if (off + n2 > len) return;
    std::string askType((const char*)body + off, n2);
    applyLpc("cmd_accept", {make_int(cl.clientNo), make_string(name), make_string(askType)});
}

void handle_vm_cmdAcceptChatGroupInvite(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t off = 0;
    size_t n1 = body[off++];
    if (off + n1 > len) return;
    std::string group((const char*)body + off, n1);
    off += n1;
    if (off + 1 > len) return;
    int v = body[off];
    applyLpc("cmd_accept_chat_group_invite", {make_int(cl.clientNo), make_string(group), make_int(v)});
}

void handle_vm_cmdAcceptChatGroupJoin(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t off = 0;
    size_t n1 = body[off++];
    if (off + n1 > len) return;
    std::string group((const char*)body + off, n1);
    off += n1;
    if (off + 1 > len) return;
    size_t n2 = body[off++];
    if (off + n2 > len) return;
    std::string name((const char*)body + off, n2);
    off += n2;
    if (off + 1 > len) return;
    int v = body[off];
    applyLpc("cmd_accept_chat_group_join", {make_int(cl.clientNo), make_string(group), make_string(name), make_int(v)});
}

void handle_vm_cmdAcceptCommonConvenant(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t n = body[0];
    if (1 + n > len) return;
    std::string pgid((const char*)body + 1, n);
    applyLpc("cmd_accept_common_convenant", {make_int(cl.clientNo), make_string(pgid)});
}

void handle_vm_cmdAcceptConvenant(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t n = body[0];
    if (1 + n > len) return;
    std::string pgid((const char*)body + 1, n);
    applyLpc("cmd_accept_convenant", {make_int(cl.clientNo), make_string(pgid)});
}

void handle_vm_cmdAddAwardTimes(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_add_award_times", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdAddBureauPage(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    applyLpc("cmd_add_bureau_page", {make_int(cl.clientNo), make_int(rd16be(body, 0))});
}

void handle_vm_cmdAddDunwuNimbus(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t id = rd32be(body, 0);
    size_t off = 4;
    if (off >= len) return;
    uint8_t sl = body[off++];
    if (off + sl > len) return;
    std::string skill((const char*)body + off, sl);
    applyLpc("cmd_add_dunwu_nimbus", {make_int(cl.clientNo), make_int(id), make_string(skill)});
}

void handle_vm_cmdAddFriend(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    uint8_t gl = body[off++];
    if (off + gl > len) return;
    std::string group((const char*)body + off, gl);
    off += gl;
    if (off >= len) return;
    uint8_t nl = body[off++];
    if (off + nl > len) return;
    std::string name((const char*)body + off, nl);
    off += nl;
    if (off >= len) return;
    applyLpc("cmd_add_friend", {make_int(cl.clientNo), make_string(group), make_string(name), make_int(body[off])});
}

void handle_vm_cmdAddGodbookDirect(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t id = rd32be(body, 0);
    size_t off = 4;
    if (off >= len) return;
    uint8_t sl = body[off++];
    if (off + sl > len) return;
    std::string name((const char*)body + off, sl);
    applyLpc("cmd_add_godbook_direct", {make_int(cl.clientNo), make_int(id), make_string(name)});
}

void handle_vm_cmdAddMaterial(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 10) return;
    uint32_t id = rd32be(body, 0);
    uint16_t qty = rd16be(body, 4);
    uint16_t v1 = rd16be(body, 6);
    uint16_t v2 = rd16be(body, 8);
    applyLpc("cmd_add_material", {make_int(cl.clientNo), make_int(id), make_int(qty), make_int(v1), make_int(v2)});
}

void handle_vm_cmdAddMountItem(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_add_mount_item", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdAddSkillEssence(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t id = rd32be(body, 0);
    size_t off = 4;
    if (off >= len) return;
    uint8_t sl = body[off++];
    if (off + sl > len) return;
    std::string skill((const char*)body + off, sl);
    applyLpc("cmd_add_skill_essence", {make_int(cl.clientNo), make_int(id), make_string(skill)});
}

void handle_vm_cmdAdminBroadcast(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    size_t off = 2;
    size_t ml = rd16be(body, 0);
    if (ml > 4095) ml = 4095;
    if (off + ml > len) return;
    std::string msg((const char*)body + off, ml);
    off += ml;
    if (off >= len) return;
    uint8_t al = body[off++];
    if (off + al > len) return;
    std::string area((const char*)body + off, al);
    applyLpc("cmd_admin_broadcast", {make_int(cl.clientNo), make_string(msg), make_string(area)});
}

void handle_vm_cmdAdminCommonCommand(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_admin_common_command", {make_int(cl.clientNo), make_buffer(body, len)});
}

void handle_vm_cmdBroadcastCombatList(GameClient& cl, const uint8_t* body, size_t len) {
    (void)body; (void)len;
    applyLpc("cmd_broadcast_combat_list", {make_int(cl.clientNo)});
}

void handle_vm_cmdButtonDoubleBonus(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 2) return;
    int a = body[0];
    int b = body[1];
    applyLpc("cmd_button_double_bonus", {make_int(cl.clientNo), make_int(a), make_int(b)});
}

void handle_vm_cmdBuyCard(GameClient& cl, const uint8_t* body, size_t len) {
    size_t off = 0;
    if (off >= len) return;
    uint8_t n1 = body[off++];
    if (off + n1 > len) return;
    std::string cardNo((const char*)body + off, n1);
    off += n1;
    if (off >= len) return;
    uint8_t n2 = body[off++];
    if (off + n2 > len) return;
    std::string account((const char*)body + off, n2);
    off += n2;
    applyLpc("cmd_buy_card", {make_int(cl.clientNo), make_string(cardNo), make_string(account)});
}

void handle_vm_cmdBuyCustomizePack(GameClient& cl, const uint8_t* body, size_t len) {
    size_t off = 0;
    if (off >= len) return;
    uint8_t n1 = body[off++];
    if (off + n1 > len) return;
    std::string coinType((const char*)body + off, n1);
    off += n1;
    if (off >= len) return;
    uint8_t count = body[off++];
    auto arr = std::make_shared<ArrayValue>();
    for (uint8_t i = 0; i < count; i++) {
        if (off >= len) return;
        uint8_t sl = body[off++];
        if (off + sl > len) return;
        std::string s((const char*)body + off, sl);
        off += sl;
        if (off >= len) return;
        int v = body[off++];
        arr->items.push_back(make_string(s));
        arr->items.push_back(make_int(v));
    }
    applyLpc("cmd_buy_customize_package", {make_int(cl.clientNo), arr, make_string(coinType)});
}

void handle_vm_cmdBuyEcard(GameClient& cl, const uint8_t* body, size_t len) {
    size_t off = 0;
    if (off >= len) return;
    uint8_t n1 = body[off++];
    if (off + n1 > len) return;
    std::string id((const char*)body + off, n1);
    off += n1;
    if (off + 4 > len) return;
    uint32_t num = rd32be(body + off);
    off += 4;
    applyLpc("cmd_buy_ecard", {make_int(cl.clientNo), make_string(id), make_int(num)});
}

void handle_vm_cmdBuyFromAskTaoShowMall(GameClient& cl, const uint8_t* body, size_t len) {
    size_t off = 0;
    if (off >= len) return;
    uint8_t n1 = body[off++];
    if (off + n1 > len) return;
    std::string barcode((const char*)body + off, n1);
    off += n1;
    if (off + 6 > len) return;
    uint16_t amount = rd16be(body + off);
    uint16_t usedDay = rd16be(body + off + 2);
    uint16_t toPos = rd16be(body + off + 4);
    off += 6;
    if (off >= len) return;
    uint8_t n2 = body[off++];
    if (off + n2 > len) return;
    std::string coinPwd((const char*)body + off, n2);
    off += n2;
    if (off >= len) return;
    uint8_t n3 = body[off++];
    if (off + n3 > len) return;
    std::string coinType((const char*)body + off, n3);
    off += n3;
    applyLpc("cmd_buy_from_asktao_show_mall", {make_int(cl.clientNo), make_string(barcode), make_int(amount), make_int(usedDay), make_int(toPos), make_string(coinPwd), make_string(coinType)});
}

void handle_vm_cmdBuyFromOnlineMall(GameClient& cl, const uint8_t* body, size_t len) {
    size_t off = 0;
    if (off >= len) return;
    uint8_t n1 = body[off++];
    if (off + n1 > len) return;
    std::string barcode((const char*)body + off, n1);
    off += n1;
    if (off + 2 > len) return;
    uint16_t amount = rd16be(body + off);
    off += 2;
    if (off >= len) return;
    uint8_t n2 = body[off++];
    if (off + n2 > len) return;
    std::string coinPwd((const char*)body + off, n2);
    off += n2;
    if (off >= len) return;
    uint8_t n3 = body[off++];
    if (off + n3 > len) return;
    std::string coinType((const char*)body + off, n3);
    off += n3;
    if (off >= len) return;
    uint8_t n4 = body[off++];
    if (off + n4 > len) return;
    std::string rebate((const char*)body + off, n4);
    off += n4;
    applyLpc("cmd_buy_from_online_mall", {make_int(cl.clientNo), make_string(barcode), make_int(amount), make_string(coinPwd), make_string(coinType), make_string(rebate)});
}

void handle_vm_cmdBuyFromStall(GameClient& cl, const uint8_t* body, size_t len) {
    size_t off = 0;
    if (off >= len) return;
    uint8_t n1 = body[off++];
    if (off + n1 > len) return;
    std::string name((const char*)body + off, n1);
    off += n1;
    if (off + 10 > len) return;
    uint32_t v1 = rd32be(body + off);
    uint16_t v2 = rd16be(body + off + 4);
    uint32_t v3 = rd32be(body + off + 6);
    off += 10;
    applyLpc("cmd_buy_from_stall", {make_int(cl.clientNo), make_string(name), make_int(v1), make_int(v2), make_int(v3)});
}

void handle_vm_cmdBuyService(GameClient& cl, const uint8_t* body, size_t len) {
    size_t off = 0;
    if (off >= len) return;
    uint8_t n1 = body[off++];
    if (off + n1 > len) return;
    std::string name((const char*)body + off, n1);
    off += n1;
    if (off >= len) return;
    uint8_t n2 = body[off++];
    if (off + n2 > len) return;
    std::string coinType((const char*)body + off, n2);
    off += n2;
    applyLpc("cmd_buy_service", {make_int(cl.clientNo), make_string(name), make_string(coinType)});
}

void handle_vm_cmdCalcMountAttrib(GameClient& cl, const uint8_t* body, size_t len) {
    size_t off = 0;
    if (off >= len) return;
    uint8_t n1 = body[off++];
    if (off + n1 > len) return;
    std::string petName((const char*)body + off, n1);
    off += n1;
    if (off + 2 > len) return;
    int a = body[off];
    int b = body[off + 1];
    off += 2;
    applyLpc("cmd_calc_mount_attrib", {make_int(cl.clientNo), make_string(petName), make_int(a), make_int(b)});
}

void handle_vm_cmdCalcPetGrow(GameClient& cl, const uint8_t* body, size_t len) {
    size_t off = 0;
    if (off >= len) return;
    uint8_t n1 = body[off++];
    if (off + n1 > len) return;
    std::string petName((const char*)body + off, n1);
    off += n1;
    if (off + 4 > len) return;
    int lifeEffect = rd32be(body + off);
    off += 4;
    if (off + 4 > len) return;
    int manaEffect = rd32be(body + off);
    off += 4;
    if (off + 4 > len) return;
    int speedEffect = rd32be(body + off);
    off += 4;
    if (off + 4 > len) return;
    int phyEffect = rd32be(body + off);
    off += 4;
    if (off + 4 > len) return;
    int magEffect = rd32be(body + off);
    off += 4;
    if (off + 3 > len) return;
    int state = body[off];
    int phyRebuildTimes = body[off + 1];
    int magRebuildTimes = body[off + 2];
    off += 3;
    if (off >= len) return;
    uint8_t n2 = body[off++];
    if (off + n2 > len) return;
    std::string evolvePetName((const char*)body + off, n2);
    off += n2;
    if (off + 4 > len) return;
    int evolveLife = rd32be(body + off);
    off += 4;
    if (off + 4 > len) return;
    int evolveMana = rd32be(body + off);
    off += 4;
    applyLpc("cmd_calc_pet_grow", {make_int(cl.clientNo), make_string(petName), make_int(lifeEffect), make_int(manaEffect), make_int(speedEffect), make_int(phyEffect), make_int(magEffect), make_int(state), make_int(phyRebuildTimes), make_int(magRebuildTimes), make_string(evolvePetName), make_int(evolveLife), make_int(evolveMana)});
}

void handle_vm_cmdCalcUserAttribs(GameClient& cl, const uint8_t* body, size_t len) {
    size_t off = 0;
    if (off >= len) return;
    uint8_t n1 = body[off++];
    if (off + n1 > len) return;
    std::string para((const char*)body + off, n1);
    off += n1;
    applyLpc("cmd_calc_user_attribs", {make_int(cl.clientNo), make_string(para)});
}

void handle_vm_cmdCancelAuthVipSms(GameClient& cl, const uint8_t* body, size_t len) {
    (void)body; (void)len;
    applyLpc("vm_cmdCancelAuthVipSms", {make_int(cl.clientNo)});
}

void handle_vm_cmdCancelReleaseSafeLogin(GameClient& cl, const uint8_t* body, size_t len) {
    (void)body; (void)len;
    applyLpc("cmd_cancel_release_safe_login", {make_int(cl.clientNo)});
}

void handle_vm_cmdCareItem(GameClient& cl, const uint8_t* body, size_t len) {
    size_t off = 0;
    if (off >= len) return;
    uint8_t n1 = body[off++];
    if (off + n1 > len) return;
    std::string name((const char*)body + off, n1);
    off += n1;
    if (off >= len) return;
    uint8_t n2 = body[off++];
    if (off + n2 > len) return;
    std::string iid((const char*)body + off, n2);
    off += n2;
    applyLpc("cmd_care_item", {make_int(cl.clientNo), make_string(name), make_string(iid)});
}

void handle_vm_cmdCast(GameClient& cl, const uint8_t* body, size_t len) {
    size_t off = 0;
    if (off + 4 > len) return;
    int victimId = rd32be(body + off);
    off += 4;
    if (off >= len) return;
    uint8_t n1 = body[off++];
    if (off + n1 > len) return;
    std::string name((const char*)body + off, n1);
    off += n1;
    if (off + 2 > len) return;
    uint16_t skillId = rd16be(body + off);
    off += 2;
    applyLpc("cmd_cast", {make_int(cl.clientNo), make_int(victimId), make_string(name), make_int(skillId)});
}

void handle_vm_cmdCasualGameDoAction(GameClient& cl, const uint8_t* body, size_t len) {
    size_t off = 0;
    if (off + 4 > len) return;
    int roomId = rd32be(body + off);
    off += 4;
    if (off >= len) return;
    uint8_t n1 = body[off++];
    if (off + n1 > len) return;
    std::string action((const char*)body + off, n1);
    off += n1;
    if (off >= len) return;
    uint8_t n2 = body[off++];
    if (off + n2 > len) return;
    std::string para((const char*)body + off, n2);
    off += n2;
    if (off >= len) return;
    uint8_t n3 = body[off++];
    if (off + n3 > len) return;
    std::string para1((const char*)body + off, n3);
    off += n3;
    if (off >= len) return;
    uint8_t n4 = body[off++];
    if (off + n4 > len) return;
    std::string para2((const char*)body + off, n4);
    off += n4;
    applyLpc("cmd_casual_game_do_action", {make_int(cl.clientNo), make_int(roomId), make_string(action), make_string(para), make_string(para1), make_string(para2)});
}

void handle_vm_cmdCasualOperGame(GameClient& cl, const uint8_t* body, size_t len) {
    size_t off = 0;
    if (off >= len) return;
    int operType = body[off++];
    if (off >= len) return;
    uint8_t n1 = body[off++];
    if (off + n1 > len) return;
    std::string index((const char*)body + off, n1);
    off += n1;
    if (off >= len) return;
    uint8_t n2 = body[off++];
    if (off + n2 > len) return;
    std::string para((const char*)body + off, n2);
    off += n2;
    applyLpc("cmd_casual_oper_game", {make_int(cl.clientNo), make_int(operType), make_string(index), make_string(para)});
}

void handle_vm_cmdCatchPker(GameClient& cl, const uint8_t* body, size_t len) {
    size_t off = 0;
    if (off >= len) return;
    uint8_t n1 = body[off++];
    if (off + n1 > len) return;
    std::string name((const char*)body + off, n1);
    off += n1;
    applyLpc("cmd_catch_pker", {make_int(cl.clientNo), make_string(name)});
}

void handle_vm_cmdCCatchPet(GameClient& cl, const uint8_t* body, size_t len) {
    size_t off = 0;
    if (off + 4 > len) return;
    int petId = rd32be(body + off);
    off += 4;
    applyLpc("cmd_c_catch_pet", {make_int(cl.clientNo), make_int(petId)});
}

void handle_vm_cmdCCDoAction(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 16) return;
    uint32_t action = rd32be(body);
    uint32_t attacker = rd32be(body + 4);
    uint32_t victim = rd32be(body + 8);
    uint32_t para = rd32be(body + 12);
    size_t off = 16;
    if (off >= len) return;
    uint8_t slen = body[off++];
    if (off + slen > len) return;
    std::string para2((const char*)body + off, slen);
    applyLpc("cmd_cc_do_action", {make_int(cl.clientNo), make_int(action), make_int(attacker), make_int(victim), make_int(para), make_string(para2)});
}

void handle_vm_cmdCCFinishRound(GameClient& cl, const uint8_t* body, size_t len) {
    (void)body; (void)len;
    applyLpc("cmd_cc_finish_round", {make_int(cl.clientNo)});
}

void handle_vm_cmdCCSurrender(GameClient& cl, const uint8_t* body, size_t len) {
    (void)body; (void)len;
    applyLpc("cmd_cc_surrender", {make_int(cl.clientNo)});
}

void handle_vm_cmdCCSwitchCard(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 2) return;
    uint16_t cnt = rd16be(body);
    if (cnt > 63 || (size_t)2 + (size_t)cnt * 2 > len) return;
    auto arr = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < cnt; ++i)
        arr->items.push_back(make_int(rd16be(body + 2 + i * 2)));
    applyLpc("cmd_cc_switch_card", {make_int(cl.clientNo), arr});
}

void handle_vm_cmdCDoAction0(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 16) return;
    uint32_t id = rd32be(body);
    uint32_t victimId = rd32be(body + 4);
    uint32_t action = rd32be(body + 8);
    uint32_t para = rd32be(body + 12);
    size_t off = 16;
    std::string para1, para2, para3, custom_msg;
    if (off >= len) return;
    {
        uint8_t slen = body[off++];
        if (off + slen > len) return;
        para1.assign((const char*)body + off, slen);
        off += slen;
    }
    if (off >= len) return;
    {
        uint8_t slen = body[off++];
        if (off + slen > len) return;
        para2.assign((const char*)body + off, slen);
        off += slen;
    }
    if (off >= len) return;
    {
        uint8_t slen = body[off++];
        if (off + slen > len) return;
        para3.assign((const char*)body + off, slen);
        off += slen;
    }
    if (off >= len) return;
    {
        uint8_t slen = body[off++];
        if (off + slen > len) return;
        custom_msg.assign((const char*)body + off, slen);
    }
    applyLpc("cmd_c_do_action", {make_int(0), make_int(cl.clientNo), make_int(id), make_int(victimId), make_int(action), make_int(para), make_string(para1), make_string(para2), make_string(para3), make_string(custom_msg)});
}

void handle_vm_cmdCDoAction1(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 16) return;
    uint32_t id = rd32be(body);
    uint32_t victimId = rd32be(body + 4);
    uint32_t action = rd32be(body + 8);
    uint32_t para = rd32be(body + 12);
    size_t off = 16;
    std::string para1, para2, para3, custom_msg;
    if (off >= len) return;
    {
        uint8_t slen = body[off++];
        if (off + slen > len) return;
        para1.assign((const char*)body + off, slen);
        off += slen;
    }
    if (off >= len) return;
    {
        uint8_t slen = body[off++];
        if (off + slen > len) return;
        para2.assign((const char*)body + off, slen);
        off += slen;
    }
    if (off >= len) return;
    {
        uint8_t slen = body[off++];
        if (off + slen > len) return;
        para3.assign((const char*)body + off, slen);
        off += slen;
    }
    if (off >= len) return;
    {
        uint8_t slen = body[off++];
        if (off + slen > len) return;
        custom_msg.assign((const char*)body + off, slen);
    }
    applyLpc("cmd_c_do_action", {make_int(1), make_int(cl.clientNo), make_int(id), make_int(victimId), make_int(action), make_int(para), make_string(para1), make_string(para2), make_string(para3), make_string(custom_msg)});
}

void handle_vm_cmdCDoAction2(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 16) return;
    uint32_t id = rd32be(body);
    uint32_t victimId = rd32be(body + 4);
    uint32_t action = rd32be(body + 8);
    uint32_t para = rd32be(body + 12);
    size_t off = 16;
    std::string para1, para2, para3, custom_msg;
    if (off >= len) return;
    {
        uint8_t slen = body[off++];
        if (off + slen > len) return;
        para1.assign((const char*)body + off, slen);
        off += slen;
    }
    if (off >= len) return;
    {
        uint8_t slen = body[off++];
        if (off + slen > len) return;
        para2.assign((const char*)body + off, slen);
        off += slen;
    }
    if (off >= len) return;
    {
        uint8_t slen = body[off++];
        if (off + slen > len) return;
        para3.assign((const char*)body + off, slen);
        off += slen;
    }
    if (off >= len) return;
    {
        uint8_t slen = body[off++];
        if (off + slen > len) return;
        custom_msg.assign((const char*)body + off, slen);
    }
    applyLpc("cmd_c_do_action", {make_int(2), make_int(cl.clientNo), make_int(id), make_int(victimId), make_int(action), make_int(para), make_string(para1), make_string(para2), make_string(para3), make_string(custom_msg)});
}

void handle_vm_cmdCdWishingGoods(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 1) return;
    uint32_t type = body[0];
    size_t off = 1;
    if (off >= len) return;
    uint8_t slen = body[off++];
    if (off + slen > len) return;
    std::string goods_name((const char*)body + off, slen);
    applyLpc("cmd_cd_wishing_goods", {make_int(cl.clientNo), make_int(type), make_string(goods_name)});
}

void handle_vm_cmdCdWishingRate(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 1) return;
    uint32_t type = body[0];
    size_t off = 1;
    if (off >= len) return;
    uint8_t slen = body[off++];
    if (off + slen > len) return;
    std::string goods_name((const char*)body + off, slen);
    off += slen;
    if (off + 2 > len) return;
    uint32_t rate = rd16be(body + off);
    applyLpc("cmd_cd_wishing_rate", {make_int(cl.clientNo), make_int(type), make_string(goods_name), make_int(rate)});
}

void handle_vm_cmdCdWishingStart(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 1) return;
    uint32_t type = body[0];
    size_t off = 1;
    if (off >= len) return;
    uint8_t slen = body[off++];
    if (off + slen > len) return;
    std::string goods_name((const char*)body + off, slen);
    off += slen;
    if (off + 2 > len) return;
    uint32_t rate = rd16be(body + off);
    applyLpc("cmd_cd_wishing_start", {make_int(cl.clientNo), make_int(type), make_string(goods_name), make_int(rate)});
}

void handle_vm_cmdCEndAnimate(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t v7 = rd32be(body);
    applyLpc("cmd_c_end_animate", {make_int(cl.clientNo), make_int(v7)});
}

void handle_vm_cmdCFlee(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_c_flee", {make_int(cl.clientNo)});
}

void handle_vm_cmdChallengeAnswer(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    uint32_t v5 = rd32be(body);
    uint8_t slen = body[4];
    if (5 + slen > len) return;
    std::string info((const char*)body + 5, slen);
    applyLpc("cmd_challenge_answer", {make_int(cl.clientNo), make_int(v5), make_string(info)});
}

void handle_vm_cmdChangeArenaRoomSeat(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t seat = rd16be(body);
    uint32_t a2 = body[2];
    uint32_t a3 = body[3];
    applyLpc("cmd_arena_room_change_seat", {make_int(cl.clientNo), make_int(seat), make_int(a2), make_int(a3)});
}

void handle_vm_cmdChangeAttribPlan(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_change_attrib_plan", {make_int(cl.clientNo)});
}

void handle_vm_cmdChangeChatGroupSetting(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t glen = body[0];
    if (1 + glen + 2 > len) return;
    std::string group((const char*)body + 1, glen);
    uint32_t oper = rd16be(body + 1 + glen);
    uint8_t plen = body[1 + glen + 2];
    if (1 + glen + 3 + plen > len) return;
    std::string para((const char*)body + 1 + glen + 3, plen);
    applyLpc("cmd_change_chat_group_setting", {make_int(cl.clientNo), make_string(group), make_int(oper), make_string(para)});
}

void handle_vm_cmdChangeFurnitureDir(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    uint32_t id = rd32be(body);
    uint32_t dir = body[4];
    applyLpc("cmd_change_furniture_direction", {make_int(cl.clientNo), make_int(id), make_int(dir)});
}

void handle_vm_cmdChangeItemAttrib(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint32_t attrib = body[0];
    applyLpc("cmd_change_item_attrib", {make_int(cl.clientNo), make_int(attrib)});
}

void handle_vm_cmdChangeTeamLeader(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t nlen = body[0];
    if (1 + nlen + 1 > len) return;
    std::string name((const char*)body + 1, nlen);
    uint32_t leader = body[1 + nlen];
    applyLpc("cmd_change_team_leader", {make_int(cl.clientNo), make_string(name), make_int(leader)});
}

void handle_vm_cmdChangeTitle(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t tlen = body[0];
    if (1 + tlen > len) return;
    std::string title((const char*)body + 1, tlen);
    applyLpc("cmd_change_title", {make_int(cl.clientNo), make_string(title)});
}

void handle_vm_cmdChangeTitleEffect(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    if (1 + slen > len) return;
    std::string titleField((const char*)body + 1, slen);
    applyLpc("cmd_change_title_effect", {make_int(cl.clientNo), make_string(titleField)});
}

void handle_vm_cmdChangeUpgradeState(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_change_upgrade_state", {make_int(cl.clientNo)});
}

void handle_vm_cmdChangeUserState(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_change_user_state", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdChangeZhenfa(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 6) return;
    uint32_t v7 = rd32be(body);
    uint16_t v8 = rd16be(body + 4);
    applyLpc("cmd_change_zhenfa", {make_int(cl.clientNo), make_int(v7), make_int(v8)});
}

void handle_vm_cmdChargeVendueFetch(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    if (1 + slen > len) return;
    std::string keyid((const char*)body + 1, slen);
    applyLpc("cmd_charge_vendue_fetch", {make_int(cl.clientNo), make_string(keyid)});
}

void handle_vm_cmdChargeVendueOfferPrice(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 3) return;
    uint16_t pos = rd16be(body);
    uint8_t slen = body[2];
    if (3 + slen > len) return;
    std::string vendue_iid((const char*)body + 3, slen);
    applyLpc("cmd_charge_vendue_offer_price", {make_int(cl.clientNo), make_int(pos), make_string(vendue_iid)});
}

void handle_vm_cmdChargeVendueRefresh(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_charge_vendue_refresh", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdChargeVendueStoreClosed(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_charge_vendue_store_closed", {make_int(cl.clientNo)});
}

void handle_vm_cmdCharPractice(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint16_t v5 = rd16be(body + 2);
    applyLpc("cmd_char_practice", {make_int(cl.clientNo), make_int(body[0]), make_int(body[1]), make_int(v5)});
}

void handle_vm_cmdChat(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 3) return;
    uint16_t channelId = rd16be(body);
    uint8_t slen = body[2];
    if (3 + slen > len) return;
    std::string msg((const char*)body + 3, slen);
    applyLpc("cmd_chat", {make_int(cl.clientNo), make_int(channelId), make_string(msg)});
}

void handle_vm_cmdChatEx(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (len < 8) return;
    uint16_t channelId = rd16be(body + off); off += 2;
    uint16_t comp = rd16be(body + off); off += 2;
    uint16_t orgLen = rd16be(body + off); off += 2;
    uint16_t msgLen = rd16be(body + off); off += 2;
    if (msgLen > len - off) return;
    std::string msg((const char*)body + off, msgLen); off += msgLen;
    auto arr = std::make_shared<ArrayValue>();
    auto arrType = std::make_shared<ArrayValue>();
    if (len - off == 1) return;
    if (len - off >= 2)
    {
        uint16_t itemCount = rd16be(body + off); off += 2;
        if (itemCount > 20) return;
        for (uint16_t i = 0; i < itemCount; ++i)
        {
            if (off >= len) return;
            uint8_t sl = body[off++];
            if (sl > len - off) return;
            arr->items.push_back(make_string(std::string((const char*)body + off, sl)));
            off += sl;
            if (off >= len) return;
            arrType->items.push_back(make_int(body[off++]));
        }
    }
    applyLpc("cmd_chat_ex", {make_int(cl.clientNo), make_int(channelId), make_int(comp), make_int(orgLen), make_string(msg), arr, arrType});
}

void handle_vm_cmdChatGroupLog(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t gl = body[0];
    if (gl > len - 1) return;
    std::string group((const char*)body + 1, gl);
    applyLpc("cmd_request_chat_group_logs", {make_int(cl.clientNo), make_string(group)});
}

void handle_vm_cmdCheckSafeAnswer(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t al = body[0];
    if (al > len - 1) return;
    std::string answer((const char*)body + 1, al);
    applyLpc("cmd_check_safe_answer", {make_int(cl.clientNo), make_string(answer)});
}

void handle_vm_cmdCheckServer(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t blen = rd32be(body);
    if (blen > len - 4) return;
    applyLpc("cmd_check_server", {make_int(cl.clientNo), make_buffer(body + 4, blen)});
}

void handle_vm_cmdChildEnterHall(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 6) return;
    uint32_t v = rd32be(body);
    uint8_t b4 = body[4];
    uint8_t b5 = body[5];
    applyLpc("cmd_child_enter_hall", {make_int(cl.clientNo), make_int((int)v), make_int(b4), make_int(b5)});
}

void handle_vm_cmdChildEnterMap(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (len < 5) return;
    uint32_t id = rd32be(body + off); off += 4;
    uint8_t nl = body[off++];
    if (nl > len - off) return;
    std::string mapName((const char*)body + off, nl); off += nl;
    if (off >= len) return;
    applyLpc("cmd_child_enter_map", {make_int(cl.clientNo), make_int((int)id), make_string(mapName), make_int(body[off])});
}

void handle_vm_cmdChildSkillLevelUp(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 8) return;
    uint32_t a = rd32be(body);
    uint32_t b = rd32be(body + 4);
    applyLpc("cmd_child_skill_level_up", {make_int(cl.clientNo), make_int((int)a), make_int((int)b)});
}

void handle_vm_cmdChildUseCombat(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 6) return;
    uint32_t v = rd32be(body);
    uint8_t b4 = body[4];
    uint8_t b5 = body[5];
    applyLpc("cmd_child_use_combat", {make_int(cl.clientNo), make_int((int)v), make_int(b4), make_int(b5)});
}

void handle_vm_cmdChildUseSkill(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 9) return;
    uint32_t a = rd32be(body);
    uint32_t b = rd32be(body + 4);
    uint8_t c = body[8];
    applyLpc("cmd_child_use_skill", {make_int(cl.clientNo), make_int((int)a), make_int((int)b), make_int(c)});
}

void handle_vm_cmdCitywarGiveOrders(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    uint8_t clen = body[off++];
    if (clen > len - off) return;
    std::string city((const char*)body + off, clen); off += clen;
    if (off >= len) return;
    uint8_t blen = body[off++];
    if (blen > len - off) return;
    std::string building((const char*)body + off, blen); off += blen;
    if (off >= len) return;
    applyLpc("cmd_citywar_give_orders", {make_int(cl.clientNo), make_string(city), make_string(building), make_int(body[off])});
}

void handle_vm_cmdCleanRequest(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    uint8_t n = body[off++];
    if (n > len - off) return;
    std::string ask_type((const char*)body + off, n);
    off += n;
    applyLpc("cmd_clean_request", {make_int(cl.clientNo), make_string(ask_type)});
}

void handle_vm_cmdClearBag(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    int type = body[off++];
    if (len - off < 2) return;
    uint16_t n1 = rd16be(body + off); off += 2;
    if (n1 > len - off) return;
    std::string pos_desc((const char*)body + off, n1); off += n1;
    if (off >= len) return;
    uint8_t n2 = body[off++];
    if (n2 > len - off) return;
    std::string para((const char*)body + off, n2); off += n2;
    applyLpc("cmd_clear_bag", {make_int(cl.clientNo), make_int(type), make_string(pos_desc), make_string(para)});
}

void handle_vm_cmdClickedLinker(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    uint8_t n = body[off++];
    if (n > len - off) return;
    std::string url((const char*)body + off, n);
    off += n;
    applyLpc("cmd_clicked_linker", {make_int(cl.clientNo), make_string(url)});
}

void handle_vm_cmdClientBehavior(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    uint8_t n = body[off++];
    if (n > len - off) return;
    std::string behavior_data((const char*)body + off, n);
    off += n;
    applyLpc("cmd_client_behavior", {make_int(cl.clientNo), make_string(behavior_data)});
}

void handle_vm_cmdCloneSpecialEquip(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    uint8_t n1 = body[off++];
    if (n1 > len - off) return;
    std::string equip_name((const char*)body + off, n1); off += n1;
    if (off >= len) return;
    int equip_type = body[off++];
    if (off >= len) return;
    uint8_t n2 = body[off++];
    if (n2 > len - off) return;
    std::string equip_color((const char*)body + off, n2); off += n2;
    if (len - off < 2) return;
    uint16_t n3 = rd16be(body + off); off += 2;
    if (n3 > len - off) return;
    std::string equip_prop((const char*)body + off, n3); off += n3;
    if (len - off < 2) return;
    uint16_t n4 = rd16be(body + off); off += 2;
    if (n4 > len - off) return;
    std::string equip_prop2((const char*)body + off, n4); off += n4;
    if (len - off < 2) return;
    uint16_t n5 = rd16be(body + off); off += 2;
    if (n5 > len - off) return;
    std::string equip_prop3((const char*)body + off, n5); off += n5;
    if (len - off < 2) return;
    uint16_t n6 = rd16be(body + off); off += 2;
    if (n6 > len - off) return;
    std::string equip_prop4((const char*)body + off, n6); off += n6;
    applyLpc("cmd_clone_special_equip", {make_int(cl.clientNo), make_string(equip_name), make_int(equip_type), make_string(equip_color), make_string(equip_prop), make_string(equip_prop2), make_string(equip_prop3), make_string(equip_prop4)});
}

void handle_vm_cmdCloneUpgradeStone(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    int equip_type = body[off++];
    if (off >= len) return;
    int level = body[off++];
    if (off >= len) return;
    uint8_t n = body[off++];
    if (n > len - off) return;
    std::string attrib((const char*)body + off, n); off += n;
    if (len - off < 4) return;
    int param = (int)rd32be(body + off); off += 4;
    if (off >= len) return;
    int flag = body[off++];
    applyLpc("cmd_clone_upgrade_stone", {make_int(cl.clientNo), make_int(equip_type), make_int(level), make_string(attrib), make_int(param), make_int(flag)});
}

void handle_vm_cmdCloseBureau(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_close_bureau", {make_int(cl.clientNo)});
}

void handle_vm_cmdCloseMenu(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (len - off < 4) return;
    int menu_type = (int)rd32be(body + off); off += 4;
    if (len - off < 4) return;
    int param = (int)rd32be(body + off); off += 4;
    applyLpc("cmd_close_menu", {make_int(cl.clientNo), make_int(menu_type), make_int(param)});
}

void handle_vm_cmdCloseRemoteStore(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (len - off < 4) return;
    int store_id = (int)rd32be(body + off); off += 4;
    applyLpc("cmd_close_remote_store", {make_int(cl.clientNo), make_int(store_id)});
}

void handle_vm_cmdCloseTalkDrama(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (len - off < 4) return;
    int drama_id = (int)rd32be(body + off); off += 4;
    if (off >= len) return;
    uint8_t n = body[off++];
    if (n > len - off) return;
    std::string para((const char*)body + off, n); off += n;
    applyLpc("cmd_close_talk_drama", {make_int(cl.clientNo), make_int(drama_id), make_string(para)});
}

void handle_vm_cmdCloseTaotaotao(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_close_taotaotao", {make_int(cl.clientNo)});
}

void handle_vm_cmdCoinCancelOrder(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    if (1 + slen > len) return;
    std::string order_id((const char*)body + 1, slen);
    applyLpc("cmd_coin_cancel_order", {make_int(cl.clientNo), make_string(order_id)});
}

void handle_vm_cmdCoinDealOrder(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 3) return;
    uint8_t oper = body[0];
    uint8_t step = body[1];
    uint8_t slen = body[2];
    if (3 + slen + 4 > len) return;
    std::string order_id((const char*)body + 3, slen);
    uint32_t orderNo = rd32be(body + 3 + slen);
    applyLpc("cmd_coin_deal_order", {make_int(cl.clientNo), make_int(oper), make_int(step), make_string(order_id), make_int(orderNo)});
}

void handle_vm_cmdCoinFetchStore(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_coin_fetch_store", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdCoinOpenMall(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_coin_open_mall", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdCoinRefreshMall(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_coin_refresh_mall", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdCoinSubmitOrder(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 9) return;
    uint8_t storeNo = body[0];
    uint32_t orderNo = rd32be(body + 1);
    uint32_t amount = rd32be(body + 5);
    applyLpc("cmd_coin_submit_order", {make_int(cl.clientNo), make_int(storeNo), make_int(orderNo), make_int(amount)});
}

void handle_vm_cmdCollectData(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 3) return;
    uint8_t type = body[0];
    uint16_t slen = rd16be(body + 1);
    if (3 + slen > len) return;
    std::string data((const char*)body + 3, slen);
    applyLpc("cmd_collect_data", {make_int(cl.clientNo), make_int(type), make_string(data)});
}

void handle_vm_cmdCommonGuessDrawBonus(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t glen = body[0];
    if (1 + glen > len) return;
    std::string guess((const char*)body + 1, glen);
    size_t off = 1 + glen;
    if (off >= len) return;
    uint8_t rlen = body[off];
    if (off + 1 + rlen > len) return;
    std::string result((const char*)body + off + 1, rlen);
    applyLpc("cmd_common_guess_draw_bonus", {make_int(cl.clientNo), make_string(guess), make_string(result)});
}

void handle_vm_cmdCommonGuessInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    if (1 + slen > len) return;
    std::string guess((const char*)body + 1, slen);
    applyLpc("cmd_common_guess_info", {make_int(cl.clientNo), make_string(guess)});
}

void handle_vm_cmdCommonGuessList(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_common_guess_list", {make_int(cl.clientNo)});
}

void handle_vm_cmdCommonGuessStartGuess(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    uint8_t gl = body[off++];
    if (gl > len - off) return;
    std::string guess((const char*)body + off, gl); off += gl;
    if (off >= len) return;
    uint8_t rl = body[off++];
    if (rl > len - off) return;
    std::string result((const char*)body + off, rl); off += rl;
    applyLpc("cmd_common_guess_start_guess", {make_int(cl.clientNo), make_string(guess), make_string(result)});
}

void handle_vm_cmdConfirmTakeProperty(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t propId = rd32be(body);
    applyLpc("cmd_confirm_take_property", {make_int(cl.clientNo), make_int(propId)});
}

void handle_vm_cmdControlPartyChannel(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    uint8_t gl = body[off++];
    if (gl > len - off) return;
    std::string gid((const char*)body + off, gl); off += gl;
    if (off >= len) return;
    uint8_t nl = body[off++];
    if (nl > len - off) return;
    std::string name((const char*)body + off, nl); off += nl;
    if (len - off < 3) return;
    uint8_t ch = body[off];
    uint16_t chnl = rd16be(body + off + 1);
    applyLpc("cmd_control_party_channel", {make_int(cl.clientNo), make_string(gid), make_string(name), make_int(ch), make_int(chnl)});
}

void handle_vm_cmdCreateChatGroup(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    uint8_t nl = body[off++];
    if (nl > len - off) return;
    std::string name((const char*)body + off, nl); off += nl;
    if (off >= len) return;
    uint8_t dl = body[off++];
    if (dl > len - off) return;
    std::string declare((const char*)body + off, dl); off += dl;
    applyLpc("cmd_create_chat_group", {make_int(cl.clientNo), make_string(name), make_string(declare)});
}

void handle_vm_cmdCreateNewChar(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    uint8_t nl = body[off++];
    if (nl > len - off) return;
    std::string name((const char*)body + off, nl); off += nl;
    if (len - off < 3) return;
    uint8_t b1 = body[off++];
    uint8_t b2 = body[off++];
    uint8_t b3 = body[off++];
    applyLpc("cmd_create_new_char", {make_int(cl.clientNo), make_string(name), make_int(b1), make_int(b2), make_int(b3)});
}

void handle_vm_cmdCreateNewRaid(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    uint8_t b0 = body[0];
    uint16_t w1 = rd16be(body + 1);
    uint16_t w2 = rd16be(body + 3);
    applyLpc("cmd_creat_new_raid", {make_int(cl.clientNo), make_int(b0), make_int(w1), make_int(w2)});
}

void handle_vm_cmdCscAddSignFee(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_csc_add_sign_fee", {make_int(cl.clientNo)});
}

void handle_vm_cmdCscViewCompeteSort(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nl = body[0];
    if (nl > len - 1) return;
    std::string csc_name((const char*)body + 1, nl);
    applyLpc("cmd_csc_view_compete_sort", {make_int(cl.clientNo), make_string(csc_name)});
}

void handle_vm_cmdCSelectMenuItem(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    uint8_t il = body[off++];
    if (il > len - off) return;
    std::string item((const char*)body + off, il); off += il;
    if (off >= len) return;
    uint8_t pl = body[off++];
    if (pl > len - off) return;
    std::string para((const char*)body + off, pl); off += pl;
    applyLpc("cmd_c_select_menu_item", {make_int(cl.clientNo), make_string(item), make_string(para)});
}

void handle_vm_cmdCSetPos(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_c_set_pos", {make_int(cl.clientNo), make_buffer(body, len)});
}

void handle_vm_cmdCsExchange(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint16_t itemId = rd16be(body);
    uint16_t count = rd16be(body + 2);
    applyLpc("cmd_cs_exchange", {make_int(cl.clientNo), make_int(itemId), make_int(count)});
}

void handle_vm_cmdCUseStunChildbook(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_c_use_stun_child_book", {make_int(cl.clientNo)});
}

void handle_vm_cmdDeleteExistedChar(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t off = 0;
    uint8_t nl = body[off++];
    if (off + nl > len) return;
    std::string name((const char*)body + off, nl);
    off += nl;
    if (off >= len) return;
    uint8_t na = body[off++];
    if (off + na > len) return;
    std::string accountId((const char*)body + off, na);
    applyLpc("cmd_delete_existed_char", {make_int(cl.clientNo), make_string(name), make_string(accountId)});
}

void handle_vm_cmdDeleteMusic(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t musicId = rd32be(body);
    applyLpc("cmd_delete_music", {make_int(cl.clientNo), make_int(musicId)});
}

void handle_vm_cmdDeposit(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 8) return;
    uint32_t itemId = rd32be(body);
    uint32_t count = rd32be(body + 4);
    applyLpc("cmd_deposit", {make_int(cl.clientNo), make_int(itemId), make_int(count)});
}

void handle_vm_cmdDevelopSkill(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 8) return;
    uint16_t v1 = rd16be(body);
    uint32_t v2 = rd32be(body + 2);
    uint16_t v3 = rd16be(body + 6);
    applyLpc("cmd_develop_skill", {make_int(cl.clientNo), make_int(v1), make_int(v2), make_int(v3)});
}

void handle_vm_cmdDigPartyFragment(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t off = 0;
    uint8_t nl = body[off++];
    if (off + nl > len) return;
    std::string partyId((const char*)body + off, nl);
    applyLpc("cmd_dig_party_fragment", {make_int(cl.clientNo), make_string(partyId)});
}

void handle_vm_cmdDismissChatGroup(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t off = 0;
    uint8_t nl = body[off++];
    if (off + nl > len) return;
    std::string group((const char*)body + off, nl);
    applyLpc("cmd_dismiss_chat_group", {make_int(cl.clientNo), make_string(group)});
}

void handle_vm_cmdDistributeRecruitment(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t off = 0;
    uint8_t nl = body[off++];
    if (off + nl > len) return;
    std::string type((const char*)body + off, nl);
    off += nl;
    if (off + 2 > len) return;
    uint16_t minLevel = rd16be(body + off);
    off += 2;
    if (off + 2 > len) return;
    uint16_t maxLevel = rd16be(body + off);
    off += 2;
    if (off >= len) return;
    uint8_t nt = body[off++];
    if (off + nt > len) return;
    std::string tip((const char*)body + off, nt);
    applyLpc("cmd_distribute_recruitment", {make_int(cl.clientNo), make_string(type), make_int(minLevel), make_int(maxLevel), make_string(tip)});
}

void handle_vm_cmdAdminFetchPics(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_admin_fetch_pics", {make_int(cl.clientNo)});
}

void handle_vm_cmdAdminKickoff(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nl = body[0];
    if (1 + nl > len) return;
    std::string name((const char*)body + 1, nl);
    applyLpc("cmd_admin_kickoff", {make_int(cl.clientNo), make_string(name)});
}

void handle_vm_cmdAdminOperSysBroadcast(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint16_t oper = rd16be(body);
    uint16_t index = rd16be(body + 2);
    applyLpc("cmd_admin_oper_sys_broadcast", {make_int(cl.clientNo), make_int(oper), make_int(index)});
}

void handle_vm_cmdAdminPrintScreen(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nl = body[0];
    if (1 + nl > len) return;
    std::string name((const char*)body + 1, nl);
    applyLpc("cmd_admin_print_screen", {make_int(cl.clientNo), make_string(name)});
}

void handle_vm_cmdAdminQueryMap(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nl = body[0];
    if (1 + nl > len) return;
    std::string mapName((const char*)body + 1, nl);
    applyLpc("cmd_admin_query_map", {make_int(cl.clientNo), make_string(mapName)});
}

void handle_vm_cmdAdminQueryNetInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t type = body[0];
    uint8_t nl = body[1];
    if (2 + nl > len) return;
    std::string userName((const char*)body + 2, nl);
    applyLpc("cmd_admin_query_net_info", {make_int(cl.clientNo), make_int(type), make_string(userName)});
}

void handle_vm_cmdAdminQueryNpc(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 3) return;
    uint8_t nl = body[0];
    if (1 + nl + 2 > len) return;
    std::string name((const char*)body + 1, nl);
    uint16_t id = rd16be(body + 1 + nl);
    applyLpc("cmd_admin_query_npc", {make_int(cl.clientNo), make_string(name), make_int(id)});
}

void handle_vm_cmdAdminQueryPlayer(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nl = body[0];
    if (1 + nl > len) return;
    std::string statement((const char*)body + 1, nl);
    applyLpc("cmd_admin_query_player", {make_int(cl.clientNo), make_string(statement)});
}

void handle_vm_cmdAdminQuerySkill(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nl = body[0];
    if (1 + nl > len) return;
    std::string name((const char*)body + 1, nl);
    applyLpc("cmd_admin_query_skill", {make_int(cl.clientNo), make_string(name)});
}

void handle_vm_cmdAdminReboot(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t delay = rd16be(body);
    applyLpc("cmd_admin_reboot", {make_int(cl.clientNo), make_int(delay)});
}

void handle_vm_cmdAdminRequestMapList(GameClient& cl, const uint8_t* body, size_t len) {
    (void)body; (void)len;
    applyLpc("cmd_admin_request_map_list", {make_int(cl.clientNo)});
}

void handle_vm_cmdAdminSetAttrib(GameClient& cl, const uint8_t* body, size_t len) {
    size_t off = 0;
    if (len - off < 1) return;
    uint8_t n1 = body[off++];
    if (len - off < n1) return;
    std::string mks((const char*)body + off, n1); off += n1;
    if (len - off < 2) return;
    uint16_t mks_type = rd16be(body + off); off += 2;
    if (len - off < 1) return;
    uint8_t n2 = body[off++];
    if (len - off < n2) return;
    std::string attrib((const char*)body + off, n2); off += n2;
    if (len - off < 1) return;
    uint8_t n3 = body[off++];
    if (len - off < n3) return;
    std::string value((const char*)body + off, n3); off += n3;
    applyLpc("cmd_admin_set_attrib", {make_int(cl.clientNo), make_string(mks), make_int(mks_type), make_string(attrib), make_string(value)});
}

void handle_vm_cmdAdminSetSkill(GameClient& cl, const uint8_t* body, size_t len) {
    size_t off = 0;
    if (len - off < 1) return;
    uint8_t n1 = body[off++];
    if (len - off < n1) return;
    std::string name((const char*)body + off, n1); off += n1;
    if (len - off < 2) return;
    uint16_t v12 = rd16be(body + off); off += 2;
    if (len - off < 2) return;
    uint16_t v15 = rd16be(body + off); off += 2;
    if (len - off < 2) return;
    uint16_t v16 = rd16be(body + off); off += 2;
    applyLpc("cmd_admin_set_skill", {make_int(cl.clientNo), make_string(name), make_int(v12), make_int(v15), make_int(v16)});
}

void handle_vm_cmdAdminShutChannel(GameClient& cl, const uint8_t* body, size_t len) {
    size_t off = 0;
    if (len - off < 1) return;
    uint8_t n1 = body[off++];
    if (len - off < n1) return;
    std::string name((const char*)body + off, n1); off += n1;
    if (len - off < 2) return;
    uint16_t v12 = rd16be(body + off); off += 2;
    if (len - off < 4) return;
    uint32_t v16 = rd32be(body + off); off += 4;
    if (len - off < 2) return;
    uint16_t v17 = rd16be(body + off); off += 2;
    applyLpc("cmd_admin_shut_channel", {make_int(cl.clientNo), make_string(name), make_int(v12), make_int(v16), make_int(v17)});
}

void handle_vm_cmdAdminSniffAt(GameClient& cl, const uint8_t* body, size_t len) {
    size_t off = 0;
    if (len - off < 1) return;
    uint8_t n1 = body[off++];
    if (len - off < n1) return;
    std::string name((const char*)body + off, n1); off += n1;
    applyLpc("cmd_admin_sniff_at", {make_int(cl.clientNo), make_string(name)});
}

void handle_vm_cmdAdminSrmouseClick(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 4) return;
    uint32_t v7 = rd32be(body);
    applyLpc("cmd_admin_srmouse_click", {make_int(cl.clientNo), make_int((int)v7)});
}

void handle_vm_cmdAdminSummon(GameClient& cl, const uint8_t* body, size_t len) {
    size_t off = 0;
    if (len - off < 1) return;
    uint8_t n1 = body[off++];
    if (len - off < n1) return;
    std::string name((const char*)body + off, n1); off += n1;
    applyLpc("cmd_admin_summon", {make_int(cl.clientNo), make_string(name)});
}

void handle_vm_cmdAdminTeleport(GameClient& cl, const uint8_t* body, size_t len) {
    size_t off = 0;
    if (len - off < 1) return;
    uint8_t n1 = body[off++];
    if (len - off < n1) return;
    std::string mapName((const char*)body + off, n1); off += n1;
    applyLpc("cmd_admin_teleport", {make_int(cl.clientNo), make_string(mapName)});
}

void handle_vm_cmdAdminThrowInJail(GameClient& cl, const uint8_t* body, size_t len) {
    size_t off = 0;
    if (len - off < 1) return;
    uint8_t n1 = body[off++];
    if (len - off < n1) return;
    std::string name((const char*)body + off, n1); off += n1;
    if (len - off < 1) return;
    uint8_t n2 = body[off++];
    if (len - off < n2) return;
    std::string ti((const char*)body + off, n2); off += n2;
    if (len - off < 1) return;
    uint8_t n3 = body[off++];
    if (len - off < n3) return;
    std::string reason((const char*)body + off, n3); off += n3;
    applyLpc("cmd_admin_throw_in_jail", {make_int(cl.clientNo), make_string(name), make_string(ti), make_string(reason)});
}

void handle_vm_cmdAdminThrowInJailEx(GameClient& cl, const uint8_t* body, size_t len) {
    size_t off = 0;
    if (len - off < 1) return;
    uint8_t n1 = body[off++];
    if (len - off < n1) return;
    std::string name((const char*)body + off, n1); off += n1;
    if (len - off < 1) return;
    uint8_t n2 = body[off++];
    if (len - off < n2) return;
    std::string ti((const char*)body + off, n2); off += n2;
    if (len - off < 1) return;
    uint8_t n3 = body[off++];
    if (len - off < n3) return;
    std::string reason((const char*)body + off, n3); off += n3;
    applyLpc("cmd_admin_throw_in_jail_ex", {make_int(cl.clientNo), make_string(name), make_string(ti), make_string(reason)});
}

void handle_vm_cmdAdminTimeBroadcast(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (len - off < 1) return;
    uint8_t n1 = body[off++];
    if (len - off < n1) return;
    std::string caption((const char*)body + off, n1); off += n1;
    if (len - off < 4) return;
    uint32_t interval = rd32be(body + off); off += 4;
    if (len - off < 4) return;
    uint32_t times = rd32be(body + off); off += 4;
    if (len - off < 2) return;
    uint16_t n2 = rd16be(body + off); off += 2;
    if (n2 > 4095) n2 = 4095;
    if (len - off < n2) return;
    std::string content((const char*)body + off, n2); off += n2;
    if (len - off < 1) return;
    uint8_t n3 = body[off++];
    if (len - off < n3) return;
    std::string area((const char*)body + off, n3); off += n3;
    if (len - off < 1) return;
    uint8_t last = body[off];
    applyLpc("cmd_admin_time_broadcast", {make_int(cl.clientNo), make_string(caption), make_int((int)interval), make_int((int)times), make_string(content), make_string(area), make_int(last)});
}

void handle_vm_cmdAdminTransport(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (len - off < 1) return;
    uint8_t n1 = body[off++];
    if (len - off < n1) return;
    std::string name((const char*)body + off, n1); off += n1;
    if (len - off < 2) return;
    uint16_t transport = rd16be(body + off); off += 2;
    applyLpc("cmd_admin_transport", {make_int(cl.clientNo), make_string(name), make_int(transport)});
}

void handle_vm_cmdAnswerResult(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (len - off < 1) return;
    uint8_t n1 = body[off++];
    if (len - off < n1) return;
    std::string answer((const char*)body + off, n1); off += n1;
    if (len - off < 1) return;
    uint8_t n2 = body[off++];
    if (len - off < n2) return;
    std::string question((const char*)body + off, n2); off += n2;
    if (len - off < 1) return;
    uint8_t result = body[off];
    applyLpc("cmd_answer_result", {make_int(cl.clientNo), make_string(answer), make_string(question), make_int(result)});
}

void handle_vm_cmdAnswerSurvey(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (len - off < 4) return;
    uint32_t npcId = rd32be(body + off); off += 4;
    if (len - off < 1) return;
    uint8_t n1 = body[off++];
    if (len - off < n1) return;
    std::string menu_item((const char*)body + off, n1); off += n1;
    if (len - off < 1) return;
    uint8_t count = body[off++];
    auto arr = std::make_shared<ArrayValue>();
    for (uint8_t i = 0; i < count; ++i)
    {
        if (len - off < 1) return;
        uint8_t n = body[off++];
        if (len - off < n) return;
        arr->items.push_back(make_string(std::string((const char*)body + off, n)));
        off += n;
    }
    applyLpc("cmd_answer_survey", {make_int(cl.clientNo), make_int((int)npcId), make_string(menu_item), arr});
}

void handle_vm_cmdAntiHackCollect(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (len - off < 1) return;
    uint8_t n1 = body[off++];
    if (len - off < n1) return;
    std::string ainfo((const char*)body + off, n1);
    applyLpc("cmd_anti_hack_collect", {make_int(cl.clientNo), make_string(ainfo)});
}

void handle_vm_cmdApex(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t n = rd16be(body);
    if (2 + n > len) return;
    applyLpc("cmd_apex", {make_int(cl.clientNo), make_buffer(body + 2, n)});
}

void handle_vm_cmdApply(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_apply", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdApplyCard(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (len - off < 1) return;
    uint8_t pos = body[off++];
    if (len - off < 1) return;
    uint8_t n1 = body[off++];
    if (len - off < n1) return;
    std::string card_name((const char*)body + off, n1); off += n1;
    if (len - off < 1) return;
    uint8_t last = body[off];
    applyLpc("cmd_apply_card", {make_int(cl.clientNo), make_string(card_name), make_int(pos), make_int(last)});
}

void handle_vm_cmdApplyEx(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_apply_ex", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdApplyExquisite(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (len - off < 1) return;
    uint8_t n1 = body[off++];
    if (len - off < n1) return;
    std::string exquisite_name((const char*)body + off, n1); off += n1;
    if (len - off < 2) return;
    uint16_t v12 = rd16be(body + off); off += 2;
    if (len - off < 1) return;
    uint8_t last = body[off];
    applyLpc("cmd_apply_exquisite", {make_int(cl.clientNo), make_string(exquisite_name), make_int(v12), make_int(last)});
}

void handle_vm_cmdApplyOfflineStall(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 1) return;
    uint8_t v0 = body[0];
    applyLpc("cmd_apply_offline_stall", {make_int(cl.clientNo), make_int(v0)});
}

void handle_vm_cmdApplyOnFriend(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 5) return;
    uint8_t v0 = body[0];
    uint32_t v1 = rd32be(body + 1);
    applyLpc("cmd_apply_on_friend", {make_int(cl.clientNo), make_int(v0), make_int(v1)});
}

void handle_vm_cmdApplyOnItem(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 2) return;
    uint8_t v0 = body[0];
    uint8_t v1 = body[1];
    applyLpc("cmd_apply_on_item", {make_int(cl.clientNo), make_int(v0), make_int(v1)});
}

void handle_vm_cmdApplyOther(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 5) return;
    uint8_t v0 = body[0];
    uint32_t v1 = rd32be(body + 1);
    applyLpc("cmd_apply_other", {make_int(cl.clientNo), make_int(v0), make_int(v1)});
}

void handle_vm_cmdApplyQuestionItem(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 1) return;
    uint8_t v0 = body[0];
    applyLpc("cmd_apply_question_item", {make_int(cl.clientNo), make_int(v0)});
}

void handle_vm_cmdArenaEnter(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 1) return;
    uint8_t v0 = body[0];
    applyLpc("cmd_arena_enter", {make_int(cl.clientNo), make_int(v0)});
}

void handle_vm_cmdArenaGoodBuy(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 1) return;
    size_t off = 1;
    uint8_t slen = body[0];
    if (off + slen > len) return;
    std::string itemName(reinterpret_cast<const char*>(body + off), slen);
    off += slen;
    if (off + 2 > len) return;
    uint16_t itemId = rd16be(body + off);
    applyLpc("cmd_arena_goods_buy", {make_int(cl.clientNo), make_string(itemName), make_int(itemId)});
}

void handle_vm_cmdArenaGoodlist(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 1) return;
    size_t off = 1;
    uint8_t slen = body[0];
    if (off + slen > len) return;
    std::string clientKey(reinterpret_cast<const char*>(body + off), slen);
    off += slen;
    if (off + 1 > len) return;
    uint8_t v0 = body[off];
    applyLpc("cmd_arena_goods_list", {make_int(cl.clientNo), make_string(clientKey), make_int(v0)});
}

void handle_vm_cmdArenaGoodsSell(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 4) return;
    uint16_t goodsId = rd16be(body);
    uint16_t amount = rd16be(body + 2);
    applyLpc("cmd_arena_goods_sell", {make_int(cl.clientNo), make_int(goodsId), make_int(amount)});
}

void handle_vm_cmdArenaHallExit(GameClient& cl, const uint8_t* body, size_t len) {
    applyLpc("cmd_arena_hall_exit", {make_int(cl.clientNo)});
}

void handle_vm_cmdArenaHallFight(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 1) return;
    uint8_t slen = body[0];
    if (len < 1u + slen) return;
    std::string name(reinterpret_cast<const char*>(body + 1), slen);
    applyLpc("cmd_arena_hall_fight", {make_int(cl.clientNo), make_string(name)});
}

void handle_vm_cmdArenaHallJoinRoom(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 3) return;
    uint16_t roomNo = rd16be(body);
    uint8_t v1 = body[2];
    applyLpc("cmd_arena_hall_join_room", {make_int(cl.clientNo), make_int(roomNo), make_int(v1)});
}

void handle_vm_cmdArenaHallRequestCI(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 2) return;
    uint8_t type = body[0];
    uint8_t slen = body[1];
    if (len < 2u + slen) return;
    std::string name(reinterpret_cast<const char*>(body + 2), slen);
    applyLpc("cmd_arena_hall_request_ci", {make_int(cl.clientNo), make_int(type), make_string(name)});
}

void handle_vm_cmdArenaHallRequestPL(GameClient& cl, const uint8_t* body, size_t len) {
    (void)body; (void)len;
    applyLpc("cmd_arena_hall_request_pl", {make_int(cl.clientNo)});
}

void handle_vm_cmdArenaHallRequestRD(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 2) return;
    uint16_t v0 = rd16be(body);
    applyLpc("cmd_arena_hall_request_rd", {make_int(cl.clientNo), make_int(v0)});
}

void handle_vm_cmdArenaHallRequestRL(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 5) return;
    uint8_t status = body[0];
    uint8_t type = body[1];
    uint8_t listType = body[2];
    uint16_t startPos = rd16be(body + 3);
    applyLpc("cmd_arena_hall_request_rl", {make_int(cl.clientNo), make_int(status), make_int(type), make_int(listType), make_int(startPos)});
}

void handle_vm_cmdArenaLookOn(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 6) return;
    uint16_t roomId = rd16be(body);
    uint32_t cookie = rd32be(body + 2);
    applyLpc("cmd_arena_look_on", {make_int(cl.clientNo), make_int(roomId), make_int(cookie)});
}

void handle_vm_cmdArenaMatchFixtures(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 2) return;
    uint16_t v0 = rd16be(body);
    applyLpc("cmd_arena_match_fixtures", {make_int(cl.clientNo), make_int(v0)});
}

void handle_vm_cmdArenaMatchJoinRoom(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 4) return;
    uint16_t matchNo = rd16be(body);
    uint16_t roomId = rd16be(body + 2);
    applyLpc("cmd_arena_match_join_room", {make_int(cl.clientNo), make_int(matchNo), make_int(roomId)});
}

void handle_vm_cmdArenaMatchList(GameClient& cl, const uint8_t* body, size_t len) {
    (void)body; (void)len;
    applyLpc("cmd_arena_match_list", {make_int(cl.clientNo)});
}

void handle_vm_cmdArenaMatchRoomDetail(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint16_t roomId = rd16be(body);
    uint16_t detailType = rd16be(body + 2);
    applyLpc("cmd_arena_match_room_detail", {make_int(cl.clientNo), make_int(roomId), make_int(detailType)});
}

void handle_vm_cmdArenaMatchRooms(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t page = rd16be(body);
    applyLpc("cmd_arena_match_rooms", {make_int(cl.clientNo), make_int(page)});
}

void handle_vm_cmdArenaOpenRank(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_arena_open_rank", {make_int(cl.clientNo)});
}

void handle_vm_cmdArenaQueryRankByName(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t off = 0;
    uint8_t nl = body[off++];
    if (off + nl > len) return;
    std::string name((const char*)body + off, nl);
    applyLpc("cmd_arena_query_rank_by_name", {make_int(cl.clientNo), make_string(name)});
}

void handle_vm_cmdArenaRankList(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 7) return;
    uint8_t rankType = body[0];
    uint16_t page = rd16be(body + 1);
    uint32_t value = rd32be(body + 3);
    applyLpc("cmd_arena_rank_list", {make_int(cl.clientNo), make_int(rankType), make_int(page), make_int(value)});
}

void handle_vm_cmdArenaRoom(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    uint16_t roomId = 0;
    std::string title, passwd;
    uint8_t roomType = 0, refuseLook = 0, autoJoin = 0, isLocked = 0;
    if (len >= 2) { roomId = rd16be(body); off = 2; }
    if (off < len) { uint8_t tl = body[off++]; if (off + tl > len) return; title.assign((const char*)body + off, tl); off += tl; }
    if (off < len) roomType = body[off++];
    if (off < len) refuseLook = body[off++];
    if (off < len) autoJoin = body[off++];
    if (off < len) isLocked = body[off++];
    if (off < len) { uint8_t pl = body[off++]; if (off + pl > len) return; passwd.assign((const char*)body + off, pl); }
    applyLpc("cmd_arena_room", {make_int(cl.clientNo), make_int(roomId), make_string(title), make_int(roomType), make_int(refuseLook), make_int(autoJoin), make_int(isLocked), make_string(passwd)});
}

void handle_vm_cmdArenaRoomAction(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint16_t roomId = rd16be(body);
    uint8_t action = body[2];
    uint8_t param = body[3];
    applyLpc("cmd_arena_room_action", {make_int(cl.clientNo), make_int(roomId), make_int(action), make_int(param)});
}

void handle_vm_cmdArenaRoomKickOut(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t off = 0;
    uint8_t gl = body[off++];
    if (off + gl > len) return;
    std::string gid((const char*)body + off, gl);
    off += gl;
    if (off + 3 > len) return;
    uint16_t kickType = rd16be(body + off);
    uint8_t arg = body[off + 2];
    applyLpc("cmd_arena_room_kick_out", {make_int(cl.clientNo), make_string(gid), make_int(kickType), make_int(arg)});
}

void handle_vm_cmdArenaServerList(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_arena_server_list", {make_int(cl.clientNo)});
}

void handle_vm_cmdArenaSwitchServer(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t off = 0;
    uint8_t sl = body[off++];
    if (off + sl > len) return;
    std::string server((const char*)body + off, sl);
    applyLpc("cmd_arena_switch_server", {make_int(cl.clientNo), make_string(server)});
}

void handle_vm_cmdAskFields(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_ask_fields", {make_int(cl.clientNo)});
}


void handle_vm_cmdAskPill(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 14) return;
    uint16_t pos1 = rd16be(body + 0);
    uint16_t pos2 = rd16be(body + 2);
    uint16_t pos3 = rd16be(body + 4);
    uint16_t v4 = rd16be(body + 6);
    uint16_t v5 = rd16be(body + 8);
    uint16_t v6 = rd16be(body + 10);
    uint16_t v7 = rd16be(body + 12);
    applyLpc("cmd_ask_pill", {make_int(cl.clientNo), make_int(pos1), make_int(pos2), make_int(pos3), make_int(v4), make_int(v5), make_int(v6), make_int(v7)});
}


void handle_vm_cmdAskSkillDesc(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t skillId = rd16be(body);
    applyLpc("cmd_ask_skill_desc", {make_int(cl.clientNo), make_int(skillId)});
}


void handle_vm_cmdAsktaoSound(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_asktao_sound", {make_int(cl.clientNo)});
}


void handle_vm_cmdAssignAttrib(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    uint32_t attribId = rd32be(body);
    size_t off = 4;
    uint8_t n = body[off++];
    if (off + n > len) return;
    std::string name(reinterpret_cast<const char*>(body + off), n);
    applyLpc("cmd_assign_attrib", {make_int(cl.clientNo), make_int(attribId), make_string(name)});
}


void handle_vm_cmdAssignChildAttrib(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 20) return;
    applyLpc("cmd_assign_child_attrib", {make_int(cl.clientNo), make_int(rd32be(body + 0)), make_int(rd32be(body + 4)), make_int(rd32be(body + 8)), make_int(rd32be(body + 12)), make_int(rd32be(body + 16))});
}


void handle_vm_cmdAssignPolar(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 6) return;
    uint32_t id = rd32be(body);
    size_t off = 4;
    uint8_t n = body[off++];
    if (off + n + 1 > len) return;
    std::string name(reinterpret_cast<const char*>(body + off), n);
    off += n;
    uint8_t val = body[off];
    applyLpc("cmd_assign_polar", {make_int(cl.clientNo), make_int(id), make_string(name), make_int(val)});
}


void handle_vm_cmdAssignResist(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 6) return;
    uint32_t id = rd32be(body);
    size_t off = 4;
    uint8_t n = body[off++];
    if (off + n + 1 > len) return;
    std::string name(reinterpret_cast<const char*>(body + off), n);
    off += n;
    uint8_t val = body[off];
    applyLpc("cmd_assign_resist", {make_int(cl.clientNo), make_int(id), make_string(name), make_int(val)});
}


void handle_vm_cmdAssignUpgrade(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n = body[0];
    if (1 + n > len) return;
    std::string name(reinterpret_cast<const char*>(body + 1), n);
    applyLpc("cmd_assign_upgrade", {make_int(cl.clientNo), make_string(name)});
}


void handle_vm_cmdAuthMatrixForGs(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t n = rd16be(body);
    if (2 + n > len) return;
    std::string name(reinterpret_cast<const char*>(body + 2), n);
    applyLpc("cmd_auth_matrix_for_gs", {make_int(cl.clientNo), make_string(name)});
}


void handle_vm_cmdAuthRefreshPicForGs(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_auth_refresh_pic_for_gs", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdAuthVerifyCode(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t sl = body[0];
    if (len < 1u + sl) return;
    std::string s((const char*)body + 1, sl);
    applyLpc("cmd_auth_verify_code", {make_int(cl.clientNo), make_string(s)});
}

void handle_vm_cmdAuthVipSms(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t sl = body[0];
    if (len < 1u + sl) return;
    std::string s((const char*)body + 1, sl);
    applyLpc("cmd_auth_vip_sms", {make_int(cl.clientNo), make_string(s)});
}

void handle_vm_cmdAutoCombatTime(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_auto_combat_time", {make_int(cl.clientNo)});
}

void handle_vm_cmdBottleDig(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_bottle_dig", {make_int(cl.clientNo)});
}

void handle_vm_cmdBottleOpen(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t sl = body[0];
    if (len < 1u + sl) return;
    std::string s((const char*)body + 1, sl);
    applyLpc("cmd_bottle_open", {make_int(cl.clientNo), make_string(s)});
}

void handle_vm_cmdBottleOpenInterface(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t sl = body[0];
    if (len < 1u + sl) return;
    std::string s((const char*)body + 1, sl);
    applyLpc("cmd_bottle_open_interface", {make_int(cl.clientNo), make_string(s)});
}

void handle_vm_cmdBottleReply(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t l1 = body[0];
    if (len < 1u + l1) return;
    std::string bid((const char*)body + 1, l1);
    size_t off = 1u + l1;
    if (off >= len) return;
    uint8_t l2 = body[off++];
    if (len < off + l2) return;
    std::string comment((const char*)body + off, l2);
    applyLpc("cmd_bottle_reply", {make_int(cl.clientNo), make_string(bid), make_string(comment)});
}

void handle_vm_cmdBottleThrow(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 3) return;
    int type = body[0];
    int v10 = body[1];
    uint8_t sl = body[2];
    if (len < 3u + sl) return;
    std::string comment((const char*)body + 3, sl);
    applyLpc("cmd_bottle_throw", {make_int(cl.clientNo), make_int(type), make_int(v10), make_string(comment)});
}

void handle_vm_cmdBottleThrowBack(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t sl = body[0];
    if (len < 1u + sl) return;
    std::string s((const char*)body + 1, sl);
    applyLpc("cmd_bottle_throw_back", {make_int(cl.clientNo), make_string(s)});
}


// ---- game commands (type=1) generated handlers, chunk 2 ----

void handle_vm_cmdDivide(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 6) return;
    uint8_t prog = body[0];
    uint8_t v1 = body[1];
    uint32_t val = rd32be(body, 2);
    applyLpc("cmd_divide", {make_int(cl.clientNo), make_int(prog), make_int(v1), make_int(val)});
}

void handle_vm_cmdDivideInStore(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 10) return;
    uint32_t prog = rd32be(body);
    uint16_t v1 = rd16be(body + 4);
    uint16_t v2 = rd16be(body + 6);
    uint16_t v3 = rd16be(body + 8);
    applyLpc("cmd_divide_in_store", {make_int(cl.clientNo), make_int(prog), make_int(v1), make_int(v2), make_int(v3)});
}

void handle_vm_cmdDoucdhChallenge(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t off = 0;
    uint8_t nl = body[off++];
    if (off + nl > len) return;
    std::string name((const char*)body + off, nl);
    off += nl;
    if (off + 2 > len) return;
    uint16_t val = rd16be(body + off);
    applyLpc("cmd_doucdh_challenge", {make_int(cl.clientNo), make_string(name), make_int(val)});
}

void handle_vm_cmdDraftModifyMyInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t type = body[0];
    size_t off = 1;
    uint8_t nl = body[off++];
    if (off + nl > len) return;
    std::string para((const char*)body + off, nl);
    applyLpc("cmd_draft_modify_my_info", {make_int(cl.clientNo), make_int(type), make_string(para)});
}

void handle_vm_cmdDraftPresentFlower(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint8_t stage = body[0];
    uint16_t page = rd16be(body + 1);
    size_t off = 3;
    uint8_t nl = body[off++];
    if (off + nl > len) return;
    std::string account((const char*)body + off, nl);
    off += nl;
    if (off >= len) return;
    uint8_t ml = body[off++];
    if (off + ml > len) return;
    std::string para((const char*)body + off, ml);
    applyLpc("cmd_draft_present_flower", {make_int(cl.clientNo), make_int(stage), make_int(page), make_string(account), make_string(para)});
}

void handle_vm_cmdDraftReport(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t stage = body[0];
    size_t off = 1;
    uint8_t nl = body[off++];
    if (off + nl > len) return;
    std::string account((const char*)body + off, nl);
    applyLpc("cmd_draft_report", {make_int(cl.clientNo), make_int(stage), make_string(account)});
}

void handle_vm_cmdDraftRequestMyInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_draft_request_my_info", {make_int(cl.clientNo)});
}

void handle_vm_cmdDraftRequestStageInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint8_t stage = body[0];
    uint16_t pageNo = rd16be(body + 1);
    size_t off = 3;
    uint8_t nl = body[off++];
    if (off + nl > len) return;
    std::string name((const char*)body + off, nl);
    off += nl;
    if (off >= len) return;
    uint8_t tail = body[off];
    applyLpc("cmd_draft_request_stage_info", {make_int(cl.clientNo), make_int(stage), make_int(pageNo), make_string(name), make_int(tail)});
}

void handle_vm_cmdDraftRequestStageRank(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t v = body[0];
    applyLpc("cmd_draft_request_stage_rank", {make_int(cl.clientNo), make_int(v)});
}

void handle_vm_cmdDraftRoll(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t gender = body[0];
    size_t off = 1;
    uint8_t nl = body[off++];
    if (off + nl > len) return;
    std::string declare((const char*)body + off, nl);
    applyLpc("cmd_draft_roll", {make_int(cl.clientNo), make_int(gender), make_string(declare)});
}

void handle_vm_cmdDraftVote(GameClient& cl, const uint8_t* body, size_t len)
{
    int stage = 0;
    int page = 0;
    std::string account;
    if (len >= 1) stage = body[0];
    if (len >= 3) page = rd16be(body, 1);
    if (len < 4) return;
    uint8_t slen = body[3];
    if (4 + slen > len) return;
    account.assign((const char*)body + 4, slen);
    applyLpc("cmd_draft_vote", {make_int(cl.clientNo), make_int(stage), make_int(page), make_string(account)});
}

void handle_vm_cmdDrawCardRankBonus(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t v = rd32be(body);
    applyLpc("cmd_draw_card_rank_bonus", {make_int(cl.clientNo), make_int(v)});
}

void handle_vm_cmdDrop(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    uint8_t v1 = body[0];
    uint32_t v2 = rd32be(body, 1);
    applyLpc("cmd_drop", {make_int(cl.clientNo), make_int(v1), make_int(v2)});
}

void handle_vm_cmdDropChild(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t v = rd32be(body);
    applyLpc("cmd_drop_child", {make_int(cl.clientNo), make_int(v)});
}

void handle_vm_cmdDropPet(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t v = rd32be(body);
    applyLpc("cmd_drop_pet", {make_int(cl.clientNo), make_int(v)});
}

void handle_vm_cmdDropRandomBoxAward(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_drop_random_box_award", {make_int(cl.clientNo)});
}

void handle_vm_cmdDropTask(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    if (1 + slen > len) return;
    std::string name((const char*)body + 1, slen);
    applyLpc("cmd_drop_task", {make_int(cl.clientNo), make_string(name)});
}

void handle_vm_cmdDungeonViewTasks(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_dungeon_view_tasks", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdEmote(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    std::string verb, target, peer;
    auto readStr = [&](std::string& s) -> bool {
        if (off >= len) return false;
        uint8_t slen = body[off++];
        if (off + slen > len) return false;
        s.assign((const char*)body + off, slen);
        off += slen;
        return true;
    };
    if (!readStr(verb) || !readStr(target)) return;
    int channel = 0;
    if (off + 2 <= len) { channel = rd16be(body, off); off += 2; }
    if (!readStr(peer)) return;
    applyLpc("cmd_emote", {make_int(cl.clientNo), make_string(verb), make_string(target), make_int(channel), make_string(peer)});
}

void handle_vm_cmdEndChallenge(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_end_challenge", {make_int(cl.clientNo)});
}

void handle_vm_cmdGuessBonus(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_guess_bonus", {make_int(cl.clientNo)});
}

void handle_vm_cmdGuessCheckLocked(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_guess_check_locked", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdGuessEnd(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_guess_end", {make_int(cl.clientNo)});
}

void handle_vm_cmdGuessOk(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t type = body[0];
    size_t nlen = body[1];
    if (2 + nlen > len) return;
    applyLpc("cmd_guess_start", {make_int(cl.clientNo), make_int(type),
        make_string((const char*)body + 2, nlen)});
}

void handle_vm_cmdHellDealer(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t nlen = body[0];
    if (1 + nlen + 4 > len) return;
    uint32_t hellId = rd32be(body + 1 + nlen);
    applyLpc("cmd_hell_dealer", {make_int(cl.clientNo),
        make_string((const char*)body + 1, nlen), make_int(hellId)});
}

void handle_vm_cmdHsslRequestInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_hssl_request_info", {make_int(cl.clientNo)});
}

void handle_vm_cmdHsslStartChallenge(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_hssl_start_challeng", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdIdentifyEquip(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint16_t bagPos = rd16be(body);
    uint16_t equipPos = rd16be(body + 2);
    applyLpc("cmd_identify_equip", {make_int(cl.clientNo), make_int(bagPos), make_int(equipPos)});
}

void handle_vm_cmdIgnoreConvenant(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t nlen = body[0];
    if (1 + nlen > len) return;
    applyLpc("cmd_ignore_convenant", {make_int(cl.clientNo),
        make_string((const char*)body + 1, nlen)});
}

void handle_vm_cmdInvite(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t nlen = body[0];
    if (1 + nlen > len) return;
    applyLpc("cmd_invite", {make_int(cl.clientNo),
        make_string((const char*)body + 1, nlen)});
}

void handle_vm_cmdInviteChatGroupMember(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t off = 0;
    uint8_t glen = body[off++];
    if (off + glen > len) return;
    std::string group((const char*)body + off, glen);
    off += glen;
    if (off >= len) return;
    uint8_t nlen = body[off++];
    if (off + nlen > len) return;
    std::string name((const char*)body + off, nlen);
    applyLpc("cmd_invite_chat_group_member", {make_int(cl.clientNo), make_string(group), make_string(name)});
}

void handle_vm_cmdJoinChatGroup(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t off = 0;
    uint8_t glen = body[off++];
    if (off + glen > len) return;
    std::string group((const char*)body + off, glen);
    applyLpc("cmd_join_chat_group", {make_int(cl.clientNo), make_string(group)});
}

void handle_vm_cmdJoinSystemGroup(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_join_system_group", {make_int(cl.clientNo)});
}

void handle_vm_cmdKickout(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    size_t off = 0;
    int id = rd32be(body, off);
    off += 4;
    if (off >= len) return;
    uint8_t nlen = body[off++];
    if (off + nlen > len) return;
    std::string name((const char*)body + off, nlen);
    off += nlen;
    if (off >= len) return;
    int extra = body[off];
    applyLpc("cmd_kickout", {make_int(cl.clientNo), make_int(id), make_string(name), make_int(extra)});
}

void handle_vm_cmdKill(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 6) return;
    size_t off = 0;
    int id = rd32be(body, off);
    off += 4;
    int val = rd16be(body, off);
    applyLpc("cmd_kill", {make_int(cl.clientNo), make_int(id), make_int(val)});
}

void handle_vm_cmdKillEx(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t off = 0;
    uint8_t nlen = body[off++];
    if (off + nlen > len) return;
    std::string name((const char*)body + off, nlen);
    applyLpc("cmd_kill_ex", {make_int(cl.clientNo), make_string(name)});
}

void handle_vm_cmdLaunchConvenant(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t off = 0;
    uint8_t glen = body[off++];
    if (off + glen > len) return;
    std::string pgid((const char*)body + off, glen);
    off += glen;
    if (off >= len) return;
    uint8_t tlen = body[off++];
    if (off + tlen > len) return;
    std::string ti_desc((const char*)body + off, tlen);
    applyLpc("cmd_launch_convenant", {make_int(cl.clientNo), make_string(pgid), make_string(ti_desc)});
}

void handle_vm_cmdLcDoAction(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    int action = rd32be(body, 0);
    applyLpc("cmd_lc_do_action", {make_int(cl.clientNo), make_int(action)});
}

void handle_vm_cmdLearnCooking(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    size_t off = 0;
    int taskType = body[off++];
    uint8_t dlen = body[off++];
    if (off + dlen > len) return;
    std::string dish((const char*)body + off, dlen);
    applyLpc("cmd_learn_cooking", {make_int(cl.clientNo), make_int(taskType), make_string(dish)});
}

void handle_vm_cmdLearnSkill(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 10) return;
    int masterId = rd32be(body, 0);
    int studentId = rd32be(body, 4);
    int skillId = rd16be(body, 8);
    applyLpc("cmd_learn_skill", {make_int(cl.clientNo), make_int(masterId), make_int(studentId), make_int(skillId)});
}

void handle_vm_cmdLearnZhenfa(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t zhenfaId = rd16be(body, 0);
    applyLpc("cmd_learn_zhenfa", {make_int(cl.clientNo), make_int(zhenfaId)});
}

void handle_vm_cmdLeaveArenaRoom(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 3) return;
    uint16_t roomId = rd16be(body, 0);
    uint8_t v = body[2];
    applyLpc("cmd_arena_room_leave", {make_int(cl.clientNo), make_int(roomId), make_int(v)});
}

void handle_vm_cmdLeaveMagicTower(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_leave_magictower", {make_int(cl.clientNo)});
}

void handle_vm_cmdLeaveTempTeam(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_leave_temp_team", {make_int(cl.clientNo)});
}

void handle_vm_cmdLGetSafeCtrlContent(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n = body[0];
    if (1 + n > len) return;
    std::string type((const char*)body + 1, n);
    applyLpc("cmd_get_safe_ctrl_content", {make_int(cl.clientNo), make_string(type)});
}

void handle_vm_cmdLgxyChallenge(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n = body[0];
    if (1 + n + 2 > len) return;
    std::string name((const char*)body + 1, n);
    uint16_t v = rd16be(body, 1 + n);
    applyLpc("cmd_lgxy_challenge", {make_int(cl.clientNo), make_string(name), make_int(v)});
}

void handle_vm_cmdLIsConvertBotton(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n = body[0];
    if (1 + n + 1 > len) return;
    std::string type((const char*)body + 1, n);
    uint8_t v = body[1 + n];
    applyLpc("cmd_is_convert_botton", {make_int(cl.clientNo), make_string(type), make_int(v)});
}

void handle_vm_cmdListenMusic(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t index = rd32be(body, 0);
    size_t off = 4;
    if (off + 1 > len) return;
    uint8_t n = body[off]; off += 1;
    if (off + n > len) return;
    std::string name((const char*)body + off, n); off += n;
    if (off + 1 > len) return;
    uint8_t m = body[off]; off += 1;
    if (off + m > len) return;
    std::string link((const char*)body + off, m); off += m;
    if (off + 1 > len) return;
    uint8_t v = body[off];
    applyLpc("cmd_listen_music", {make_int(cl.clientNo), make_int(index), make_string(name), make_string(link), make_int(v)});
}

void handle_vm_cmdLoadExistedChar(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n = body[0];
    if (1 + n > len) return;
    std::string name((const char*)body + 1, n);
    applyLpc("cmd_load_existed_char", {make_int(cl.clientNo), make_string(name)});
}

void handle_vm_cmdLocalEncryptBorrow(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n1 = body[off]; off += 1;
    if (off + n1 > len) return;
    std::string account((const char*)body + off, n1); off += n1;
    if (off + 1 > len) return;
    uint8_t n2 = body[off]; off += 1;
    if (off + n2 > len) return;
    std::string password((const char*)body + off, n2); off += n2;
    if (off + 1 > len) return;
    uint8_t safe_level = body[off]; off += 1;
    if (off + 1 > len) return;
    uint8_t n3 = body[off]; off += 1;
    if (off + n3 > len) return;
    std::string file_name((const char*)body + off, n3); off += n3;
    if (off + 1 > len) return;
    uint8_t n4 = body[off]; off += 1;
    if (off + n4 > len) return;
    std::string auth_key((const char*)body + off, n4);
    applyLpc("cmd_local_encrypt_borrow", {make_int(cl.clientNo), make_string(account), make_string(password), make_int(safe_level), make_string(file_name), make_string(auth_key)});
}

void handle_vm_cmdLocalEncryptLocked(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t accountLen = body[off++];
    if (off + accountLen > len) return;
    std::string account((const char*)body + off, accountLen);
    off += accountLen;
    if (off + 1 > len) return;
    uint8_t authKeyLen = body[off++];
    if (off + authKeyLen > len) return;
    std::string authKey((const char*)body + off, authKeyLen);
    off += authKeyLen;
    applyLpc("cmd_local_encrypt_locked", {make_int(cl.clientNo), make_string(account), make_string(authKey)});
}

void handle_vm_cmdLocalEncryptLockFailed(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_local_encrypt_lock_failed", {make_int(cl.clientNo)});
}

void handle_vm_cmdLocalEncryptRequest(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t accountLen = body[off++];
    if (off + accountLen > len) return;
    std::string account((const char*)body + off, accountLen);
    off += accountLen;
    if (off + 1 > len) return;
    uint8_t passwordLen = body[off++];
    if (off + passwordLen > len) return;
    std::string password((const char*)body + off, passwordLen);
    off += passwordLen;
    if (off + 1 > len) return;
    int encryptType = body[off++];
    applyLpc("cmd_local_encrypt_request", {make_int(cl.clientNo), make_string(account), make_string(password), make_int(encryptType)});
}

void handle_vm_cmdLocalEncryptUnlock(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t accountLen = body[off++];
    if (off + accountLen > len) return;
    std::string account((const char*)body + off, accountLen);
    off += accountLen;
    applyLpc("cmd_local_encrypt_unlock", {make_int(cl.clientNo), make_string(account)});
}

void handle_vm_cmdLockAccount(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_lock_account", {make_int(cl.clientNo)});
}

void handle_vm_cmdLogin(GameClient& cl, const uint8_t* body, size_t len)
{
    if (std::getenv("LPCVM_TAIL"))
        std::cerr << "[cmdLogin] cno=" << cl.clientNo << " bodylen=" << len << " first="
                  << (len ? static_cast<unsigned>(body[0]) : 0) << "\n";
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t accountLen = body[off++];
    if (off + accountLen > len) return;
    std::string account((const char*)body + off, accountLen);
    off += accountLen;
    if (off + 4 > len) return;
    uint32_t authKey = rd32be(body + off);
    off += 4;
    if (off + 4 > len) return;
    uint32_t seed = rd32be(body + off);
    off += 4;
    if (off + 1 > len) return;
    uint8_t versionLen = body[off++];
    if (off + versionLen > len) return;
    std::string version((const char*)body + off, versionLen);
    off += versionLen;
    if (off + 1 > len) return;
    int clientType = body[off++];
    if (off + 1 > len) return;
    uint8_t keyLen = body[off++];
    if (off + keyLen > len) return;
    std::string key((const char*)body + off, keyLen);
    off += keyLen;
    if (off + 1 > len) return;
    uint8_t computerIdLen = body[off++];
    if (off + computerIdLen > len) return;
    std::string computerId((const char*)body + off, computerIdLen);
    off += computerIdLen;
    if (std::getenv("LPCVM_TAIL"))
        std::cerr << "[cmdLogin] cno=" << cl.clientNo << " account=\"" << account
                  << "\" authKey=" << authKey << " seed=" << seed
                  << " version=\"" << version << "\" clientType=" << clientType
                  << " key=\"" << key << "\" compId=\"" << computerId << "\"\n";
    applyLpc("cmd_login", {make_int(cl.clientNo), make_string(account), make_int((int)authKey), make_int((int)seed), make_string(version), make_int(clientType), make_string(key), make_string(computerId)});
}

void handle_vm_cmdLogout(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_logout", {make_int(cl.clientNo)});
}

void handle_vm_cmdLook(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_look", {make_int(cl.clientNo)});
}

void handle_vm_cmdLookOn(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t target = rd32be(body);
    applyLpc("cmd_look_on", {make_int(cl.clientNo), make_int((int)target)});
}

void handle_vm_cmdLookonBroadcastCombat(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t idLen = body[off++];
    if (off + idLen > len) return;
    std::string id((const char*)body + off, idLen);
    off += idLen;
    applyLpc("cmd_lookon_broadcast_combat", {make_int(cl.clientNo), make_string(id)});
}

void handle_vm_cmdLookPlayer(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n = body[0];
    if ((size_t)1 + n > len) return;
    std::string name((const char*)body + 1, n);
    applyLpc("cmd_look_player", {make_int(cl.clientNo), make_string(name)});
}

void handle_vm_cmdLotterySelect(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 7) return;
    auto arr = std::make_shared<ArrayValue>();
    for (int i = 0; i < 7; ++i)
        arr->items.push_back(make_int(body[i]));
    applyLpc("cmd_lottery_select", {make_int(cl.clientNo), arr});
}

void handle_vm_cmdLSetSafeCtrlContent(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n1 = body[0];
    if ((size_t)1 + n1 > len) return;
    std::string type((const char*)body + 1, n1);
    size_t off = 1 + n1;
    if (off + 4 > len) return;
    uint32_t n2 = rd32be(body, off);
    off += 4;
    if (off + n2 > len) return;
    std::string content((const char*)body + off, n2);
    applyLpc("cmd_set_safe_ctrl_content", {make_int(cl.clientNo), make_string(type), make_string(content)});
}

void handle_vm_cmdLuckySign(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_lucky_sign", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdMakeDecoration(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n = body[0];
    if ((size_t)1 + n > len) return;
    std::string decoration((const char*)body + 1, n);
    applyLpc("cmd_make_decoration", {make_int(cl.clientNo), make_string(decoration)});
}

void handle_vm_cmdMakePill(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 28) return;
    int pos1 = rd16be(body);
    auto arr1 = std::make_shared<ArrayValue>();
    auto arr2 = std::make_shared<ArrayValue>();
    size_t off = 2;
    for (int i = 0; i < 6; ++i)
    {
        arr1->items.push_back(make_int(rd16be(body, off))); off += 2;
        arr2->items.push_back(make_int(rd16be(body, off))); off += 2;
    }
    int v13 = rd16be(body, off);
    applyLpc("cmd_make_pill", {make_int(cl.clientNo), make_int(pos1), arr1, arr2, make_int(v13)});
}

void handle_vm_cmdManageCity(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n = body[0];
    if ((size_t)1 + n > len) return;
    std::string city((const char*)body + 1, n);
    applyLpc("cmd_manage_city", {make_int(cl.clientNo), make_string(city)});
}

void handle_vm_cmdManageHouse(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 7) return;
    int npc_id = rd32be(body);
    int type = body[4];
    int oper = body[5];
    uint8_t cnt = body[6];
    if ((size_t)7 + (size_t)cnt * 4 > len) return;
    auto arr = std::make_shared<ArrayValue>();
    size_t off = 7;
    for (int i = 0; i < cnt; ++i)
    {
        arr->items.push_back(make_int(rd32be(body, off)));
        off += 4;
    }
    applyLpc("cmd_manage_house", {make_int(cl.clientNo), make_int(npc_id), make_int(type), make_int(oper), arr});
}

void handle_vm_cmdManageMpDevice(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    int oper = body[0];
    uint8_t n = body[1];
    if ((size_t)2 + n > len) return;
    std::string device((const char*)body + 2, n);
    applyLpc("cmd_manage_mp_device", {make_int(cl.clientNo), make_int(oper), make_string(device)});
}

void handle_vm_cmdMergeMagicPicture(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 9) return;
    int id = rd32be(body);
    int picture_type = body[4];
    int n = body[5];
    int v2 = body[6];
    int v3 = body[7];
    int v4 = body[8];
    applyLpc("cmd_merge_magic_picture", {make_int(cl.clientNo), make_int(id), make_int(picture_type), make_int(n), make_int(v2), make_int(v3), make_int(v4)});
}

void handle_vm_cmdMobileOperTask(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    uint8_t oper_type = 0;
    if (off + 1 <= len) { oper_type = body[off]; off += 1; }
    std::string task;
    if (off + 1 <= len) { uint8_t n = body[off]; off += 1; if (off + (size_t)n <= len) { task.assign((const char*)body + off, n); off += n; } }
    std::string para;
    if (off + 1 <= len) { uint8_t n = body[off]; off += 1; if (off + (size_t)n <= len) { para.assign((const char*)body + off, n); off += n; } }
    applyLpc("cmd_mobile_oper_task", {make_int(cl.clientNo), make_int(oper_type), make_string(task), make_string(para)});
}

void handle_vm_cmdMobileRequestNpcList(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_mobile_request_npc_list", {make_int(cl.clientNo)});
}

void handle_vm_cmdModifyCustomNpcList(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    uint8_t oper_type = 0;
    if (off + 1 <= len) { oper_type = body[off]; off += 1; }
    std::string para;
    if (off + 1 <= len) { uint8_t n = body[off]; off += 1; if (off + (size_t)n <= len) { para.assign((const char*)body + off, n); off += n; } }
    std::string per_name;
    if (off + 1 <= len) { uint8_t n = body[off]; off += 1; if (off + (size_t)n <= len) { per_name.assign((const char*)body + off, n); off += n; } }
    applyLpc("cmd_modify_custom_npc_list", {make_int(cl.clientNo), make_int(oper_type), make_string(para), make_string(per_name)});
}

void handle_vm_cmdModifyItemRemark(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    uint8_t pos = 0;
    if (off + 1 <= len) { pos = body[off]; off += 1; }
    std::string remark;
    if (off + 1 <= len) { uint8_t n = body[off]; off += 1; if (off + (size_t)n <= len) { remark.assign((const char*)body + off, n); off += n; } }
    applyLpc("cmd_modify_item_remark", {make_int(cl.clientNo), make_int(pos), make_string(remark)});
}

void handle_vm_cmdModifySchedule(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    uint16_t year = 0, mon = 0, day = 0, hour = 0, min = 0, pre_time = 0, repeat = 0;
    if (off + 2 <= len) { year = rd16be(body, off); off += 2; }
    if (off + 2 <= len) { mon = rd16be(body, off); off += 2; }
    if (off + 2 <= len) { day = rd16be(body, off); off += 2; }
    if (off + 2 <= len) { hour = rd16be(body, off); off += 2; }
    if (off + 2 <= len) { min = rd16be(body, off); off += 2; }
    if (off + 2 <= len) { pre_time = rd16be(body, off); off += 2; }
    if (off + 2 <= len) { repeat = rd16be(body, off); off += 2; }
    std::string affair;
    if (off + 1 <= len) { uint8_t n = body[off]; off += 1; if (off + (size_t)n <= len) { affair.assign((const char*)body + off, n); off += n; } }
    uint16_t end_time = 0;
    if (off + 2 <= len) { end_time = rd16be(body, off); off += 2; }
    applyLpc("cmd_modify_schedule", {make_int(cl.clientNo), make_int(year), make_int(mon), make_int(day), make_int(hour), make_int(min), make_int(pre_time), make_int(repeat), make_string(affair), make_int(end_time)});
}

void handle_vm_cmdMountInbornStoneList(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t list_type = rd32be(body, off); off += 4;
    applyLpc("cmd_open_inborn_stone_list", {make_int(cl.clientNo), make_int(list_type)});
}

void handle_vm_cmdMoveChildItem(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 7 > len) return;
    uint32_t item_id = rd32be(body, off); off += 4;
    uint8_t pos_from = body[off]; off += 1;
    uint8_t pos_to = body[off]; off += 1;
    uint8_t count = body[off]; off += 1;
    applyLpc("cmd_move_child_item", {make_int(cl.clientNo), make_int(item_id), make_int(pos_from), make_int(pos_to), make_int(count)});
}

void handle_vm_cmdMoveOnCarpet(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t carpet_id = rd32be(body, off); off += 4;
    applyLpc("cmd_move_on_carpet", {make_int(cl.clientNo), make_int(carpet_id)});
}

void handle_vm_cmdMoveTo(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t map_id = rd32be(body, off); off += 4;
    if (off + 4 > len) return;
    uint32_t target_id = rd32be(body, off); off += 4;
    if (off + 2 > len) return;
    uint16_t x = rd16be(body, off); off += 2;
    if (off + 2 > len) return;
    uint16_t y = rd16be(body, off); off += 2;
    if (off + 2 > len) return;
    uint16_t dir = rd16be(body, off); off += 2;
    applyLpc("cmd_move_to", {make_int(cl.clientNo), make_int(map_id), make_int(target_id), make_int(x), make_int(y), make_int(dir)});
}

void handle_vm_cmdMovFromQiankundai(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    std::string qiankundai;
    if (off + 1 <= len) { uint8_t n = body[off]; off += 1; if (off + (size_t)n <= len) { qiankundai.assign((const char*)body + off, n); off += n; } }
    uint16_t p1 = 0, p2 = 0, p3 = 0;
    if (off + 2 <= len) { p1 = rd16be(body, off); off += 2; }
    if (off + 2 <= len) { p2 = rd16be(body, off); off += 2; }
    if (off + 2 <= len) { p3 = rd16be(body, off); off += 2; }
    applyLpc("cmd_mov_from_qiankundai", {make_int(cl.clientNo), make_string(qiankundai), make_int(p1), make_int(p2), make_int(p3)});
}

void handle_vm_cmdMovToQiankundai(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t nameLen = body[off++];
    if (off + nameLen > len) return;
    std::string name(reinterpret_cast<const char*>(body + off), nameLen);
    off += nameLen;
    if (off + 2 > len) return;
    uint16_t x = rd16be(body, off); off += 2;
    if (off + 2 > len) return;
    uint16_t y = rd16be(body, off); off += 2;
    if (off + 2 > len) return;
    uint16_t z = rd16be(body, off); off += 2;
    applyLpc("cmd_mov_to_qiankundai", {make_int(cl.clientNo), make_string(name), make_int(x), make_int(y), make_int(z)});
}

void handle_vm_cmdMulAssignAttrib(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t a = rd32be(body, off); off += 4;
    if (off + 4 > len) return;
    uint32_t b = rd32be(body, off); off += 4;
    if (off + 4 > len) return;
    uint32_t c = rd32be(body, off); off += 4;
    if (off + 4 > len) return;
    uint32_t d = rd32be(body, off); off += 4;
    if (off + 4 > len) return;
    uint32_t e = rd32be(body, off); off += 4;
    applyLpc("cmd_mul_assign_attrib", {make_int(cl.clientNo), make_int(a), make_int(b), make_int(c), make_int(d), make_int(e)});
}

void handle_vm_cmdMulGoodsBuy(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t id = rd32be(body, off); off += 4;
    if (off + 2 > len) return;
    uint16_t count = rd16be(body, off); off += 2;
    if (count == 0 || count > 100 || off + (size_t)count * 6 > len) return;
    auto arr = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < count; ++i)
    {
        arr->items.push_back(make_int(rd16be(body, off))); off += 2;
        arr->items.push_back(make_int(rd16be(body, off))); off += 2;
        arr->items.push_back(make_int(rd16be(body, off))); off += 2;
    }
    applyLpc("cmd_mul_goods_buy", {make_int(cl.clientNo), make_int(id), make_int(count), arr});
}

void handle_vm_cmdMultiMoveTo(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t v = rd32be(body, off); off += 4;
    if (off + 4 > len) return;
    uint32_t mapId = rd32be(body, off); off += 4;
    if (off + 2 > len) return;
    uint16_t count = rd16be(body, off); off += 2;
    if (count == 0 || count > 13 || off + (size_t)count * 4 > len) return;
    auto arr = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < count; ++i)
    {
        arr->items.push_back(make_int(rd16be(body, off))); off += 2;
        arr->items.push_back(make_int(rd16be(body, off))); off += 2;
    }
    if (off + 2 > len) return;
    uint16_t dir = rd16be(body, off); off += 2;
    applyLpc("cmd_multi_move_to", {make_int(cl.clientNo), make_int(v), make_int(mapId), arr, make_int(dir)});
}

void handle_vm_cmdMusicList(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_music_list", {make_int(cl.clientNo)});
}

void handle_vm_cmdNewbieFirstAction(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_newbie_first_action", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdNewbiePlayedDrama(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t sLen = body[off++];
    if (off + sLen > len) return;
    std::string actionList(reinterpret_cast<const char*>(body + off), sLen);
    off += sLen;
    if (off + 1 > len) return;
    uint8_t playedCount = body[off++];
    applyLpc("cmd_newbie_played_drama", {make_int(cl.clientNo), make_string(actionList), make_int(playedCount)});
}

void handle_vm_cmdNextQuestion(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_next_question", {make_int(cl.clientNo)});
}

void handle_vm_cmdNotifySendWeibo(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_notify_send_weibo", {make_int(cl.clientNo)});
}

void handle_vm_cmdOpenAdventrueBox(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t boxId = rd16be(body, 0);
    applyLpc("cmd_open_adventrue_box", {make_int(cl.clientNo), make_int(boxId)});
}

void handle_vm_cmdOpenArenaRoomSeat(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint16_t room_id = rd16be(body, 0);
    uint8_t seat_order = body[2];
    uint8_t seat_status = body[3];
    applyLpc("cmd_arena_room_open_seat", {make_int(cl.clientNo), make_int(room_id), make_int(seat_order), make_int(seat_status)});
}

void handle_vm_cmdOpenAskTaoShowGiftBox(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_open_asktao_show_gift_box", {make_int(cl.clientNo)});
}

void handle_vm_cmdOpenAskTaoShowMall(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n = body[0];
    if ((size_t)n > len - 1) return;
    applyLpc("cmd_open_asktao_show_mall", {make_int(cl.clientNo), make_string((const char*)body + 1, n)});
}

void handle_vm_cmdOpenAuthAllProtect(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 6) return;
    uint32_t seq_no = rd32be(body, 0);
    uint8_t is_confirm = body[4];
    uint8_t n1 = body[5];
    size_t off = 6;
    if (off + n1 > len) return;
    std::string passpod((const char*)body + off, n1);
    off += n1;
    if (off + 2 > len) return;
    uint16_t n2 = rd16be(body, off);
    off += 2;
    if (off + n2 > len) return;
    std::string second((const char*)body + off, n2);
    applyLpc("cmd_open_auth_all_protect", {make_int(cl.clientNo), make_int(seq_no), make_int(is_confirm), make_string(passpod), make_string(second)});
}

void handle_vm_cmdOpenBoxAdventrueInterface(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_open_box_adventrue_interface", {make_int(cl.clientNo)});
}

void handle_vm_cmdOpenBureau(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n = body[0];
    size_t off = 1;
    if (off + n > len) return;
    std::string name((const char*)body + off, n);
    off += n;
    if (off + 1 > len) return;
    uint8_t sync = body[off];
    applyLpc("cmd_open_bureau", {make_int(cl.clientNo), make_string(name), make_int(sync)});
}

void handle_vm_cmdOpenConfirmDlg(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t type = body[0];
    uint8_t n = body[1];
    size_t off = 2;
    if (off + n > len) return;
    std::string para((const char*)body + off, n);
    applyLpc("cmd_open_confirm_dlg", {make_int(cl.clientNo), make_int(type), make_string(para)});
}

void handle_vm_cmdOpenMenu0(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t id = rd32be(body, 0);
    applyLpc("cmd_open_menu", {make_int(0), make_int(cl.clientNo), make_int(id)});
}

void handle_vm_cmdOpenMenu1(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t id = rd32be(body, 0);
    applyLpc("cmd_open_menu", {make_int(1), make_int(cl.clientNo), make_int(id)});
}

void handle_vm_cmdOpenMenu2(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t id = rd32be(body, 0);
    applyLpc("cmd_open_menu", {make_int(2), make_int(cl.clientNo), make_int(id)});
}

void handle_vm_cmdOpenMountInbornSkills(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t skillId = rd32be(body);
    applyLpc("cmd_open_inborn_skill_list", {make_int(cl.clientNo), make_int(skillId)});
}

void handle_vm_cmdOpenMusic(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t musicId = rd32be(body);
    applyLpc("cmd_open_music", {make_int(cl.clientNo), make_int(musicId)});
}

void handle_vm_cmdOpenOnlineMall(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t nameLen = body[off++];
    if (off + nameLen > len) return;
    std::string name((const char*)body + off, nameLen);
    off += nameLen;
    if (off + 1 > len) return;
    uint8_t keyLen = body[off++];
    if (off + keyLen > len) return;
    std::string clientKey((const char*)body + off, keyLen);
    off += keyLen;
    if (off + 1 > len) return;
    uint8_t v = body[off];
    applyLpc("cmd_open_online_mall", {make_int(cl.clientNo), make_string(name), make_string(clientKey), make_int(v)});
}

void handle_vm_cmdOpenSafeSetting(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_open_safe_setting", {make_int(cl.clientNo)});
}

void handle_vm_cmdOperatePetStore(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 7) return;
    uint8_t type = body[0];
    uint16_t pos = rd16be(body + 1);
    uint32_t npcId = rd32be(body + 3);
    applyLpc("cmd_operate_pet_store", {make_int(cl.clientNo), make_int(type), make_int(pos), make_int(npcId)});
}

void handle_vm_cmdOperCasualGame(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t operLen = body[off++];
    if (off + operLen > len) return;
    std::string operType((const char*)body + off, operLen);
    off += operLen;
    if (off + 1 > len) return;
    uint8_t gameType = body[off++];
    if (off + 4 > len) return;
    uint32_t roomId = rd32be(body + off);
    off += 4;
    if (off + 1 > len) return;
    uint8_t paraLen = body[off++];
    if (off + paraLen > len) return;
    std::string para1((const char*)body + off, paraLen);
    applyLpc("cmd_oper_casual_game", {make_int(cl.clientNo), make_string(operType), make_int(gameType), make_int(roomId), make_string(para1)});
}

void handle_vm_cmdOperDiscoverEvent(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t operLen = body[off++];
    if (off + operLen > len) return;
    std::string operType((const char*)body + off, operLen);
    off += operLen;
    if (off + 2 > len) return;
    uint8_t discoverMapId = body[off++];
    uint8_t eventId = body[off++];
    if (off + 1 > len) return;
    uint8_t paraLen = body[off++];
    if (off + paraLen > len) return;
    std::string para1((const char*)body + off, paraLen);
    applyLpc("cmd_oper_discover_event", {make_int(cl.clientNo), make_string(operType), make_int(discoverMapId), make_int(eventId), make_string(para1)});
}

void handle_vm_cmdOperElixir(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    uint8_t type = body[0];
    uint8_t oper = body[1];
    uint16_t num = rd16be(body + 2);
    uint8_t autoExchange = body[4];
    applyLpc("cmd_oper_elixir", {make_int(cl.clientNo), make_int(type), make_int(oper), make_int(num), make_int(autoExchange)});
}

void handle_vm_cmdOperEnvelope(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t oper = body[0];
    uint8_t iidLen = body[1];
    if (2 + iidLen > len) return;
    std::string iid((const char*)body + 2, iidLen);
    applyLpc("cmd_oper_envelope", {make_int(cl.clientNo), make_int(oper), make_string(iid)});
}

void handle_vm_cmdoperlockexp(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    uint8_t flag = body[0];
    uint32_t exp = rd32be(body + 1);
    applyLpc("cmd_oper_lock_exp", {make_int(cl.clientNo), make_int(flag), make_int(exp)});
}

void handle_vm_cmdOperLongtengShengshi(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    int32_t id = rd32be(body, 0);
    uint8_t slen = body[4];
    if (5 + slen > len) return;
    std::string s((const char*)body + 5, slen);
    applyLpc("cmd_oper_longteng_shengshi", {make_int(cl.clientNo), make_int(id), make_string(s)});
}

void handle_vm_cmdOperLotteryTicket(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    applyLpc("cmd_oper_lottery_ticket", {make_int(cl.clientNo), make_int(body[0]), make_int(body[1])});
}

void handle_vm_cmdOperMagicPicture(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 10) return;
    int32_t id = rd32be(body, 0);
    uint8_t b4 = body[4];
    uint8_t b5 = body[5];
    uint16_t b6 = rd16be(body, 6);
    uint8_t b8 = body[8];
    uint8_t b9 = body[9];
    applyLpc("cmd_oper_magic_picture", {make_int(cl.clientNo), make_int(id), make_int(b4), make_int(b5), make_int(b6), make_int(b8), make_int(b9)});
}

void handle_vm_cmdOperMaster(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 7) return;
    int32_t id = rd32be(body, 0);
    uint16_t flag = rd16be(body, 4);
    uint8_t slen = body[6];
    if (7 + slen > len) return;
    std::string signature((const char*)body + 7, slen);
    applyLpc("cmd_oper_master", {make_int(cl.clientNo), make_int(id), make_int(flag), make_string(signature)});
}

void handle_vm_cmdoperpackeditem(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    applyLpc("cmd_oper_packed_item", {make_int(cl.clientNo), make_int(body[0]), make_int(body[1]), make_int(body[2]), make_int(body[3]), make_int(body[4])});
}

void handle_vm_cmdOperSecurity(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 7) return;
    uint16_t v0 = rd16be(body, 0);
    int32_t v2 = rd32be(body, 2);
    uint8_t v6 = body[6];
    applyLpc("cmd_oper_security", {make_int(cl.clientNo), make_int(v0), make_int(v2), make_int(v6)});
}

void handle_vm_cmdopersixanniinfo(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    uint8_t b0 = body[0];
    int32_t b1 = rd32be(body, 1);
    applyLpc("cmd_oper_six_anni_info", {make_int(cl.clientNo), make_int(b0), make_int(b1)});
}

void handle_vm_cmdOperSortTask(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint16_t task_type = rd16be(body, 0);
    uint8_t oper_type = body[2];
    uint8_t slen = body[3];
    if (4 + slen > len) return;
    std::string para((const char*)body + 4, slen);
    applyLpc("cmd_oper_sort_task", {make_int(cl.clientNo), make_int(task_type), make_int(oper_type), make_string(para)});
}

void handle_vm_cmdoperstrategywar(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    uint8_t len1 = body[off++];
    if (off + len1 > len) return;
    std::string oper_type((const char*)body + off, len1);
    off += len1;
    if (off >= len) return;
    uint8_t len2 = body[off++];
    if (off + len2 > len) return;
    std::string para((const char*)body + off, len2);
    applyLpc("cmd_oper_strategy_war", {make_int(cl.clientNo), make_string(oper_type), make_string(para)});
}

void handle_vm_cmdOperTeleportItem(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 9) return;
    int32_t item_id = rd32be(body, 0);
    uint16_t oper = rd16be(body, 4);
    int16_t para1 = (int16_t)rd16be(body, 6);
    uint8_t slen = body[8];
    if (9 + slen > len) return;
    std::string para2((const char*)body + 9, slen);
    applyLpc("cmd_oper_teleport_item", {make_int(cl.clientNo), make_int(item_id), make_int(oper), make_int(para1), make_string(para2)});
}

void handle_vm_cmdEndRaise(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    int id = (int)rd32be(body, 0);
    int type = body[4];
    applyLpc("cmd_end_raise", {make_int(cl.clientNo), make_int(id), make_int(type)});
}

void handle_vm_cmdEndRecruitment(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    if (1 + slen > len) return;
    std::string type((const char*)body + 1, slen);
    applyLpc("cmd_end_recruitment", {make_int(cl.clientNo), make_string(type)});
}

void handle_vm_cmdEnterArenaRoom(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    int room = body[0];
    applyLpc("cmd_enter_arena_room", {make_int(cl.clientNo), make_int(room)});
}

void handle_vm_cmdEnterRoom0(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    if (1 + slen > len) return;
    std::string newRoom((const char*)body + 1, slen);
    applyLpc("cmd_enter_room", {make_int(0), make_int(cl.clientNo), make_string(newRoom)});
}

void handle_vm_cmdEnterRoom1(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    if (1 + slen > len) return;
    std::string newRoom((const char*)body + 1, slen);
    applyLpc("cmd_enter_room", {make_int(1), make_int(cl.clientNo), make_string(newRoom)});
}

void handle_vm_cmdEnterRoom2(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    if (1 + slen > len) return;
    std::string newRoom((const char*)body + 1, slen);
    applyLpc("cmd_enter_room", {make_int(2), make_int(cl.clientNo), make_string(newRoom)});
}

void handle_vm_cmdEquip(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    int pos = body[0];
    int part = body[1];
    applyLpc("cmd_equip", {make_int(cl.clientNo), make_int(pos), make_int(part)});
}

void handle_vm_cmdEquipInbornStone(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 8) return;
    int petId = (int)rd32be(body, 0);
    int fromPos = rd16be(body, 4);
    int toPos = rd16be(body, 6);
    applyLpc("cmd_equip_inborn_stone", {make_int(cl.clientNo), make_int(petId), make_int(fromPos), make_int(toPos)});
}

void handle_vm_cmdEquipPetDress(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 6) return;
    int petId = (int)rd32be(body, 0);
    int pos = rd16be(body, 4);
    applyLpc("cmd_equip_pet_dress", {make_int(cl.clientNo), make_int(petId), make_int(pos)});
}

void handle_vm_cmdExchangeAppraise(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    int report = body[0];
    int star = body[1];
    size_t off = 2;
    if (off + 1 > len) return;
    uint8_t slen = body[off];
    if (off + 1 + slen > len) return;
    std::string content((const char*)body + off + 1, slen);
    off += 1 + slen;
    if (off + 1 > len) return;
    slen = body[off];
    if (off + 1 + slen > len) return;
    std::string phone((const char*)body + off + 1, slen);
    off += 1 + slen;
    if (off + 1 > len) return;
    slen = body[off];
    if (off + 1 + slen > len) return;
    std::string bargainer((const char*)body + off + 1, slen);
    applyLpc("cmd_exchange_appraise", {make_int(cl.clientNo), make_int(report), make_int(star), make_string(content), make_string(phone), make_string(bargainer)});
}

void handle_vm_cmdExchangeCancel(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_exchange_cancel", {make_int(cl.clientNo)});
}

void handle_vm_cmdExchangeChildAppellation(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint32_t no = body[off++];
    if (off + 1 > len) return;
    uint32_t slen = body[off++];
    if (off + slen > len) return;
    std::string appellation(reinterpret_cast<const char*>(body + off), slen);
    off += slen;
    applyLpc("cmd_exchange_child_appellation", {make_int(cl.clientNo), make_int(no), make_string(appellation)});
}

void handle_vm_cmdExchangeOk(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_exchange_ok", {make_int(cl.clientNo)});
}

void handle_vm_cmdExchangeOpen(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t v = rd32be(body, 0);
    applyLpc("cmd_exchange_open", {make_int(cl.clientNo), make_int(v)});
}

void handle_vm_cmdExchangeOpenEx(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint32_t slen = body[off++];
    if (off + slen > len) return;
    std::string name(reinterpret_cast<const char*>(body + off), slen);
    off += slen;
    applyLpc("cmd_exchange_open_ex", {make_int(cl.clientNo), make_string(name)});
}

void handle_vm_cmdExchangeSelect(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint32_t slen = body[off++];
    if (off + slen > len) return;
    std::string type(reinterpret_cast<const char*>(body + off), slen);
    off += slen;
    if (off + 1 > len) return;
    uint32_t v1 = body[off++];
    if (off + 1 > len) return;
    uint32_t v2 = body[off++];
    if (off + 4 > len) return;
    uint32_t v3 = rd32be(body, off);
    off += 4;
    applyLpc("cmd_exchange_select", {make_int(cl.clientNo), make_string(type), make_int(v1), make_int(v2), make_int(v3)});
}

void handle_vm_cmdFeedChild(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    applyLpc("cmd_feed_child", {make_int(cl.clientNo), make_int(body[0]), make_int(body[1])});
}

void handle_vm_cmdFeedPet(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    applyLpc("cmd_feed_pet", {make_int(cl.clientNo), make_int(body[0]), make_int(body[1])});
}

void handle_vm_cmdFeedPetXinfaExp(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    applyLpc("cmd_feed_pet_xinfa_exp", {make_int(cl.clientNo), make_int(body[0]), make_int(body[1])});
}

void handle_vm_cmdFetchGbuyingCoin(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_fetch_gbuying_coin", {make_int(cl.clientNo)});
}

void handle_vm_cmdFetchOtherStore(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    uint32_t storeNo = rd32be(body, 1);
    applyLpc("cmd_fetch_other_store", {make_int(cl.clientNo), make_int(body[0]), make_int(storeNo)});
}

void handle_vm_cmdFight(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nameLen = body[0];
    if (1 + nameLen > len) return;
    std::string name((const char*)body + 1, nameLen);
    applyLpc("cmd_fight", {make_int(cl.clientNo), make_string(name)});
}

void handle_vm_cmdFinger(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nameLen = body[0];
    if (2 + nameLen > len) return;
    std::string name((const char*)body + 1, nameLen);
    uint8_t arg1 = body[1 + nameLen];
    applyLpc("cmd_finger", {make_int(cl.clientNo), make_string(name), make_int(arg1)});
}

void handle_vm_cmdFinishCombatGuide(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint16_t taskNo = rd16be(body, 0);
    uint16_t guideIndex = rd16be(body, 2);
    applyLpc("cmd_finish_combat_guide", {make_int(cl.clientNo), make_int(taskNo), make_int(guideIndex)});
}

void handle_vm_cmdFinishInterfaceGuide(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_finish_interface_guide", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdFinishInterfaceStatus(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    applyLpc("cmd_finish_interface_status", {make_int(cl.clientNo), make_int(body[0]), make_int(body[1])});
}

void handle_vm_cmdFinishQuestion(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_finish_question", {make_int(cl.clientNo)});
}

void handle_vm_cmdFinishStall(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_finish_stall", {make_int(cl.clientNo)});
}

void handle_vm_cmdFishingApplyMultiCard(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 3) return;
    uint16_t cardNo = rd16be(body, 1);
    applyLpc("cmd_fishing_apply_multi_card", {make_int(cl.clientNo), make_int(body[0]), make_int(cardNo)});
}

void handle_vm_cmdFishingAutoBuyWeapon(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_fishing_auto_buy_weapon", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdFishingBuyWeapon(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 6) return;
    uint16_t weaponId = rd16be(body, 0);
    uint32_t val = rd32be(body, 2);
    applyLpc("cmd_fishing_buy_weapon", {make_int(cl.clientNo), make_int(weaponId), make_int(val)});
}

void handle_vm_cmdFishingChangeWeapon(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t weaponId = rd16be(body, 0);
    applyLpc("cmd_fishing_change_weapon", {make_int(cl.clientNo), make_int(weaponId)});
}

void handle_vm_cmdFishingFireBullet(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 8) return;
    uint32_t a = rd32be(body, 0);
    uint32_t b = rd32be(body, 4);
    applyLpc("cmd_fishing_fire_bullet", {make_int(cl.clientNo), make_int(a), make_int(b)});
}

void handle_vm_cmdFishingOpenRemoteStore(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_fishing_open_remote_store", {make_int(cl.clientNo)});
}

void handle_vm_cmdFishingQuit(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_fishing_quit", {make_int(cl.clientNo)});
}

void handle_vm_cmdFishSwallowDie(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t val = rd32be(body, 0);
    applyLpc("cmd_fish_swallow_die", {make_int(cl.clientNo), make_int(val)});
}

void handle_vm_cmdFishSwallowFinish(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 3) return;
    uint8_t die = body[0];
    uint16_t statLen = rd16be(body, 1);
    size_t off = 3;
    if (off + statLen > len) return;
    std::string stat((const char*)body + off, statLen);
    off += statLen;
    if (off >= len) return;
    uint8_t csLen = body[off++];
    if (off + csLen > len) return;
    std::string checksum((const char*)body + off, csLen);
    applyLpc("cmd_fish_swallow_finish", {make_int(cl.clientNo), make_int(die), make_string(stat), make_string(checksum)});
}

void handle_vm_cmdFriendDrinkInvite(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nameLen = body[0];
    if (1 + nameLen > len) return;
    std::string name((const char*)body + 1, nameLen);
    applyLpc("cmd_friend_drink_invite", {make_int(cl.clientNo), make_string(name)});
}

void handle_vm_cmdFriendDrinkRefresh(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_friend_drink_refresh", {make_int(cl.clientNo)});
}

void handle_vm_cmdFriendTell(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 3) return;
    uint16_t flag = rd16be(body, 0);
    size_t off = 2;
    uint8_t nameLen = body[off++];
    if (off + nameLen > len) return;
    std::string name((const char*)body + off, nameLen);
    off += nameLen;
    if (off >= len) return;
    uint8_t msgLen = body[off++];
    if (off + msgLen > len) return;
    std::string msg((const char*)body + off, msgLen);
    applyLpc("cmd_friend_tell", {make_int(cl.clientNo), make_int(flag), make_string(name), make_string(msg)});
}

void handle_vm_cmdFriendTellAuth(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t nameLen = body[off++];
    if (off + nameLen > len) return;
    std::string name((const char*)body + off, nameLen);
    off += nameLen;
    applyLpc("cmd_friend_tell_auth", {make_int(cl.clientNo), make_string(name)});
}

void handle_vm_cmdFriendTellEx(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 2 > len) return;
    uint16_t flag = rd16be(body, off); off += 2;
    if (off + 1 > len) return;
    uint8_t nameLen = body[off++];
    if (off + nameLen > len) return;
    std::string name((const char*)body + off, nameLen); off += nameLen;
    if (off + 2 > len) return;
    uint16_t comp = rd16be(body, off); off += 2;
    if (off + 2 > len) return;
    uint16_t orgLen = rd16be(body, off); off += 2;
    if (off + 2 > len) return;
    uint16_t msgLen = rd16be(body, off); off += 2;
    if (off + msgLen > len) return;
    std::string msg((const char*)body + off, msgLen); off += msgLen;
    if (off + 2 > len) return;
    uint16_t itemCount = rd16be(body, off); off += 2;
    auto pArray = std::make_shared<ArrayValue>();
    auto pArrayType = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < itemCount; i++) {
        if (off + 1 > len) return;
        uint8_t slen = body[off++];
        if (off + slen > len) return;
        pArray->items.push_back(make_string(std::string((const char*)body + off, slen)));
        off += slen;
        if (off + 1 > len) return;
        pArrayType->items.push_back(make_int(body[off++]));
    }
    uint8_t v22 = 0;
    if (off < len) v22 = body[off++];
    applyLpc("cmd_friend_tell_ex", {make_int(cl.clientNo), make_int(flag), make_string(name), make_int(comp), make_int(orgLen), make_string(msg), pArray, pArrayType, make_int(v22)});
}

void handle_vm_cmdGather(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t value = rd32be(body, off); off += 4;
    applyLpc("cmd_gather", {make_int(cl.clientNo), make_int(value)});
}

void handle_vm_cmdGbuyingBuyGoods(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t idLen = body[off++];
    if (off + idLen > len) return;
    std::string gbgId((const char*)body + off, idLen); off += idLen;
    if (off + 1 > len) return;
    uint8_t v11 = body[off++];
    if (off + 1 > len) return;
    uint8_t v12 = body[off++];
    applyLpc("cmd_gbuying_buy_goods", {make_int(cl.clientNo), make_string(gbgId), make_int(v11), make_int(v12)});
}

void handle_vm_cmdGbuyingFetchCoin(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t coinType = body[off++];
    if (off + 4 > len) return;
    uint32_t coin = rd32be(body, off); off += 4;
    applyLpc("cmd_gbuying_fetch_coin", {make_int(cl.clientNo), make_int(coinType), make_int(coin)});
}

void handle_vm_cmdGbuyingFetchGoods(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t idLen = body[off++];
    if (off + idLen > len) return;
    std::string gbgId((const char*)body + off, idLen); off += idLen;
    if (off + 1 > len) return;
    uint8_t dnLen = body[off++];
    if (off + dnLen > len) return;
    std::string distname((const char*)body + off, dnLen); off += dnLen;
    applyLpc("cmd_gbuying_fetch_goods", {make_int(cl.clientNo), make_string(gbgId), make_string(distname)});
}

void handle_vm_cmdGbuyingInjectCoin(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t coinType = body[off++];
    if (off + 4 > len) return;
    uint32_t coin = rd32be(body, off); off += 4;
    applyLpc("cmd_gbuying_inject_coin", {make_int(cl.clientNo), make_int(coinType), make_int(coin)});
}

void handle_vm_cmdGbuyingOpenPage(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t titleLen = body[off++];
    if (off + titleLen > len) return;
    std::string title((const char*)body + off, titleLen); off += titleLen;
    if (off + 1 > len) return;
    uint8_t v11 = body[off++];
    if (off + 1 > len) return;
    uint8_t v12 = body[off++];
    applyLpc("cmd_gbuying_open_page", {make_int(cl.clientNo), make_string(title), make_int(v11), make_int(v12)});
}

void handle_vm_cmdGeneralNotify(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 2 > len) return;
    uint16_t type = rd16be(body, off); off += 2;
    if (off + 1 > len) return;
    uint8_t p1Len = body[off++];
    if (off + p1Len > len) return;
    std::string para1((const char*)body + off, p1Len); off += p1Len;
    if (off + 1 > len) return;
    uint8_t p2Len = body[off++];
    if (off + p2Len > len) return;
    std::string para2((const char*)body + off, p2Len); off += p2Len;
    applyLpc("cmd_general_notify", {make_int(cl.clientNo), make_int(type), make_string(para1), make_string(para2)});
}

void handle_vm_cmdGeneralRequest(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 2 > len) return;
    uint16_t value = rd16be(body, off); off += 2;
    applyLpc("cmd_general_request", {make_int(cl.clientNo), make_int(value)});
}

void handle_vm_cmdGet(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    int id = rd32be(body);
    applyLpc("cmd_get", {make_int(cl.clientNo), make_int(id)});
}

void handle_vm_cmdGetAchieveBonus(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    int id = rd32be(body);
    applyLpc("cmd_get_achieve_bonus", {make_int(cl.clientNo), make_int(id)});
}

void handle_vm_cmdGetAwardInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    int v = 0;
    if (len > 0) v = body[0];
    applyLpc("cmd_get_award_info", {make_int(cl.clientNo), make_int(v)});
}

void handle_vm_cmdGetClientAction(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t n = rd16be(body);
    if (n > 1023) n = 1023;
    if (2 + n > len) return;
    std::string s((const char*)body + 2, n);
    applyLpc("cmd_get_client_action", {make_int(cl.clientNo), make_string(s)});
}

void handle_vm_cmdGetClientPerformance(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t n = rd16be(body);
    if (n > 1023) n = 1023;
    if (2 + n > len) return;
    std::string s((const char*)body + 2, n);
    applyLpc("cmd_get_client_performance", {make_int(cl.clientNo), make_string(s)});
}

void handle_vm_cmdGetClientPhyConfig(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t n = rd16be(body);
    if (n > 1023) n = 1023;
    if (2 + n > len) return;
    std::string s((const char*)body + 2, n);
    applyLpc("cmd_get_client_phy_config", {make_int(cl.clientNo), make_string(s)});
}

void handle_vm_cmdGetDailyLoop(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n = body[0];
    if (1 + n > len) return;
    std::string s((const char*)body + 1, n);
    applyLpc("cmd_get_daily_loop", {make_int(cl.clientNo), make_string(s)});
}

void handle_vm_cmdGetImage(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n1 = body[0];
    if (1 + n1 > len) return;
    std::string md5((const char*)body + 1, n1);
    size_t off = 1 + n1;
    if (off >= len) return;
    uint8_t n2 = body[off];
    if (off + 1 + n2 > len) return;
    std::string gid((const char*)body + off + 1, n2);
    applyLpc("cmd_get_image_buffer", {make_int(cl.clientNo), make_string(gid), make_string(md5)});
}

void handle_vm_cmdGetNameByGid(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    int type = body[0];
    uint8_t n = body[1];
    if (2 + n > len) return;
    std::string gid((const char*)body + 2, n);
    applyLpc("cmd_get_name_by_gid", {make_int(cl.clientNo), make_int(type), make_string(gid)});
}

void handle_vm_cmdGetOrderRank(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n1 = body[0];
    if (1 + n1 > len) return;
    std::string order_id((const char*)body + 1, n1);
    size_t off = 1 + n1;
    if (off >= len) return;
    int type = body[off];
    ++off;
    if (off >= len) return;
    uint8_t n2 = body[off];
    if (off + 1 + n2 > len) return;
    std::string name((const char*)body + off + 1, n2);
    applyLpc("cmd_get_order_rank", {make_int(cl.clientNo), make_string(order_id), make_int(type), make_string(name)});
}

void handle_vm_cmdGetPartyChannelDenyList(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_get_party_channel_deny_list", {make_int(cl.clientNo)});
}

void handle_vm_cmdGetPartyLog(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_get_party_log", {make_int(cl.clientNo)});
}

void handle_vm_cmdGetPetEvolveList(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_get_pet_evolve_list", {make_int(cl.clientNo)});
}

void handle_vm_cmdGetPhoneAuthBindTime(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_get_phone_auth_bind_time", {make_int(cl.clientNo)});
}

void handle_vm_cmdGetRaiseBonus(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 6) return;
    uint32_t v = rd32be(body);
    uint8_t b1 = body[4];
    uint8_t b2 = body[5];
    applyLpc("cmd_get_raise_bonus", {make_int(cl.clientNo), make_int(v), make_int(b1), make_int(b2)});
}

void handle_vm_cmdGetRandomBoxAward(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t pos = body[0];
    uint8_t n = body[1];
    if (2 + n > len) return;
    std::string coin_type((const char*)body + 2, n);
    applyLpc("cmd_get_random_box_award", {make_int(cl.clientNo), make_int(pos), make_string(coin_type)});
}

void handle_vm_cmdGetSafeQuestion(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_get_safe_question", {make_int(cl.clientNo)});
}

void handle_vm_cmdGetSafeState(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_get_safe_state", {make_int(cl.clientNo)});
}

void handle_vm_cmdGetTaskItem(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n = body[0];
    if (1 + n > len) return;
    std::string item_name((const char*)body + 1, n);
    applyLpc("cmd_get_task_item", {make_int(cl.clientNo), make_string(item_name)});
}

void handle_vm_cmdGetTodayStat(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_get_today_stat", {make_int(cl.clientNo)});
}

void handle_vm_cmdGetUpdateNotice(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_get_update_notice", {make_int(cl.clientNo)});
}

void handle_vm_cmdGoodsBuy(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 10) return;
    uint32_t goodsId = rd32be(body, 0);
    uint16_t n1 = rd16be(body, 4);
    uint16_t n2 = rd16be(body, 6);
    uint16_t n3 = rd16be(body, 8);
    applyLpc("cmd_goods_buy", {make_int(cl.clientNo), make_int(goodsId), make_int(n1), make_int(n2), make_int(n3)});
}

void handle_vm_cmdGoodsSell(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 8) return;
    uint32_t goodsId = rd32be(body, 0);
    uint16_t n1 = rd16be(body, 4);
    uint16_t n2 = rd16be(body, 6);
    applyLpc("cmd_goods_sell", {make_int(cl.clientNo), make_int(goodsId), make_int(n1), make_int(n2)});
}

void handle_vm_cmdGroupChannel(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    uint8_t nameLen = body[off++];
    if (nameLen > len - off) return;
    std::string name((const char*)body + off, nameLen);
    off += nameLen;
    if (len - off < 2) return;
    uint16_t comp = rd16be(body, off); off += 2;
    if (len - off < 2) return;
    uint16_t orgLen = rd16be(body, off); off += 2;
    if (len - off < 2) return;
    uint16_t msgLen = rd16be(body, off); off += 2;
    if (msgLen > len - off) return;
    std::string msg((const char*)body + off, msgLen);
    off += msgLen;
    if (len - off < 2) return;
    uint16_t itemCount = rd16be(body, off); off += 2;
    auto arr = std::make_shared<ArrayValue>();
    auto arrType = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < itemCount; i++)
    {
        if (off >= len) return;
        uint8_t sLen = body[off++];
        if (sLen > len - off) return;
        arr->items.push_back(make_string(std::string((const char*)body + off, sLen)));
        off += sLen;
        if (off >= len) return;
        uint8_t ty = body[off++];
        arrType->items.push_back(make_int(ty));
    }
    applyLpc("cmd_group_channel", {make_int(cl.clientNo), make_string(name), make_int(comp), make_int(orgLen), make_string(msg), arr, arrType});
}

void handle_vm_cmdGuardsChangeName(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    uint32_t guardId = rd32be(body, 0);
    size_t off = 4;
    uint8_t nameLen = body[off++];
    if (nameLen > len - off) return;
    std::string newname((const char*)body + off, nameLen);
    applyLpc("cmd_guards_change_name", {make_int(cl.clientNo), make_int(guardId), make_string(newname)});
}

void handle_vm_cmdGuardsCheer(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    uint32_t guardId = rd32be(body, 0);
    uint8_t cheer = body[4];
    applyLpc("cmd_guards_cheer", {make_int(cl.clientNo), make_int(guardId), make_int(cheer)});
}

void handle_vm_cmdGuardsDropGuard(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t guardId = rd32be(body, 0);
    applyLpc("cmd_guards_drop_guard", {make_int(cl.clientNo), make_int(guardId)});
}

void handle_vm_cmdGuardsRecover(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t guardId = rd32be(body, 0);
    applyLpc("cmd_guards_recover", {make_int(cl.clientNo), make_int(guardId)});
}

void handle_vm_cmdGuardsRefresh(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_guards_refresh", {make_int(cl.clientNo)});
}

void handle_vm_cmdGuardsSetTactics(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    uint32_t guardId = rd32be(body, 0);
    uint8_t tactics = body[4];
    applyLpc("cmd_guards_set_tactics", {make_int(cl.clientNo), make_int(guardId), make_int(tactics)});
}

// ---- game commands (type=1) generated handlers, chunk 3 ----

void handle_vm_cmdOperTMoodInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_oper_mood_info", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdopervisualpacks(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 3) return;
    applyLpc("cmd_oper_visual_packs", {make_int(cl.clientNo), make_int(body[0]), make_int(body[1]), make_int(body[2])});
}

void handle_vm_cmdOperXiaobaoSongshenbing(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    int type = body[0];
    uint8_t elen = body[1];
    if (2 + elen > len) return;
    const uint8_t* equip = body + 2;
    size_t off = 2 + elen;
    if (off >= len) return;
    uint8_t alen = body[off];
    off += 1;
    if (off + alen > len) return;
    applyLpc("cmd_oper_xiaobao_songshenbing", {make_int(cl.clientNo), make_int(type), make_string(equip, elen), make_string(body + off, alen)});
}

void handle_vm_cmdOtherMoveTo(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 14) return;
    uint32_t program = rd32be(body, 0);
    uint32_t map = rd32be(body, 4);
    uint16_t x = rd16be(body, 8);
    uint16_t y = rd16be(body, 10);
    uint16_t z = rd16be(body, 12);
    applyLpc("cmd_other_move_to", {make_int(cl.clientNo), make_int(program), make_int(map), make_int(x), make_int(y), make_int(z)});
}

void handle_vm_cmdPartyBlackList(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    int oper = body[0];
    uint8_t nlen = body[1];
    if (2 + nlen > len) return;
    applyLpc("cmd_party_black_list", {make_int(cl.clientNo), make_int(oper), make_string(body + 2, nlen)});
}

void handle_vm_cmdPartyInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_party_info", {make_int(cl.clientNo)});
}

void handle_vm_cmdPartyJobPrivilegeList(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_party_job_privilege_list", {make_int(cl.clientNo)});
}

void handle_vm_cmdPartyMembers(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 3) return;
    uint16_t page = rd16be(body, 0);
    uint8_t nlen = body[2];
    if (3 + nlen > len) return;
    const uint8_t* nm = body + 3;
    size_t off = 3 + nlen;
    if (off >= len) return;
    uint8_t glen = body[off];
    off += 1;
    if (off + glen > len) return;
    applyLpc("cmd_party_members", {make_int(cl.clientNo), make_int(page), make_string(nm, nlen), make_string(body + off, glen)});
}

void handle_vm_cmdPartyModifyAnnouce(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t slen = rd16be(body, 0);
    if (2 + slen > len) return;
    applyLpc("cmd_party_modify_annouce", {make_int(cl.clientNo), make_string(body + 2, slen)});
}

void handle_vm_cmdPartyModifyMember(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nlen = body[0];
    if (1 + nlen > len) return;
    const uint8_t* nm = body + 1;
    size_t off = 1 + nlen;
    if (off >= len) return;
    uint8_t ilen = body[off];
    off += 1;
    if (off + ilen > len) return;
    const uint8_t* idp = body + off;
    off += ilen;
    if (off >= len) return;
    uint8_t tlen = body[off];
    off += 1;
    if (off + tlen + 4 > len) return;
    const uint8_t* tp = body + off;
    off += tlen;
    uint16_t priv = rd16be(body, off);
    uint16_t flag = rd16be(body, off + 2);
    applyLpc("cmd_party_modify_member", {make_int(cl.clientNo), make_string(nm, nlen), make_string(idp, ilen), make_string(tp, tlen), make_int(priv), make_int(flag)});
}

void handle_vm_cmdPartyRelation(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t nameLen = body[off++];
    if (off + nameLen > len) return;
    const uint8_t* name = body + off; off += nameLen;
    if (off + 1 > len) return;
    uint8_t idLen = body[off++];
    if (off + idLen > len) return;
    const uint8_t* id = body + off; off += idLen;
    if (off + 2 > len) return;
    uint16_t relation = rd16be(body, off);
    applyLpc("cmd_party_relation", {make_int(cl.clientNo), make_string(name, nameLen), make_string(id, idLen), make_int(relation)});
}

void handle_vm_cmdPartyRequestList(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_party_request_list", {make_int(cl.clientNo)});
}

void handle_vm_cmdPartySendMessage(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_party_send_message", {make_int(cl.clientNo)});
}

void handle_vm_cmdPasswordDlg(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t pwLen = body[0];
    if (1 + pwLen > len) return;
    applyLpc("cmd_password_dlg", {make_int(cl.clientNo), make_string(body + 1, pwLen)});
}

void handle_vm_cmdPlaceBox(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t pLen = body[0];
    if (1 + pLen > len) return;
    applyLpc("cmd_place_box", {make_int(cl.clientNo), make_string(body + 1, pLen)});
}

void handle_vm_cmdPlaceFurniture(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 9) return;
    uint16_t pos = rd16be(body, 0);
    uint16_t x = rd16be(body, 2);
    uint16_t y = rd16be(body, 4);
    uint8_t dir = body[6];
    uint8_t v7 = body[7];
    uint8_t v8 = body[8];
    applyLpc("cmd_place_furniture", {make_int(cl.clientNo), make_int(pos), make_int(x), make_int(y), make_int(dir), make_int(v7), make_int(v8)});
}

void handle_vm_cmdPreAssignAttrib(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 20) return;
    uint32_t a1 = rd32be(body, 0);
    uint32_t a2 = rd32be(body, 4);
    uint32_t a3 = rd32be(body, 8);
    uint32_t a4 = rd32be(body, 12);
    uint32_t a5 = rd32be(body, 16);
    applyLpc("cmd_pre_assign_attrib", {make_int(cl.clientNo), make_int(a1), make_int(a2), make_int(a3), make_int(a4), make_int(a5)});
}

void handle_vm_cmdPreAssignChildAttrib(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 20) return;
    uint32_t a1 = rd32be(body, 0);
    uint32_t a2 = rd32be(body, 4);
    uint32_t a3 = rd32be(body, 8);
    uint32_t a4 = rd32be(body, 12);
    uint32_t a5 = rd32be(body, 16);
    applyLpc("cmd_pre_assign_child_attrib", {make_int(cl.clientNo), make_int(a1), make_int(a2), make_int(a3), make_int(a4), make_int(a5)});
}

void handle_vm_cmdPresentAskTaoShow(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t oppLen = body[off++];
    if (off + oppLen > len) return;
    const uint8_t* opponent = body + off; off += oppLen;
    if (off + 1 > len) return;
    uint8_t pwdLen = body[off++];
    if (off + pwdLen > len) return;
    const uint8_t* coin_pwd = body + off; off += pwdLen;
    if (off + 1 > len) return;
    uint8_t barLen = body[off++];
    if (off + barLen > len) return;
    const uint8_t* barcode = body + off; off += barLen;
    if (off + 2 > len) return;
    uint16_t v18 = rd16be(body, off);
    applyLpc("cmd_present_asktao_show", {make_int(cl.clientNo), make_string(opponent, oppLen), make_string(coin_pwd, pwdLen), make_string(barcode, barLen), make_int(v18)});
}

void handle_vm_cmdPresentMusic(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t type = body[off++];
    if (off + 1 > len) return;
    uint8_t nameLen = body[off++];
    if (off + nameLen > len) return;
    const uint8_t* name_info = body + off; off += nameLen;
    if (off + 1 > len) return;
    uint8_t coin_type = body[off++];
    if (off + 1 > len) return;
    uint8_t msgLen = body[off++];
    if (off + msgLen > len) return;
    const uint8_t* wish_msg = body + off; off += msgLen;
    if (off + 1 > len) return;
    uint8_t v17 = body[off++];
    if (off + 4 > len) return;
    uint32_t v20 = rd32be(body, off);
    applyLpc("cmd_present_music", {make_int(cl.clientNo), make_int(type), make_string(name_info, nameLen), make_int(coin_type), make_string(wish_msg, msgLen), make_int(v17), make_int(v20)});
}

void handle_vm_cmdRequestDeckInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t deckId = rd16be(body);
    applyLpc("cmd_request_deck_info", {make_int(cl.clientNo), make_int(deckId)});
}

void handle_vm_cmdRequestDeckList(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_request_deck_list", {make_int(cl.clientNo)});
}

void handle_vm_cmdRequestDecomposeCard(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n = body[0];
    if (1 + n > len) return;
    applyLpc("cmd_request_decompose_card", {make_int(cl.clientNo), make_string(body + 1, n)});
}

void handle_vm_cmdRequestDeleteDeck(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t deckId = rd16be(body);
    applyLpc("cmd_request_delete_deck", {make_int(cl.clientNo), make_int(deckId)});
}

void handle_vm_cmdRequestDrawCardCompeteBonus(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_request_draw_card_compete_bonus", {make_int(cl.clientNo)});
}

void handle_vm_cmdRequestDropCardTask(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n = body[0];
    if (1 + n > len) return;
    applyLpc("cmd_request_drop_card_task", {make_int(cl.clientNo), make_string(body + 1, n)});
}

void handle_vm_cmdRequestEnterCitywarRoom(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n1 = body[off];
    if (off + 1 + n1 > len) return;
    const uint8_t* city = body + off + 1;
    size_t cityLen = n1;
    off += 1 + n1;
    if (off + 1 > len) return;
    uint8_t n2 = body[off];
    if (off + 1 + n2 > len) return;
    const uint8_t* building = body + off + 1;
    size_t buildingLen = n2;
    applyLpc("cmd_request_enter_citywar_room", {make_int(cl.clientNo), make_string(city, cityLen), make_string(building, buildingLen)});
}

void handle_vm_cmdRequestEnterXGame(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n = body[0];
    if (1 + n > len) return;
    applyLpc("cmd_request_enter_x_game", {make_int(cl.clientNo), make_string(body + 1, n)});
}

void handle_vm_cmdRequestFinishCardCompete(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_request_finish_card_compete", {make_int(cl.clientNo)});
}

void handle_vm_cmdRequestFriendVerify(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n1 = body[off];
    if (off + 1 + n1 > len) return;
    const uint8_t* charName = body + off + 1;
    size_t charNameLen = n1;
    off += 1 + n1;
    if (off + 1 > len) return;
    uint8_t n2 = body[off];
    if (off + 1 + n2 > len) return;
    const uint8_t* reqMsg = body + off + 1;
    size_t reqMsgLen = n2;
    applyLpc("cmd_request_friend_verify", {make_int(cl.clientNo), make_string(charName, charNameLen), make_string(reqMsg, reqMsgLen)});
}

void handle_vm_cmdRequestIcon(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n = body[off++];
    if (off + n > len) return;
    applyLpc("cmd_request_icon", {make_int(cl.clientNo), make_string(body + off, n)});
}

void handle_vm_cmdRequestInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t infoType = rd32be(body, off); off += 4;
    if (off + 1 > len) return;
    uint8_t n = body[off++];
    if (off + n > len) return;
    applyLpc("cmd_request_info", {make_int(cl.clientNo), make_int(infoType), make_string(body + off, n)});
}

void handle_vm_cmdRequestItemInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n = body[off++];
    if (off + n > len) return;
    applyLpc("cmd_request_item_info", {make_int(cl.clientNo), make_string(body + off, n)});
}

void handle_vm_cmdRequestJoin(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n1 = body[off++];
    if (off + n1 > len) return;
    auto name = make_string(body + off, n1); off += n1;
    if (off + 1 > len) return;
    uint8_t n2 = body[off++];
    if (off + n2 > len) return;
    applyLpc("cmd_request_join", {make_int(cl.clientNo), name, make_string(body + off, n2)});
}

void handle_vm_cmdRequestJoinRecruitment(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n1 = body[off++];
    if (off + n1 > len) return;
    auto type = make_string(body + off, n1); off += n1;
    if (off + 1 > len) return;
    uint8_t n2 = body[off++];
    if (off + n2 > len) return;
    auto targetName = make_string(body + off, n2); off += n2;
    if (off + 1 > len) return;
    uint8_t n3 = body[off++];
    if (off + n3 > len) return;
    applyLpc("cmd_request_join_recruitment", {make_int(cl.clientNo), type, targetName, make_string(body + off, n3)});
}

void handle_vm_cmdRequestLevelUp(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_request_levelup", {make_int(cl.clientNo)});
}

void handle_vm_cmdRequestMineralsInMap(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n = body[off++];
    if (off + n > len) return;
    applyLpc("cmd_request_minerals_in_map", {make_int(cl.clientNo), make_string(body + off, n)});
}

void handle_vm_cmdRequestMyCardPackages(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_request_my_card_packages", {make_int(cl.clientNo)});
}

void handle_vm_cmdRequestNewDeck(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 2 > len) return;
    uint16_t group = rd16be(body, off); off += 2;
    if (off + 1 > len) return;
    uint8_t n = body[off++];
    if (off + n > len) return;
    applyLpc("cmd_request_new_deck", {make_int(cl.clientNo), make_int(group), make_string(body + off, n)});
}

void handle_vm_cmdRequestOpenCardPackage(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 2 > len) return;
    uint16_t cardPackageId = rd16be(body, off);
    applyLpc("cmd_request_open_card_package", {make_int(cl.clientNo), make_int(cardPackageId)});
}

void handle_vm_cmdRequestOpenSystemDlg(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    if (1 + (size_t)slen > len) return;
    applyLpc("cmd_request_open_system", {make_int(cl.clientNo), make_string(body + 1, slen)});
}

void handle_vm_cmdRequestPropertyBind(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_request_property_bind", {make_int(cl.clientNo)});
}

void handle_vm_cmdRequestReceiveCityTask(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t cityLen = body[0];
    if (1 + (size_t)cityLen + 1 >= len) return;
    size_t off = 1 + cityLen;
    uint8_t bldLen = body[off];
    if (off + 1 + (size_t)bldLen >= len) return;
    off += 1 + bldLen;
    uint8_t taskIdx = body[off];
    applyLpc("cmd_request_receive_city_task", {make_int(cl.clientNo), make_string(body + 1, cityLen), make_string(body + 2 + cityLen, bldLen), make_int(taskIdx)});
}

void handle_vm_cmdRequestRecharge(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_request_recharge", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdRequestRefreshCityTask(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t cityLen = body[0];
    if (1 + (size_t)cityLen >= len) return;
    size_t off = 1 + cityLen;
    uint8_t bldLen = body[off];
    if (off + 1 + (size_t)bldLen > len) return;
    applyLpc("cmd_request_refresh_city_task", {make_int(cl.clientNo), make_string(body + 1, cityLen), make_string(body + off + 1, bldLen)});
}

void handle_vm_cmdRequestRenameDeck(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 3) return;
    uint16_t no = rd16be(body, 0);
    uint8_t slen = body[2];
    if (3 + (size_t)slen > len) return;
    applyLpc("cmd_request_rename_deck", {make_int(cl.clientNo), make_int(no), make_string(body + 3, slen)});
}

void handle_vm_cmdRequestServerStatus(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_request_server_status", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdRequestShiling(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_request_shiling", {make_int(cl.clientNo)});
}

void handle_vm_cmdRequestStallList(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_request_stall_list", {make_int(cl.clientNo)});
}

void handle_vm_cmdRequestStartCardCompete(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_request_start_card_compete", {make_int(cl.clientNo)});
}

void handle_vm_cmdRequestStartMatchCardGame(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint16_t mode = rd16be(body, 0);
    uint16_t no = rd16be(body, 2);
    applyLpc("cmd_request_start_match_card_game", {make_int(cl.clientNo), make_int(mode), make_int(no)});
}

void handle_vm_cmdRequestTakeProperty(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t propNo = rd32be(body, 0);
    applyLpc("cmd_request_take_property", {make_int(cl.clientNo), make_int(propNo)});
}

void handle_vm_cmdRequestToBeTeamLeader(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_request_to_be_team_leader", {make_int(cl.clientNo)});
}

void handle_vm_cmdRequestTopTaoPage(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    size_t off = 0;
    uint8_t nameLen = body[off++];
    if (off + nameLen + 1 > len) return;
    auto name = make_string(body + off, nameLen);
    off += nameLen;
    uint8_t value = body[off];
    applyLpc("cmd_request_top_tao_page", {make_int(cl.clientNo), name, make_int(value)});
}

void handle_vm_cmdReselectChar(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_reselect_char", {make_int(cl.clientNo)});
}

void handle_vm_cmdReselectVerifyCode(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_reselect_verify_code", {make_int(cl.clientNo)});
}

void handle_vm_cmdResendVipSms(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_resend_vip_sms", {make_int(cl.clientNo)});
}

void handle_vm_cmdResetAppellationTaskDue(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t off = 0;
    uint8_t nameLen = body[off++];
    if (off + nameLen > len) return;
    auto name = make_string(body + off, nameLen);
    applyLpc("cmd_reset_appellation_task_due", {make_int(cl.clientNo), name});
}

void handle_vm_cmdResetAppellationTaskMode(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    size_t off = 0;
    uint8_t nameLen = body[off++];
    if (off + nameLen + 1 > len) return;
    auto name = make_string(body + off, nameLen);
    off += nameLen;
    uint8_t value = body[off];
    applyLpc("cmd_reset_appellation_task_mode", {make_int(cl.clientNo), name, make_int(value)});
}

void handle_vm_cmdReturnTeam(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_return_team", {make_int(cl.clientNo)});
}

void handle_vm_cmdRevokeConvenant(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n = body[off++];
    if (off + n > len) return;
    applyLpc("cmd_revoke_convenant", { make_int(cl.clientNo), make_string(body + off, n) });
}

void handle_vm_cmdRewriteChildBook(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    applyLpc("cmd_rewrite_child_book", { make_int(cl.clientNo), make_int(rd32be(body, 0)) });
}

void handle_vm_cmdSearchChatGroup(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n = body[off++];
    if (off + n > len) return;
    applyLpc("cmd_search_chat_group", { make_int(cl.clientNo), make_string(body + off, n) });
}

void handle_vm_cmdSearchStallGoods(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t type = body[off++];
    if (off + 1 > len) return;
    uint8_t nameLen = body[off++];
    if (off + nameLen > len) return;
    auto name = make_string(body + off, nameLen); off += nameLen;
    if (off + 1 > len) return;
    uint8_t levelLen = body[off++];
    if (off + levelLen > len) return;
    auto level = make_string(body + off, levelLen); off += levelLen;
    if (off + 1 > len) return;
    uint8_t qualityLen = body[off++];
    if (off + qualityLen > len) return;
    auto quality = make_string(body + off, qualityLen); off += qualityLen;
    if (off + 1 > len) return;
    uint8_t subType = body[off++];
    if (off + 4 > len) return;
    uint32_t cookie = rd32be(body, off); off += 4;
    if (off + 4 > len) return;
    uint32_t id = rd32be(body, off); off += 4;
    if (off + 4 > len) return;
    uint32_t point = rd32be(body, off); off += 4;
    if (off + 1 > len) return;
    uint8_t shopLen = body[off++];
    if (off + shopLen > len) return;
    auto shopType = make_string(body + off, shopLen); off += shopLen;
    applyLpc("cmd_search_stall_goods", { make_int(cl.clientNo), make_int(type), name, level, quality, make_int(subType), make_int(cookie), make_int(id), make_int(point), shopType });
}

void handle_vm_cmdSelectCurrentChild(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 6) return;
    applyLpc("cmd_select_current_child", { make_int(cl.clientNo), make_int(rd32be(body, 0)), make_int(rd16be(body, 4)) });
}

void handle_vm_cmdSelectCurrentMount(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    applyLpc("cmd_select_current_mount", { make_int(cl.clientNo), make_int(rd32be(body, 0)) });
}

void handle_vm_cmdSelectCurrentPet(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 6) return;
    applyLpc("cmd_select_current_pet", { make_int(cl.clientNo), make_int(rd32be(body, 0)), make_int(rd16be(body, 4)) });
}

void handle_vm_cmdSelectMenuItem0(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t id = rd32be(body, off); off += 4;
    if (off + 1 > len) return;
    uint8_t itemLen = body[off++];
    if (off + itemLen > len) return;
    auto menuItem = make_string(body + off, itemLen); off += itemLen;
    if (off + 1 > len) return;
    uint8_t paraLen = body[off++];
    if (off + paraLen > len) return;
    auto para = make_string(body + off, paraLen); off += paraLen;
    applyLpc("cmd_select_menu_item", { make_int(0), make_int(cl.clientNo), make_int(id), menuItem, para });
}

void handle_vm_cmdSelectMenuItem1(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t id = rd32be(body, off); off += 4;
    if (off + 1 > len) return;
    uint8_t itemLen = body[off++];
    if (off + itemLen > len) return;
    auto menuItem = make_string(body + off, itemLen); off += itemLen;
    if (off + 1 > len) return;
    uint8_t paraLen = body[off++];
    if (off + paraLen > len) return;
    auto para = make_string(body + off, paraLen); off += paraLen;
    applyLpc("cmd_select_menu_item", { make_int(1), make_int(cl.clientNo), make_int(id), menuItem, para });
}

void handle_vm_cmdSelectMenuItem2(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t id = rd32be(body, off); off += 4;
    if (off + 1 > len) return;
    uint8_t itemLen = body[off++];
    if (off + itemLen > len) return;
    auto menuItem = make_string(body + off, itemLen); off += itemLen;
    if (off + 1 > len) return;
    uint8_t paraLen = body[off++];
    if (off + paraLen > len) return;
    auto para = make_string(body + off, paraLen); off += paraLen;
    applyLpc("cmd_select_menu_item", { make_int(2), make_int(cl.clientNo), make_int(id), menuItem, para });
}

void handle_vm_cmdSelectSpecifiedCombatDeck(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t deckNo = rd16be(body, 0);
    applyLpc("cmd_select_specified_combat_deck", {make_int(cl.clientNo), make_int(deckNo)});
}

void handle_vm_cmdSelectVisibleChild(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t childId = rd32be(body, 0);
    applyLpc("cmd_select_visible_child", {make_int(cl.clientNo), make_int(childId)});
}

void handle_vm_cmdSelectVisiblePet(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t petId = rd32be(body, 0);
    applyLpc("cmd_select_visible_pet", {make_int(cl.clientNo), make_int(petId)});
}

void handle_vm_cmdSendEnvelope(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 3) return;
    size_t off = 0;
    uint8_t type = body[off++];
    uint8_t subType = body[off++];
    uint8_t pLen = body[off++];
    if (off + pLen > len) return;
    auto para = make_string((const char*)body + off, pLen);
    off += pLen;
    if (off + 4 > len) return;
    uint32_t n1 = rd32be(body, off); off += 4;
    if (off + 4 > len) return;
    uint32_t n2 = rd32be(body, off); off += 4;
    if (off >= len) return;
    uint8_t tail = body[off];
    applyLpc("cmd_send_envelope", {make_int(cl.clientNo), make_int(type), make_int(subType), para, make_int(n1), make_int(n2), make_int(tail)});
}

void handle_vm_cmdSendFriendCard(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    size_t off = 0;
    uint8_t toLen = body[off++];
    if (off + toLen > len) return;
    auto to = make_string((const char*)body + off, toLen);
    off += toLen;
    if (off >= len) return;
    uint8_t nameLen = body[off++];
    if (off + nameLen > len) return;
    auto name = make_string((const char*)body + off, nameLen);
    applyLpc("cmd_send_friend_card", {make_int(cl.clientNo), to, name});
}

void handle_vm_cmdSetAchieveTrace(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    uint32_t traceId = rd32be(body, 0);
    uint8_t flag = body[4];
    applyLpc("cmd_set_achieve_trace", {make_int(cl.clientNo), make_int(traceId), make_int(flag)});
}

void handle_vm_cmdSetAttention(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 3) return;
    size_t off = 0;
    uint8_t typeLen = body[off++];
    if (off + typeLen > len) return;
    auto type = make_string((const char*)body + off, typeLen);
    off += typeLen;
    if (off >= len) return;
    uint8_t idLen = body[off++];
    if (off + idLen > len) return;
    auto id = make_string((const char*)body + off, idLen);
    off += idLen;
    if (off >= len) return;
    uint8_t flag = body[off];
    applyLpc("cmd_set_attention", {make_int(cl.clientNo), type, id, make_int(flag)});
}

void handle_vm_cmdSetAutoMsg(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t off = 0;
    uint8_t msgLen = body[off++];
    if (off + msgLen > len) return;
    auto msg = make_string((const char*)body + off, msgLen);
    applyLpc("cmd_set_automatic_reply_msg", {make_int(cl.clientNo), msg});
}

void handle_vm_cmdSetBirthday(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    size_t off = 0;
    uint8_t bLen = body[off++];
    if (off + bLen > len) return;
    auto birthday = make_string((const char*)body + off, bLen);
    off += bLen;
    if (off >= len) return;
    uint8_t flag = body[off];
    applyLpc("cmd_set_birthday", {make_int(cl.clientNo), birthday, make_int(flag)});
}

void handle_vm_cmdSetCardTopmost(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 3) return;
    uint16_t pos = rd16be(body, 0);
    size_t off = 2;
    uint8_t nameLen = body[off++];
    if (off + nameLen > len) return;
    auto name = make_string((const char*)body + off, nameLen);
    applyLpc("cmd_set_card_topmost", {make_int(cl.clientNo), make_int(pos), name});
}

void handle_vm_cmdSetChildName(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t pos = body[0];
    uint8_t n = body[1];
    if (2 + n > len) return;
    applyLpc("cmd_set_child_name", {make_int(cl.clientNo), make_int(pos), make_string(body + 2, n)});
}

void handle_vm_cmdSetDecoration(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 3) return;
    uint8_t type = body[0];
    uint16_t count = rd16be(body, 1);
    if (count > 0x3F) return;
    size_t off = 3;
    auto arr = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < count; ++i) {
        if (off >= len) return;
        uint8_t n = body[off++];
        if (off + n > len) return;
        arr->items.push_back(make_string(body + off, n));
        off += n;
    }
    applyLpc("cmd_set_decoration", {make_int(cl.clientNo), make_int(type), arr});
}

void handle_vm_cmdSetDoucdhPetPos(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 24) return;
    applyLpc("cmd_set_doucdh_pet_pos", {make_int(cl.clientNo), make_int((int)rd32be(body, 0)), make_int((int)rd32be(body, 4)), make_int((int)rd32be(body, 8)), make_int((int)rd32be(body, 12)), make_int((int)rd32be(body, 16)), make_int((int)rd32be(body, 20))});
}

void handle_vm_cmdSetFireworksReleaseTime(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    applyLpc("cmd_set_fireworks_release_time", {make_int(cl.clientNo), make_int(rd16be(body)), make_int(body[2]), make_int(body[3])});
}

void handle_vm_cmdSetFriendRemark(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n1 = body[0];
    if (1 + n1 > len) return;
    size_t off = 1 + n1;
    if (off >= len) return;
    uint8_t n2 = body[off++];
    if (off + n2 > len) return;
    applyLpc("cmd_set_friend_remark", {make_int(cl.clientNo), make_string(body + 1, n1), make_string(body + off, n2)});
}

void handle_vm_cmdSetGcslAttribs(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n1 = body[0];
    if (1 + n1 > len) return;
    size_t off = 1 + n1;
    if (off >= len) return;
    uint8_t n2 = body[off++];
    if (off + n2 > len) return;
    applyLpc("cmd_set_gcsl_attribs", {make_int(cl.clientNo), make_string(body + 1, n1), make_string(body + off, n2)});
}

void handle_vm_cmdSetHotkeys(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t n = rd16be(body);
    if (2 + n > len) return;
    applyLpc("cmd_set_hotkeys", {make_int(cl.clientNo), make_string(body + 2, n)});
}

void handle_vm_cmdSetImage(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n = body[0];
    if (1 + n > len) return;
    applyLpc("cmd_set_individuality_image", {make_int(cl.clientNo), make_string(body + 1, n)});
}

void handle_vm_cmdSetpass(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n = body[0];
    if (1 + n > len) return;
    applyLpc("cmd_setpass", {make_int(cl.clientNo), make_string(body + 1, n), make_string("", 0)});
}

void handle_vm_cmdSetPassword(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_set_password", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdSetPennantSkill(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t pennantSkill = rd16be(body, 0);
    applyLpc("cmd_set_pennant_skill", {make_int(cl.clientNo), make_int(pennantSkill)});
}

void handle_vm_cmdSetPetName(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t pos = body[0];
    uint8_t nameLen = body[1];
    if ((size_t)2 + nameLen > len) return;
    applyLpc("cmd_set_pet_name", {make_int(cl.clientNo), make_int(pos), make_string(body + 2, nameLen)});
}

void handle_vm_cmdSetRecommendAttrib(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 9) return;
    uint32_t id = rd32be(body, 0);
    uint8_t con = body[4];
    uint8_t wiz = body[5];
    uint8_t str = body[6];
    uint8_t dex = body[7];
    uint8_t isAuto = body[8];
    applyLpc("cmd_set_recommend_attrib", {make_int(cl.clientNo), make_int(id), make_int(con), make_int(wiz), make_int(str), make_int(dex), make_int(isAuto)});
}

void handle_vm_cmdSetRecPerson(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t nameLen = body[off++];
    if (off + nameLen > len) return;
    const uint8_t* name = body + off;
    off += nameLen;
    if (off + 1 > len) return;
    uint8_t gidLen = body[off++];
    if (off + gidLen > len) return;
    applyLpc("cmd_set_rec_person", {make_int(cl.clientNo), make_string(name, nameLen), make_string(body + off, gidLen)});
}

void handle_vm_cmdSetRegion(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t addrLen = body[0];
    if ((size_t)1 + addrLen > len) return;
    applyLpc("cmd_set_region", {make_int(cl.clientNo), make_string(body + 1, addrLen)});
}

void handle_vm_cmdSetSafeLoginTime(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t safeLoginTime = rd16be(body, 0);
    applyLpc("cmd_set_safe_login_time", {make_int(cl.clientNo), make_int(safeLoginTime)});
}

void handle_vm_cmdSetSafeMechanism(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint16_t count = rd16be(body, off); off += 2;
    uint16_t num = rd16be(body, off); off += 2;
    if (off + 1 > len) return;
    uint8_t questionLen = body[off++];
    if (off + questionLen > len) return;
    const uint8_t* question = body + off;
    off += questionLen;
    if (off + 1 > len) return;
    uint8_t answerLen = body[off++];
    if (off + answerLen > len) return;
    applyLpc("cmd_set_safe_mechanism", {make_int(cl.clientNo), make_int(count), make_int(num), make_string(question, questionLen), make_string(body + off, answerLen)});
}

void handle_vm_cmdSetSafeState(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t safeState = rd16be(body, 0);
    applyLpc("cmd_set_safe_state", {make_int(cl.clientNo), make_int(safeState)});
}

void handle_vm_cmdSetSetting(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t settingLen = body[0];
    if ((size_t)1 + settingLen + 2 > len) return;
    const uint8_t* setting = body + 1;
    uint16_t param = rd16be(body, 1 + settingLen);
    applyLpc("cmd_set_setting", {make_int(cl.clientNo), make_string(setting, settingLen), make_int(param)});
}

void handle_vm_cmdSetSignature(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t msgLen = body[0];
    if ((size_t)1 + msgLen > len) return;
    applyLpc("cmd_set_individuality_signature", {make_int(cl.clientNo), make_string(body + 1, msgLen)});
}

void handle_vm_cmdSetStallGoods(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 12) return;
    uint32_t goodsId = rd32be(body, 0);
    uint16_t pos = rd16be(body, 4);
    uint32_t price = rd32be(body, 6);
    uint16_t amount = rd16be(body, 10);
    applyLpc("cmd_set_stall_goods", {make_int(cl.clientNo), make_int(goodsId), make_int(pos), make_int(price), make_int(amount)});
}

void handle_vm_cmdSetTaskTrace(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t sLen = body[0];
    if (1 + sLen + 1 > len) return;
    const uint8_t* str = body + 1;
    uint8_t val = body[1 + sLen];
    applyLpc("cmd_set_task_trace", {make_int(cl.clientNo), make_string(str, sLen), make_int(val)});
}

void handle_vm_cmdSetUserMood(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t no = body[0];
    uint8_t sLen = body[1];
    if (2 + sLen > len) return;
    applyLpc("cmd_set_user_mood", {make_int(cl.clientNo), make_int(no), make_string(body + 2, sLen)});
}

void handle_vm_cmdSetUserState(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_set_user_state", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdSetWeddingList(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t count = rd16be(body, 0);
    if (count > 0x3F) return;
    size_t off = 2;
    auto arr = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < count; ++i)
    {
        if (off >= len) return;
        uint8_t sLen = body[off++];
        if (off + sLen > len) return;
        arr->items.push_back(make_string(body + off, sLen));
        off += sLen;
    }
    applyLpc("cmd_set_wedding_list", {make_int(cl.clientNo), arr});
}

void handle_vm_cmdSetWeddingPacket(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 10) return;
    uint32_t num = rd32be(body, 0);
    uint32_t cash = rd32be(body, 4);
    uint16_t lastTime = rd16be(body, 8);
    applyLpc("cmd_set_wedding_packet", {make_int(cl.clientNo), make_int(num), make_int(cash), make_int(lastTime)});
}

void handle_vm_cmdSetWeiboInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nLen = body[0];
    if (1 + nLen > len) return;
    const uint8_t* name = body + 1;
    size_t off = 1 + nLen;
    if (off >= len) return;
    uint8_t tLen = body[off++];
    if (off + tLen > len) return;
    applyLpc("cmd_set_weibo_info", {make_int(cl.clientNo), make_string(name, nLen), make_string(body + off, tLen)});
}

void handle_vm_cmdSetXinfaSkillStatus(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 3) return;
    uint16_t petPos = rd16be(body, 0);
    uint8_t sLen = body[2];
    if (3 + sLen + 1 > len) return;
    const uint8_t* name = body + 3;
    uint8_t status = body[3 + sLen];
    applyLpc("cmd_set_xinfa_skill_status", {make_int(cl.clientNo), make_int(petPos), make_string(name, sLen), make_int(status)});
}

void handle_vm_cmdShift(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 10) return;
    uint32_t arg1 = rd32be(body, 0);
    uint16_t arg2 = rd16be(body, 4);
    uint16_t arg3 = rd16be(body, 6);
    uint16_t arg4 = rd16be(body, 8);
    applyLpc("cmd_shift", {make_int(cl.clientNo), make_int(arg1), make_int(arg2), make_int(arg3), make_int(arg4)});
}

void handle_vm_cmdShopAdEnter(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t sLen = body[0];
    if (1 + sLen > len) return;
    applyLpc("cmd_shop_ad_enter", {make_int(cl.clientNo), make_string(body + 1, sLen)});
}

void handle_vm_cmdShopAdFill(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    if (1 + (size_t)slen + 4 > len) return;
    size_t off = 1 + slen;
    applyLpc("cmd_shop_ad_fill", {make_int(cl.clientNo), make_string(body + 1, slen), make_int((int)rd32be(body, off))});
}

void handle_vm_cmdShopBuyProduct(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    size_t off = 1;
    if (off + (size_t)slen + 4 > len) return;
    off += slen;
    uint8_t counter_no = body[off++];
    uint8_t pos = body[off++];
    uint8_t count = body[off++];
    uint8_t clen = body[off++];
    if (off + (size_t)clen > len) return;
    applyLpc("cmd_shop_buy_product", {make_int(cl.clientNo), make_string(body + 1, slen), make_int(counter_no), make_int(pos), make_int(count), make_string(body + off, clen)});
}

void handle_vm_cmdShopBuyShop(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    size_t off = 1;
    if (off + (size_t)slen + 1 > len) return;
    off += slen;
    uint8_t tlen = body[off++];
    if (off + (size_t)tlen > len) return;
    applyLpc("cmd_shop_buy_shop", {make_int(cl.clientNo), make_string(body + 1, slen), make_string(body + off, tlen)});
}

void handle_vm_cmdShopChangeInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    size_t off = 1;
    if (off + (size_t)slen + 2 > len) return;
    off += slen;
    uint8_t modify_type = body[off++];
    uint8_t nlen = body[off++];
    if (off + (size_t)nlen > len) return;
    applyLpc("cmd_shop_change_info", {make_int(cl.clientNo), make_string(body + 1, slen), make_int(modify_type), make_string(body + off, nlen)});
}

void handle_vm_cmdShopDoBusiness(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    size_t off = 1;
    if (off + (size_t)slen + 1 > len) return;
    off += slen;
    applyLpc("cmd_shop_do_business", {make_int(cl.clientNo), make_string(body + 1, slen), make_int(body[off])});
}

void handle_vm_cmdShopEnterShop(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    if (1 + (size_t)slen > len) return;
    applyLpc("cmd_shop_enter_shop", {make_int(cl.clientNo), make_string(body + 1, slen)});
}

void handle_vm_cmdShopEnterShopType(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    if (1 + (size_t)slen > len) return;
    applyLpc("cmd_shop_enter_shop_type", {make_int(cl.clientNo), make_string(body + 1, slen)});
}

void handle_vm_cmdShopFavorite(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    size_t off = 1;
    if (off + (size_t)slen + 1 > len) return;
    off += slen;
    applyLpc("cmd_shop_favorite", {make_int(cl.clientNo), make_string(body + 1, slen), make_int(body[off])});
}

void handle_vm_cmdShopManageCounter(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    size_t off = 1;
    if (off + (size_t)slen + 2 > len) return;
    off += slen;
    uint8_t counter_no = body[off++];
    uint8_t count = body[off];
    applyLpc("cmd_shop_manage_counter", {make_int(cl.clientNo), make_string(body + 1, slen), make_int(counter_no), make_int(count)});
}

void handle_vm_cmdShopManageMoney(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    size_t off = 1;
    if (off + (size_t)slen + 5 > len) return;
    off += slen;
    uint8_t manage_type = body[off++];
    applyLpc("cmd_shop_manage_money", {make_int(cl.clientNo), make_string(body + 1, slen), make_int(manage_type), make_int((int)rd32be(body, off))});
}

void handle_vm_cmdPreviewPetPracticeBonus(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    applyLpc("cmd_preview_pet_practice_bonus",
        {make_int(cl.clientNo), make_int(body[0]), make_int(body[1]), make_int(rd16be(body, 2))});
}

void handle_vm_cmdPrompt(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t nlen = body[0];
    if (1 + nlen > len) return;
    const uint8_t* name = body + 1;
    size_t off = 1 + nlen;
    if (off >= len) return;
    uint8_t rlen = body[off];
    if (off + 1 + rlen > len) return;
    const uint8_t* right = body + off + 1;
    applyLpc("cmd_prompt",
        {make_int(cl.clientNo), make_string(name, nlen), make_string(right, rlen)});
}

void handle_vm_cmdPromptBeforeLeave(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_prompt_before_leave", {make_int(cl.clientNo)});
}

void handle_vm_cmdPurchaseAddOrder(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t type = body[0];
    size_t off = 1;
    if (off >= len) return;
    uint8_t nlen = body[off]; ++off;
    if (off + nlen > len) return;
    const uint8_t* name = body + off; off += nlen;
    if (off + 2 > len) return;
    uint16_t amount = rd16be(body, off); off += 2;
    if (off + 4 > len) return;
    uint32_t price = rd32be(body, off); off += 4;
    if (off >= len) return;
    uint8_t olen = body[off]; ++off;
    if (off + olen > len) return;
    const uint8_t* orderInfo = body + off;
    applyLpc("cmd_purchase_add_order",
        {make_int(cl.clientNo), make_int(type), make_string(name, nlen),
         make_int(amount), make_int(price), make_string(orderInfo, olen)});
}

void handle_vm_cmdPurchaseCancleOrder(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t type = body[0];
    uint8_t olen = body[1];
    if (2 + olen > len) return;
    const uint8_t* orderId = body + 2;
    applyLpc("cmd_purchase_cancle_order",
        {make_int(cl.clientNo), make_int(type), make_string(orderId, olen)});
}

void handle_vm_cmdPurchaseFetchCash(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_purchase_fetch_cash", {make_int(cl.clientNo)});
}

void handle_vm_cmdPurchaseFetchItem(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t type = body[0];
    uint8_t olen = body[1];
    if (2 + olen > len) return;
    const uint8_t* orderId = body + 2;
    applyLpc("cmd_purchase_fetch_item",
        {make_int(cl.clientNo), make_int(type), make_string(orderId, olen)});
}

void handle_vm_cmdPurchaseGetObjProp(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t type = body[0];
    uint8_t olen = body[1];
    if (2 + olen > len) return;
    const uint8_t* orderId = body + 2;
    applyLpc("cmd_purchase_get_obj_prop",
        {make_int(cl.clientNo), make_int(type), make_string(orderId, olen)});
}

void handle_vm_cmdPurchaseGetOrderList(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_purchase_get_order_list",
        {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdPurchaseMatchOrder(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t type = body[0];
    uint8_t olen = body[1];
    size_t off = 2 + olen;
    if (off + 4 > len) return;
    uint16_t v12 = rd16be(body, off);
    uint16_t v14 = rd16be(body, off + 2);
    applyLpc("cmd_purchase_match_order",
        {make_int(cl.clientNo), make_int(type), make_string(body + 2, olen),
         make_int(v12), make_int(v14)});
}

void handle_vm_cmdPurchaseMultiMatch(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t off = 0;
    uint8_t count = body[off++];
    auto arr = std::make_shared<ArrayValue>();
    for (uint8_t i = 0; i < count; i++) {
        if (off >= len) return;
        uint8_t slen = body[off++];
        if (off + slen > len) return;
        arr->items.push_back(make_string(body + off, slen));
        off += slen;
        if (off + 1 > len) return;
        arr->items.push_back(make_int(body[off++]));
        if (off + 2 > len) return;
        arr->items.push_back(make_int(rd16be(body, off)));
        off += 2;
        if (off + 2 > len) return;
        arr->items.push_back(make_int(rd16be(body, off)));
        off += 2;
    }
    applyLpc("cmd_purchase_multi_match",
        {make_int(cl.clientNo), arr});
}

void handle_vm_cmdPutChildItemDown(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    applyLpc("cmd_put_child_item_down",
        {make_int(cl.clientNo), make_int(rd32be(body, 0))});
}

void handle_vm_cmdPutDown(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_put_down", {make_int(cl.clientNo)});
}

void handle_vm_cmdPutInItem(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 8) return;
    applyLpc("cmd_putin_item",
        {make_int(cl.clientNo), make_int(rd32be(body, 0)), make_int(rd32be(body, 4))});
}

void handle_vm_cmdQueryActivity(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 6) return;
    applyLpc("cmd_query_activity",
        {make_int(cl.clientNo), make_int(rd16be(body, 0)), make_int(rd16be(body, 2)), make_int(rd16be(body, 4))});
}

void handle_vm_cmdQueryPracticeChildren(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_query_practice_children", {make_int(cl.clientNo)});
}

void handle_vm_cmdQueryPracticePets(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_query_practice_pets", {make_int(cl.clientNo)});
}

void handle_vm_cmdQueryRecruitInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    uint8_t tlen = body[off++];
    if (off + tlen > len) return;
    const uint8_t* type = body + off;
    off += tlen;
    if (off >= len) return;
    uint8_t nlen = body[off++];
    if (off + nlen > len) return;
    applyLpc("cmd_query_recruit_info",
        {make_int(cl.clientNo), make_string(type, tlen), make_string(body + off, nlen)});
}

void handle_vm_cmdQueryReportInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nlen = body[0];
    if (1 + nlen > len) return;
    applyLpc("cmd_admin_query_report_info",
        {make_int(cl.clientNo), make_string(body + 1, nlen)});
}

void handle_vm_cmdQueryUpgradeCash(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 6) return;
    size_t off = 6;
    auto arr = std::make_shared<ArrayValue>();
    for (int i = 0; i < 4; i++) {
        if (off + 4 > len) return;
        arr->items.push_back(make_int(rd32be(body, off)));
        off += 4;
    }
    applyLpc("cmd_query_upgrade_cash",
        {make_int(cl.clientNo), make_int(rd32be(body, 0)), make_int(rd16be(body, 4)), arr});
}

void handle_vm_cmdQueryUpgradeCashEx(GameClient& cl, const uint8_t* body, size_t len)
{
    uint32_t npcId = 0;
    uint16_t type = 0;
    auto arr = std::make_shared<ArrayValue>();
    if (len >= 4)
    {
        npcId = rd32be(body, 0);
        if (len >= 6)
        {
            type = rd16be(body, 4);
            size_t off = 6;
            if (off < len)
            {
                uint8_t count = body[off++];
                for (uint8_t i = 0; i < count && off + 4 <= len; ++i, off += 4)
                    arr->items.push_back(make_int((int)rd32be(body, off)));
            }
        }
    }
    applyLpc("cmd_query_upgrade_cash", {make_int(cl.clientNo), make_int((int)npcId), make_int(type), arr});
}

void handle_vm_cmdQueryUpgradeCoin(GameClient& cl, const uint8_t* body, size_t len)
{
    uint32_t npcId = 0;
    uint16_t type = 0;
    auto arr = std::make_shared<ArrayValue>();
    if (len >= 4)
    {
        npcId = rd32be(body, 0);
        if (len >= 6)
        {
            type = rd16be(body, 4);
            size_t off = 6;
            if (off < len)
            {
                uint8_t count = body[off++];
                for (uint8_t i = 0; i < count && off + 4 <= len; ++i, off += 4)
                    arr->items.push_back(make_int((int)rd32be(body, off)));
            }
        }
    }
    applyLpc("cmd_query_upgrade_coin", {make_int(cl.clientNo), make_int((int)npcId), make_int(type), arr});
}

void handle_vm_cmdQuitChatGroup(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    if (1 + slen > len) return;
    applyLpc("cmd_quit_chat_group", {make_int(cl.clientNo), make_string(body + 1, slen)});
}

void handle_vm_cmdQuitLookOn(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_quit_look_on", {make_int(cl.clientNo)});
}

void handle_vm_cmdQuitLookonCombat(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_quit_lookon_combat", {make_int(cl.clientNo)});
}

void handle_vm_cmdQuitTeam(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_quit_team", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdRaceFeedNpc(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    applyLpc("cmd_race_feed_npc", {make_int(cl.clientNo), make_int((int)rd32be(body, 0))});
}

void handle_vm_cmdRaceHitNpc(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    applyLpc("cmd_race_hit_npc", {make_int(cl.clientNo), make_int((int)rd32be(body, 0))});
}

void handle_vm_cmdRaceRequestBonus(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_race_request_bonus", {make_int(cl.clientNo)});
}

void handle_vm_cmdRaidKickoutMember(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    if (1 + slen > len) return;
    applyLpc("cmd_raid_kickout_member", {make_int(cl.clientNo), make_string(body + 1, slen)});
}

void handle_vm_cmdRaidLeaveRaid(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_raid_leave_raid", {make_int(cl.clientNo)});
}

void handle_vm_cmdRandomUserName(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_build_random_user_name", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdReceiveAskTaoShowGift(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t type = body[0];
    uint8_t idLen = body[1];
    if (2 + idLen > len) return;
    applyLpc("cmd_receive_asktao_show_gift", {make_int(cl.clientNo), make_int(type), make_string(body + 2, idLen)});
}

void handle_vm_cmdRecharge(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t noLen = body[off++];
    if (off + noLen > len) return;
    const uint8_t* ecardNo = body + off; off += noLen;
    if (off + 1 > len) return;
    uint8_t pwdLen = body[off++];
    if (off + pwdLen > len) return;
    const uint8_t* ecardPwd = body + off; off += pwdLen;
    if (off + 1 > len) return;
    uint8_t v15 = body[off++];
    if (off + 4 > len) return;
    uint32_t v20 = rd32be(body, off); off += 4;
    if (off + 1 > len) return;
    uint8_t v17 = body[off];
    applyLpc("cmd_recharge", {make_int(cl.clientNo), make_string(ecardNo, noLen), make_string(ecardPwd, pwdLen), make_int(v15), make_int((int)v20), make_int(v17)});
}

void handle_vm_cmdReciveInstructionTask(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nameLen = body[0];
    if (1 + nameLen > len) return;
    applyLpc("cmd_recive_instruction_task", {make_int(cl.clientNo), make_string(body + 1, nameLen)});
}

void handle_vm_cmdRecoverAskTaoShow(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_recover_asktao_show", {make_int(cl.clientNo)});
}

void handle_vm_cmdRecoverChildBook(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    applyLpc("cmd_recover_child_book", {make_int(cl.clientNo), make_int((int)rd32be(body, 0))});
}

void handle_vm_cmdRecoverMagicPennant(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_recover_magic_pennant", {make_int(cl.clientNo)});
}

void handle_vm_cmdRefresh(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_refresh", {make_int(cl.clientNo)});
}

void handle_vm_cmdRefreshFriend(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t gLen = body[off++];
    if (off + gLen > len) return;
    const uint8_t* group = body + off; off += gLen;
    if (off + 1 > len) return;
    uint8_t nLen = body[off++];
    if (off + nLen > len) return;
    const uint8_t* name = body + off; off += nLen;
    std::string gid;
    if (off < len)
    {
        uint8_t idLen = body[off++];
        if (off + idLen > len) return;
        gid.assign((const char*)body + off, idLen);
    }
    applyLpc("cmd_refresh_friend", {make_int(cl.clientNo), make_string(group, gLen), make_string(name, nLen), make_string(gid)});
}

void handle_vm_cmdrefreshmountattrib(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t mountId = rd32be(body, 0);
    applyLpc("cmd_refresh_mount_attrib", {make_int(cl.clientNo), make_int(mountId)});
}

void handle_vm_cmdRefreshpartyFragment(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_refresh_party_fragment", {make_int(cl.clientNo)});
}

void handle_vm_cmdRefreshPetGodbookSkills(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t petId = rd32be(body, 0);
    applyLpc("cmd_refresh_pet_godbook_skills", {make_int(cl.clientNo), make_int(petId)});
}

void handle_vm_cmdRefreshRecruitmentInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t tl = body[0];
    if (1 + tl > len) return;
    applyLpc("cmd_refresh_recruitment_info", {make_int(cl.clientNo), make_string(body + 1, tl)});
}

void handle_vm_cmdRefreshSafeLogin(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_refresh_safe_login", {make_int(cl.clientNo)});
}

void handle_vm_cmdRefreshServiceLog(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t sl = body[0];
    if (1 + sl > len) return;
    applyLpc("cmd_refresh_service_log", {make_int(cl.clientNo), make_string(body + 1, sl)});
}

void handle_vm_cmdRefreshTaskLog(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t tl = body[0];
    if (len < 2 + tl) return;
    applyLpc("cmd_refresh_task_log", {make_int(cl.clientNo), make_string(body + 1, tl), make_int(body[1 + tl])});
}

void handle_vm_cmdReject(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nl = body[0];
    if (len < 1 + nl) return;
    const uint8_t* p = body + 1 + nl;
    size_t rem = len - (1 + nl);
    if (rem < 1) return;
    uint8_t al = p[0];
    if (rem < 1 + al) return;
    applyLpc("cmd_reject", {make_int(cl.clientNo), make_string(body + 1, nl), make_string(p + 1, al)});
}

void handle_vm_cmdRemoveAskTaoShowGift(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t type = body[0];
    uint8_t il = body[1];
    if (len < 2 + il) return;
    applyLpc("cmd_remove_asktao_show_gift", {make_int(cl.clientNo), make_int(type), make_string(body + 2, il)});
}

void handle_vm_cmdRemoveBureauItem(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t itemId = rd16be(body, 0);
    applyLpc("cmd_remove_bureau_item", {make_int(cl.clientNo), make_int(itemId)});
}

void handle_vm_cmdRemoveChatGroupMember(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t gLen = body[0];
    if (1 + gLen > len) return;
    const uint8_t* group = body + 1;
    size_t off = 1 + gLen;
    if (off >= len) return;
    uint8_t nLen = body[off];
    if (off + 1 + nLen > len) return;
    const uint8_t* name = body + off + 1;
    applyLpc("cmd_remove_chat_group_member", {make_int(cl.clientNo), make_string(group, gLen), make_string(name, nLen)});
}

void handle_vm_cmdRemoveFriend(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t gLen = body[0];
    if (1 + gLen > len) return;
    const uint8_t* group = body + 1;
    size_t off = 1 + gLen;
    if (off >= len) return;
    uint8_t nLen = body[off];
    if (off + 1 + nLen > len) return;
    const uint8_t* name = body + off + 1;
    applyLpc("cmd_remove_friend", {make_int(cl.clientNo), make_string(group, gLen), make_string(name, nLen)});
}

void handle_vm_cmdRenameStall(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nLen = body[0];
    if (1 + nLen > len) return;
    const uint8_t* name = body + 1;
    applyLpc("cmd_rename_stall", {make_int(cl.clientNo), make_string(name, nLen)});
}

void handle_vm_cmdRepair(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 6) return;
    uint32_t itemId = rd32be(body, 0);
    uint16_t repairType = rd16be(body, 4);
    applyLpc("cmd_repair", {make_int(cl.clientNo), make_int(itemId), make_int(repairType)});
}

void handle_vm_cmdRepairFurniture(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    uint8_t furnitureType = body[0];
    uint32_t itemId = rd32be(body, 1);
    applyLpc("cmd_repair_furniture", {make_int(cl.clientNo), make_int(furnitureType), make_int(itemId)});
}

void handle_vm_cmdReplaceLogin(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_replace_login", {make_int(cl.clientNo)});
}

void handle_vm_cmdReplyClientDoubtInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t type = body[0];
    uint8_t nLen = body[1];
    if (2 + nLen + 2 > len) return;
    const uint8_t* name = body + 2;
    size_t off = 2 + nLen;
    uint16_t mLen = rd16be(body, off);
    if (off + 2 + mLen > len) return;
    const uint8_t* msg = body + off + 2;
    applyLpc("cmd_reply_client_doubt_info", {make_int(cl.clientNo), make_int(type), make_string(name, nLen), make_string(msg, mLen)});
}

void handle_vm_cmdReplyClientProcessInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 3) return;
    uint8_t type = body[0];
    uint16_t sLen = rd16be(body, 1);
    if (3 + sLen > len) return;
    const uint8_t* info = body + 3;
    applyLpc("cmd_reply_client_process_info", {make_int(cl.clientNo), make_int(type), make_string(info, sLen)});
}

void handle_vm_cmdReplyFriendVerify(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nLen = body[0];
    if (1 + nLen + 1 > len) return;
    const uint8_t* charName = body + 1;
    size_t off = 1 + nLen;
    uint8_t pass = body[off];
    ++off;
    if (off >= len) return;
    uint8_t mLen = body[off];
    ++off;
    if (off + mLen > len) return;
    const uint8_t* reqMsg = body + off;
    off += mLen;
    if (off >= len) return;
    uint8_t rLen = body[off];
    ++off;
    if (off + rLen > len) return;
    const uint8_t* requested = body + off;
    applyLpc("cmd_reply_friend_verify", {make_int(cl.clientNo), make_string(charName, nLen), make_int(pass), make_string(reqMsg, mLen), make_string(requested, rLen)});
}

void handle_vm_cmdReplyRequestAsktaoShow(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 3) return;
    uint8_t operType = body[0];
    uint16_t sLen = rd16be(body, 1);
    if (3 + sLen > len) return;
    const uint8_t* ids = body + 3;
    applyLpc("cmd_reply_request_asktao_show", {make_int(cl.clientNo), make_int(operType), make_string(ids, sLen)});
}

void handle_vm_cmdReportCheater(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t nameLen = body[off++];
    if (off + nameLen > len) return;
    const uint8_t* name = body + off;
    off += nameLen;
    if (off + 1 > len) return;
    uint8_t msgLen = body[off++];
    if (off + msgLen > len) return;
    const uint8_t* msg = body + off;
    off += msgLen;
    applyLpc("cmd_report_cheater", {make_int(cl.clientNo), make_string(name, nameLen), make_string(msg, msgLen)});
}

void handle_vm_cmdReportIllegal(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t nameLen = body[off++];
    if (off + nameLen > len) return;
    const uint8_t* name = body + off;
    off += nameLen;
    if (off + 1 > len) return;
    uint8_t msgLen = body[off++];
    if (off + msgLen > len) return;
    const uint8_t* msg = body + off;
    off += msgLen;
    if (off + 1 > len) return;
    uint8_t type = body[off++];
    applyLpc("cmd_report_illegal", {make_int(cl.clientNo), make_string(name, nameLen), make_string(msg, msgLen), make_int(type)});
}

void handle_vm_cmdReportIllegalTool(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t nameLen = body[off++];
    if (off + nameLen > len) return;
    applyLpc("cmd_report_illegal_tool", {make_int(cl.clientNo), make_string(body + off, nameLen)});
}

void handle_vm_cmdRepurchase(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    uint8_t b0 = body[0];
    uint32_t v7 = rd32be(body, 1);
    applyLpc("cmd_repurchase", {make_int(cl.clientNo), make_int(b0), make_int(v7)});
}

void handle_vm_cmdRequestAddStallPage(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_request_add_stall_page", {make_int(cl.clientNo)});
}

void handle_vm_cmdRequestAskTaoShow(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t oppLen = body[off++];
    if (off + oppLen > len) return;
    const uint8_t* opponent = body + off;
    off += oppLen;
    if (off + 1 > len) return;
    uint8_t barLen = body[off++];
    if (off + barLen > len) return;
    const uint8_t* barcode = body + off;
    off += barLen;
    if (off + 2 > len) return;
    uint16_t showType = rd16be(body, off);
    applyLpc("cmd_request_asktao_show", {make_int(cl.clientNo), make_string(opponent, oppLen), make_string(barcode, barLen), make_int(showType)});
}

void handle_vm_cmdRequestBuildDeck(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    size_t off = 0;
    uint16_t no = rd16be(body, off);
    uint16_t count = rd16be(body, off + 2);
    off += 4;
    if (count > 63) return;
    auto arr = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < count; ++i)
    {
        if (off + 1 > len) return;
        uint8_t clen = body[off++];
        if (off + clen > len) return;
        arr->items.push_back(make_string(body + off, clen));
        off += clen;
    }
    applyLpc("cmd_request_build_deck", {make_int(cl.clientNo), make_int(no), arr});
}

void handle_vm_cmdRequestBuyCardPackage(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 6) return;
    uint16_t v6 = rd16be(body, 0);
    uint32_t v8 = rd32be(body, 2);
    applyLpc("cmd_request_buy_card_package", {make_int(cl.clientNo), make_int(v6), make_int(v8)});
}

void handle_vm_cmdRequestCancelMatchCardGame(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_request_cancel_match_card_game", {make_int(cl.clientNo)});
}

void handle_vm_cmdRequestCardActivityList(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_request_card_activity_list", {make_int(cl.clientNo)});
}

void handle_vm_cmdRequestCardGameSortDeck(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    if (1 + (size_t)slen > len) return;
    applyLpc("cmd_request_card_game_sort_deck", {make_int(cl.clientNo), make_string(body + 1, slen)});
}

void handle_vm_cmdRequestCardGameSortList(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    applyLpc("cmd_request_card_game_sort_list", {make_int(cl.clientNo), make_int((int)rd32be(body, 0))});
}

void handle_vm_cmdRequestCardList(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_request_card_list", {make_int(cl.clientNo)});
}

void handle_vm_cmdRequestCardTodayStat(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_request_card_today_stat", {make_int(cl.clientNo)});
}

void handle_vm_cmdRequestChatGroupInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    if (1 + (size_t)slen > len) return;
    applyLpc("cmd_request_chat_group_info", {make_int(cl.clientNo), make_string(body + 1, slen)});
}

void handle_vm_cmdRequestCityBuildingInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t cityLen = body[0];
    if (1 + (size_t)cityLen >= len) return;
    size_t off = 1 + cityLen;
    uint8_t bldLen = body[off];
    if (off + 1 + (size_t)bldLen > len) return;
    applyLpc("cmd_request_city_building_info", {make_int(cl.clientNo), make_string(body + 1, cityLen), make_string(body + off + 1, bldLen)});
}

void handle_vm_cmdRequestCityInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    if (1 + (size_t)slen > len) return;
    applyLpc("cmd_request_city_info", {make_int(cl.clientNo), make_string(body + 1, slen)});
}

void handle_vm_cmdRequestCityList(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_request_city_list", {make_int(cl.clientNo)});
}

void handle_vm_cmdRequestCombatDeckList(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_request_combat_deck_list", {make_int(cl.clientNo)});
}

void handle_vm_cmdRequestComposeCard(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    if (1 + (size_t)slen > len) return;
    applyLpc("cmd_request_compose_card", {make_int(cl.clientNo), make_string(body + 1, slen)});
}

// ---- game commands (type=1) generated handlers, chunk 4 ----

void handle_vm_cmdShopMoveMerch(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t shop_id_len = body[off++];
    if (off + shop_id_len > len) return;
    auto shop_id = make_string(body + off, shop_id_len);
    off += shop_id_len;
    if (off + 4 > len) return;
    uint8_t v1 = body[off++];
    uint8_t v2 = body[off++];
    uint8_t v3 = body[off++];
    uint8_t v4 = body[off++];
    applyLpc("cmd_shop_move_merch", {make_int(cl.clientNo), shop_id, make_int(v1), make_int(v2), make_int(v3), make_int(v4)});
}

void handle_vm_cmdShopQueryCounter(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t shop_id_len = body[off++];
    if (off + shop_id_len > len) return;
    auto shop_id = make_string(body + off, shop_id_len);
    off += shop_id_len;
    if (off + 2 > len) return;
    uint8_t v1 = body[off++];
    uint8_t v2 = body[off++];
    applyLpc("cmd_shop_query_counter", {make_int(cl.clientNo), shop_id, make_int(v1), make_int(v2)});
}

void handle_vm_cmdShopReqRunnings(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t shop_type_len = body[off++];
    if (off + shop_type_len > len) return;
    auto shop_type = make_string(body + off, shop_type_len);
    applyLpc("cmd_shop_req_runnings", {make_int(cl.clientNo), shop_type});
}

void handle_vm_cmdShopReqVendues(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t shop_type_len = body[off++];
    if (off + shop_type_len > len) return;
    auto shop_type = make_string(body + off, shop_type_len);
    applyLpc("cmd_shop_req_vendues", {make_int(cl.clientNo), shop_type});
}

void handle_vm_cmdShopSale(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t shop_id_len = body[off++];
    if (off + shop_id_len > len) return;
    auto shop_id = make_string(body + off, shop_id_len);
    off += shop_id_len;
    if (off + 1 > len) return;
    uint8_t shop_type_len = body[off++];
    if (off + shop_type_len > len) return;
    auto shop_type = make_string(body + off, shop_type_len);
    off += shop_type_len;
    if (off + 11 > len) return;
    uint8_t v1 = body[off++];
    uint8_t counter_no = body[off++];
    uint8_t counter_pos = body[off++];
    uint32_t v26 = rd32be(body, off); off += 4;
    uint32_t v30 = rd32be(body, off); off += 4;
    uint8_t v_last = body[off++];
    applyLpc("cmd_shop_sale", {make_int(cl.clientNo), shop_id, shop_type, make_int(v1), make_int(counter_no), make_int(counter_pos), make_int(v26), make_int(v30), make_int(v_last)});
}

void handle_vm_cmdShopSearch(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t index_len = body[off++];
    if (off + index_len > len) return;
    auto index = make_string(body + off, index_len);
    off += index_len;
    if (off + 1 > len) return;
    uint8_t v1 = body[off++];
    applyLpc("cmd_shop_search", {make_int(cl.clientNo), index, make_int(v1)});
}

void handle_vm_cmdShopSubmit(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t shop_type_len = body[off++];
    if (off + shop_type_len > len) return;
    auto shop_type = make_string(body + off, shop_type_len);
    off += shop_type_len;
    if (off + 1 > len) return;
    uint8_t shop_name_len = body[off++];
    if (off + shop_name_len > len) return;
    auto shop_name = make_string(body + off, shop_name_len);
    applyLpc("cmd_shop_submit", {make_int(cl.clientNo), shop_type, shop_name});
}

void handle_vm_cmdShopVendueShop(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t shop_id_len = body[off++];
    if (off + shop_id_len > len) return;
    auto shop_id = make_string(body + off, shop_id_len);
    off += shop_id_len;
    if (off + 1 > len) return;
    uint8_t v1 = body[off++];
    applyLpc("cmd_shop_vendue_shop", {make_int(cl.clientNo), shop_id, make_int(v1)});
}

void handle_vm_cmdShopViewOwnShop(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t shop_id_len = body[off++];
    if (off + shop_id_len > len) return;
    auto shop_id = make_string(body + off, shop_id_len);
    applyLpc("cmd_shop_view_own_shop", {make_int(cl.clientNo), shop_id});
}

void handle_vm_cmdShopViewSaleLog(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t shop_id_len = body[off++];
    if (off + shop_id_len > len) return;
    auto shop_id = make_string(body + off, shop_id_len);
    off += shop_id_len;
    if (off + 1 > len) return;
    uint8_t v1 = body[off++];
    applyLpc("cmd_shop_view_sale_log", {make_int(cl.clientNo), shop_id, make_int(v1)});
}

void handle_vm_cmdShout(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t mlen = body[0];
    if (1 + mlen > len) return;
    applyLpc("cmd_shout", {make_int(cl.clientNo), make_string(body + 1, mlen)});
}

void handle_vm_cmdShouWuZuDao(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 8) return;
    int mode_type = body[0];
    int map_id = body[1];
    int npc_id = rd32be(body, 2);
    int type = body[6];
    size_t slen = body[7];
    if (8 + slen > len) return;
    applyLpc("cmd_shouwu_zudao", {make_int(cl.clientNo), make_int(mode_type), make_int(map_id), make_int(npc_id), make_int(type), make_string(body + 8, slen)});
}

void handle_vm_cmdShowSaleLog(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_show_sale_log", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdShowUpgradeEquipItems(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_show_upgrade_equip_items", {make_int(cl.clientNo)});
}

void handle_vm_cmdSignIn(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_sign_in", {make_int(cl.clientNo)});
}

void handle_vm_cmdSignInDraw(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_sign_in_draw", {make_int(cl.clientNo)});
}

void handle_vm_cmdSignInOpen(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_sign_in_open", {make_int(cl.clientNo)});
}

void handle_vm_cmdSkillLevelDown(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 12) return;
    int masterId = rd32be(body, 0);
    int studentId = rd32be(body, 4);
    int skillId = rd16be(body, 8);
    int level = rd16be(body, 10);
    applyLpc("cmd_skill_level_down", {make_int(cl.clientNo), make_int(masterId), make_int(studentId), make_int(skillId), make_int(level)});
}

void handle_vm_cmdSortpack(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    int count = rd16be(body, 0);
    size_t slen = rd16be(body, 2);
    if (4 + slen + 6 > len) return;
    int npc_id = rd32be(body, 4 + slen);
    int u16v = rd16be(body, 8 + slen);
    applyLpc("cmd_sort_pack", {make_int(cl.clientNo), make_int(count), make_string(body + 4, slen), make_int(npc_id), make_int(u16v)});
}

void handle_vm_cmdSpecialMoveEvent(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t nlen = body[0];
    if (1 + nlen > len) return;
    applyLpc("cmd_special_move_event", {make_int(cl.clientNo), make_string(body + 1, nlen)});
}

void handle_vm_cmdVendueRefresh(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 3) return;
    applyLpc("cmd_vendue_refresh", {make_int(cl.clientNo), make_int(body[0]), make_int(body[1]), make_int(body[2])});
}

void handle_vm_cmdVendueStoreClosed(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_vendue_store_closed", {make_int(cl.clientNo)});
}

void handle_vm_cmdVendueTryVendue(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_vendue_try_vendue", {make_int(cl.clientNo)});
}

void handle_vm_cmdVendueVendueMerch(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 14) return;
    applyLpc("cmd_vendue_vendue_merch", {make_int(cl.clientNo), make_int(body[0]), make_int(body[1]), make_int(rd32be(body, 2)), make_int(rd32be(body, 6)), make_int(rd32be(body, 10))});
}

void handle_vm_cmdVerifyCharName(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nl = body[0];
    if (len < 1u + nl) return;
    applyLpc("cmd_verify_char_name", {make_int(cl.clientNo), make_string(body + 1, nl)});
}

void handle_vm_cmdViewAchieve(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nl = body[0];
    if (len < 1u + nl) return;
    applyLpc("cmd_view_achieve", {make_int(cl.clientNo), make_string(body + 1, nl)});
}

void handle_vm_cmdViewAchieveOverview(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_view_achieve_overriew", {make_int(cl.clientNo)});
}

void handle_vm_cmdViewCouponShow(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nl = body[0];
    if (len < 1u + nl) return;
    applyLpc("cmd_view_coupon_show", {make_int(cl.clientNo), make_string(body + 1, nl)});
}

void handle_vm_cmdViewCsFixtures(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t l1 = body[0];
    if (len < 1u + l1) return;
    uint8_t l2 = body[1u + l1];
    if (len < 2u + l1 + l2) return;
    uint8_t l3 = body[2u + l1 + l2];
    if (len < 3u + l1 + l2 + l3) return;
    applyLpc("cmd_view_cs_fixtures", {make_int(cl.clientNo), make_string(body + 1, l1), make_string(body + 2u + l1, l2), make_string(body + 3u + l1 + l2, l3)});
}

void handle_vm_cmdViewCsMatchDetail(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t l1 = body[0];
    if (len < 1u + l1) return;
    uint8_t l2 = body[1u + l1];
    if (len < 2u + l1 + l2) return;
    uint8_t l3 = body[2u + l1 + l2];
    if (len < 3u + l1 + l2 + l3) return;
    uint8_t l4 = body[3u + l1 + l2 + l3];
    if (len < 4u + l1 + l2 + l3 + l4) return;
    applyLpc("cmd_view_cs_match_detail", {make_int(cl.clientNo), make_string(body + 1, l1), make_string(body + 2u + l1, l2), make_string(body + 3u + l1 + l2, l3), make_string(body + 4u + l1 + l2 + l3, l4)});
}

void handle_vm_cmdViewCsMatches(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n0 = body[off++];
    if (off + n0 > len) return;
    auto s0 = make_string(body + off, n0);
    off += n0;
    if (off + 1 > len) return;
    uint8_t n1 = body[off++];
    if (off + n1 > len) return;
    auto s1 = make_string(body + off, n1);
    off += n1;
    if (off + 1 > len) return;
    uint8_t n2 = body[off++];
    if (off + n2 > len) return;
    auto s2 = make_string(body + off, n2);
    off += n2;
    applyLpc("cmd_view_cs_matches", {make_int(cl.clientNo), s0, s1, s2});
}

void handle_vm_cmdViewCsMatchesSummary(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n0 = body[off++];
    if (off + n0 > len) return;
    auto s0 = make_string(body + off, n0);
    off += n0;
    if (off + 1 > len) return;
    uint8_t n1 = body[off++];
    if (off + n1 > len) return;
    auto s1 = make_string(body + off, n1);
    off += n1;
    applyLpc("cmd_view_cs_matches_summary", {make_int(cl.clientNo), s0, s1});
}

void handle_vm_cmdViewCsPwFixtures(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    applyLpc("cmd_view_cs_pw_fixtures", {make_int(cl.clientNo), make_int(body[0]), make_int(body[1])});
}

void handle_vm_cmdViewPartyWarHistory(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    uint32_t v = rd32be(body, 0);
    applyLpc("cmd_view_party_war_history", {make_int(cl.clientNo), make_int(v), make_int(body[4])});
}

void handle_vm_cmdViewStall(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t v = rd32be(body, 0);
    applyLpc("cmd_view_stall", {make_int(cl.clientNo), make_int(v)});
}

void handle_vm_cmdViewTop(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    uint32_t v = rd32be(body, 1);
    applyLpc("cmd_view_top", {make_int(cl.clientNo), make_int(body[0]), make_int(v)});
}

void handle_vm_cmdWarcraftCancel(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n = body[off++];
    if (off + n > len) return;
    auto s = make_string(body + off, n);
    off += n;
    applyLpc("cmd_warcraft_cancel", {make_int(cl.clientNo), s});
}

void handle_vm_cmdWarcraftRefresh(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n = body[off++];
    if (off + n > len) return;
    auto s = make_string(body + off, n);
    off += n;
    applyLpc("cmd_warcraft_refresh", {make_int(cl.clientNo), s});
}

void handle_vm_cmdWarcraftStart(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n = body[off++];
    if (off + n > len) return;
    auto s = make_string(body + off, n);
    off += n;
    applyLpc("cmd_warcraft_start", {make_int(cl.clientNo), s});
}

void handle_vm_cmdWarViewCurrentInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_view_war_current_info", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdWarViewHistoryInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_view_war_history_info", {make_int(cl.clientNo), make_int(body[0])});
}


void handle_vm_cmdWearAskTaoShow(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    if (1u + slen > len) return;
    applyLpc("cmd_wear_asktao_show", {make_int(cl.clientNo), make_string(body + 1, slen)});
}


void handle_vm_cmdWho(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_who", {make_int(cl.clientNo)});
}


void handle_vm_cmdWindowSizeChange(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_window_size_change", {make_int(cl.clientNo), make_int(body[0])});
}


void handle_vm_cmdWishing(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint16_t num = rd16be(body, 0);
    uint16_t count = rd16be(body, 2);
    if (count > 63) return;
    size_t off = 4;
    auto m = std::make_shared<MappingValue>();
    for (uint16_t i = 0; i < count; i++) {
        if (off >= len) return;
        uint8_t tl = body[off++];
        if (off + tl > len) return;
        std::string type((const char*)body + off, tl);
        off += tl;
        if (off >= len) return;
        uint8_t nl = body[off++];
        if (off + nl > len) return;
        std::string name((const char*)body + off, nl);
        off += nl;
        m->insert(make_string(type.data(), type.size()), make_string(name.data(), name.size()));
    }
    applyLpc("cmd_wishing", {make_int(cl.clientNo), make_int(num), m});
}


void handle_vm_cmdWishingBuyItem(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_wishing_buy_item", {make_int(cl.clientNo)});
}


void handle_vm_cmdWishingOpen(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_wishing_open", {make_int(cl.clientNo)});
}


void handle_vm_cmdWishingSaleItem(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_wishing_sale_item", {make_int(cl.clientNo)});
}


void handle_vm_cmdWithdraw(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 8) return;
    uint32_t v1 = rd32be(body, 0);
    uint32_t v2 = rd32be(body, 4);
    applyLpc("cmd_withdraw", {make_int(cl.clientNo), make_int(v1), make_int(v2)});
}


void handle_vm_cmdWLResetSeal(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_world_level_reset_seal", {make_int(cl.clientNo)});
}


void handle_vm_cmdWLSealInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t level = rd16be(body, 0);
    applyLpc("cmd_world_level_seal_info", {make_int(cl.clientNo), make_int(level)});
}

void handle_vm_cmdWorkshopBuyOrder(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    if (1u + slen > len) return;
    size_t off = 1u + slen;
    if (off + 1 > len) return;
    uint8_t cnt = body[off++];
    if (off + (size_t)cnt * 4 > len) return;
    auto arr = std::make_shared<ArrayValue>();
    for (uint8_t i = 0; i < cnt; ++i) {
        arr->items.push_back(make_int(rd32be(body, off)));
        off += 4;
    }
    applyLpc("cmd_workshop_buy_order", {make_int(cl.clientNo), make_string(body + 1, slen), arr});
}

void handle_vm_cmdWorkshopQueryOrderCash(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    if (1u + slen > len) return;
    size_t off = 1u + slen;
    if (off + 1 > len) return;
    uint8_t cnt = body[off++];
    if (off + (size_t)cnt * 4 > len) return;
    auto arr = std::make_shared<ArrayValue>();
    for (uint8_t i = 0; i < cnt; ++i) {
        arr->items.push_back(make_int(rd32be(body, off)));
        off += 4;
    }
    applyLpc("cmd_workshop_query_order_cash", {make_int(cl.clientNo), make_string(body + 1, slen), arr});
}

void handle_vm_cmdWorkshopQueryOrderRate(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    if (1u + slen > len) return;
    size_t off = 1u + slen;
    if (off + 1 > len) return;
    uint8_t cnt = body[off++];
    if (off + (size_t)cnt * 4 > len) return;
    auto arr = std::make_shared<ArrayValue>();
    for (uint8_t i = 0; i < cnt; ++i) {
        arr->items.push_back(make_int(rd32be(body, off)));
        off += 4;
    }
    applyLpc("cmd_workshop_query_order_rate", {make_int(cl.clientNo), make_string(body + 1, slen), arr});
}

void handle_vm_cmdWorkshopReleaseOrder(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 9) return;
    uint8_t v0 = body[0];
    uint32_t v1 = rd32be(body, 1);
    uint32_t v2 = rd32be(body, 5);
    applyLpc("cmd_workshop_release_order", {make_int(cl.clientNo), make_int(v0), make_int(v1), make_int(v2)});
}

void handle_vm_cmdWudaoAddAttribAuto(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_wudao_add_attrib_auto", {make_int(cl.clientNo)});
}

void handle_vm_cmdWuDaoAddAttribRep(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 16) return;
    uint32_t a = rd32be(body, 0);
    uint32_t b = rd32be(body, 4);
    uint32_t c = rd32be(body, 8);
    uint32_t d = rd32be(body, 12);
    applyLpc("cmd_wudao_add_attrib_rep", {make_int(cl.clientNo), make_int(a), make_int(b), make_int(c), make_int(d)});
}

void handle_vm_cmdWudaoBatchAddRep(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_wudao_batch_add_rep", {make_int(cl.clientNo)});
}

void handle_vm_cmdWuDaoBuyRepCard(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    if (1u + slen > len) return;
    applyLpc("cmd_wudao_buy_rep_card", {make_int(cl.clientNo), make_string(body + 1, slen)});
}

void handle_vm_cmdWudaoConvertExtraAttrib(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_wudao_convert_extra_attrib", {make_int(cl.clientNo)});
}

void handle_vm_cmdWuDaoDrawRepCard(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n = rd8(body, 0);
    if ((size_t)1 + n > len) return;
    applyLpc("cmd_wudao_draw_rep_card", {make_int(cl.clientNo), make_string(body + 1, n)});
}

void handle_vm_cmdWuDaoPreAddAttribRep(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 16) return;
    uint32_t a = rd32be(body, 0);
    uint32_t b = rd32be(body, 4);
    uint32_t c = rd32be(body, 8);
    uint32_t d = rd32be(body, 12);
    applyLpc("cmd_wudao_pre_add_attrib_rep", {make_int(cl.clientNo), make_int(a), make_int(b), make_int(c), make_int(d)});
}

void handle_vm_cmdWudaoSetAttribStatus(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n = rd8(body, 0);
    if ((size_t)2 + n + 2 > len) return;
    applyLpc("cmd_wudao_set_attrib_status", {make_int(cl.clientNo), make_string(body + 1, n), make_int(rd8(body, 1 + n)), make_int(rd16be(body, 2 + n))});
}

void handle_vm_cmdWuDaoStageInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    applyLpc("cmd_wudao_stage_info", {make_int(cl.clientNo), make_int(rd16be(body, 0))});
}

void handle_vm_cmdXSetSetting(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nl = rd8(body, 0);
    if ((size_t)2 + nl > len) return;
    uint8_t vl = rd8(body, 1 + nl);
    if ((size_t)2 + nl + vl > len) return;
    applyLpc("cmd_x_set_setting", {make_int(cl.clientNo), make_string(body + 1, nl), make_string(body + 2 + nl, vl)});
}

void handle_vm_doubleBonusTime(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 6) return;
    applyLpc("cmd_double_bonus_time", {make_int(cl.clientNo), make_int(rd32be(body, 0)), make_int(rd8(body, 4)), make_int(rd8(body, 5))});
}

void handle_vm_getInstructionBonus(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n = rd8(body, 0);
    if ((size_t)1 + n > len) return;
    applyLpc("cmd_get_instruction_bonus", {make_int(cl.clientNo), make_string(body + 1, n)});
}

void handle_vm_operGardenInstruction(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n = rd8(body, 0);
    if ((size_t)2 + n > len) return;
    applyLpc("cmd_oper_garden_instruction", {make_int(cl.clientNo), make_string(body + 1, n), make_int(rd8(body, 1 + n))});
}

void handle_vm_operHellInstruction(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n = rd8(body, 0);
    if ((size_t)2 + n > len) return;
    applyLpc("cmd_oper_hell_instruction", {make_int(cl.clientNo), make_string(body + 1, n), make_int(rd8(body, 1 + n))});
}

void handle_vm_cmdStallInputPrice(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    uint32_t v1 = rd32be(body, 0);
    uint8_t v2 = body[4];
    applyLpc("cmd_stall_input_price", {make_int(cl.clientNo), make_int(v1), make_int(v2)});
}

void handle_vm_cmdStartAutomatismStall(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_start_automatism_stall", {make_int(cl.clientNo)});
}

void handle_vm_cmdStartBoxAdventrue(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_start_box_adventrue", {make_int(cl.clientNo)});
}

void handle_vm_cmdStartCardActivity(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nlen = body[0];
    if (1 + (size_t)nlen + 2 > len) return;
    auto name = make_string(body + 1, nlen);
    uint16_t v1 = rd16be(body, 1 + nlen);
    applyLpc("cmd_start_card_activity", {make_int(cl.clientNo), name, make_int(v1)});
}

void handle_vm_cmdStartCardDuel(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_start_card_duel", {make_int(cl.clientNo)});
}

void handle_vm_cmdStartCardFriendPk(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nlen = body[0];
    if (1 + (size_t)nlen > len) return;
    applyLpc("cmd_start_card_friend_pk", {make_int(cl.clientNo), make_string(body + 1, nlen)});
}

void handle_vm_cmdStartCardInstruction(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_start_card_instruction", {make_int(cl.clientNo)});
}

void handle_vm_cmdStartPetPractice(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    uint8_t v1 = body[0];
    uint8_t v2 = body[1];
    uint16_t v3 = rd16be(body, 2);
    uint8_t v4 = body[4];
    applyLpc("cmd_start_pet_practice", {make_int(cl.clientNo), make_int(v1), make_int(v2), make_int(v3), make_int(v4)});
}

void handle_vm_cmdStartRaise(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 6) return;
    uint32_t id = rd32be(body, 0);
    uint8_t type = body[4];
    uint8_t nlen = body[5];
    if (6 + (size_t)nlen > len) return;
    applyLpc("cmd_start_raise", {make_int(cl.clientNo), make_int(id), make_int(type), make_string(body + 6, nlen)});
}

void handle_vm_cmdStartStall(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_start_stall", {make_int(cl.clientNo)});
}

void handle_vm_cmdStartTaotaotaoRun(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_start_taotaotao_run", {make_int(cl.clientNo)});
}

void handle_vm_cmdStore(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 10) return;
    uint32_t v6 = rd32be(body, 0);
    uint16_t v7 = rd16be(body, 4);
    uint16_t v8 = rd16be(body, 6);
    uint16_t v9 = rd16be(body, 8);
    applyLpc("cmd_store", {make_int(cl.clientNo), make_int(v6), make_int(v7), make_int(v8), make_int(v9)});
}

void handle_vm_cmdSubmitConfirmDlg(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t nl = body[off++];
    if (off + nl > len) return;
    const uint8_t* dialog_item = body + off; off += nl;
    if (off + 1 > len) return;
    uint8_t pl = body[off++];
    if (off + pl > len) return;
    const uint8_t* para = body + off; off += pl;
    applyLpc("cmd_submit_confirm_dlg", {make_int(cl.clientNo), make_string(dialog_item, nl), make_string(para, pl)});
}

void handle_vm_cmdSubmitIcon(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    int oper_type = body[off++];
    if (off + 1 > len) return;
    uint8_t ml = body[off++];
    if (off + ml > len) return;
    const uint8_t* md5_value = body + off; off += ml;
    const uint8_t* buf = body + off;
    size_t bufLen = len - off;
    applyLpc("cmd_submit_icon", {make_int(cl.clientNo), make_int(oper_type), make_string(md5_value, ml), make_buffer(buf, bufLen)});
}

void handle_vm_cmdSubmitImage(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_submit_individuality_image", {make_int(cl.clientNo), make_buffer(body, len)});
}

void handle_vm_cmdSubmitPasspodOrder(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t l1 = body[off++];
    if (off + l1 > len) return;
    const uint8_t* passpod_iid = body + off; off += l1;
    if (off + 1 > len) return;
    uint8_t l2 = body[off++];
    if (off + l2 > len) return;
    const uint8_t* name = body + off; off += l2;
    if (off + 1 > len) return;
    uint8_t l3 = body[off++];
    if (off + l3 > len) return;
    const uint8_t* addres = body + off; off += l3;
    if (off + 1 > len) return;
    uint8_t l4 = body[off++];
    if (off + l4 > len) return;
    const uint8_t* phone = body + off; off += l4;
    if (off + 1 > len) return;
    uint8_t l5 = body[off++];
    if (off + l5 > len) return;
    const uint8_t* extra_phone = body + off; off += l5;
    if (off + 1 > len) return;
    uint8_t l6 = body[off++];
    if (off + l6 > len) return;
    const uint8_t* zip = body + off; off += l6;
    applyLpc("cmd_submit_passpod_order", {make_int(cl.clientNo), make_string(passpod_iid, l1), make_string(name, l2), make_string(addres, l3), make_string(phone, l4), make_string(extra_phone, l5), make_string(zip, l6)});
}

void handle_vm_cmdSwitchBackEquip(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_switch_back_equip", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdSwitchChildItem(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 6) return;
    uint32_t v6 = rd32be(body, 0);
    uint8_t b4 = body[4];
    uint8_t b5 = body[5];
    applyLpc("cmd_switch_child_item", {make_int(cl.clientNo), make_int(v6), make_int(b4), make_int(b5)});
}

void handle_vm_cmdSwitchGroup(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t l1 = body[off++];
    if (off + l1 > len) return;
    const uint8_t* name = body + off; off += l1;
    if (off + 1 > len) return;
    uint8_t l2 = body[off++];
    if (off + l2 > len) return;
    const uint8_t* fromGroup = body + off; off += l2;
    if (off + 1 > len) return;
    uint8_t l3 = body[off++];
    if (off + l3 > len) return;
    const uint8_t* toGroup = body + off; off += l3;
    applyLpc("cmd_switch_group", {make_int(cl.clientNo), make_string(name, l1), make_string(fromGroup, l2), make_string(toGroup, l3)});
}

void handle_vm_cmdSwitchGroupEx(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t l1 = body[off++];
    if (off + l1 > len) return;
    const uint8_t* name = body + off; off += l1;
    if (off + 1 > len) return;
    uint8_t l2 = body[off++];
    if (off + l2 > len) return;
    const uint8_t* group = body + off; off += l2;
    if (off + 1 > len) return;
    int friend_verify = body[off++];
    if (off + 1 > len) return;
    uint8_t l3 = body[off++];
    if (off + l3 > len) return;
    const uint8_t* to_group = body + off; off += l3;
    applyLpc("cmd_switch_group_ex", {make_int(cl.clientNo), make_string(name, l1), make_string(group, l2), make_int(friend_verify), make_string(to_group, l3)});
}

void handle_vm_cmdSwitchInStore(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 8) return;
    uint32_t npcId = rd32be(body, 0);
    uint16_t storeFromPos = rd16be(body, 4);
    uint16_t storeToPos = rd16be(body, 6);
    applyLpc("cmd_switch_in_store", {make_int(cl.clientNo), make_int(npcId), make_int(storeFromPos), make_int(storeToPos)});
}

void handle_vm_cmdSwitchItem(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t item1 = body[0];
    uint8_t item2 = body[1];
    applyLpc("cmd_switch_item", {make_int(cl.clientNo), make_int(item1), make_int(item2)});
}

void handle_vm_cmdSwitchServer(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n = body[0];
    if (n > len - 1) return;
    applyLpc("cmd_switch_server", {make_int(cl.clientNo), make_string(body + 1, n)});
}

void handle_vm_cmdSysPurchaseBuy(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    uint8_t n = body[0];
    if (n + 4 > len - 1) return;
    uint32_t value = rd32be(body, 1 + n);
    applyLpc("cmd_sys_purchase_buy", {make_int(cl.clientNo), make_string(body + 1, n), make_int(value)});
}

void handle_vm_cmdSysPurchaseSell(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t n = body[0];
    if (n > len - 1) return;
    applyLpc("cmd_sys_purchase_sell", {make_int(cl.clientNo), make_string(body + 1, n)});
}

void handle_vm_cmdSysPurchaseViewHistory(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_sys_purchase_view_history", {make_int(cl.clientNo)});
}

void handle_vm_cmdSysPurchaseViewOrders(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_sys_purchase_view_orders", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdTake(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 10) return;
    uint32_t v0 = rd32be(body, 0);
    uint16_t v4 = rd16be(body, 4);
    uint16_t v6 = rd16be(body, 6);
    uint16_t v8 = rd16be(body, 8);
    applyLpc("cmd_take", {make_int(cl.clientNo), make_int(v0), make_int(v4), make_int(v6), make_int(v8)});
}

void handle_vm_cmdTakeChildItemUp(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 6) return;
    uint32_t v0 = rd32be(body, 0);
    uint16_t v4 = rd16be(body, 4);
    applyLpc("cmd_take_child_item_up", {make_int(cl.clientNo), make_int(v0), make_int(v4)});
}

void handle_vm_cmdTakeoutCard(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 13) return;
    uint32_t fromPos = rd32be(body, 0);
    uint32_t toPos = rd32be(body, 4);
    uint8_t n = body[8];
    if (n + 4 > len - 9) return;
    uint32_t value = rd32be(body, 9 + n);
    applyLpc("cmd_takeout_card", {make_int(cl.clientNo), make_int(fromPos), make_int(toPos), make_string(body + 9, n), make_int(value)});
}

void handle_vm_cmdTakeoutGodbook(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    uint32_t bookId = rd32be(body, 0);
    size_t off = 4;
    uint8_t nameLen = body[off++];
    if (off + nameLen > len) return;
    applyLpc("cmd_takeout_godbook", {make_int(cl.clientNo), make_int(bookId), make_string(body + off, nameLen)});
}

void handle_vm_cmdTakeoutItem(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 3) return;
    uint8_t nameLen = body[0];
    size_t off = 1;
    if (off + nameLen + 2 > len) return;
    uint8_t v11 = body[off + nameLen];
    uint8_t v12 = body[off + nameLen + 1];
    applyLpc("cmd_takeout_item", {make_int(cl.clientNo), make_string(body + off, nameLen), make_int(v11), make_int(v12)});
}

void handle_vm_cmdTakeStallCash(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    applyLpc("cmd_take_stall_cash", {make_int(cl.clientNo), make_int(rd32be(body, 0))});
}

void handle_vm_cmdTakeStoringCash(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t idLen = body[0];
    size_t off = 1;
    if (off + idLen > len) return;
    applyLpc("cmd_take_storing_cash", {make_int(cl.clientNo), make_string(body + off, idLen)});
}

void handle_vm_cmdTakeUp(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    applyLpc("cmd_take_up", {make_int(cl.clientNo), make_int(rd16be(body, 0))});
}

void handle_vm_cmdTaskScoreExchangeBonus(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 10) return;
    uint16_t id = rd16be(body, 0);
    uint32_t npcId = rd32be(body, 2);
    uint16_t amount = rd16be(body, 6);
    uint16_t type = rd16be(body, 8);
    applyLpc("cmd_task_score_exchange_bonus", {make_int(cl.clientNo), make_int(id), make_int(npcId), make_int(amount), make_int(type)});
}

void handle_vm_cmdTeamCancelSeek(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_team_cancel_seek", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdTeamMatchSeek(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t seekType = body[0];
    uint8_t idLen = body[1];
    size_t off = 2;
    if (off + idLen > len) return;
    applyLpc("cmd_team_match_seek", {make_int(cl.clientNo), make_int(seekType), make_string(body + off, idLen)});
}

void handle_vm_cmdTeamOpenPlatform(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_team_open_platform", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdTeamRequestTask(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_team_request_task", {make_int(cl.clientNo)});
}

void handle_vm_cmdTeamSeekLeader(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t nameLen = body[off++];
    if (off + nameLen > len) return;
    auto target = make_string(body + off, nameLen);
    off += nameLen;
    if (off + 2 > len) return;
    uint16_t v12 = rd16be(body, off); off += 2;
    if (off + 2 > len) return;
    uint16_t v15 = rd16be(body, off); off += 2;
    if (off + 1 > len) return;
    uint8_t v13 = body[off];
    applyLpc("cmd_team_seek_leader", {make_int(cl.clientNo), target, make_int(v12), make_int(v15), make_int(v13)});
}

void handle_vm_cmdTeamSeekMember(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t nameLen = body[off++];
    if (off + nameLen > len) return;
    auto target = make_string(body + off, nameLen);
    off += nameLen;
    if (off + 2 > len) return;
    uint16_t minLv = rd16be(body, off); off += 2;
    if (off + 2 > len) return;
    uint16_t v14 = rd16be(body, off); off += 2;
    if (off + 4 > len) return;
    uint32_t v19 = rd32be(body, off); off += 4;
    if (off + 4 > len) return;
    uint32_t v23 = rd32be(body, off); off += 4;
    if (off + 2 > len) return;
    uint8_t v25 = body[off++];
    uint8_t v26 = body[off];
    applyLpc("cmd_team_seek_member", {make_int(cl.clientNo), target, make_int(minLv), make_int(v14), make_int(v19), make_int(v23), make_int(v25), make_int(v26)});
}

void handle_vm_cmdTeleportEx(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 6 > len) return;
    uint32_t itemId = rd32be(body, off); off += 4;
    uint16_t operType = rd16be(body, off); off += 2;
    if (off + 1 > len) return;
    uint8_t paraLen = body[off++];
    if (off + paraLen > len) return;
    auto para = make_string(body + off, paraLen);
    applyLpc("cmd_teleport_ex", {make_int(cl.clientNo), make_int(itemId), make_int(operType), para});
}

void handle_vm_cmdTell(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t nameLen = body[off++];
    if (off + nameLen > len) return;
    auto name = make_string(body + off, nameLen);
    off += nameLen;
    if (off + 1 > len) return;
    uint8_t msgLen = body[off++];
    if (off + msgLen > len) return;
    auto msg = make_string(body + off, msgLen);
    applyLpc("cmd_tell", {make_int(cl.clientNo), name, msg});
}

void handle_vm_cmdTellEx(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t nameLen = body[off++];
    if (off + nameLen > len) return;
    auto name = make_string(body + off, nameLen);
    off += nameLen;
    if (off + 6 > len) return;
    uint16_t comp = rd16be(body, off); off += 2;
    uint16_t orgLen = rd16be(body, off); off += 2;
    uint16_t msgLen = rd16be(body, off); off += 2;
    if (off + msgLen > len) return;
    auto msg = make_string(body + off, msgLen);
    off += msgLen;
    if (off + 2 > len) return;
    uint16_t itemCount = rd16be(body, off); off += 2;
    auto arrStr = std::make_shared<ArrayValue>();
    auto arrInt = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < itemCount; i++)
    {
        if (off + 1 > len) return;
        uint8_t sLen = body[off++];
        if (off + sLen > len) return;
        arrStr->items.push_back(make_string(body + off, sLen));
        off += sLen;
        if (off + 1 > len) return;
        arrInt->items.push_back(make_int(body[off++]));
    }
    applyLpc("cmd_tell_ex", {make_int(cl.clientNo), name, make_int(comp), make_int(orgLen), msg, arrStr, arrInt});
}

void handle_vm_cmdTempNpcEvent(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nameLen = body[0];
    if (1 + nameLen > len) return;
    auto name = make_string(body + 1, nameLen);
    applyLpc("cmd_temp_npc_event", {make_int(cl.clientNo), name});
}

void handle_vm_cmdTestCsLogin(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t actLen = body[0];
    if (1 + actLen > len) return;
    auto activity = make_string(body + 1, actLen);
    applyLpc("cmd_test_cs_login", {make_int(cl.clientNo), activity});
}

void handle_vm_cmdTowerClicked(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_tower_clicked", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdTowerSelected(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    uint32_t towerId = rd32be(body, 0);
    applyLpc("cmd_tower_selected", {make_int(cl.clientNo), make_int(towerId), make_int(body[4])});
}

void handle_vm_cmdTradeAddGoods(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t gLen = body[0];
    if (1 + gLen > len) return;
    auto goodsId = make_string(body + 1, gLen);
    size_t off = 1 + gLen;
    if (off + 1 > len) return;
    applyLpc("cmd_trade_add_goods", {make_int(cl.clientNo), goodsId, make_int(body[off])});
}

void handle_vm_cmdTradeBuyService(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t s1_len = body[0];
    if (1 + s1_len > len) return;
    applyLpc("cmd_trade_buy_service", {make_int(cl.clientNo), make_string(body + 1, s1_len)});
}

void handle_vm_cmdTradeCancelSale(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t s1_len = body[0];
    if (1 + s1_len > len) return;
    const uint8_t* s1 = body + 1;
    size_t off = 1 + s1_len;
    if (off + 1 > len) return;
    uint8_t i1 = body[off];
    applyLpc("cmd_trade_cancel_sale", {make_int(cl.clientNo), make_string(s1, s1_len), make_int(i1)});
}

void handle_vm_cmdTradeChangePrice(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 8) return;
    uint32_t price = rd32be(body, 0);
    uint32_t upset_price = rd32be(body, 4);
    size_t off = 8;
    if (off >= len) return;
    uint8_t s1_len = body[off++];
    if (off + s1_len > len) return;
    const uint8_t* s1 = body + off;
    applyLpc("cmd_trade_change_price", {make_int(cl.clientNo), make_int(price), make_int(upset_price), make_string(s1, s1_len)});
}

void handle_vm_cmdTradeContinueSale(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 8) return;
    uint8_t type = body[0];
    uint16_t count = rd16be(body, 1);
    uint32_t price = rd32be(body, 3);
    uint8_t day = body[7];
    size_t off = 8;
    if (off >= len) return;
    uint8_t s1_len = body[off++];
    if (off + s1_len > len) return;
    const uint8_t* s1 = body + off;
    off += s1_len;
    if (off >= len) return;
    uint8_t s2_len = body[off++];
    if (off + s2_len > len) return;
    const uint8_t* s2 = body + off;
    off += s2_len;
    if (off >= len) return;
    uint8_t i1 = body[off++];
    if (off + 4 > len) return;
    uint32_t i2 = rd32be(body, off);
    applyLpc("cmd_trade_continue_sale", {make_int(cl.clientNo), make_int(type), make_int(count), make_int(price), make_int(day), make_string(s1, s1_len), make_string(s2, s2_len), make_int(i1), make_int(i2)});
}

void handle_vm_cmdTradeFavorite(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t mode = body[0];
    size_t off = 1;
    if (off >= len) return;
    uint8_t s1_len = body[off++];
    if (off + s1_len > len) return;
    const uint8_t* s1 = body + off;
    off += s1_len;
    if (off + 4 > len) return;
    uint16_t i1 = rd16be(body, off);
    uint16_t i2 = rd16be(body, off + 2);
    applyLpc("cmd_trade_favorite", {make_int(cl.clientNo), make_int(mode), make_string(s1, s1_len), make_int(i1), make_int(i2)});
}

void handle_vm_cmdTradeGetData(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 7) return;
    uint8_t t = body[0];
    uint16_t v5 = rd16be(body, 1);
    uint32_t v7 = rd32be(body, 3);
    applyLpc("cmd_trade_get_data", {make_int(cl.clientNo), make_int(t), make_int(v5), make_int(v7)});
}

void handle_vm_cmdTradeGetObj(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t mode = body[0];
    size_t off = 1;
    if (off >= len) return;
    uint8_t s1_len = body[off++];
    if (off + s1_len > len) return;
    const uint8_t* s1 = body + off;
    applyLpc("cmd_trade_get_obj", {make_int(cl.clientNo), make_int(mode), make_string(s1, s1_len)});
}

void handle_vm_cmdTradeGetObjProp(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t s1_len = body[0];
    if (1 + s1_len > len) return;
    const uint8_t* s1 = body + 1;
    size_t off = 1 + s1_len;
    if (off + 1 > len) return;
    uint8_t i1 = body[off];
    applyLpc("cmd_trade_get_obj_prop", {make_int(cl.clientNo), make_string(s1, s1_len), make_int(i1)});
}

void handle_vm_cmdTradeGetRecord(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_trade_get_record", {make_int(cl.clientNo)});
}

void handle_vm_cmdTradeIntroduce(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_trade_introduce", {make_int(cl.clientNo)});
}

void handle_vm_cmdTradeOpenUrl(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t goodsLen = body[off++];
    if (off + goodsLen > len) return;
    std::string goods_id((const char*)body + off, goodsLen);
    off += goodsLen;
    if (off + 2 > len) return;
    uint16_t v12 = rd16be(body, off); off += 2;
    if (off + 2 > len) return;
    uint16_t v13 = rd16be(body, off); off += 2;
    applyLpc("cmd_trade_open_url", {make_int(cl.clientNo), make_string(goods_id.data(), goods_id.size()), make_int(v12), make_int(v13)});
}

void handle_vm_cmdTradeSale(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t type = body[off++];
    if (off + 2 > len) return;
    uint16_t count = rd16be(body, off); off += 2;
    if (off + 4 > len) return;
    uint32_t price = rd32be(body, off); off += 4;
    if (off + 1 > len) return;
    uint8_t day = body[off++];
    if (off + 4 > len) return;
    uint32_t para = rd32be(body, off); off += 4;
    if (off + 1 > len) return;
    uint8_t appLen = body[off++];
    if (off + appLen > len) return;
    std::string appointer((const char*)body + off, appLen);
    off += appLen;
    if (off + 1 > len) return;
    uint8_t v11 = body[off++];
    if (off + 4 > len) return;
    uint32_t v14 = rd32be(body, off); off += 4;
    applyLpc("cmd_trade_sale", {make_int(cl.clientNo), make_int(type), make_int(count), make_int(price), make_int(day), make_int(para), make_string(appointer.data(), appointer.size()), make_int(v11), make_int(v14)});
}

void handle_vm_cmdTradeViewLog(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_trade_view_log", {make_int(cl.clientNo)});
}

void handle_vm_cmdTrain(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t v7 = rd32be(body, off); off += 4;
    applyLpc("cmd_train", {make_int(cl.clientNo), make_int(v7)});
}

void handle_vm_cmdTryPlacePattern(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t oper_type = body[off++];
    if (off + 4 > len) return;
    uint32_t item_id = rd32be(body, off); off += 4;
    if (off + 4 > len) return;
    uint32_t start_time = rd32be(body, off); off += 4;
    if (off + 4 > len) return;
    uint32_t interval = rd32be(body, off); off += 4;
    if (off + 2 > len) return;
    uint16_t x = rd16be(body, off); off += 2;
    if (off + 2 > len) return;
    uint16_t y = rd16be(body, off); off += 2;
    if (off + 1 > len) return;
    uint8_t coinLen = body[off++];
    if (off + coinLen > len) return;
    std::string coin_type((const char*)body + off, coinLen);
    off += coinLen;
    if (off + 2 > len) return;
    uint16_t itemLen = rd16be(body, off); off += 2;
    if (off + itemLen > len) return;
    std::string item_str((const char*)body + off, itemLen);
    off += itemLen;
    applyLpc("cmd_try_place_pattern", {make_int(cl.clientNo), make_int(oper_type), make_int(item_id), make_int(start_time), make_int(interval), make_int(x), make_int(y), make_string(coin_type.data(), coin_type.size()), make_string(item_str.data(), item_str.size())});
}

void handle_vm_cmdTurnDir(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t v7 = rd32be(body, off); off += 4;
    if (off + 2 > len) return;
    uint16_t v8 = rd16be(body, off); off += 2;
    applyLpc("cmd_turn_dir", {make_int(cl.clientNo), make_int(v7), make_int(v8)});
}

void handle_vm_cmdTxssResetBingo(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_txss_reset_bingo", {make_int(cl.clientNo)});
}

void handle_vm_cmdTxssStartAward(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 2 > len) return;
    uint16_t v6 = rd16be(body, off); off += 2;
    applyLpc("cmd_txss_start_award", {make_int(cl.clientNo), make_int(v6)});
}

void handle_vm_cmdUnequip(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t v0 = body[off++];
    if (off + 1 > len) return;
    uint8_t v1 = body[off++];
    applyLpc("cmd_unequip", {make_int(cl.clientNo), make_int(v0), make_int(v1)});
}

void handle_vm_cmdUnequipInbornStone(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t v6 = rd32be(body, off); off += 4;
    if (off + 2 > len) return;
    uint16_t v7 = rd16be(body, off); off += 2;
    if (off + 2 > len) return;
    uint16_t v8 = rd16be(body, off); off += 2;
    applyLpc("cmd_unequip_inborn_stone", {make_int(cl.clientNo), make_int(v6), make_int(v7), make_int(v8)});
}

void handle_vm_cmdUnequipPetDress(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    int itemId = rd32be(body, 0);
    applyLpc("cmd_unequip_pet_dress", {make_int(cl.clientNo), make_int(itemId)});
}

void handle_vm_cmdUpgrade(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 22) return;
    int npcId = rd32be(body, 0);
    int type = rd16be(body, 4);
    auto arr = std::make_shared<ArrayValue>();
    for (int i = 0; i < 4; i++)
        arr->items.push_back(make_int(rd32be(body, 6 + i * 4)));
    applyLpc("cmd_upgrade", {make_int(cl.clientNo), make_int(npcId), make_int(type), arr});
}

void handle_vm_cmdUpgradeByAmount(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 7) return;
    int npcId = rd32be(body, 0);
    int type = rd16be(body, 4);
    int cnt = body[6];
    auto pArray = std::make_shared<ArrayValue>();
    auto nArray = std::make_shared<ArrayValue>();
    size_t off = 7;
    for (int i = 0; i < cnt; i++)
    {
        if (off + 4 > len) return;
        pArray->items.push_back(make_int(rd16be(body, off)));
        nArray->items.push_back(make_int(rd16be(body, off + 2)));
        off += 4;
    }
    applyLpc("cmd_upgrade_by_amount", {make_int(cl.clientNo), make_int(npcId), make_int(type), pArray, nArray});
}

void handle_vm_cmdUpgradeEx(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 7) return;
    int npcId = rd32be(body, 0);
    int type = rd16be(body, 4);
    int cnt = body[6];
    auto arr = std::make_shared<ArrayValue>();
    size_t off = 7;
    for (int i = 0; i < cnt; i++)
    {
        if (off + 4 > len) return;
        arr->items.push_back(make_int(rd32be(body, off)));
        off += 4;
    }
    applyLpc("cmd_upgrade", {make_int(cl.clientNo), make_int(npcId), make_int(type), arr});
}

void handle_vm_cmdUpgradeMagicPicture(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    int itemId = rd32be(body, 0);
    int flag = body[4];
    applyLpc("cmd_upgrade_magic_picture", {make_int(cl.clientNo), make_int(itemId), make_int(flag)});
}

void handle_vm_cmdVendueCancel(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    int interface_type = body[0];
    int merch_type = body[1];
    int pos = rd16be(body, 2);
    int slen = body[4];
    if (5 + slen > len) return;
    applyLpc("cmd_vendue_cancel", {make_int(cl.clientNo), make_int(interface_type), make_int(merch_type), make_int(pos), make_string(body + 5, slen)});
}

void handle_vm_cmdVendueCanVendue(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    int vtype = body[0];
    int itemId = rd32be(body, 1);
    applyLpc("cmd_vendue_can_vendue", {make_int(cl.clientNo), make_int(vtype), make_int(itemId)});
}

void handle_vm_cmdVendueFetch(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    int slen = body[0];
    if (1 + slen > len) return;
    applyLpc("cmd_vendue_fetch", {make_int(cl.clientNo), make_string(body + 1, slen)});
}

void handle_vm_cmdVendueManage(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_vendue_manage", {make_int(cl.clientNo), make_int(body[0])});
}

void handle_vm_cmdVendueOffer(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 6) return;
    int interface_type = body[0];
    int merch_type = body[1];
    int vendue_type = body[2];
    int pos = rd16be(body, 3);
    int slen = body[5];
    if (6 + slen > len) return;
    applyLpc("cmd_vendue_offer", {make_int(cl.clientNo), make_int(interface_type), make_int(merch_type), make_int(vendue_type), make_int(pos), make_string(body + 6, slen)});
}

// ---- type=2 (INTERNAL_COMM) generated handlers, chunk 5 ----

void handle_vm_msgLOperProtectRange(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t accountLen = rd8(body, off);
    off += 1;
    if (off + accountLen > len) return;
    std::string account((const char*)body + off, accountLen);
    off += accountLen;
    if (off + 1 > len) return;
    uint8_t operLen = rd8(body, off);
    off += 1;
    if (off + operLen > len) return;
    std::string oper((const char*)body + off, operLen);
    off += operLen;
    if (off + 2 > len) return;
    int v1 = rd8(body, off);
    off += 1;
    int v2 = rd8(body, off);
    off += 1;
    applyLpc("msg_l_oper_protect_range", {make_int(cl.clientNo), make_string(account.data(), account.size()), make_string(oper.data(), oper.size()), make_int(v1), make_int(v2)});
}

void handle_vm_msgLAuthAllProtect(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t accountLen = rd8(body, off);
    off += 1;
    if (off + accountLen > len) return;
    std::string account((const char*)body + off, accountLen);
    off += accountLen;
    if (off + 1 > len) return;
    int program = rd8(body, off);
    off += 1;
    if (off + 4 > len) return;
    uint32_t value = rd32be(body, off);
    off += 4;
    if (off + 2 > len) return;
    uint16_t strLen = rd16be(body, off);
    off += 2;
    if (off + strLen > len) return;
    std::string str((const char*)body + off, strLen);
    off += strLen;
    applyLpc("msg_l_auth_all_protect", {make_int(cl.clientNo), make_string(account.data(), account.size()), make_int(program), make_int((int)value), make_string(str.data(), str.size())});
}

void handle_vm_cmdIntersvrRpcCall(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 6 > len) return;
    uint32_t cookie = rd32be(body, off);
    off += 4;
    uint16_t method = rd16be(body, off);
    off += 2;
    if (off + 1 > len) return;
    uint8_t ownerLen = rd8(body, off);
    off += 1;
    if (off + ownerLen > len) return;
    std::string owner((const char*)body + off, ownerLen);
    off += ownerLen;
    if (off + 1 > len) return;
    uint8_t serverLen = rd8(body, off);
    off += 1;
    if (off + serverLen > len) return;
    std::string server((const char*)body + off, serverLen);
    off += serverLen;
    if (off + 1 > len) return;
    uint8_t obLen = rd8(body, off);
    off += 1;
    if (off + obLen > len) return;
    std::string ob((const char*)body + off, obLen);
    off += obLen;
    if (off + 1 > len) return;
    uint8_t fLen = rd8(body, off);
    off += 1;
    if (off + fLen > len) return;
    std::string f((const char*)body + off, fLen);
    off += fLen;
    if (off + 2 > len) return;
    uint16_t data1Len = rd16be(body, off);
    off += 2;
    if (off + data1Len > len) return;
    std::string data1((const char*)body + off, data1Len);
    off += data1Len;
    if (off + 4 > len) return;
    uint32_t data2Len = rd32be(body, off);
    off += 4;
    if (off + data2Len > len) return;
    std::string data2((const char*)body + off, data2Len);
    off += data2Len;
    applyLpc("cmd_intersvr_rpc_call", {make_int(cl.clientNo), make_int((int)cookie), make_int((int)method), make_string(owner.data(), owner.size()), make_string(server.data(), server.size()), make_string(ob.data(), ob.size()), make_string(f.data(), f.size()), make_string(data1.data(), data1.size()), make_string(data2.data(), data2.size())});
}

void handle_vm_cmdLAgentConnectServer(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t accountLen = rd8(body, off);
    off += 1;
    if (off + accountLen > len) return;
    std::string account((const char*)body + off, accountLen);
    off += accountLen;
    if (off + 4 > len) return;
    uint32_t program = rd32be(body, off);
    off += 4;
    if (off + 2 > len) return;
    uint16_t srvLen = rd16be(body, off);
    off += 2;
    if (off + srvLen > len) return;
    std::string srv((const char*)body + off, srvLen);
    off += srvLen;
    applyLpc("cmd_l_agent_connect_server", {make_int(cl.clientNo), make_string(account.data(), account.size()), make_int((int)program), make_string(srv.data(), srv.size())});
}

void handle_vm_cmdLPendShop(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t accountLen = rd8(body, off);
    off += 1;
    if (off + accountLen > len) return;
    std::string account((const char*)body + off, accountLen);
    off += accountLen;
    applyLpc("cmd_l_pend_shop", {make_int(cl.clientNo), make_string(account.data(), account.size())});
}

void handle_vm_cmdLWhenBatchBlockAccount(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 2 > len) return;
    uint16_t accountLen = rd16be(body, off);
    off += 2;
    if (off + accountLen > len) return;
    std::string account((const char*)body + off, accountLen);
    off += accountLen;
    applyLpc("cmd_l_when_batch_block_account", {make_int(cl.clientNo), make_string(account.data(), account.size())});
}

void handle_vm_cmdLWhenBlockAccount(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t accountLen = rd8(body, off);
    off += 1;
    if (off + accountLen > len) return;
    std::string account((const char*)body + off, accountLen);
    off += accountLen;
    applyLpc("cmd_l_when_block_account", {make_int(cl.clientNo), make_string(account.data(), account.size())});
}

void handle_vm_cmdLRecycleStall(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t accountLen = rd8(body, off);
    off += 1;
    if (off + accountLen > len) return;
    std::string account((const char*)body + off, accountLen);
    off += accountLen;
    applyLpc("cmd_l_recycle_stall", {make_int(cl.clientNo), make_string(account.data(), account.size())});
}

void handle_vm_cmdLBlockShareStore(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t accountLen = rd8(body, off);
    off += 1;
    if (off + accountLen > len) return;
    std::string account((const char*)body + off, accountLen);
    off += accountLen;
    if (off + 4 > len) return;
    uint32_t value = rd32be(body, off);
    off += 4;
    applyLpc("cmd_l_block_share_store", {make_int(cl.clientNo), make_string(account.data(), account.size()), make_int((int)value)});
}

void handle_vm_msgInternalAuth(GameClient& cl, const uint8_t* body, size_t len)
{
    uint32_t off = 0;
    uint16_t replyServerType = rd16be(body, off); off += 2;
    if (off + 1 > len) return;
    uint8_t n = body[off++];
    if (off + n > len) return;
    std::string replyServerName((const char*)body + off, n); off += n;
    uint16_t v12 = rd16be(body, off); off += 2;
    uint32_t v14 = rd32be(body, off); off += 4;
    applyLpc("msg_internal_auth", {make_int(cl.clientNo), make_int(replyServerType), make_string(replyServerName.data(), replyServerName.size()), make_int(v12), make_int(v14)});
}

void handle_vm_msgLAddFillInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    uint32_t off = 0;
    if (off + 1 > len) return;
    uint8_t n = body[off++];
    if (off + n > len) return;
    std::string account((const char*)body + off, n); off += n;
    if (off + 1 > len) return;
    uint8_t n2 = body[off++];
    if (off + n2 > len) return;
    std::string gid((const char*)body + off, n2); off += n2;
    uint16_t result = rd16be(body, off); off += 2;
    if (off + 1 > len) return;
    uint8_t n3 = body[off++];
    if (off + n3 > len) return;
    std::string type((const char*)body + off, n3); off += n3;
    applyLpc("msg_l_add_fill_info", {make_int(cl.clientNo), make_string(account.data(), account.size()), make_string(gid.data(), gid.size()), make_int(result), make_string(type.data(), type.size())});
}

void handle_vm_msgLBuyCard(GameClient& cl, const uint8_t* body, size_t len)
{
    uint32_t off = 0;
    if (off + 1 > len) return;
    uint8_t n = body[off++];
    if (off + n > len) return;
    std::string cookie((const char*)body + off, n); off += n;
    if (off + 1 > len) return;
    uint8_t n2 = body[off++];
    if (off + n2 > len) return;
    std::string number((const char*)body + off, n2); off += n2;
    uint32_t v17 = rd32be(body, off); off += 4;
    uint16_t v18 = rd16be(body, off); off += 2;
    applyLpc("msg_l_buy_card", {make_int(cl.clientNo), make_string(cookie.data(), cookie.size()), make_string(number.data(), number.size()), make_int(v17), make_int(v18)});
}

void handle_vm_msgLChargeResult(GameClient& cl, const uint8_t* body, size_t len)
{
    uint32_t off = 0;
    if (off + 1 > len) return;
    uint8_t n = body[off++];
    if (off + n > len) return;
    std::string account((const char*)body + off, n); off += n;
    uint32_t v13 = rd32be(body, off); off += 4;
    uint32_t v18 = rd32be(body, off); off += 4;
    uint16_t v21 = rd16be(body, off); off += 2;
    uint16_t v22 = rd16be(body, off); off += 2;
    applyLpc("msg_l_charge_result", {make_int(cl.clientNo), make_string(account.data(), account.size()), make_int(v13), make_int(v18), make_int(v21), make_int(v22)});
}

void handle_vm_msgLCostCoin(GameClient& cl, const uint8_t* body, size_t len)
{
    uint32_t off = 0;
    if (off + 1 > len) return;
    uint8_t n = body[off++];
    if (off + n > len) return;
    std::string gid((const char*)body + off, n); off += n;
    if (off + 1 > len) return;
    uint8_t n2 = body[off++];
    if (off + n2 > len) return;
    std::string account((const char*)body + off, n2); off += n2;
    uint16_t result = rd16be(body, off); off += 2;
    if (off + 1 > len) return;
    uint8_t n3 = body[off++];
    if (off + n3 > len) return;
    std::string gs_para1((const char*)body + off, n3); off += n3;
    if (off + 1 > len) return;
    uint8_t n4 = body[off++];
    if (off + n4 > len) return;
    std::string gs_para2((const char*)body + off, n4); off += n4;
    uint16_t n5 = rd16be(body, off); off += 2;
    if (off + n5 > len) return;
    std::string gs_para3((const char*)body + off, n5); off += n5;
    applyLpc("msg_l_cost_coin", {make_int(cl.clientNo), make_string(gid.data(), gid.size()), make_string(account.data(), account.size()), make_int(result), make_string(gs_para1.data(), gs_para1.size()), make_string(gs_para2.data(), gs_para2.size()), make_string(gs_para3.data(), gs_para3.size())});
}

void handle_vm_msgLAddCoin(GameClient& cl, const uint8_t* body, size_t len)
{
    uint32_t off = 0;
    if (off + 1 > len) return;
    uint8_t n = body[off++];
    if (off + n > len) return;
    std::string gid((const char*)body + off, n); off += n;
    if (off + 1 > len) return;
    uint8_t n2 = body[off++];
    if (off + n2 > len) return;
    std::string account((const char*)body + off, n2); off += n2;
    if (off + 1 > len) return;
    uint8_t n3 = body[off++];
    if (off + n3 > len) return;
    std::string aaa_cookie((const char*)body + off, n3); off += n3;
    if (off + 1 > len) return;
    uint8_t n4 = body[off++];
    if (off + n4 > len) return;
    std::string gs_para1((const char*)body + off, n4); off += n4;
    if (off + 1 > len) return;
    uint8_t n5 = body[off++];
    if (off + n5 > len) return;
    std::string gs_para2((const char*)body + off, n5); off += n5;
    uint16_t result = rd16be(body, off); off += 2;
    uint32_t v32 = rd32be(body, off); off += 4;
    uint32_t v36 = rd32be(body, off); off += 4;
    uint32_t v40 = rd32be(body, off); off += 4;
    uint32_t v44 = rd32be(body, off); off += 4;
    uint32_t v46 = rd32be(body, off); off += 4;
    applyLpc("msg_l_add_coin", {make_int(cl.clientNo), make_string(gid.data(), gid.size()), make_string(account.data(), account.size()), make_string(aaa_cookie.data(), aaa_cookie.size()), make_string(gs_para1.data(), gs_para1.size()), make_string(gs_para2.data(), gs_para2.size()), make_int(result), make_int(v32), make_int(v36), make_int(v40), make_int(v44), make_int(v46)});
}

void handle_vm_msgLCostFillInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    uint32_t off = 0;
    if (off + 1 > len) return;
    uint8_t n = body[off++];
    if (off + n > len) return;
    std::string account((const char*)body + off, n); off += n;
    if (off + 1 > len) return;
    uint8_t n2 = body[off++];
    if (off + n2 > len) return;
    std::string gid((const char*)body + off, n2); off += n2;
    uint16_t times = rd16be(body, off); off += 2;
    uint16_t result = rd16be(body, off); off += 2;
    if (off + 1 > len) return;
    uint8_t n3 = body[off++];
    if (off + n3 > len) return;
    std::string type((const char*)body + off, n3); off += n3;
    applyLpc("msg_l_cost_fill_info", {make_int(cl.clientNo), make_string(account.data(), account.size()), make_string(gid.data(), gid.size()), make_int(times), make_int(result), make_string(type.data(), type.size())});
}

void handle_vm_msgLEnableAutoLock(GameClient& cl, const uint8_t* body, size_t len)
{
    uint32_t off = 0;
    if (off + 1 > len) return;
    uint8_t n = body[off++];
    if (off + n > len) return;
    std::string account((const char*)body + off, n); off += n;
    uint16_t v12 = rd16be(body, off); off += 2;
    uint16_t v13 = rd16be(body, off); off += 2;
    applyLpc("msg_l_enable_auto_lock", {make_int(cl.clientNo), make_string(account.data(), account.size()), make_int(v12), make_int(v13)});
}

void handle_vm_msgLLockAccount(GameClient& cl, const uint8_t* body, size_t len)
{
    uint32_t off = 0;
    if (off + 1 > len) return;
    uint8_t n = body[off++];
    if (off + n > len) return;
    std::string account((const char*)body + off, n); off += n;
    if (off + 1 > len) return;
    uint8_t n2 = body[off++];
    if (off + n2 > len) return;
    std::string ret((const char*)body + off, n2); off += n2;
    if (off + 1 > len) return;
    uint8_t n3 = body[off++];
    if (off + n3 > len) return;
    std::string msg((const char*)body + off, n3); off += n3;
    applyLpc("msg_l_lock_account", {make_int(cl.clientNo), make_string(account.data(), account.size()), make_string(ret.data(), ret.size()), make_string(msg.data(), msg.size())});
}

void handle_vm_msgCcsFriendStatus(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    uint8_t fnlen = body[off++];
    if (off + fnlen > len) return;
    auto friend_name = make_string(body + off, fnlen);
    off += fnlen;
    if (off >= len) return;
    uint8_t unlen = body[off++];
    if (off + unlen > len) return;
    auto user_name = make_string(body + off, unlen);
    off += unlen;
    if (off + 2 > len) return;
    uint16_t status = rd16be(body, off);
    off += 2;
    applyLpc("msg_ccs_friend_status", {make_int(cl.clientNo), friend_name, user_name, make_int(status)});
}

void handle_vm_msgCcsOnlineFriend(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    uint8_t fnlen = body[off++];
    if (off + fnlen > len) return;
    auto friend_name = make_string(body + off, fnlen);
    off += fnlen;
    if (off >= len) return;
    uint8_t unlen = body[off++];
    if (off + unlen > len) return;
    auto user_name = make_string(body + off, unlen);
    off += unlen;
    applyLpc("msg_ccs_online_friend", {make_int(cl.clientNo), friend_name, user_name});
}

void handle_vm_msgCcsWorldChannel(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    uint8_t mlen = body[off++];
    if (off + mlen > len) return;
    auto msg = make_string(body + off, mlen);
    off += mlen;
    applyLpc("msg_ccs_world_channel", {make_int(cl.clientNo), msg});
}

void handle_vm_msgCcsSystemChannel(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    uint8_t mlen = body[off++];
    if (off + mlen > len) return;
    auto msg = make_string(body + off, mlen);
    off += mlen;
    applyLpc("msg_ccs_system_channel", {make_int(cl.clientNo), msg});
}

void handle_vm_msgGotServerTime(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    uint8_t nlen = body[off++];
    if (off + nlen > len) return;
    auto name = make_string(body + off, nlen);
    off += nlen;
    if (off + 4 > len) return;
    uint32_t time = rd32be(body, off);
    off += 4;
    applyLpc("msg_got_server_time", {make_int(cl.clientNo), name, make_int(time)});
}

void handle_vm_msgGsEmoteData(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    uint8_t vlen = body[off++];
    if (off + vlen > len) return;
    auto verb = make_string(body + off, vlen);
    off += vlen;
    if (off >= len) return;
    uint8_t tlen = body[off++];
    if (off + tlen > len) return;
    auto target = make_string(body + off, tlen);
    off += tlen;
    if (off + 2 > len) return;
    uint16_t channel = rd16be(body, off);
    off += 2;
    if (off >= len) return;
    uint8_t plen = body[off++];
    if (off + plen > len) return;
    auto peer = make_string(body + off, plen);
    off += plen;
    if (off + 2 > len) return;
    off += 2; // 2 bytes skipped, never read (per asm add p,2 / sub mlen,2)
    auto tail = make_string(body + off, len - off);
    applyLpc("msg_gs_emote_data", {make_int(cl.clientNo), verb, target, make_int(channel), peer, tail});
}

void handle_vm_msgGsFinger(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    uint8_t nlen = body[off++];
    if (off + nlen > len) return;
    auto char_name = make_string(body + off, nlen);
    off += nlen;
    applyLpc("msg_gs_finger", {make_int(cl.clientNo), char_name});
}

void handle_vm_msgGsFriendList(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    uint8_t unlen = body[off++];
    if (off + unlen > len) return;
    auto user_name = make_string(body + off, unlen);
    off += unlen;
    if (off + 2 > len) return;
    uint16_t count = rd16be(body, off);
    off += 2;
    auto arr = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < count; ++i) {
        if (off >= len) return;
        uint8_t flen = body[off++];
        if (off + flen > len) return;
        arr->items.push_back(make_string(body + off, flen));
        off += flen;
    }
    applyLpc("msg_gs_friend_list", {make_int(cl.clientNo), user_name, arr});
}

void handle_vm_msgGsFriendTell(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    uint8_t flen = body[off++];
    if (off + flen > len) return;
    auto from = make_string(body + off, flen);
    off += flen;
    if (off >= len) return;
    uint8_t tlen = body[off++];
    if (off + tlen > len) return;
    auto to = make_string(body + off, tlen);
    off += tlen;
    if (off >= len) return;
    uint8_t mlen = body[off++];
    if (off + mlen > len) return;
    auto msg = make_string(body + off, mlen);
    off += mlen;
    if (off + 4 > len) return;
    uint32_t time = rd32be(body, off);
    off += 4;
    applyLpc("msg_gs_friend_tell", {make_int(cl.clientNo), from, to, msg, make_int(time)});
}

void handle_vm_msgGsLogin(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    // string id (u8 len)
    uint8_t idLen = rd8(body, off); off += 1;
    if (off + idLen > len) return;
    const uint8_t* id = body + off; off += idLen;
    // string name (u8 len)
    uint8_t nameLen = rd8(body, off); off += 1;
    if (off + nameLen > len) return;
    const uint8_t* name = body + off; off += nameLen;
    applyLpc("msg_gs_login", {make_int(cl.clientNo), make_string(id, idLen), make_string(name, nameLen)});
}

void handle_vm_msgGsLogout(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    // string id (u8 len)
    uint8_t idLen = rd8(body, off); off += 1;
    if (off + idLen > len) return;
    const uint8_t* id = body + off; off += idLen;
    // string name (u8 len)
    uint8_t nameLen = rd8(body, off); off += 1;
    if (off + nameLen > len) return;
    const uint8_t* name = body + off; off += nameLen;
    applyLpc("msg_gs_logout", {make_int(cl.clientNo), make_string(id, idLen), make_string(name, nameLen)});
}

void handle_vm_msgGsWorldChannel(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    // string msg (u8 len)
    uint8_t msgLen = rd8(body, off); off += 1;
    if (off + msgLen > len) return;
    const uint8_t* msg = body + off; off += msgLen;
    applyLpc("msg_gs_world_channel", {make_int(cl.clientNo), make_string(msg, msgLen)});
}

void handle_vm_msgGsSystemChannel(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    // string msg (u8 len)
    uint8_t msgLen = rd8(body, off); off += 1;
    if (off + msgLen > len) return;
    const uint8_t* msg = body + off; off += msgLen;
    applyLpc("msg_gs_system_channel", {make_int(cl.clientNo), make_string(msg, msgLen)});
}

void handle_vm_msgRpcServerName(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    // string name (u8 len)
    uint8_t nameLen = rd8(body, off); off += 1;
    if (off + nameLen > len) return;
    const uint8_t* name = body + off; off += nameLen;
    // string version (u8 len)
    uint8_t versionLen = rd8(body, off); off += 1;
    if (off + versionLen > len) return;
    const uint8_t* version = body + off; off += versionLen;
    // int val (u32 BE)
    if (off + 4 > len) return;
    uint32_t val = rd32be(body, off); off += 4;
    applyLpc("msg_rpc_server_name", {make_int(cl.clientNo), make_string(name, nameLen), make_string(version, versionLen), make_int((int)val)});
}

void handle_vm_msgIntersvrChannelMsg(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    // int channel (u16 BE)
    uint16_t channel = rd16be(body, off); off += 2;
    // string sender (u8 len)
    uint8_t senderLen = rd8(body, off); off += 1;
    if (off + senderLen > len) return;
    const uint8_t* sender = body + off; off += senderLen;
    // buffer data (u16 BE len)
    uint16_t dataLen = rd16be(body, off); off += 2;
    if (off + dataLen > len) return;
    const uint8_t* data = body + off; off += dataLen;
    // string msg (u16 BE len)
    uint16_t msgLen = rd16be(body, off); off += 2;
    if (off + msgLen > len) return;
    const uint8_t* msg = body + off; off += msgLen;
    applyLpc("msg_intersvr_channel_msg", {make_int(cl.clientNo), make_int(channel), make_string(sender, senderLen), make_buffer(data, dataLen), make_string(msg, msgLen)});
}

void handle_vm_msgDataMining(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    // string type (u8 len)
    uint8_t typeLen = rd8(body, off); off += 1;
    if (off + typeLen > len) return;
    const uint8_t* type = body + off; off += typeLen;
    // buffer data (u16 BE len)
    uint16_t dataLen = rd16be(body, off); off += 2;
    if (off + dataLen > len) return;
    const uint8_t* data = body + off; off += dataLen;
    applyLpc("msg_data_mining", {make_int(cl.clientNo), make_string(type, typeLen), make_buffer(data, dataLen)});
}

void handle_vm_cmdLeagueOper(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    // string command (u8 len)
    uint8_t cmdLen = rd8(body, off); off += 1;
    if (off + cmdLen > len) return;
    const uint8_t* command = body + off; off += cmdLen;
    // array of strings: u16 BE count, then count x (u16 BE len + bytes)
    uint16_t count = rd16be(body, off); off += 2;
    auto arr = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < count; ++i) {
        uint16_t elLen = rd16be(body, off); off += 2;
        if (off + elLen > len) return;
        arr->items.push_back(make_string(body + off, elLen));
        off += elLen;
    }
    applyLpc("cmd_league_oper", {make_int(cl.clientNo), make_string(command, cmdLen), arr});
}

void handle_vm_msgLeagueOper(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    // string command (u8 len)
    uint8_t cmdLen = rd8(body, off); off += 1;
    if (off + cmdLen > len) return;
    const uint8_t* command = body + off; off += cmdLen;
    // array of strings: u16 BE count, then count x (u16 BE len + bytes)
    uint16_t count = rd16be(body, off); off += 2;
    auto arr = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < count; ++i) {
        uint16_t elLen = rd16be(body, off); off += 2;
        if (off + elLen > len) return;
        arr->items.push_back(make_string(body + off, elLen));
        off += elLen;
    }
    applyLpc("msg_league_oper", {make_int(cl.clientNo), make_string(command, cmdLen), arr});
}

void handle_vm_cmdBottleOper(GameClient& cl, const uint8_t* body, size_t len)
{
  size_t off = 0;
  if (off + 1 > len) return;
  uint8_t cmdLen = body[off++];
  if (off + cmdLen > len) return;
  const uint8_t* cmd = body + off; off += cmdLen;
  if (off + 2 > len) return;
  uint16_t cnt = rd16be(body, off); off += 2;
  auto arr = std::make_shared<ArrayValue>();
  for (uint16_t i = 0; i < cnt; ++i) {
    if (off + 2 > len) return;
    uint16_t slen = rd16be(body, off); off += 2;
    if (off + slen > len) return;
    arr->items.push_back(make_string(body + off, slen));
    off += slen;
  }
  applyLpc("cmd_bottle_oper", {make_int(cl.clientNo), make_string(cmd, cmdLen), arr});
}

void handle_vm_msgBottleOper(GameClient& cl, const uint8_t* body, size_t len)
{
  size_t off = 0;
  if (off + 1 > len) return;
  uint8_t cmdLen = body[off++];
  if (off + cmdLen > len) return;
  const uint8_t* cmd = body + off; off += cmdLen;
  if (off + 2 > len) return;
  uint16_t cnt = rd16be(body, off); off += 2;
  auto arr = std::make_shared<ArrayValue>();
  for (uint16_t i = 0; i < cnt; ++i) {
    if (off + 2 > len) return;
    uint16_t slen = rd16be(body, off); off += 2;
    if (off + slen > len) return;
    arr->items.push_back(make_string(body + off, slen));
    off += slen;
  }
  applyLpc("msg_bottle_oper", {make_int(cl.clientNo), make_string(cmd, cmdLen), arr});
}

void handle_vm_cmdXGameOper(GameClient& cl, const uint8_t* body, size_t len)
{
  size_t off = 0;
  if (off + 1 > len) return;
  uint8_t cmdLen = body[off++];
  if (off + cmdLen > len) return;
  const uint8_t* cmd = body + off; off += cmdLen;
  if (off + 2 > len) return;
  uint16_t cnt = rd16be(body, off); off += 2;
  auto arr = std::make_shared<ArrayValue>();
  for (uint16_t i = 0; i < cnt; ++i) {
    if (off + 2 > len) return;
    uint16_t slen = rd16be(body, off); off += 2;
    if (off + slen > len) return;
    arr->items.push_back(make_string(body + off, slen));
    off += slen;
  }
  applyLpc("cmd_xgame_oper", {make_int(cl.clientNo), make_string(cmd, cmdLen), arr});
}

void handle_vm_msgXGameOper(GameClient& cl, const uint8_t* body, size_t len)
{
  size_t off = 0;
  if (off + 1 > len) return;
  uint8_t cmdLen = body[off++];
  if (off + cmdLen > len) return;
  const uint8_t* cmd = body + off; off += cmdLen;
  if (off + 2 > len) return;
  uint16_t cnt = rd16be(body, off); off += 2;
  auto arr = std::make_shared<ArrayValue>();
  for (uint16_t i = 0; i < cnt; ++i) {
    if (off + 2 > len) return;
    uint16_t slen = rd16be(body, off); off += 2;
    if (off + slen > len) return;
    arr->items.push_back(make_string(body + off, slen));
    off += slen;
  }
  applyLpc("msg_xgame_oper", {make_int(cl.clientNo), make_string(cmd, cmdLen), arr});
}

void handle_vm_msgDoInquiry(GameClient& cl, const uint8_t* body, size_t len)
{
  size_t off = 0;
  if (off + 1 > len) return;
  uint8_t gidLen = body[off++];
  if (off + gidLen > len) return;
  const uint8_t* gid = body + off; off += gidLen;
  if (off + 1 > len) return;
  uint8_t ordLen = body[off++];
  if (off + ordLen > len) return;
  const uint8_t* orderId = body + off; off += ordLen;
  if (off + 1 > len) return;
  uint8_t payLen = body[off++];
  if (off + payLen > len) return;
  const uint8_t* payType = body + off; off += payLen;
  if (off + 1 > len) return;
  uint8_t a = body[off++];
  if (off + 1 > len) return;
  uint8_t b = body[off++];
  if (off + 4 > len) return;
  uint32_t c = rd32be(body, off);
  applyLpc("msg_l_do_inquiry", {make_int(cl.clientNo), make_string(gid, gidLen), make_string(orderId, ordLen), make_string(payType, payLen), make_int(a), make_int(b), make_int(c)});
}

void handle_vm_msgDoRecharge(GameClient& cl, const uint8_t* body, size_t len)
{
  size_t off = 0;
  if (off + 1 > len) return;
  uint8_t gidLen = body[off++];
  if (off + gidLen > len) return;
  const uint8_t* gid = body + off; off += gidLen;
  if (off + 1 > len) return;
  uint8_t ordLen = body[off++];
  if (off + ordLen > len) return;
  const uint8_t* orderId = body + off; off += ordLen;
  if (off + 1 > len) return;
  uint8_t payLen = body[off++];
  if (off + payLen > len) return;
  const uint8_t* payType = body + off; off += payLen;
  if (off + 1 > len) return;
  uint8_t a = body[off++];
  if (off + 1 > len) return;
  uint8_t b = body[off++];
  if (off + 4 > len) return;
  uint32_t c = rd32be(body, off);
  applyLpc("msg_l_do_recharge", {make_int(cl.clientNo), make_string(gid, gidLen), make_string(orderId, ordLen), make_string(payType, payLen), make_int(a), make_int(b), make_int(c)});
}

void handle_vm_cmdPartywarOper(GameClient& cl, const uint8_t* body, size_t len)
{
  size_t off = 0;
  if (off + 1 > len) return;
  uint8_t cmdLen = body[off++];
  if (off + cmdLen > len) return;
  const uint8_t* cmd = body + off; off += cmdLen;
  if (off + 2 > len) return;
  uint16_t cnt = rd16be(body, off); off += 2;
  auto arr = std::make_shared<ArrayValue>();
  for (uint16_t i = 0; i < cnt; ++i) {
    if (off + 2 > len) return;
    uint16_t slen = rd16be(body, off); off += 2;
    if (off + slen > len) return;
    arr->items.push_back(make_string(body + off, slen));
    off += slen;
  }
  applyLpc("cmd_partywar_oper", {make_int(cl.clientNo), make_string(cmd, cmdLen), arr});
}

void handle_vm_msgPartywarOper(GameClient& cl, const uint8_t* body, size_t len)
{
  size_t off = 0;
  if (off + 1 > len) return;
  uint8_t cmdLen = body[off++];
  if (off + cmdLen > len) return;
  const uint8_t* cmd = body + off; off += cmdLen;
  if (off + 2 > len) return;
  uint16_t cnt = rd16be(body, off); off += 2;
  auto arr = std::make_shared<ArrayValue>();
  for (uint16_t i = 0; i < cnt; ++i) {
    if (off + 2 > len) return;
    uint16_t slen = rd16be(body, off); off += 2;
    if (off + slen > len) return;
    arr->items.push_back(make_string(body + off, slen));
    off += slen;
  }
  applyLpc("msg_partywar_oper", {make_int(cl.clientNo), make_string(cmd, cmdLen), arr});
}

void handle_vm_cmdArenaOper(GameClient& cl, const uint8_t* body, size_t len)
{
  size_t off = 0;
  if (off + 1 > len) return;
  uint8_t cmdLen = body[off++];
  if (off + cmdLen > len) return;
  const uint8_t* cmd = body + off; off += cmdLen;
  if (off + 2 > len) return;
  uint16_t cnt = rd16be(body, off); off += 2;
  auto arr = std::make_shared<ArrayValue>();
  for (uint16_t i = 0; i < cnt; ++i) {
    if (off + 2 > len) return;
    uint16_t slen = rd16be(body, off); off += 2;
    if (off + slen > len) return;
    arr->items.push_back(make_string(body + off, slen));
    off += slen;
  }
  applyLpc("cmd_arena_oper", {make_int(cl.clientNo), make_string(cmd, cmdLen), arr});
}

void handle_vm_msgArenaOper(GameClient& cl, const uint8_t* body, size_t len)
{
  size_t off = 0;
  if (off + 1 > len) return;
  uint8_t cmdLen = body[off++];
  if (off + cmdLen > len) return;
  const uint8_t* cmd = body + off; off += cmdLen;
  if (off + 2 > len) return;
  uint16_t cnt = rd16be(body, off); off += 2;
  auto arr = std::make_shared<ArrayValue>();
  for (uint16_t i = 0; i < cnt; ++i) {
    if (off + 2 > len) return;
    uint16_t slen = rd16be(body, off); off += 2;
    if (off + slen > len) return;
    arr->items.push_back(make_string(body + off, slen));
    off += slen;
  }
  applyLpc("msg_arena_oper", {make_int(cl.clientNo), make_string(cmd, cmdLen), arr});
}

void handle_vm_cmdDraftOper(GameClient& cl, const uint8_t* body, size_t len)
{
  size_t off = 0;
  if (off + 1 > len) return;
  uint8_t cmdLen = body[off++];
  if (off + cmdLen > len) return;
  const uint8_t* cmd = body + off; off += cmdLen;
  if (off + 2 > len) return;
  uint16_t cnt = rd16be(body, off); off += 2;
  auto arr = std::make_shared<ArrayValue>();
  for (uint16_t i = 0; i < cnt; ++i) {
    if (off + 2 > len) return;
    uint16_t slen = rd16be(body, off); off += 2;
    if (off + slen > len) return;
    arr->items.push_back(make_string(body + off, slen));
    off += slen;
  }
  applyLpc("cmd_draft_oper", {make_int(cl.clientNo), make_string(cmd, cmdLen), arr});
}

void handle_vm_msgDraftOper(GameClient& cl, const uint8_t* body, size_t len)
{
  size_t off = 0;
  if (off + 1 > len) return;
  uint8_t cmdLen = body[off++];
  if (off + cmdLen > len) return;
  const uint8_t* cmd = body + off; off += cmdLen;
  if (off + 2 > len) return;
  uint16_t cnt = rd16be(body, off); off += 2;
  auto arr = std::make_shared<ArrayValue>();
  for (uint16_t i = 0; i < cnt; ++i) {
    if (off + 2 > len) return;
    uint16_t slen = rd16be(body, off); off += 2;
    if (off + slen > len) return;
    arr->items.push_back(make_string(body + off, slen));
    off += slen;
  }
  applyLpc("msg_draft_oper", {make_int(cl.clientNo), make_string(cmd, cmdLen), arr});
}

void handle_vm_cmdSDDHOper(GameClient& cl, const uint8_t* body, size_t len)
{
  size_t off = 0;
  if (off + 1 > len) return;
  uint8_t cmdLen = body[off++];
  if (off + cmdLen > len) return;
  const uint8_t* cmd = body + off; off += cmdLen;
  if (off + 2 > len) return;
  uint16_t cnt = rd16be(body, off); off += 2;
  auto arr = std::make_shared<ArrayValue>();
  for (uint16_t i = 0; i < cnt; ++i) {
    if (off + 2 > len) return;
    uint16_t slen = rd16be(body, off); off += 2;
    if (off + slen > len) return;
    arr->items.push_back(make_string(body + off, slen));
    off += slen;
  }
  applyLpc("cmd_sddh_oper", {make_int(cl.clientNo), make_string(cmd, cmdLen), arr});
}

void handle_vm_msgSDDHOper(GameClient& cl, const uint8_t* body, size_t len)
{
  size_t off = 0;
  if (off + 1 > len) return;
  uint8_t cmdLen = body[off++];
  if (off + cmdLen > len) return;
  const uint8_t* cmd = body + off; off += cmdLen;
  if (off + 2 > len) return;
  uint16_t cnt = rd16be(body, off); off += 2;
  auto arr = std::make_shared<ArrayValue>();
  for (uint16_t i = 0; i < cnt; ++i) {
    if (off + 2 > len) return;
    uint16_t slen = rd16be(body, off); off += 2;
    if (off + slen > len) return;
    arr->items.push_back(make_string(body + off, slen));
    off += slen;
  }
  applyLpc("msg_sddh_oper", {make_int(cl.clientNo), make_string(cmd, cmdLen), arr});
}

void handle_vm_cmdCelebrityOper(GameClient& cl, const uint8_t* body, size_t len)
{
  size_t off = 0;
  if (off + 1 > len) return;
  uint8_t cmdLen = body[off++];
  if (off + cmdLen > len) return;
  const uint8_t* cmd = body + off; off += cmdLen;
  if (off + 2 > len) return;
  uint16_t cnt = rd16be(body, off); off += 2;
  auto arr = std::make_shared<ArrayValue>();
  for (uint16_t i = 0; i < cnt; ++i) {
    if (off + 2 > len) return;
    uint16_t slen = rd16be(body, off); off += 2;
    if (off + slen > len) return;
    arr->items.push_back(make_string(body + off, slen));
    off += slen;
  }
  applyLpc("cmd_celebrity_oper", {make_int(cl.clientNo), make_string(cmd, cmdLen), arr});
}

void handle_vm_msgCelebrityOper(GameClient& cl, const uint8_t* body, size_t len)
{
  size_t off = 0;
  if (off + 1 > len) return;
  uint8_t cmdLen = body[off++];
  if (off + cmdLen > len) return;
  const uint8_t* cmd = body + off; off += cmdLen;
  if (off + 2 > len) return;
  uint16_t cnt = rd16be(body, off); off += 2;
  auto arr = std::make_shared<ArrayValue>();
  for (uint16_t i = 0; i < cnt; ++i) {
    if (off + 2 > len) return;
    uint16_t slen = rd16be(body, off); off += 2;
    if (off + slen > len) return;
    arr->items.push_back(make_string(body + off, slen));
    off += slen;
  }
  applyLpc("msg_celebrity_oper", {make_int(cl.clientNo), make_string(cmd, cmdLen), arr});
}

void handle_vm_cmdSamsungOper(GameClient& cl, const uint8_t* body, size_t len)
{
  size_t off = 0;
  if (off + 1 > len) return;
  uint8_t cmdLen = body[off++];
  if (off + cmdLen > len) return;
  const uint8_t* cmd = body + off; off += cmdLen;
  if (off + 2 > len) return;
  uint16_t cnt = rd16be(body, off); off += 2;
  auto arr = std::make_shared<ArrayValue>();
  for (uint16_t i = 0; i < cnt; ++i) {
    if (off + 2 > len) return;
    uint16_t slen = rd16be(body, off); off += 2;
    if (off + slen > len) return;
    arr->items.push_back(make_string(body + off, slen));
    off += slen;
  }
  applyLpc("cmd_samsung_oper", {make_int(cl.clientNo), make_string(cmd, cmdLen), arr});
}

void handle_vm_msgSamsungOper(GameClient& cl, const uint8_t* body, size_t len)
{
  size_t off = 0;
  if (off + 1 > len) return;
  uint8_t cmdLen = body[off++];
  if (off + cmdLen > len) return;
  const uint8_t* cmd = body + off; off += cmdLen;
  if (off + 2 > len) return;
  uint16_t cnt = rd16be(body, off); off += 2;
  auto arr = std::make_shared<ArrayValue>();
  for (uint16_t i = 0; i < cnt; ++i) {
    if (off + 2 > len) return;
    uint16_t slen = rd16be(body, off); off += 2;
    if (off + slen > len) return;
    arr->items.push_back(make_string(body + off, slen));
    off += slen;
  }
  applyLpc("msg_samsung_oper", {make_int(cl.clientNo), make_string(cmd, cmdLen), arr});
}

void handle_vm_msgEveryonePkOper(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t cmdLen = rd8(body, off); off += 1;
    if (off + cmdLen > len) return;
    auto command = make_string(body + off, cmdLen); off += cmdLen;
    if (off + 2 > len) return;
    uint16_t count = rd16be(body, off); off += 2;
    auto arr = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < count; ++i) {
        if (off + 2 > len) return;
        uint16_t slen = rd16be(body, off); off += 2;
        if (off + slen > len) return;
        arr->items.push_back(make_string(body + off, slen)); off += slen;
    }
    applyLpc("msg_everyone_pk_oper", {make_int(cl.clientNo), command, arr});
}

void handle_vm_msgTreasureTradingOper(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t cmdLen = rd8(body, off); off += 1;
    if (off + cmdLen > len) return;
    auto command = make_string(body + off, cmdLen); off += cmdLen;
    if (off + 2 > len) return;
    uint16_t count = rd16be(body, off); off += 2;
    auto arr = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < count; ++i) {
        if (off + 2 > len) return;
        uint16_t slen = rd16be(body, off); off += 2;
        if (off + slen > len) return;
        arr->items.push_back(make_string(body + off, slen)); off += slen;
    }
    applyLpc("msg_treasure_trading_oper", {make_int(cl.clientNo), command, arr});
}

void handle_vm_msgLConvertSilverCoin(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n0 = rd8(body, off); off += 1;
    if (off + n0 > len) return;
    auto gid = make_string(body + off, n0); off += n0;
    if (off + 1 > len) return;
    uint8_t n1 = rd8(body, off); off += 1;
    if (off + n1 > len) return;
    auto account = make_string(body + off, n1); off += n1;
    if (off + 1 > len) return;
    uint8_t n2 = rd8(body, off); off += 1;
    if (off + n2 > len) return;
    auto aaa_cookie = make_string(body + off, n2); off += n2;
    if (off + 1 > len) return;
    uint8_t n3 = rd8(body, off); off += 1;
    if (off + n3 > len) return;
    auto gs_para1 = make_string(body + off, n3); off += n3;
    if (off + 1 > len) return;
    uint8_t n4 = rd8(body, off); off += 1;
    if (off + n4 > len) return;
    auto gs_para2 = make_string(body + off, n4); off += n4;
    if (off + 2 > len) return;
    uint16_t result = rd16be(body, off); off += 2;
    if (off + 4 > len) return;
    uint32_t v32 = rd32be(body, off); off += 4;
    if (off + 4 > len) return;
    uint32_t v36 = rd32be(body, off); off += 4;
    if (off + 4 > len) return;
    uint32_t v40 = rd32be(body, off); off += 4;
    if (off + 4 > len) return;
    uint32_t v42 = rd32be(body, off); off += 4;
    applyLpc("msg_l_convert_silver_coin", {make_int(cl.clientNo), gid, account, aaa_cookie, gs_para1, gs_para2, make_int(result), make_int(v32), make_int(v36), make_int(v40), make_int(v42)});
}

void handle_vm_cmdChannelOper(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t cmdLen = rd8(body, off); off += 1;
    if (off + cmdLen > len) return;
    auto command = make_string(body + off, cmdLen); off += cmdLen;
    if (off + 2 > len) return;
    uint16_t count = rd16be(body, off); off += 2;
    auto arr = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < count; ++i) {
        if (off + 2 > len) return;
        uint16_t slen = rd16be(body, off); off += 2;
        if (off + slen > len) return;
        arr->items.push_back(make_string(body + off, slen)); off += slen;
    }
    applyLpc("cmd_channel_oper", {make_int(cl.clientNo), command, arr});
}

void handle_vm_msgChannelOper(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t cmdLen = rd8(body, off); off += 1;
    if (off + cmdLen > len) return;
    auto command = make_string(body + off, cmdLen); off += cmdLen;
    if (off + 2 > len) return;
    uint16_t count = rd16be(body, off); off += 2;
    auto arr = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < count; ++i) {
        if (off + 2 > len) return;
        uint16_t slen = rd16be(body, off); off += 2;
        if (off + slen > len) return;
        arr->items.push_back(make_string(body + off, slen)); off += slen;
    }
    applyLpc("msg_channel_oper", {make_int(cl.clientNo), command, arr});
}

void handle_vm_cmdCsaCommonOper(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t cmdLen = rd8(body, off); off += 1;
    if (off + cmdLen > len) return;
    auto command = make_string(body + off, cmdLen); off += cmdLen;
    if (off + 2 > len) return;
    uint16_t count = rd16be(body, off); off += 2;
    auto arr = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < count; ++i) {
        if (off + 2 > len) return;
        uint16_t slen = rd16be(body, off); off += 2;
        if (off + slen > len) return;
        arr->items.push_back(make_string(body + off, slen)); off += slen;
    }
    applyLpc("cmd_csa_common_oper", {make_int(cl.clientNo), command, arr});
}

void handle_vm_msgCsaCommonOper(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t cmdLen = rd8(body, off); off += 1;
    if (off + cmdLen > len) return;
    auto command = make_string(body + off, cmdLen); off += cmdLen;
    if (off + 2 > len) return;
    uint16_t count = rd16be(body, off); off += 2;
    auto arr = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < count; ++i) {
        if (off + 2 > len) return;
        uint16_t slen = rd16be(body, off); off += 2;
        if (off + slen > len) return;
        arr->items.push_back(make_string(body + off, slen)); off += slen;
    }
    applyLpc("msg_csa_common_oper", {make_int(cl.clientNo), command, arr});
}

void handle_vm_cmdCitywarOper(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t cmdLen = rd8(body, off); off += 1;
    if (off + cmdLen > len) return;
    auto command = make_string(body + off, cmdLen); off += cmdLen;
    if (off + 2 > len) return;
    uint16_t count = rd16be(body, off); off += 2;
    auto arr = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < count; ++i) {
        if (off + 2 > len) return;
        uint16_t slen = rd16be(body, off); off += 2;
        if (off + slen > len) return;
        arr->items.push_back(make_string(body + off, slen)); off += slen;
    }
    applyLpc("cmd_citywar_oper", {make_int(cl.clientNo), command, arr});
}

void handle_vm_msgCitywarOper(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t cmdLen = rd8(body, off); off += 1;
    if (off + cmdLen > len) return;
    auto command = make_string(body + off, cmdLen); off += cmdLen;
    if (off + 2 > len) return;
    uint16_t count = rd16be(body, off); off += 2;
    auto arr = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < count; ++i) {
        if (off + 2 > len) return;
        uint16_t slen = rd16be(body, off); off += 2;
        if (off + slen > len) return;
        arr->items.push_back(make_string(body + off, slen)); off += slen;
    }
    applyLpc("msg_citywar_oper", {make_int(cl.clientNo), command, arr});
}

void handle_vm_cmdActivityOper(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t cmdLen = rd8(body, off); off += 1;
    if (off + cmdLen > len) return;
    auto command = make_string(body + off, cmdLen); off += cmdLen;
    if (off + 2 > len) return;
    uint16_t count = rd16be(body, off); off += 2;
    auto arr = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < count; ++i) {
        if (off + 2 > len) return;
        uint16_t slen = rd16be(body, off); off += 2;
        if (off + slen > len) return;
        arr->items.push_back(make_string(body + off, slen)); off += slen;
    }
    applyLpc("cmd_activity_oper", {make_int(cl.clientNo), command, arr});
}

void handle_vm_msgActivityOper(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t cmdLen = rd8(body, off); off += 1;
    if (off + cmdLen > len) return;
    auto command = make_string(body + off, cmdLen); off += cmdLen;
    if (off + 2 > len) return;
    uint16_t count = rd16be(body, off); off += 2;
    auto arr = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < count; ++i) {
        if (off + 2 > len) return;
        uint16_t slen = rd16be(body, off); off += 2;
        if (off + slen > len) return;
        arr->items.push_back(make_string(body + off, slen)); off += slen;
    }
    applyLpc("msg_activity_oper", {make_int(cl.clientNo), command, arr});
}

void handle_vm_msgLLockCard(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t cookieLen = body[off++];
    if (off + cookieLen > len) return;
    const uint8_t* cookie = body + off; off += cookieLen;
    if (off + 1 > len) return;
    uint8_t numberLen = body[off++];
    if (off + numberLen > len) return;
    const uint8_t* number = body + off; off += numberLen;
    if (off + 2 > len) return;
    uint16_t n1 = rd16be(body, off); off += 2;
    if (off + 4 > len) return;
    uint32_t n2 = rd32be(body, off); off += 4;
    applyLpc("msg_l_lock_card", {make_int(cl.clientNo), make_string(cookie, cookieLen), make_string(number, numberLen), make_int(n1), make_int(n2)});
}

void handle_vm_msgLManageVipSms(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t gidLen = body[off++];
    if (off + gidLen > len) return;
    const uint8_t* gid = body + off; off += gidLen;
    if (off + 2 > len) return;
    uint16_t oper = rd16be(body, off); off += 2;
    if (off + 2 > len) return;
    uint16_t ret = rd16be(body, off); off += 2;
    if (off + 1 > len) return;
    uint8_t paraLen = body[off++];
    if (off + paraLen > len) return;
    const uint8_t* para = body + off; off += paraLen;
    applyLpc("msg_l_manage_vip_sms", {make_int(cl.clientNo), make_string(gid, gidLen), make_int(oper), make_int(ret), make_string(para, paraLen)});
}

void handle_vm_msgLManageMpDevice(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t gidLen = body[off++];
    if (off + gidLen > len) return;
    const uint8_t* gid = body + off; off += gidLen;
    if (off + 1 > len) return;
    uint8_t oper = body[off++];
    if (off + 1 > len) return;
    uint8_t ret = body[off++];
    if (off + 1 > len) return;
    uint8_t paraLen = body[off++];
    if (off + paraLen > len) return;
    const uint8_t* para = body + off; off += paraLen;
    applyLpc("msg_l_manage_mp_device", {make_int(cl.clientNo), make_string(gid, gidLen), make_int(oper), make_int(ret), make_string(para, paraLen)});
}

void handle_vm_msgLPrecostCoin(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t gidLen = body[off++];
    if (off + gidLen > len) return;
    const uint8_t* gid = body + off; off += gidLen;
    if (off + 1 > len) return;
    uint8_t accountLen = body[off++];
    if (off + accountLen > len) return;
    const uint8_t* account = body + off; off += accountLen;
    if (off + 1 > len) return;
    uint8_t cookieLen = body[off++];
    if (off + cookieLen > len) return;
    const uint8_t* cookie = body + off; off += cookieLen;
    if (off + 2 > len) return;
    uint16_t result = rd16be(body, off); off += 2;
    if (off + 1 > len) return;
    uint8_t p1Len = body[off++];
    if (off + p1Len > len) return;
    const uint8_t* gs_para1 = body + off; off += p1Len;
    if (off + 2 > len) return;
    uint16_t p2Len = rd16be(body, off); off += 2;
    if (off + p2Len > len) return;
    const uint8_t* gs_para2 = body + off; off += p2Len;
    if (off + 1 > len) return;
    uint8_t pwdLen = body[off++];
    if (off + pwdLen > len) return;
    const uint8_t* pwd = body + off; off += pwdLen;
    if (off + 4 > len) return;
    uint32_t n1 = rd32be(body, off); off += 4;
    if (off + 4 > len) return;
    uint32_t n2 = rd32be(body, off); off += 4;
    applyLpc("msg_l_precost_coin", {make_int(cl.clientNo), make_string(gid, gidLen), make_string(account, accountLen), make_string(cookie, cookieLen), make_int(result), make_string(gs_para1, p1Len), make_string(gs_para2, p2Len), make_string(pwd, pwdLen), make_int(n1), make_int(n2)});
}

void handle_vm_msgLQueryAccountState(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t accountLen = body[off++];
    if (off + accountLen > len) return;
    const uint8_t* account = body + off; off += accountLen;
    if (off + 1 > len) return;
    uint8_t retLen = body[off++];
    if (off + retLen > len) return;
    const uint8_t* ret = body + off; off += retLen;
    applyLpc("msg_l_query_account_state", {make_int(cl.clientNo), make_string(account, accountLen), make_string(ret, retLen)});
}

void handle_vm_msgLQueryCard(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t cookieLen = body[off++];
    if (off + cookieLen > len) return;
    const uint8_t* cookie = body + off; off += cookieLen;
    if (off + 2 > len) return;
    uint16_t n1 = rd16be(body, off); off += 2;
    if (off + 4 > len) return;
    uint32_t n2 = rd32be(body, off); off += 4;
    if (off + 2 > len) return;
    uint16_t n3 = rd16be(body, off); off += 2;
    if (off + 2 > len) return;
    uint16_t n4 = rd16be(body, off); off += 2;
    applyLpc("msg_l_query_card", {make_int(cl.clientNo), make_string(cookie, cookieLen), make_int(n1), make_int(n2), make_int(n3), make_int(n4)});
}

void handle_vm_msgLQueryCoin(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t gidLen = body[off++];
    if (off + gidLen > len) return;
    const uint8_t* gid = body + off; off += gidLen;
    if (off + 1 > len) return;
    uint8_t accountLen = body[off++];
    if (off + accountLen > len) return;
    const uint8_t* account = body + off; off += accountLen;
    if (off + 1 > len) return;
    uint8_t p1Len = body[off++];
    if (off + p1Len > len) return;
    const uint8_t* gs_para1 = body + off; off += p1Len;
    if (off + 1 > len) return;
    uint8_t p2Len = body[off++];
    if (off + p2Len > len) return;
    const uint8_t* gs_para2 = body + off; off += p2Len;
    if (off + 2 > len) return;
    uint16_t have_coin_pwd = rd16be(body, off); off += 2;
    if (off + 4 > len) return;
    uint32_t n1 = rd32be(body, off); off += 4;
    if (off + 4 > len) return;
    uint32_t n2 = rd32be(body, off); off += 4;
    if (off + 2 > len) return;
    uint16_t lastLen = rd16be(body, off); off += 2;
    if (off + lastLen > len) return;
    const uint8_t* last = body + off; off += lastLen;
    applyLpc("msg_l_query_coin", {make_int(cl.clientNo), make_string(gid, gidLen), make_string(account, accountLen), make_string(gs_para1, p1Len), make_string(gs_para2, p2Len), make_int(have_coin_pwd), make_int(n1), make_int(n2), make_string(last, lastLen)});
}

void handle_vm_msgLQueryFillInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t accountLen = body[off++];
    if (off + accountLen > len) return;
    const uint8_t* account = body + off; off += accountLen;
    if (off + 1 > len) return;
    uint8_t gidLen = body[off++];
    if (off + gidLen > len) return;
    const uint8_t* gid = body + off; off += gidLen;
    if (off + 2 > len) return;
    uint16_t result = rd16be(body, off); off += 2;
    if (off + 1 > len) return;
    uint8_t typeLen = body[off++];
    if (off + typeLen > len) return;
    const uint8_t* type = body + off; off += typeLen;
    applyLpc("msg_l_query_fill_info", {make_int(cl.clientNo), make_string(account, accountLen), make_string(gid, gidLen), make_int(result), make_string(type, typeLen)});
}

void handle_vm_msgLQueryAddiction(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t accountLen = body[off++];
    if (off + accountLen > len) return;
    const uint8_t* account = body + off; off += accountLen;
    if (off + 1 > len) return;
    uint8_t retLen = body[off++];
    if (off + retLen > len) return;
    const uint8_t* result_text = body + off; off += retLen;
    applyLpc("msg_l_query_addiction", {make_int(cl.clientNo), make_string(account, accountLen), make_string(result_text, retLen)});
}

void handle_vm_msgLSendVipSms(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t gidLen = rd8(body, off); off += 1;
    if (off + gidLen > len) return;
    auto gid = make_string(body + off, gidLen); off += gidLen;
    if (off + 1 > len) return;
    uint8_t authLen = rd8(body, off); off += 1;
    if (off + authLen > len) return;
    auto authType = make_string(body + off, authLen); off += authLen;
    if (off + 1 > len) return;
    uint8_t smsTypeLen = rd8(body, off); off += 1;
    if (off + smsTypeLen > len) return;
    auto smsType = make_string(body + off, smsTypeLen); off += smsTypeLen;
    if (off + 1 > len) return;
    uint8_t smsCodeLen = rd8(body, off); off += 1;
    if (off + smsCodeLen > len) return;
    auto smsCode = make_string(body + off, smsCodeLen); off += smsCodeLen;
    if (off + 2 > len) return;
    uint16_t vip = rd16be(body, off); off += 2;
    applyLpc("msg_l_send_vip_sms", {make_int(cl.clientNo), gid, authType, smsType, smsCode, make_int(vip)});
}

void handle_vm_msgLDoAlipayRecharge(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t gidLen = rd8(body, off); off += 1;
    if (off + gidLen > len) return;
    auto gid = make_string(body + off, gidLen); off += gidLen;
    if (off + 4 > len) return;
    uint32_t money = rd32be(body, off); off += 4;
    applyLpc("msg_l_do_alipay_recharge", {make_int(cl.clientNo), gid, make_int(money)});
}

void handle_vm_msgLSsbCopyAccount(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t userLen = rd8(body, off); off += 1;
    if (off + userLen > len) return;
    auto userName = make_string(body + off, userLen); off += userLen;
    if (off + 1 > len) return;
    uint8_t resLen = rd8(body, off); off += 1;
    if (off + resLen > len) return;
    auto result = make_string(body + off, resLen); off += resLen;
    applyLpc("msg_l_ssb_copy_account", {make_int(cl.clientNo), userName, result});
}

void handle_vm_msgLSsbUpdateAccount(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t userLen = rd8(body, off); off += 1;
    if (off + userLen > len) return;
    auto userName = make_string(body + off, userLen); off += userLen;
    if (off + 1 > len) return;
    uint8_t resLen = rd8(body, off); off += 1;
    if (off + resLen > len) return;
    auto result = make_string(body + off, resLen); off += resLen;
    applyLpc("msg_l_ssb_update_account", {make_int(cl.clientNo), userName, result});
}

void handle_vm_msgLGetSsbCombatConf(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t nameLen = rd32be(body, off); off += 4;
    if (off + nameLen > len) return;
    auto name = make_string(body + off, nameLen); off += nameLen;
    applyLpc("msg_l_get_ssb_combatd_conf", {make_int(cl.clientNo), name});
}

void handle_vm_msgLGetSsbFinishFlag(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t nameLen = rd8(body, off); off += 1;
    if (off + nameLen > len) return;
    auto name = make_string(body + off, nameLen); off += nameLen;
    applyLpc("msg_l_get_ssb_finish_flag", {make_int(cl.clientNo), name});
}

void handle_vm_msgLGetSsbPromptMsg(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t nameLen = rd32be(body, off); off += 4;
    if (off + nameLen > len) return;
    auto name = make_string(body + off, nameLen); off += nameLen;
    applyLpc("msg_l_get_ssb_prompt_msg", {make_int(cl.clientNo), name});
}

void handle_vm_msgLGetSsbCombatRes(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t nameLen = rd32be(body, off); off += 4;
    if (off + nameLen > len) return;
    auto name = make_string(body + off, nameLen); off += nameLen;
    applyLpc("msg_l_get_ssb_combatd_res", {make_int(cl.clientNo), name});
}

void handle_vm_msgLRequestIpCard(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t gidLen = rd8(body, off); off += 1;
    if (off + gidLen > len) return;
    auto gid = make_string(body + off, gidLen); off += gidLen;
    if (off + 1 > len) return;
    uint8_t typeLen = rd8(body, off); off += 1;
    if (off + typeLen > len) return;
    auto type = make_string(body + off, typeLen); off += typeLen;
    if (off + 1 > len) return;
    uint8_t cardLen = rd8(body, off); off += 1;
    if (off + cardLen > len) return;
    auto cardNo = make_string(body + off, cardLen); off += cardLen;
    if (off + 2 > len) return;
    uint16_t card = rd16be(body, off); off += 2;
    applyLpc("msg_l_request_ip_card", {make_int(cl.clientNo), gid, type, cardNo, make_int(card)});
}

void handle_vm_msgLRequestCoupon(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t gidLen = rd8(body, off); off += 1;
    if (off + gidLen > len) return;
    auto gid = make_string(body + off, gidLen); off += gidLen;
    if (off + 1 > len) return;
    uint8_t couponTypeLen = rd8(body, off); off += 1;
    if (off + couponTypeLen > len) return;
    auto couponType = make_string(body + off, couponTypeLen); off += couponTypeLen;
    if (off + 2 > len) return;
    uint16_t result = rd16be(body, off); off += 2;
    if (off + 1 > len) return;
    uint8_t typeLen = rd8(body, off); off += 1;
    if (off + typeLen > len) return;
    auto type = make_string(body + off, typeLen); off += typeLen;
    if (off + 1 > len) return;
    uint8_t paraLen = rd8(body, off); off += 1;
    if (off + paraLen > len) return;
    auto para = make_string(body + off, paraLen); off += paraLen;
    if (off + 1 > len) return;
    uint8_t cookieLen = rd8(body, off); off += 1;
    if (off + cookieLen > len) return;
    auto cookie = make_string(body + off, cookieLen); off += cookieLen;
    applyLpc("msg_l_request_coupon", {make_int(cl.clientNo), gid, couponType, make_int(result), type, para, cookie});
}

void handle_vm_msgLChangeCoupon(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t accountLen = rd8(body, off); off += 1;
    if (off + accountLen > len) return;
    auto account = make_string(body + off, accountLen); off += accountLen;
    if (off + 1 > len) return;
    uint8_t gidLen = rd8(body, off); off += 1;
    if (off + gidLen > len) return;
    auto gid = make_string(body + off, gidLen); off += gidLen;
    if (off + 2 > len) return;
    uint16_t result = rd16be(body, off); off += 2;
    if (off + 4 > len) return;
    uint32_t costOnce = rd32be(body, off); off += 4;
    if (off + 1 > len) return;
    uint8_t paraLen = rd8(body, off); off += 1;
    if (off + paraLen > len) return;
    auto para = make_string(body + off, paraLen); off += paraLen;
    if (off + 1 > len) return;
    uint8_t cookieLen = rd8(body, off); off += 1;
    if (off + cookieLen > len) return;
    auto cookie = make_string(body + off, cookieLen); off += cookieLen;
    applyLpc("msg_l_change_coupon", {make_int(cl.clientNo), account, gid, make_int(result), make_int(costOnce), para, cookie});
}

void handle_vm_msgLQueryRandomCoupon(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t accountLen = rd8(body, off); off += 1;
    if (off + accountLen > len) return;
    auto account = make_string(body + off, accountLen); off += accountLen;
    if (off + 1 > len) return;
    uint8_t gidLen = rd8(body, off); off += 1;
    if (off + gidLen > len) return;
    auto gid = make_string(body + off, gidLen); off += gidLen;
    if (off + 2 > len) return;
    uint16_t result = rd16be(body, off); off += 2;
    if (off + 1 > len) return;
    uint8_t paraLen = rd8(body, off); off += 1;
    if (off + paraLen > len) return;
    auto para = make_string(body + off, paraLen); off += paraLen;
    if (off + 4 > len) return;
    uint32_t value = rd32be(body, off); off += 4;
    applyLpc("msg_l_query_random_coupon", {make_int(cl.clientNo), account, gid, make_int(result), para, make_int(value)});
}

void handle_vm_msgUpdateFirstCreditBonus(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t accountLen = rd8(body, off); off += 1;
    if (off + accountLen > len) return;
    auto account = make_string(body + off, accountLen); off += accountLen;
    if (off + 1 > len) return;
    uint8_t gidLen = rd8(body, off); off += 1;
    if (off + gidLen > len) return;
    auto gid = make_string(body + off, gidLen); off += gidLen;
    if (off + 1 > len) return;
    uint8_t descLen = rd8(body, off); off += 1;
    if (off + descLen > len) return;
    auto resultDesc = make_string(body + off, descLen); off += descLen;
    applyLpc("msg_l_update_first_credit_bonus", {make_int(cl.clientNo), account, gid, resultDesc});
}

void handle_vm_msgUpdateRechargeReturnCredit(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t accountLen = rd8(body, off); off += 1;
    if (off + accountLen > len) return;
    auto account = make_string(body + off, accountLen); off += accountLen;
    if (off + 1 > len) return;
    uint8_t gidLen = rd8(body, off); off += 1;
    if (off + gidLen > len) return;
    auto gid = make_string(body + off, gidLen); off += gidLen;
    if (off + 4 > len) return;
    uint32_t val1 = rd32be(body, off); off += 4;
    if (off + 4 > len) return;
    uint32_t val2 = rd32be(body, off); off += 4;
    applyLpc("msg_l_update_recharge_return_credit", {make_int(cl.clientNo), account, gid, make_int(val1), make_int(val2)});
}

void handle_vm_msgLUpdateAddiction(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t accountLen = rd8(body, off); off += 1;
    if (off + accountLen > len) return;
    auto account = make_string(body + off, accountLen); off += accountLen;
    if (off + 1 > len) return;
    uint8_t textLen = rd8(body, off); off += 1;
    if (off + textLen > len) return;
    auto resultText = make_string(body + off, textLen); off += textLen;
    applyLpc("msg_l_update_addiction", {make_int(cl.clientNo), account, resultText});
}

void handle_vm_msgLUnlockCard(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t cookieLen = rd8(body, off); off += 1;
    if (off + cookieLen > len) return;
    auto cookie = make_string(body + off, cookieLen); off += cookieLen;
    if (off + 1 > len) return;
    uint8_t numberLen = rd8(body, off); off += 1;
    if (off + numberLen > len) return;
    auto number = make_string(body + off, numberLen); off += numberLen;
    if (off + 2 > len) return;
    uint16_t cardId = rd16be(body, off); off += 2;
    applyLpc("msg_l_unlock_card", {make_int(cl.clientNo), cookie, number, make_int(cardId)});
}

void handle_vm_msgLVerifyPassword(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t accountLen = rd8(body, off); off += 1;
    if (off + accountLen > len) return;
    auto account = make_string(body + off, accountLen); off += accountLen;
    if (off + 1 > len) return;
    uint8_t gidLen = rd8(body, off); off += 1;
    if (off + gidLen > len) return;
    auto gid = make_string(body + off, gidLen); off += gidLen;
    if (off + 1 > len) return;
    uint8_t typeLen = rd8(body, off); off += 1;
    if (off + typeLen > len) return;
    auto type = make_string(body + off, typeLen); off += typeLen;
    if (off + 2 > len) return;
    uint16_t result = rd16be(body, off); off += 2;
    applyLpc("msg_l_verify_password", {make_int(cl.clientNo), account, gid, type, make_int(result)});
}

void handle_vm_msgVerifyCodeResult(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t gidLen = rd8(body, off); off += 1;
    if (off + gidLen > len) return;
    auto gid = make_string(body + off, gidLen); off += gidLen;
    if (off + 1 > len) return;
    uint8_t codeLen = rd8(body, off); off += 1;
    if (off + codeLen > len) return;
    auto code = make_string(body + off, codeLen); off += codeLen;
    if (off + 1 > len) return;
    uint8_t typeLen = rd8(body, off); off += 1;
    if (off + typeLen > len) return;
    auto type = make_string(body + off, typeLen); off += typeLen;
    if (off + 2 > len) return;
    uint16_t result = rd16be(body, off); off += 2;
    applyLpc("msg_verify_code_result", {make_int(cl.clientNo), gid, code, type, make_int(result)});
}

void handle_vm_msgCfgAaaList(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 2 > len) return;
    uint16_t count = rd16be(body, off); off += 2;
    auto m = std::make_shared<MappingValue>();
    for (uint16_t i = 0; i < count; i++) {
        if (off + 1 > len) return;
        uint8_t nameLen = rd8(body, off); off += 1;
        if (off + nameLen > len) return;
        const uint8_t* name = body + off; off += nameLen;
        if (off + 1 > len) return;
        uint8_t ipLen = rd8(body, off); off += 1;
        if (off + ipLen > len) return;
        const uint8_t* ip = body + off; off += ipLen;
        if (off + 2 > len) return;
        uint16_t port = rd16be(body, off); off += 2;
        if (off + 1 > len) return;
        uint8_t attribLen = rd8(body, off); off += 1;
        if (off + attribLen > len) return;
        const uint8_t* attrib = body + off; off += attribLen;
        auto arr = std::make_shared<ArrayValue>();
        arr->items.push_back(make_string(ip, ipLen));
        arr->items.push_back(make_int(port));
        arr->items.push_back(make_string(attrib, attribLen));
        m->insert(make_string(name, nameLen), arr);
    }
    applyLpc("msg_cfg_aaa_list", {make_int(cl.clientNo), m});
}

void handle_vm_msgCfgCcsInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t ipLen = rd8(body, off); off += 1;
    if (off + ipLen > len) return;
    const uint8_t* ip = body + off; off += ipLen;
    if (off + 2 > len) return;
    uint16_t port = rd16be(body, off); off += 2;
    if (off + 1 > len) return;
    uint8_t nameLen = rd8(body, off); off += 1;
    if (off + nameLen > len) return;
    const uint8_t* name = body + off; off += nameLen;
    applyLpc("msg_cfg_ccs_info", {make_int(cl.clientNo), make_string(ip, ipLen), make_int(port), make_string(name, nameLen)});
}

void handle_vm_msgCfgDdbList(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 4; // u32 header, unused by parser
    if (off + 2 > len) return;
    uint16_t count = rd16be(body, off); off += 2;
    auto arr = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < count; i++) {
        if (off + 1 > len) return;
        uint8_t sLen = rd8(body, off); off += 1;
        if (off + sLen > len) return;
        arr->items.push_back(make_string(body + off, sLen));
        off += sLen;
    }
    applyLpc("msg_cfg_ddb_list", {make_int(cl.clientNo), arr});
}

void handle_vm_msgCfgDistList(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t distNo = rd32be(body, off); off += 4;
    if (off + 2 > len) return;
    uint16_t count = rd16be(body, off); off += 2;
    auto arr = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < count; i++) {
        if (off + 1 > len) return;
        uint8_t sLen = rd8(body, off); off += 1;
        if (off + sLen > len) return;
        arr->items.push_back(make_string(body + off, sLen));
        off += sLen;
    }
    applyLpc("msg_cfg_dist_list", {make_int(cl.clientNo), make_int(distNo), make_int(count), arr});
}

void handle_vm_msgCfgEtcPageData(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t pageNo = rd32be(body, off); off += 4;
    if (off + 4 > len) return;
    uint32_t dataLen = rd32be(body, off); off += 4;
    if (dataLen > len - off) return;
    if (std::getenv("LPCVM_TAIL"))
        std::cerr << "[cfg-pg] cookie=" << pageNo << " rawlen=" << (len - off) << "\n";
    applyLpc("msg_cfg_etc_page_data", {make_int(cl.clientNo), make_int(pageNo), make_buffer(body + off, len - off)});
}

void handle_vm_msgCfgMyDist(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t sLen = rd8(body, off); off += 1;
    if (off + sLen > len) return;
    applyLpc("msg_cfg_my_dist", {make_int(cl.clientNo), make_string(body + off, sLen)});
}

void handle_vm_msgCfgReadEtc(GameClient& cl, const uint8_t* body, size_t len)
{
    // Original vm_msgCfgReadEtc (0x8065dd0): parses two BE u32s from the
    // body and applies msg_cfg_read_etc(clientNo, u32a, u32b) on the MASTER
    // program.  start_ccs.o's msg_cfg_read_etc is declared with 3 args
    // (client_no, cookie, result) and forwards (cookie, result) to
    // /daemons/configd.c.  So the 3-arg form with clientNo FIRST is correct;
    // aaa answers the 17520 read-etc request with 9331 (cookie, result).
    size_t off = 0;
    if (off + 8 > len) return;
    uint32_t cookie = rd32be(body, off); off += 4;
    uint32_t result = rd32be(body, off); off += 4;
    if (std::getenv("LPCVM_TAIL"))
        std::cerr << "[cfg-res] cookie=" << cookie << " result=" << result << "\n";
    applyLpc("msg_cfg_read_etc", {make_int(cl.clientNo), make_int(cookie), make_int(result)});
}

void handle_vm_msgCfgWriteEtc(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 8 > len) return;
    uint32_t pageNo = rd32be(body, off); off += 4;
    uint32_t dataLen = rd32be(body, off); off += 4;
    applyLpc("msg_cfg_write_etc", {make_int(cl.clientNo), make_int(pageNo), make_int(dataLen)});
}

void handle_vm_msgCfgCsaInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 2; // u16 header, unused by parser
    if (off + 1 > len) return;
    uint8_t nameLen = rd8(body, off); off += 1;
    if (off + nameLen > len) return;
    const uint8_t* name = body + off; off += nameLen;
    if (off + 1 > len) return;
    uint8_t ipLen = rd8(body, off); off += 1;
    if (off + ipLen > len) return;
    const uint8_t* ip = body + off; off += ipLen;
    if (off + 2 > len) return;
    uint16_t port = rd16be(body, off); off += 2;
    applyLpc("msg_cfg_csa_info", {make_int(cl.clientNo), make_string(ip, ipLen), make_int(port), make_string(name, nameLen)});
}

void handle_vm_msgCfgTtsInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    // first 2 body bytes are skipped (unused u16) by the original handler
    size_t off = 2;
    if (off + 1 > len) return;
    uint8_t nameLen = body[off++];
    if (off + nameLen > len) return;
    auto name = make_string(body + off, nameLen);
    off += nameLen;
    if (off + 1 > len) return;
    uint8_t ipLen = body[off++];
    if (off + ipLen > len) return;
    auto ip = make_string(body + off, ipLen);
    off += ipLen;
    if (off + 2 > len) return;
    uint16_t port = rd16be(body, off);
    off += 2;
    (void)off;
    applyLpc("msg_cfg_tts_info", {make_int(cl.clientNo), ip, make_int(port), name});
}

void handle_vm_msgCfgAHServerInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t ipLen = body[off++];
    if (off + ipLen > len) return;
    auto ip = make_string(body + off, ipLen);
    off += ipLen;
    if (off + 2 > len) return;
    uint16_t port = rd16be(body, off);
    off += 2;
    if (off + 1 > len) return;
    uint8_t nameLen = body[off++];
    if (off + nameLen > len) return;
    auto name = make_string(body + off, nameLen);
    off += nameLen;
    (void)off;
    applyLpc("msg_cfg_ah_server_info", {make_int(cl.clientNo), ip, make_int(port), name});
}

void handle_vm_msgGetGoodsFromTts(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t gidLen = body[off++];
    if (off + gidLen > len) return;
    auto gid = make_string(body + off, gidLen);
    off += gidLen;
    if (off + 1 > len) return;
    uint8_t mode = body[off++];
    if (off + 4 > len) return;
    uint32_t storageFee = rd32be(body, off);
    off += 4;
    if (off + 2 > len) return;
    uint16_t type = rd16be(body, off);
    off += 2;
    if (off + 1 > len) return;
    uint8_t goodsIdLen = body[off++];
    if (off + goodsIdLen > len) return;
    auto goodsId = make_string(body + off, goodsIdLen);
    off += goodsIdLen;
    if (off + 4 > len) return;
    uint32_t dataLen = rd32be(body, off);
    off += 4;
    if (off + dataLen > len) return;
    auto data = make_buffer(body + off, dataLen);
    off += dataLen;
    (void)off;
    applyLpc("msg_get_goods_from_tts", {make_int(cl.clientNo), gid, make_int(mode),
        make_int((int)storageFee), make_int(type), goodsId, data});
}

void handle_vm_msgCheckRoleOnSale(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t accountLen = body[off++];
    if (off + accountLen > len) return;
    auto account = make_string(body + off, accountLen);
    off += accountLen;
    if (off + 4 > len) return;
    uint32_t roleId = rd32be(body, off);
    off += 4;
    if (off + 4 > len) return;
    uint32_t dataLen = rd32be(body, off);
    off += 4;
    if (off + dataLen > len) return;
    auto data = make_buffer(body + off, dataLen);
    off += dataLen;
    (void)off;
    applyLpc("msg_check_role_on_sale", {make_int(cl.clientNo), account, make_int((int)roleId), data});
}

void handle_vm_msgSellTreasure(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t idLen = body[off++];
    if (off + idLen > len) return;
    auto id = make_string(body + off, idLen);
    off += idLen;
    if (off + 2 > len) return;
    uint16_t type = rd16be(body, off);
    off += 2;
    if (off + 2 > len) return;
    uint16_t state = rd16be(body, off);
    off += 2;
    if (off + 4 > len) return;
    uint32_t saleStartTime = rd32be(body, off);
    off += 4;
    if (off + 2 > len) return;
    uint16_t day = rd16be(body, off);
    off += 2;
    if (off + 4 > len) return;
    uint32_t price = rd32be(body, off);
    off += 4;
    if (off + 4 > len) return;
    uint32_t upsetPrice = rd32be(body, off);
    off += 4;
    if (off + 1 > len) return;
    uint8_t sellerLen = body[off++];
    if (off + sellerLen > len) return;
    auto seller = make_string(body + off, sellerLen);
    off += sellerLen;
    if (off + 1 > len) return;
    uint8_t sellerAccLen = body[off++];
    if (off + sellerAccLen > len) return;
    auto sellerAccount = make_string(body + off, sellerAccLen);
    off += sellerAccLen;
    if (off + 4 > len) return;
    uint32_t dataLen = rd32be(body, off);
    off += 4;
    if (off + dataLen > len) return;
    auto data = make_buffer(body + off, dataLen);
    off += dataLen;
    (void)off;
    applyLpc("msg_sell_treasure", {make_int(cl.clientNo), id, make_int(type), make_int(state),
        make_int((int)saleStartTime), make_int(day), make_int((int)price), make_int((int)upsetPrice),
        seller, sellerAccount, data});
}

void handle_vm_msgGetSysEnvelope(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t v1 = rd32be(body, off);
    off += 4;
    if (off + 4 > len) return;
    uint32_t v2 = rd32be(body, off);
    off += 4;
    if (off + 4 > len) return;
    uint32_t strLen = rd32be(body, off);
    off += 4;
    if (off + strLen > len) return;
    auto str = make_string(body + off, strLen);
    off += strLen;
    (void)off;
    applyLpc("msg_get_sys_envelope", {make_int(cl.clientNo), make_int((int)v1), make_int((int)v2), str});
}

void handle_vm_msgIntersvrRpcRet(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t cookie = rd32be(body, off);
    off += 4;
    if (off + 1 > len) return;
    uint8_t ownerLen = body[off++];
    if (off + ownerLen > len) return;
    auto owner = make_string(body + off, ownerLen);
    off += ownerLen;
    if (off + 2 > len) return;
    uint16_t funcLen = rd16be(body, off);
    off += 2;
    if (off + funcLen > len) return;
    auto func = make_string(body + off, funcLen);
    off += funcLen;
    // trailing data only when more than 3 bytes remain (u32be len + payload)
    auto data = make_buffer(body + off, 0);
    if (len - off > 3) {
        uint32_t dataLen = rd32be(body, off);
        off += 4;
        if (off + dataLen > len) return;
        data = make_buffer(body + off, dataLen);
        off += dataLen;
    }
    (void)off;
    applyLpc("msg_intersvr_rpc_ret", {make_int(cl.clientNo), make_int((int)cookie), owner, func, data});
}

void handle_vm_msgXgCreateName(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t v7 = rd32be(body, off);
    off += 4;
    if (off + 4 > len) return;
    uint32_t v9 = rd32be(body, off);
    off += 4;
    (void)off;
    applyLpc("msg_xg_create_name", {make_int(cl.clientNo), make_int((int)v7), make_int((int)v9)});
}

void handle_vm_msgXgGetAccount(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t cookie = rd32be(body, off);
    off += 4;
    if (off + 4 > len) return;
    uint32_t value = rd32be(body, off);
    off += 4;
    if (off + 1 > len) return;
    uint8_t accountLen = body[off++];
    if (off + accountLen > len) return;
    auto account = make_string(body + off, accountLen);
    off += accountLen;
    (void)off;
    applyLpc("msg_xg_get_account", {make_int(cl.clientNo), make_int((int)cookie), make_int((int)value), account});
}

void handle_vm_msgXgGetName(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t cookie = rd32be(body, off);
    off += 4;
    if (off + 4 > len) return;
    uint32_t result = rd32be(body, off);
    off += 4;
    if (off + 1 > len) return;
    uint8_t nameLen = body[off++];
    if (off + nameLen > len) return;
    auto name = make_string(body + off, nameLen);
    off += nameLen;
    (void)off;
    applyLpc("msg_xg_get_name", {make_int(cl.clientNo), make_int((int)cookie), make_int((int)result), name});
}

void handle_vm_msgLEcardBusiness(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t actionLen = body[off++];
    if (off + actionLen > len) return;
    auto action = make_string(body + off, actionLen);
    off += actionLen;
    if (off + 2 > len) return;
    uint16_t mapParaLen = rd16be(body, off);
    off += 2;
    if (off + mapParaLen > len) return;
    auto mapPara = make_string(body + off, mapParaLen);
    off += mapParaLen;
    if (off + 1 > len) return;
    uint8_t cookieLen = body[off++];
    if (off + cookieLen > len) return;
    auto cookie = make_string(body + off, cookieLen);
    off += cookieLen;
    (void)off;
    applyLpc("msg_l_ecard_business", {make_int(cl.clientNo), action, mapPara, cookie});
}

void handle_vm_msgLQueryIpArea(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t accountLen = body[off++];
    if (off + accountLen > len) return;
    auto account = make_string(body + off, accountLen);
    off += accountLen;
    if (off + 1 > len) return;
    uint8_t digitalIpLen = body[off++];
    if (off + digitalIpLen > len) return;
    auto digitalIp = make_string(body + off, digitalIpLen);
    off += digitalIpLen;
    if (off + 1 > len) return;
    uint8_t moduleLen = body[off++];
    if (off + moduleLen > len) return;
    auto module = make_string(body + off, moduleLen);
    off += moduleLen;
    if (off + 1 > len) return;
    uint8_t resultLen = body[off++];
    if (off + resultLen > len) return;
    auto result = make_string(body + off, resultLen);
    off += resultLen;
    (void)off;
    applyLpc("msg_l_query_ip_area", {make_int(cl.clientNo), account, digitalIp, module, result});
}

void handle_vm_msgLGetVoteResult(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t meLen = body[off++];
    if (off + meLen > len) return;
    auto me = make_string(body + off, meLen);
    off += meLen;
    if (off + 4 > len) return;
    uint32_t r1 = rd32be(body, off);
    off += 4;
    if (off + 4 > len) return;
    uint32_t r2 = rd32be(body, off);
    off += 4;
    if (off + 4 > len) return;
    uint32_t r3 = rd32be(body, off);
    off += 4;
    if (off + 4 > len) return;
    uint32_t r4 = rd32be(body, off);
    off += 4;
    (void)off;
    applyLpc("msg_l_get_vote_result", {make_int(cl.clientNo), me, make_int((int)r1), make_int((int)r2), make_int((int)r3), make_int((int)r4)});
}

void handle_vm_msgLCloneAccountResult(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t accountLen = body[off++];
    if (off + accountLen > len) return;
    auto account = make_string(body + off, accountLen);
    off += accountLen;
    if (off + 1 > len) return;
    uint8_t insertNameLen = body[off++];
    if (off + insertNameLen > len) return;
    auto insertName = make_string(body + off, insertNameLen);
    off += insertNameLen;
    if (off + 4 > len) return;
    uint32_t r1 = rd32be(body, off);
    off += 4;
    if (off + 4 > len) return;
    uint32_t r2 = rd32be(body, off);
    off += 4;
    (void)off;
    applyLpc("msg_l_clone_account_result", {make_int(cl.clientNo), account, insertName, make_int((int)r1), make_int((int)r2)});
}

void handle_vm_msgLSetSafeCtrlStr(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t moduleLen = body[off++];
    if (off + moduleLen > len) return;
    auto module = make_string(body + off, moduleLen);
    off += moduleLen;
    if (off + 1 > len) return;
    uint8_t accountLen = body[off++];
    if (off + accountLen > len) return;
    auto account = make_string(body + off, accountLen);
    off += accountLen;
    if (off + 4 > len) return;
    uint32_t id = rd32be(body, off);
    off += 4;
    if (off + 4 > len) return;
    uint32_t strLen = rd32be(body, off);
    off += 4;
    if (off + strLen > len) return;
    auto str = make_string(body + off, strLen);
    off += strLen;
    (void)off;
    applyLpc("msg_l_set_safe_ctrl_str", {make_int(cl.clientNo), module, account, make_int((int)id), str});
}

void handle_vm_msgLSetSafeCtrlVar(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t moduleLen = body[off++];
    if (off + moduleLen > len) return;
    auto module = make_string(body + off, moduleLen);
    off += moduleLen;
    if (off + 1 > len) return;
    uint8_t accountLen = body[off++];
    if (off + accountLen > len) return;
    auto account = make_string(body + off, accountLen);
    off += accountLen;
    if (off + 4 > len) return;
    uint32_t var1 = rd32be(body, off);
    off += 4;
    if (off + 4 > len) return;
    uint32_t var2 = rd32be(body, off);
    off += 4;
    (void)off;
    applyLpc("msg_l_set_safe_ctrl_var", {make_int(cl.clientNo), module, account, make_int((int)var1), make_int((int)var2)});
}

void handle_vm_msgLGetSafeCtrlInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t moduleLen = body[off++];
    if (off + moduleLen > len) return;
    auto module = make_string(body + off, moduleLen);
    off += moduleLen;
    if (off + 1 > len) return;
    uint8_t accountLen = body[off++];
    if (off + accountLen > len) return;
    auto account = make_string(body + off, accountLen);
    off += accountLen;
    if (off + 1 > len) return;
    uint8_t attribLen = body[off++];
    if (off + attribLen > len) return;
    auto attrib = make_string(body + off, attribLen);
    off += attribLen;
    if (off + 4 > len) return;
    uint32_t resultLen = rd32be(body, off);
    off += 4;
    if (off + resultLen > len) return;
    auto result = make_string(body + off, resultLen);
    off += resultLen;
    (void)off;
    applyLpc("msg_l_get_safe_ctrl_info", {make_int(cl.clientNo), module, account, attrib, result});
}

void handle_vm_msgLGetCreditCoins(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t gidLen = body[off++];
    if (off + gidLen > len) return;
    auto gid = make_string(body + off, gidLen);
    off += gidLen;
    if (off + 1 > len) return;
    uint8_t coinsLen = body[off++];
    if (off + coinsLen > len) return;
    auto coins = make_string(body + off, coinsLen);
    off += coinsLen;
    (void)off;
    applyLpc("msg_l_get_credit_coins", {make_int(cl.clientNo), gid, coins});
}

void handle_vm_msgLGetProtectInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n1 = rd8(body, off);
    if (off + n1 > len) return;
    const uint8_t* account = body + off; off += n1;
    if (off + 1 > len) return;
    uint8_t n2 = rd8(body, off);
    if (off + n2 > len) return;
    const uint8_t* result = body + off; off += n2;
    applyLpc("msg_l_get_protect_info", {make_int(cl.clientNo), make_string(account, n1), make_string(result, n2)});
}

void handle_vm_msgLQueryCoupon(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n1 = rd8(body, off);
    if (off + n1 > len) return;
    const uint8_t* gid = body + off; off += n1;
    if (off + 1 > len) return;
    uint8_t n2 = rd8(body, off);
    if (off + n2 > len) return;
    const uint8_t* para = body + off; off += n2;
    if (off + 2 > len) return;
    uint16_t n3 = rd16be(body, off);
    if (off + n3 > len) return;
    const uint8_t* coupon = body + off; off += n3;
    applyLpc("msg_l_query_coupon", {make_int(cl.clientNo), make_string(gid, n1), make_string(para, n2), make_string(coupon, n3)});
}

void handle_vm_msgLGetLocalEncryptInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n1 = rd8(body, off);
    if (off + n1 > len) return;
    const uint8_t* account = body + off; off += n1;
    if (off + 1 > len) return;
    uint8_t n2 = rd8(body, off);
    if (off + n2 > len) return;
    const uint8_t* password = body + off; off += n2;
    if (off + 2 > len) return;
    uint16_t flag = rd16be(body, off);
    applyLpc("msg_l_get_local_encrypt_info", {make_int(cl.clientNo), make_string(account, n1), make_string(password, n2), make_int(flag)});
}

void handle_vm_msgLSetLocalEncryptInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n1 = rd8(body, off);
    if (off + n1 > len) return;
    const uint8_t* account = body + off; off += n1;
    if (off + 1 > len) return;
    uint8_t flag = rd8(body, off);
    applyLpc("msg_l_set_local_encrypt_info", {make_int(cl.clientNo), make_string(account, n1), make_int(flag)});
}

void handle_vm_msgLBorrowLocalEncrypt(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n1 = rd8(body, off);
    if (off + n1 > len) return;
    const uint8_t* account = body + off; off += n1;
    if (off + 1 > len) return;
    uint8_t flag = rd8(body, off);
    applyLpc("msg_l_borrow_local_encrypt", {make_int(cl.clientNo), make_string(account, n1), make_int(flag)});
}

void handle_vm_msgSetPhahBonusInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n1 = rd8(body, off);
    if (off + n1 > len) return;
    const uint8_t* account = body + off; off += n1;
    if (off + 1 > len) return;
    uint8_t n2 = rd8(body, off);
    if (off + n2 > len) return;
    const uint8_t* ret = body + off; off += n2;
    applyLpc("msg_l_set_phah_bonus_info", {make_int(cl.clientNo), make_string(account, n1), make_string(ret, n2)});
}

void handle_vm_msgLQueryCitys(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n1 = rd8(body, off);
    if (off + n1 > len) return;
    const uint8_t* gid = body + off; off += n1;
    if (off + 1 > len) return;
    uint8_t n2 = rd8(body, off);
    if (off + n2 > len) return;
    const uint8_t* module = body + off; off += n2;
    if (off + 1 > len) return;
    uint8_t n3 = rd8(body, off);
    if (off + n3 > len) return;
    const uint8_t* country = body + off; off += n3;
    if (off + 1 > len) return;
    uint8_t n4 = rd8(body, off);
    if (off + n4 > len) return;
    const uint8_t* province = body + off; off += n4;
    if (off + 4 > len) return;
    uint32_t n5 = rd32be(body, off);
    if (off + n5 > len) return;
    const uint8_t* citys = body + off; off += n5;
    applyLpc("msg_l_query_citys", {make_int(cl.clientNo), make_string(gid, n1), make_string(module, n2), make_string(country, n3), make_string(province, n4), make_string(citys, n5)});
}

void handle_vm_msgLGetAccCompChar(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n1 = rd8(body, off);
    if (off + n1 > len) return;
    const uint8_t* gid = body + off; off += n1;
    if (off + 1 > len) return;
    uint8_t n2 = rd8(body, off);
    if (off + n2 > len) return;
    const uint8_t* comp = body + off; off += n2;
    if (off + 1 > len) return;
    uint8_t n3 = rd8(body, off);
    if (off + n3 > len) return;
    const uint8_t* m_str = body + off; off += n3;
    applyLpc("msg_l_get_acc_comp_char", {make_int(cl.clientNo), make_string(gid, n1), make_string(comp, n2), make_string(m_str, n3)});
}

void handle_vm_cmdCtaToServer(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_cta_to_server", {make_int(cl.clientNo), make_buffer(body, len)});
}

void handle_vm_cmdMaOper(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t cookie = rd32be(body, off);
    if (off + 1 > len) return;
    uint8_t n1 = rd8(body, off);
    if (off + n1 > len) return;
    auto command = make_string(body + off, n1); off += n1;
    if (off + 1 > len) return;
    uint8_t n2 = rd8(body, off);
    if (off + n2 > len) return;
    auto maAccount = make_string(body + off, n2); off += n2;
    if (off + 1 > len) return;
    uint8_t n3 = rd8(body, off);
    if (off + n3 > len) return;
    off += n3; // target string, parsed but not passed to LPC
    if (off + 2 > len) return;
    off += 2; // u16 field, ignored by driver
    if (off + 2 > len) return;
    uint16_t count = rd16be(body, off);
    if (count > 0x3F) return;
    auto arr = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < count; ++i)
    {
        if (off + 2 > len) return;
        uint16_t sl = rd16be(body, off);
        if (off + sl > len) return;
        arr->items.push_back(make_string(body + off, sl));
        off += sl;
    }
    applyLpc("cmd_ma_oper", {make_int(cl.clientNo), make_int((int)cookie), command, maAccount, arr});
}

void handle_vm_cmdCcsShutdownBySpa(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n1 = rd8(body, off);
    if (off + n1 > len) return;
    auto interval = make_string(body + off, n1); off += n1;
    if (off + 2 > len) return;
    uint16_t v10 = rd16be(body, off);
    applyLpc("cmd_ccs_shutdown_by_spa", {make_int(cl.clientNo), interval, make_int(v10)});
}

void handle_vm_cmdMaSet(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t cookie = rd32be(body, off);
    if (off + 1 > len) return;
    uint8_t n1 = rd8(body, off);
    if (off + n1 > len) return;
    auto command = make_string(body + off, n1); off += n1;
    if (off + 1 > len) return;
    uint8_t n2 = rd8(body, off);
    if (off + n2 > len) return;
    auto maAccount = make_string(body + off, n2); off += n2;
    if (off + 1 > len) return;
    uint8_t n3 = rd8(body, off);
    if (off + n3 > len) return;
    off += n3; // target string, parsed but not passed to LPC
    if (off + 2 > len) return;
    off += 2; // u16 field, ignored by driver
    if (off + 2 > len) return;
    uint16_t count = rd16be(body, off);
    if (count > 0x3F) return;
    auto arr = std::make_shared<ArrayValue>();
    for (uint16_t i = 0; i < count; ++i)
    {
        if (off + 2 > len) return;
        uint16_t sl = rd16be(body, off);
        if (off + sl > len) return;
        arr->items.push_back(make_string(body + off, sl));
        off += sl;
    }
    applyLpc("cmd_ma_set", {make_int(cl.clientNo), make_int((int)cookie), command, maAccount, arr});
}

void handle_vm_msgSpaBusy(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint32_t v1 = rd32be(body, off);
    if (off + 4 > len) return;
    uint32_t v2 = rd32be(body, off);
    if (off + 4 > len) return;
    uint32_t v3 = rd32be(body, off);
    applyLpc("msg_spa_busy", {make_int(cl.clientNo), make_int((int)v1), make_int((int)v2), make_int((int)v3)});
}

void handle_vm_msgCcsRebootResult(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t result = rd8(body, off);
    applyLpc("msg_ccs_reboot_result", {make_int(cl.clientNo), make_int(result)});
}

void handle_vm_cmdGetServerTime(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n1 = rd8(body, off);
    if (off + n1 > len) return;
    auto name = make_string(body + off, n1); off += n1;
    applyLpc("cmd_get_server_time", {make_int(cl.clientNo), name});
}

void handle_vm_msgCcsEmoteData(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n1 = rd8(body, off);
    if (off + n1 > len) return;
    auto verb = make_string(body + off, n1); off += n1;
    if (off + 1 > len) return;
    uint8_t n2 = rd8(body, off);
    if (off + n2 > len) return;
    auto target = make_string(body + off, n2); off += n2;
    if (off + 2 > len) return;
    uint16_t channel = rd16be(body, off);
    if (off + 1 > len) return;
    uint8_t n3 = rd8(body, off);
    if (off + n3 > len) return;
    auto peer = make_string(body + off, n3); off += n3;
    if (off + 2 > len) return;
    off += 2; // u16 field, ignored by driver
    size_t sl = 0;
    while (off + sl < len && body[off + sl] != 0) ++sl;
    auto tail = make_string(body + off, sl);
    applyLpc("msg_ccs_emote_data", {make_int(cl.clientNo), verb, target, make_int(channel), peer, tail});
}

void handle_vm_msgCcsFingerRes(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n1 = rd8(body, off);
    if (off + n1 > len) return;
    auto char_name = make_string(body + off, n1); off += n1;
    if (off + 2 > len) return;
    uint16_t v10 = rd16be(body, off);
    applyLpc("msg_ccs_finger_res", {make_int(cl.clientNo), char_name, make_int(v10)});
}

void handle_vm_msgCcsFriendTell(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t n1 = rd8(body, off);
    if (off + n1 > len) return;
    auto from = make_string(body + off, n1); off += n1;
    if (off + 1 > len) return;
    uint8_t n2 = rd8(body, off);
    if (off + n2 > len) return;
    auto to = make_string(body + off, n2); off += n2;
    if (off + 1 > len) return;
    uint8_t n3 = rd8(body, off);
    if (off + n3 > len) return;
    auto msg = make_string(body + off, n3); off += n3;
    if (off + 4 > len) return;
    uint32_t v19 = rd32be(body, off);
    applyLpc("msg_ccs_friend_tell", {make_int(cl.clientNo), from, to, msg, make_int((int)v19)});
}

// ---- filed<->dba first batch (6 commands) ----
// vm_msgReadContent (0x80674ec): body = [u32 BE a][u32 BE b][u32 BE strLen][str]
// -> msg_read_content(clientNo, a, b, str)
void handleMsgReadContent(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 12) return;
    uint32_t a = rd32be(body);
    uint32_t b = rd32be(body + 4);
    uint32_t slen = rd32be(body + 8);
    if (slen > len - 12) return;
    std::string s(reinterpret_cast<const char*>(body + 12), slen);
    // result is a SIGNED int32 in the original wire protocol (0xFFFFFFFF = -1
    // "not found"); passing it unsigned made ccs's filed do_operate see
    // 4294967295 instead of -1 and share_datad's restore loop forever.
    if (std::getenv("LPCVM_SVC_TRACE"))
        std::cerr << "[readcontent] client=" << cl.clientNo << " cookie=" << a
                  << " result=" << (int32_t)b << " len=" << slen << "\n";
    applyLpc("msg_read_content",
             {make_int(cl.clientNo), make_int((int32_t)a), make_int((int32_t)b), make_string(s)});
}

// vm_msgWrittenContent (0x8067634): body = [u32 BE a][u32 BE b]
// -> msg_written_content(clientNo, a, b)
void handleMsgWrittenContent(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 8) return;
    uint32_t a = rd32be(body);
    int32_t b = static_cast<int32_t>(rd32be(body + 4));   // result (signed)
    applyLpc("msg_written_content",
             {make_int(cl.clientNo), make_int(a), make_int(b)});
}

// vm_msgCreatedGid (0x8067b80): body = [u32 BE cookie][u32 BE result][u8 gidLen][gid]
// -> msg_created_gid(clientNo, cookie, result, gid)
void handleMsgCreatedGid(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 9) return;
    uint32_t cookie = rd32be(body);
    int32_t result = static_cast<int32_t>(rd32be(body + 4));   // result (signed)
    uint32_t glen = body[8];
    if (glen > len - 9) return;
    std::string gid(reinterpret_cast<const char*>(body + 9), glen);
    applyLpc("msg_created_gid",
             {make_int(cl.clientNo), make_int(cookie), make_int(result), make_string(gid)});
}

// vm_msgGotGid (0x8067d38): body = [u32 BE cookie][u32 BE result][u8 gidLen][gid]
// -> msg_got_gid(clientNo, cookie, result, gid)
void handleMsgGotGid(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 9) return;
    uint32_t cookie = rd32be(body);
    int32_t result = static_cast<int32_t>(rd32be(body + 4));   // result (signed)
    uint32_t glen = body[8];
    if (glen > len - 9) return;
    std::string gid(reinterpret_cast<const char*>(body + 9), glen);
    applyLpc("msg_got_gid",
             {make_int(cl.clientNo), make_int(cookie), make_int(result), make_string(gid)});
}

// vm_msgGotGidArr (0x8067e20): body = [u32 BE a][u32 BE b][u32 BE strLen][str]
// -> msg_got_gid_arr(clientNo, a, b, str)
void handleMsgGotGidArr(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 12) return;
    uint32_t a = rd32be(body);
    int32_t b = static_cast<int32_t>(rd32be(body + 4));   // result (signed)
    uint32_t slen = rd32be(body + 8);
    if (slen > len - 12) return;
    std::string s(reinterpret_cast<const char*>(body + 12), slen);
    applyLpc("msg_got_gid_arr",
             {make_int(cl.clientNo), make_int(a), make_int(b), make_string(s)});
}

// vm_msgGotGidInfo (0x8067ec4): body = [u32 BE cookie][u32 BE result]
//   [u8 typeLen][type][u8 nameLen][name]
// -> msg_got_gid_info(clientNo, cookie, result, type, name)
void handleMsgGotGidInfo(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 9) return;
    uint32_t cookie = rd32be(body);
    int32_t result = static_cast<int32_t>(rd32be(body + 4));   // result (signed)
    size_t off = 8;
    uint32_t tlen = body[off++];
    if (tlen > len - off) return;
    std::string type(reinterpret_cast<const char*>(body + off), tlen);
    off += tlen;
    if (off >= len) return;
    uint32_t nlen = body[off++];
    if (nlen > len - off) return;
    std::string name(reinterpret_cast<const char*>(body + off), nlen);
    applyLpc("msg_got_gid_info",
             {make_int(cl.clientNo), make_int(cookie), make_int(result),
              make_string(type), make_string(name)});
}

// Dispatcher: run the C handler for a parsed packet.
// Original logic (vm_clientSocketCallback LABEL_42):
//   if (vm_pktAnalyse(cmdNo, body, len) != 1) {
//     ... cmdIndex lookup + cmdFunc fallback ...
//   }
void dispatchCommand(GameClient& cl, uint16_t cmdNo, const uint8_t* body, size_t len) {
    if (std::getenv("LPCVM_TRACE") || std::getenv("LPCVM_EVENT_TRACE")) {
        std::string hex;
        static const char* hx = "0123456789ABCDEF";
        for (size_t i = 0; i < len && i < 24; ++i) { hex += hx[body[i] >> 4]; hex += hx[body[i] & 15]; }
        std::cerr << "[dispatch] cmd=0x" << std::hex << cmdNo << std::dec
                  << " client=" << cl.clientNo << " attrib=" << cl.attrib
                  << " len=" << len << " body=" << hex << "\n";
    }
    // Primary path: packet protocol analyser (comm_desc.txt interfaces).
    int pa = pktAnalyse(cl.clientNo, cl.attrib, cmdNo, body, len);
    if (pa == 1) return;   // handled by the protocol interface
    if (pa < 0) {          // parse error
        ++cl.recvBadPkts;
        return;
    }
    // Fallback: cmdList/cmdIndex dispatch.
    const CmdEntry* e = findCmd(cmdNo);
    if (!e) {
        if (std::getenv("LPCVM_TRACE") || (std::getenv("LPCVM_TAIL") && (cmdNo == 0x2471 || cmdNo == 0x2473)))
            std::cerr << "[game] bad command 0x" << std::hex << cmdNo << std::dec
                      << " from client " << cl.clientNo << " (len=" << len << ")\n";
        ++cl.recvBadPkts;
        return;
    }
    // attrib permission check: (attrib & type & 7) != 0
    if ((cl.attrib & e->type & 7) == 0) {
        if (std::getenv("LPCVM_TRACE") || (std::getenv("LPCVM_TAIL") && (cmdNo == 0x2471 || cmdNo == 0x2473)))
            std::cerr << "[game] denied cmd 0x" << std::hex << cmdNo << std::dec
                      << " client=" << cl.clientNo << " attrib=" << cl.attrib
                      << " type=" << e->type << "\n";
        ++cl.recvBadPkts;
        return;
    }
    if (std::getenv("LPCVM_TAIL") && (cmdNo == 0x2471 || cmdNo == 0x2473))
        std::cerr << "[game] dispatch cmd=0x" << std::hex << cmdNo << std::dec
                  << " client=" << cl.clientNo << " attrib=" << cl.attrib
                  << " type=" << e->type << " OK\n";
    switch (cmdNo) {
    case 0x5351: handleMsgLAuth(cl, body, len); break;
    case 0x20D2: handleCmdEcho(cl, body, len); break;
    case 0xF9C3: handleMsgReplyEcho(cl, body, len); break;
    case 0x3365: handleMsgCfgMyInfo(cl, body, len); break;
    case 0x40F5: handleMsgCfgSpaInfo(cl, body, len); break;
    case 0x435F: handleMsgCfgGameServerList(cl, body, len); break;
    case 0x3361: handleMsgCfgDbaList(cl, body, len, "msg_cfg_dba_list"); break;
    case 0x3363: handleMsgCfgDbaList(cl, body, len, "msg_cfg_ldba_list"); break;
    case 0x236F: handleMsgCfgCtaList(cl, body, len); break;
    // filed<->dba data protocol (second batch):
    case 0x2405: handleMsgIntInt(cl, body, len, "msg_deleted_content"); break;
    case 0x2407: handleMsgIntInt(cl, body, len, "msg_created_content"); break;
    case 0x54A1: handleMsgIntInt(cl, body, len, "msg_moved_content"); break;
    case 0x2409: handleMsgIntInt(cl, body, len, "msg_got_catalog"); break;
    case 0x240F: handleMsgIntInt(cl, body, len, "msg_got_catalog_ex"); break;
    case 0x241B: handleMsgIntInt(cl, body, len, "msg_read_log"); break;
    case 0x0F51F: handleMsgIntInt(cl, body, len, "msg_update_log"); break;
    case 0x240B: handleMsgGotPageData(cl, body, len); break;
    case 0x240D: handleMsgGotPageDataEx(cl, body, len); break;
    case 0x2411: handleMsgIntInt(cl, body, len, "msg_deleted_gid"); break;
    case 0x2413: handleMsgIntInt(cl, body, len, "msg_updated_gid"); break;
    case 0x5409: handleMsgIntInt(cl, body, len, "msg_create_pet_deposit"); break;
    case 0x5401: handleMsgReadPetDeposit(cl, body, len); break;
    case 0x5405: handleMsgIntInt(cl, body, len, "msg_write_pet_deposit"); break;
    case 0x4402: handleMsgGetCounts(cl, body, len); break;
    case 0x7008: handle_vm_cmd_Apprentice_List(cl, body, len); break;
    case 0x5012: handle_vm_cmd_Recruit_Apprentice(cl, body, len); break;
    case 0x21BE: handle_vm_cmd_Start_Award(cl, body, len); break;
    case 0x70A6: handle_vm_cmd_StartMultibonus(cl, body, len); break;
    case 0x2242: handle_vm_cmdAccelerateMoveTo(cl, body, len); break;
    case 0xF6: handle_vm_cmdAccept(cl, body, len); break;
    case 0x2164: handle_vm_cmdAcceptChatGroupInvite(cl, body, len); break;
    case 0x2158: handle_vm_cmdAcceptChatGroupJoin(cl, body, len); break;
    case 0x21C6: handle_vm_cmdAcceptCommonConvenant(cl, body, len); break;
    case 0x6400: handle_vm_cmdAcceptConvenant(cl, body, len); break;
    case 0xA0C: handle_vm_cmdAddAwardTimes(cl, body, len); break;
    case 0x50A2: handle_vm_cmdAddBureauPage(cl, body, len); break;
    case 0xF0C2: handle_vm_cmdAddDunwuNimbus(cl, body, len); break;
    case 0x3058: handle_vm_cmdAddFriend(cl, body, len); break;
    case 0xFA49: handle_vm_cmdAddGodbookDirect(cl, body, len); break;
    case 0x2284: handle_vm_cmdAddMaterial(cl, body, len); break;
    case 0x2154: handle_vm_cmdAddMountItem(cl, body, len); break;
    case 0x125E: handle_vm_cmdAddSkillEssence(cl, body, len); break;
    case 0x2344: handle_vm_cmdAdminBroadcast(cl, body, len); break;
    case 0x2314: handle_vm_cmdAdminCommonCommand(cl, body, len); break;
    case 0x217E: handle_vm_cmdBroadcastCombatList(cl, body, len); break;
    case 0xF908: handle_vm_cmdButtonDoubleBonus(cl, body, len); break;
    case 0x506E: handle_vm_cmdBuyCard(cl, body, len); break;
    case 0x2312: handle_vm_cmdBuyCustomizePack(cl, body, len); break;
    case 0x4114: handle_vm_cmdBuyEcard(cl, body, len); break;
    case 0x21C8: handle_vm_cmdBuyFromAskTaoShowMall(cl, body, len); break;
    case 0x1166: handle_vm_cmdBuyFromOnlineMall(cl, body, len); break;
    case 0x350C: handle_vm_cmdBuyFromStall(cl, body, len); break;
    case 0x281C: handle_vm_cmdBuyService(cl, body, len); break;
    case 0x218A: handle_vm_cmdCalcMountAttrib(cl, body, len); break;
    case 0x2216: handle_vm_cmdCalcPetGrow(cl, body, len); break;
    case 0x2816: handle_vm_cmdCalcUserAttribs(cl, body, len); break;
    case 0x1262: handle_vm_cmdCancelAuthVipSms(cl, body, len); break;
    case 0x21BC: handle_vm_cmdCancelReleaseSafeLogin(cl, body, len); break;
    case 0x1098: handle_vm_cmdCareItem(cl, body, len); break;
    case 0x1156: handle_vm_cmdCast(cl, body, len); break;
    case 0x1114: handle_vm_cmdCasualGameDoAction(cl, body, len); break;
    case 0x50A4: handle_vm_cmdCasualOperGame(cl, body, len); break;
    case 0x1A06: handle_vm_cmdCatchPker(cl, body, len); break;
    case 0x226A: handle_vm_cmdCCatchPet(cl, body, len); break;
    case 0x3E10: handle_vm_cmdCCDoAction(cl, body, len); break;
    case 0x2240: handle_vm_cmdCCFinishRound(cl, body, len); break;
    case 0xF0C6: handle_vm_cmdCCSurrender(cl, body, len); break;
    case 0xCF02: handle_vm_cmdCCSwitchCard(cl, body, len); break;
    case 0x217A: handle_vm_cmdCDoAction0(cl, body, len); break;
    case 0x1138: handle_vm_cmdCDoAction1(cl, body, len); break;
    case 0x1104: handle_vm_cmdCDoAction2(cl, body, len); break;
    case 0x2094: handle_vm_cmdCdWishingGoods(cl, body, len); break;
    case 0x70AA: handle_vm_cmdCdWishingRate(cl, body, len); break;
    case 0x5106: handle_vm_cmdCdWishingStart(cl, body, len); break;
    case 0x4104: handle_vm_cmdCEndAnimate(cl, body, len); break;
    case 0x1108: handle_vm_cmdCFlee(cl, body, len); break;
    case 0x2178: handle_vm_cmdChallengeAnswer(cl, body, len); break;
    case 0x10B6: handle_vm_cmdChangeArenaRoomSeat(cl, body, len); break;
    case 0xFDA6: handle_vm_cmdChangeAttribPlan(cl, body, len); break;
    case 0x101A: handle_vm_cmdChangeChatGroupSetting(cl, body, len); break;
    case 0x2336: handle_vm_cmdChangeFurnitureDir(cl, body, len); break;
    case 0x211C: handle_vm_cmdChangeItemAttrib(cl, body, len); break;
    case 0x21B4: handle_vm_cmdChangeTeamLeader(cl, body, len); break;
    case 0x2AE8: handle_vm_cmdChangeTitle(cl, body, len); break;
    case 0x1102: handle_vm_cmdChangeTitleEffect(cl, body, len); break;
    case 0x407C: handle_vm_cmdChangeUpgradeState(cl, body, len); break;
    case 0xFD24: handle_vm_cmdChangeUserState(cl, body, len); break;
    case 0x3062: handle_vm_cmdChangeZhenfa(cl, body, len); break;
    case 0xF0DD: handle_vm_cmdChargeVendueFetch(cl, body, len); break;
    case 0x23B3: handle_vm_cmdChargeVendueOfferPrice(cl, body, len); break;
    case 0x3501: handle_vm_cmdChargeVendueRefresh(cl, body, len); break;
    case 0x10DF: handle_vm_cmdChargeVendueStoreClosed(cl, body, len); break;
    case 0x13AA: handle_vm_cmdCharPractice(cl, body, len); break;
    case 0x2276: handle_vm_cmdChat(cl, body, len); break;
    case 0x3AE4: handle_vm_cmdChatEx(cl, body, len); break;
    case 0x10F4: handle_vm_cmdChatGroupLog(cl, body, len); break;
    case 0x2290: handle_vm_cmdCheckSafeAnswer(cl, body, len); break;
    case 0x5108: handle_vm_cmdCheckServer(cl, body, len); break;
    case 0x4122: handle_vm_cmdChildEnterHall(cl, body, len); break;
    case 0x23B6: handle_vm_cmdChildEnterMap(cl, body, len); break;
    case 0x5008: handle_vm_cmdChildSkillLevelUp(cl, body, len); break;
    case 0x1AFA: handle_vm_cmdChildUseCombat(cl, body, len); break;
    case 0x2800: handle_vm_cmdChildUseSkill(cl, body, len); break;
    case 0x3408: handle_vm_cmdCitywarGiveOrders(cl, body, len); break;
    case 0x1A0C: handle_vm_cmdCleanRequest(cl, body, len); break;
    case 0x1FB4: handle_vm_cmdClearBag(cl, body, len); break;
    case 0x2233: handle_vm_cmdClickedLinker(cl, body, len); break;
    case 0x1136: handle_vm_cmdClientBehavior(cl, body, len); break;
    case 0x1A: handle_vm_cmdCloneSpecialEquip(cl, body, len); break;
    case 0x61: handle_vm_cmdCloneUpgradeStone(cl, body, len); break;
    case 0x113E: handle_vm_cmdCloseBureau(cl, body, len); break;
    case 0xFDA0: handle_vm_cmdCloseMenu(cl, body, len); break;
    case 0x4108: handle_vm_cmdCloseRemoteStore(cl, body, len); break;
    case 0x2308: handle_vm_cmdCloseTalkDrama(cl, body, len); break;
    case 0x2080: handle_vm_cmdCloseTaotaotao(cl, body, len); break;
    case 0x2A02: handle_vm_cmdCoinCancelOrder(cl, body, len); break;
    case 0x7E06: handle_vm_cmdCoinDealOrder(cl, body, len); break;
    case 0x3806: handle_vm_cmdCoinFetchStore(cl, body, len); break;
    case 0x280A: handle_vm_cmdCoinOpenMall(cl, body, len); break;
    case 0xC4: handle_vm_cmdCoinRefreshMall(cl, body, len); break;
    case 0xFD18: handle_vm_cmdCoinSubmitOrder(cl, body, len); break;
    case 0x2184: handle_vm_cmdCollectData(cl, body, len); break;
    case 0x54: handle_vm_cmdCommonGuessDrawBonus(cl, body, len); break;
    case 0x1028: handle_vm_cmdCommonGuessInfo(cl, body, len); break;
    case 0x2222: handle_vm_cmdCommonGuessList(cl, body, len); break;
    case 0x3206: handle_vm_cmdCommonGuessStartGuess(cl, body, len); break;
    case 0x3E30: handle_vm_cmdConfirmTakeProperty(cl, body, len); break;
    case 0x2EF0: handle_vm_cmdControlPartyChannel(cl, body, len); break;
    case 0x306E: handle_vm_cmdCreateChatGroup(cl, body, len); break;
    case 0x205C: handle_vm_cmdCreateNewChar(cl, body, len); break;
    case 0x2130: handle_vm_cmdCreateNewRaid(cl, body, len); break;
    case 0x24: handle_vm_cmdCscAddSignFee(cl, body, len); break;
    case 0xFA4A: handle_vm_cmdCscViewCompeteSort(cl, body, len); break;
    case 0xA: handle_vm_cmdCSelectMenuItem(cl, body, len); break;
    case 0x120C: handle_vm_cmdCSetPos(cl, body, len); break;
    case 0x272C: handle_vm_cmdCsExchange(cl, body, len); break;
    case 0x211A: handle_vm_cmdCUseStunChildbook(cl, body, len); break;
    case 0x125A: handle_vm_cmdDeleteExistedChar(cl, body, len); break;
    case 0x281E: handle_vm_cmdDeleteMusic(cl, body, len); break;
    case 0x2066: handle_vm_cmdDeposit(cl, body, len); break;
    case 0xFD60: handle_vm_cmdDevelopSkill(cl, body, len); break;
    case 0x111C: handle_vm_cmdDigPartyFragment(cl, body, len); break;
    case 0x2DCA: handle_vm_cmdDismissChatGroup(cl, body, len); break;
    case 0x3044: handle_vm_cmdDistributeRecruitment(cl, body, len); break;
    case 0x2EEE: handle_vm_cmdAdminFetchPics(cl, body, len); break;
    case 0xCF06: handle_vm_cmdAdminKickoff(cl, body, len); break;
    case 0x2176: handle_vm_cmdAdminOperSysBroadcast(cl, body, len); break;
    case 0x3A12: handle_vm_cmdAdminPrintScreen(cl, body, len); break;
    case 0x1AFC: handle_vm_cmdAdminQueryMap(cl, body, len); break;
    case 0x20F6: handle_vm_cmdAdminQueryNetInfo(cl, body, len); break;
    case 0x301C: handle_vm_cmdAdminQueryNpc(cl, body, len); break;
    case 0xFA: handle_vm_cmdAdminQueryPlayer(cl, body, len); break;
    case 0x272E: handle_vm_cmdAdminQuerySkill(cl, body, len); break;
    case 0x2442: handle_vm_cmdAdminReboot(cl, body, len); break;
    case 0x700A: handle_vm_cmdAdminRequestMapList(cl, body, len); break;
    case 0x1364: handle_vm_cmdAdminSetAttrib(cl, body, len); break;
    case 0x2818: handle_vm_cmdAdminSetSkill(cl, body, len); break;
    case 0xFD58: handle_vm_cmdAdminShutChannel(cl, body, len); break;
    case 0xFD2C: handle_vm_cmdAdminSniffAt(cl, body, len); break;
    case 0x2248: handle_vm_cmdAdminSrmouseClick(cl, body, len); break;
    case 0x2278: handle_vm_cmdAdminSummon(cl, body, len); break;
    case 0x5A: handle_vm_cmdAdminTeleport(cl, body, len); break;
    case 0x1096: handle_vm_cmdAdminThrowInJail(cl, body, len); break;
    case 0x20BE: handle_vm_cmdAdminThrowInJailEx(cl, body, len); break;
    case 0x6002: handle_vm_cmdAdminTimeBroadcast(cl, body, len); break;
    case 0x2204: handle_vm_cmdAdminTransport(cl, body, len); break;
    case 0x10BE: handle_vm_cmdAnswerResult(cl, body, len); break;
    case 0x110E: handle_vm_cmdAnswerSurvey(cl, body, len); break;
    case 0xFD2E: handle_vm_cmdAntiHackCollect(cl, body, len); break;
    case 0x1B01: handle_vm_cmdApex(cl, body, len); break;
    case 0x220C: handle_vm_cmdApply(cl, body, len); break;
    case 0xFAF8: handle_vm_cmdApplyCard(cl, body, len); break;
    case 0xFD36: handle_vm_cmdApplyEx(cl, body, len); break;
    case 0xB22: handle_vm_cmdApplyExquisite(cl, body, len); break;
    case 0x10D6: handle_vm_cmdApplyOfflineStall(cl, body, len); break;
    case 0x2802: handle_vm_cmdApplyOnFriend(cl, body, len); break;
    case 0x206C: handle_vm_cmdApplyOnItem(cl, body, len); break;
    case 0x2606: handle_vm_cmdApplyOther(cl, body, len); break;
    case 0x3A10: handle_vm_cmdApplyQuestionItem(cl, body, len); break;
    case 0x4100: handle_vm_cmdArenaEnter(cl, body, len); break;
    case 0x1FC8: handle_vm_cmdArenaGoodBuy(cl, body, len); break;
    case 0x113C: handle_vm_cmdArenaGoodlist(cl, body, len); break;
    case 0xA12: handle_vm_cmdArenaGoodsSell(cl, body, len); break;
    case 0x1126: handle_vm_cmdArenaHallExit(cl, body, len); break;
    case 0x40D6: handle_vm_cmdArenaHallFight(cl, body, len); break;
    case 0x2428: handle_vm_cmdArenaHallJoinRoom(cl, body, len); break;
    case 0x280E: handle_vm_cmdArenaHallRequestCI(cl, body, len); break;
    case 0x2114: handle_vm_cmdArenaHallRequestPL(cl, body, len); break;
    case 0xF0A8: handle_vm_cmdArenaHallRequestRD(cl, body, len); break;
    case 0x202C: handle_vm_cmdArenaHallRequestRL(cl, body, len); break;
    case 0xF0D4: handle_vm_cmdArenaLookOn(cl, body, len); break;
    case 0x2022: handle_vm_cmdArenaMatchFixtures(cl, body, len); break;
    case 0x2028: handle_vm_cmdArenaMatchJoinRoom(cl, body, len); break;
    case 0x2038: handle_vm_cmdArenaMatchList(cl, body, len); break;
    case 0x2026: handle_vm_cmdArenaMatchRoomDetail(cl, body, len); break;
    case 0x2024: handle_vm_cmdArenaMatchRooms(cl, body, len); break;
    case 0x2342: handle_vm_cmdArenaOpenRank(cl, body, len); break;
    case 0x20DC: handle_vm_cmdArenaQueryRankByName(cl, body, len); break;
    case 0x2260: handle_vm_cmdArenaRankList(cl, body, len); break;
    case 0x1086: handle_vm_cmdArenaRoom(cl, body, len); break;
    case 0x202A: handle_vm_cmdArenaRoomAction(cl, body, len); break;
    case 0x2722: handle_vm_cmdArenaRoomKickOut(cl, body, len); break;
    case 0x3204: handle_vm_cmdArenaServerList(cl, body, len); break;
    case 0x20A4: handle_vm_cmdArenaSwitchServer(cl, body, len); break;
    case 0x700E: handle_vm_cmdAskFields(cl, body, len); break;
    case 0xB0C: handle_vm_cmdAskPill(cl, body, len); break;
    case 0x1366: handle_vm_cmdAskSkillDesc(cl, body, len); break;
    case 0x1171: handle_vm_cmdAsktaoSound(cl, body, len); break;
    case 0x40E2: handle_vm_cmdAssignAttrib(cl, body, len); break;
    case 0xFD74: handle_vm_cmdAssignChildAttrib(cl, body, len); break;
    case 0x23A2: handle_vm_cmdAssignPolar(cl, body, len); break;
    case 0x2252: handle_vm_cmdAssignResist(cl, body, len); break;
    case 0xF0CE: handle_vm_cmdAssignUpgrade(cl, body, len); break;
    case 0xFD97: handle_vm_cmdAuthMatrixForGs(cl, body, len); break;
    case 0x3018: handle_vm_cmdAuthRefreshPicForGs(cl, body, len); break;
    case 0xF8: handle_vm_cmdAuthVerifyCode(cl, body, len); break;
    case 0xF374: handle_vm_cmdAuthVipSms(cl, body, len); break;
    case 0xF0A1: handle_vm_cmdAutoCombatTime(cl, body, len); break;
    case 0xFD80: handle_vm_cmdBottleDig(cl, body, len); break;
    case 0xFD32: handle_vm_cmdBottleOpen(cl, body, len); break;
    case 0x2190: handle_vm_cmdBottleOpenInterface(cl, body, len); break;
    case 0x4008: handle_vm_cmdBottleReply(cl, body, len); break;
    case 0x1380: handle_vm_cmdBottleThrow(cl, body, len); break;
    case 0x2048: handle_vm_cmdBottleThrowBack(cl, body, len); break;

    case 0xC: handle_vm_cmdReturnTeam(cl, body, len); break;
    case 0xE: handle_vm_cmdRequestCityBuildingInfo(cl, body, len); break;
    case 0x1C: handle_vm_cmdReciveInstructionTask(cl, body, len); break;
    case 0x1E: handle_vm_cmdRaceFeedNpc(cl, body, len); break;
    case 0x22: handle_vm_cmdDivideInStore(cl, body, len); break;
    case 0x26: handle_vm_cmdPartyModifyAnnouce(cl, body, len); break;
    case 0x3A: handle_vm_cmdIntersvrRpcCall(cl, body, len); break;
    case 0x3C: handle_vm_cmdUpgradeEx(cl, body, len); break;
    case 0x56: handle_vm_cmdSelectCurrentMount(cl, body, len); break;
    case 0x58: handle_vm_cmdRequestOpenCardPackage(cl, body, len); break;
    case 0x9E: handle_vm_cmdSwitchGroup(cl, body, len); break;
    case 0xB2: handle_vm_cmdGetPhoneAuthBindTime(cl, body, len); break;
    case 0xCC: handle_vm_cmdPartyMembers(cl, body, len); break;
    case 0xDE: handle_vm_cmdRequestServerStatus(cl, body, len); break;
    case 0xEC: handle_vm_cmdOpenAskTaoShowGiftBox(cl, body, len); break;
    case 0xF3: handle_vm_msgSamsungOper(cl, body, len); break;
    case 0xF4: handle_vm_cmdOperXiaobaoSongshenbing(cl, body, len); break;
    case 0x329: handle_vm_msgEveryonePkOper(cl, body, len); break;
    case 0x604: handle_vm_msgSpaBusy(cl, body, len); break;
    case 0xA04: handle_vm_cmdTradeGetObjProp(cl, body, len); break;
    case 0xA0A: handle_vm_cmdGroupChannel(cl, body, len); break;
    case 0xA10: handle_vm_cmdStartCardFriendPk(cl, body, len); break;
    case 0x100E: handle_vm_cmdLockAccount(cl, body, len); break;
    case 0x1010: handle_vm_cmdQuitTeam(cl, body, len); break;
    case 0x1012: handle_vm_cmdSetAttention(cl, body, len); break;
    case 0x1014: handle_vm_cmdWarViewCurrentInfo(cl, body, len); break;
    case 0x1018: handle_vm_cmdMulAssignAttrib(cl, body, len); break;
    case 0x1020: handle_vm_cmdRequestJoin(cl, body, len); break;
    case 0x1024: handle_vm_cmdStartCardDuel(cl, body, len); break;
    case 0x1026: handle_vm_cmdEnterRoom0(cl, body, len); break;
    case 0x1036: handle_vm_cmdMakeDecoration(cl, body, len); break;
    case 0x103C: handle_vm_cmdQuitLookonCombat(cl, body, len); break;
    case 0x1042: handle_vm_cmdSelectMenuItem0(cl, body, len); break;
    case 0x1043: handle_vm_msgCcsFriendStatus(cl, body, len); break;
    case 0x1044: handle_vm_cmdRemoveFriend(cl, body, len); break;
    case 0x104A: handle_vm_cmdLeaveTempTeam(cl, body, len); break;
    case 0x1052: handle_vm_cmdSelectCurrentChild(cl, body, len); break;
    case 0x1054: handle_vm_cmdTradeOpenUrl(cl, body, len); break;
    case 0x1060: handle_vm_cmdLoadExistedChar(cl, body, len); break;
    case 0x1070: handle_vm_cmdTurnDir(cl, body, len); break;
    case 0x1072: handle_vm_cmdSearchChatGroup(cl, body, len); break;
    case 0x107A: handle_vm_cmdRequestRefreshCityTask(cl, body, len); break;
    case 0x107B: handle_vm_cmdShopEnterShopType(cl, body, len); break;
    case 0x1084: handle_vm_cmdReportCheater(cl, body, len); break;
    case 0x108A: handle_vm_cmdTradeGetObj(cl, body, len); break;
    case 0x1090: handle_vm_cmdRemoveAskTaoShowGift(cl, body, len); break;
    case 0x109A: handle_vm_cmdCsaCommonOper(cl, body, len); break;
    case 0x10A0: handle_vm_cmdCcsShutdownBySpa(cl, body, len); break;
    case 0x10AA: handle_vm_cmdRequestDecomposeCard(cl, body, len); break;
    case 0x10AC: handle_vm_cmdGetSafeQuestion(cl, body, len); break;
    case 0x10AE: handle_vm_cmdStartAutomatismStall(cl, body, len); break;
    case 0x10B2: handle_vm_cmdWudaoAddAttribAuto(cl, body, len); break;
    case 0x10B3: handle_vm_cmdShopViewSaleLog(cl, body, len); break;
    case 0x10BD: handle_vm_cmdVendueVendueMerch(cl, body, len); break;
    case 0x10C0: handle_vm_cmdStartStall(cl, body, len); break;
    case 0x10C2: handle_vm_cmdGetPetEvolveList(cl, body, len); break;
    case 0x10C8: handle_vm_cmdResendVipSms(cl, body, len); break;
    case 0x10D2: handle_vm_cmdSetPennantSkill(cl, body, len); break;
    case 0x10D4: handle_vm_cmdSwitchServer(cl, body, len); break;
    case 0x10D9: handle_vm_msgGsFinger(cl, body, len); break;
    case 0x10DC: handle_vm_cmdOperLongtengShengshi(cl, body, len); break;
    case 0x10EA: handle_vm_cmdPresentAskTaoShow(cl, body, len); break;
    case 0x10EC: handle_vm_cmdViewCsMatchDetail(cl, body, len); break;
    case 0x10F0: handle_vm_cmdSetBirthday(cl, body, len); break;
    case 0x10F2: handle_vm_cmdExchangeSelect(cl, body, len); break;
    case 0x10F9: handle_vm_cmdGuardsSetTactics(cl, body, len); break;
    case 0x1106: handle_vm_cmdSortpack(cl, body, len); break;
    case 0x110A: handle_vm_cmdSendFriendCard(cl, body, len); break;
    case 0x110F: handle_vm_msgCelebrityOper(cl, body, len); break;
    case 0x1112: handle_vm_cmdUnequipInbornStone(cl, body, len); break;
    case 0x1115: handle_vm_cmdReplyRequestAsktaoShow(cl, body, len); break;
    case 0x1119: handle_vm_cmdGetClientAction(cl, body, len); break;
    case 0x111A: handle_vm_cmdOpenAdventrueBox(cl, body, len); break;
    case 0x111E: handle_vm_cmdRequestCardGameSortList(cl, body, len); break;
    case 0x1124: handle_vm_cmdFishingAutoBuyWeapon(cl, body, len); break;
    case 0x112D: handle_vm_cmdKillEx(cl, body, len); break;
    case 0x1130: handle_vm_cmdSetCardTopmost(cl, body, len); break;
    case 0x1132: handle_vm_cmdResetAppellationTaskDue(cl, body, len); break;
    case 0x1134: handle_vm_cmdRemoveChatGroupMember(cl, body, len); break;
    case 0x1140: handle_vm_cmdDivide(cl, body, len); break;
    case 0x1142: handle_vm_cmdRequestShiling(cl, body, len); break;
    case 0x1144: handle_vm_cmdTakeStallCash(cl, body, len); break;
    case 0x1145: handle_vm_cmdDraftModifyMyInfo(cl, body, len); break;
    case 0x1146: handle_vm_cmdRefreshFriend(cl, body, len); break;
    case 0x1148: handle_vm_cmdRequestInfo(cl, body, len); break;
    case 0x1154: handle_vm_cmdSwitchItem(cl, body, len); break;
    case 0x1158: handle_vm_cmdTradeBuyService(cl, body, len); break;
    case 0x115A: handle_vm_cmdCelebrityOper(cl, body, len); break;
    case 0x115C: handle_vm_cmdSetFireworksReleaseTime(cl, body, len); break;
    case 0x115E: handle_vm_cmdManageHouse(cl, body, len); break;
    case 0x1160: handle_vm_cmdWorkshopReleaseOrder(cl, body, len); break;
    case 0x1162: handle_vm_cmdOperTeleportItem(cl, body, len); break;
    case 0x1164: handle_vm_cmdVerifyCharName(cl, body, len); break;
    case 0x1168: handle_vm_cmdFishSwallowDie(cl, body, len); break;
    case 0x116A: handle_vm_cmdFeedPetXinfaExp(cl, body, len); break;
    case 0x1202: handle_vm_cmdRepurchase(cl, body, len); break;
    case 0x1204: handle_vm_cmdShout(cl, body, len); break;
    case 0x1206: handle_vm_cmdDraftRequestMyInfo(cl, body, len); break;
    case 0x1208: handle_vm_cmdSetGcslAttribs(cl, body, len); break;
    case 0x120E: handle_vm_cmdGetUpdateNotice(cl, body, len); break;
    case 0x1212: handle_vm_cmdQueryUpgradeCoin(cl, body, len); break;
    case 0x1216: handle_vm_cmdStartRaise(cl, body, len); break;
    case 0x1218: handle_vm_cmdFriendDrinkRefresh(cl, body, len); break;
    case 0x121A: handle_vm_cmdRequestStartMatchCardGame(cl, body, len); break;
    case 0x121C: handle_vm_cmdReportIllegal(cl, body, len); break;
    case 0x121E: handle_vm_cmdGuessBonus(cl, body, len); break;
    case 0x121F: handle_vm_cmdRemoveBureauItem(cl, body, len); break;
    case 0x1220: handle_vm_cmdTowerClicked(cl, body, len); break;
    case 0x1221: handle_vm_msgLGetProtectInfo(cl, body, len); break;
    case 0x1226: handle_vm_cmdPutDown(cl, body, len); break;
    case 0x1228: handle_vm_cmdEquip(cl, body, len); break;
    case 0x122A: handle_vm_cmdOperElixir(cl, body, len); break;
    case 0x122C: handle_vm_cmdTxssStartAward(cl, body, len); break;
    case 0x122E: handle_vm_cmdTxssResetBingo(cl, body, len); break;
    case 0x1230: handle_vm_cmdRequestCityList(cl, body, len); break;
    case 0x123E: handle_vm_cmdRefreshSafeLogin(cl, body, len); break;
    case 0x1247: handle_vm_cmdShopSale(cl, body, len); break;
    case 0x1249: handle_vm_msgGsLogin(cl, body, len); break;
    case 0x1252: handle_vm_cmdMovFromQiankundai(cl, body, len); break;
    case 0x1254: handle_vm_cmdWudaoBatchAddRep(cl, body, len); break;
    case 0x1256: handle_vm_cmdRaceHitNpc(cl, body, len); break;
    case 0x1258: handle_vm_cmdFight(cl, body, len); break;
    case 0x125C: handle_vm_cmdRecharge(cl, body, len); break;
    case 0x1260: handle_vm_cmdRequestTopTaoPage(cl, body, len); break;
    case 0x1265: handle_vm_cmdRequestIcon(cl, body, len); break;
    case 0x1347: handle_vm_cmdRaidLeaveRaid(cl, body, len); break;
    case 0x135D: handle_vm_msgCfgMyDist(cl, body, len); break;
    case 0x1360: handle_vm_cmdFishingQuit(cl, body, len); break;
    case 0x1362: handle_vm_cmdRecoverAskTaoShow(cl, body, len); break;
    case 0x1367: handle_vm_cmdSubmitImage(cl, body, len); break;
    case 0x1384: handle_vm_cmdRequestDeckList(cl, body, len); break;
    case 0x1386: handle_vm_cmdGbuyingFetchGoods(cl, body, len); break;
    case 0x1392: handle_vm_cmdGbuyingInjectCoin(cl, body, len); break;
    case 0x1395: handle_vm_msgLGetLocalEncryptInfo(cl, body, len); break;
    case 0x1397: handle_vm_msgLSetLocalEncryptInfo(cl, body, len); break;
    case 0x1398: handle_vm_cmdRequestDeleteDeck(cl, body, len); break;
    case 0x13A0: handle_vm_cmdGetSafeState(cl, body, len); break;
    case 0x13A3: handle_vm_msgLGetAccCompChar(cl, body, len); break;
    case 0x13A4: handle_vm_cmdWearAskTaoShow(cl, body, len); break;
    case 0x13AC: handle_vm_cmdSkillLevelDown(cl, body, len); break;
    case 0x13AE: handle_vm_cmdFinishStall(cl, body, len); break;
    case 0x13B0: handle_vm_cmdPurchaseMultiMatch(cl, body, len); break;
    case 0x13B1: handle_vm_msgLBorrowLocalEncrypt(cl, body, len); break;
    case 0x1416: handle_vm_cmdMobileRequestNpcList(cl, body, len); break;
    case 0x1417: handle_vm_msgRpcServerName(cl, body, len); break;
    case 0x1418: handle_vm_cmdSelectSpecifiedCombatDeck(cl, body, len); break;
    case 0x141A: handle_vm_cmdSpecialMoveEvent(cl, body, len); break;
    case 0x141C: handle_vm_cmdViewAchieveOverview(cl, body, len); break;
    case 0x1A02: handle_vm_cmdQueryRecruitInfo(cl, body, len); break;
    case 0x1A04: handle_vm_cmdPartyBlackList(cl, body, len); break;
    case 0x1A08: handle_vm_cmdWarViewHistoryInfo(cl, body, len); break;
    case 0x1AE6: handle_vm_cmdTeamCancelSeek(cl, body, len); break;
    case 0x1AEA: handle_vm_cmdSelectVisiblePet(cl, body, len); break;
    case 0x1AEC: handle_vm_cmdGoodsBuy(cl, body, len); break;
    case 0x1AEE: handle_vm_cmdJoinSystemGroup(cl, body, len); break;
    case 0x1AF6: handle_vm_cmdFinger(cl, body, len); break;
    case 0x1AF8: handle_vm_cmdopervisualpacks(cl, body, len); break;
    case 0x1AFE: handle_vm_cmdWishingSaleItem(cl, body, len); break;
    case 0x1B0A: handle_vm_cmdCitywarOper(cl, body, len); break;
    case 0x1B0D: handle_vm_cmdGuardsCheer(cl, body, len); break;
    case 0x1B0E: handle_vm_cmdSetWeddingList(cl, body, len); break;
    case 0x1C03: handle_vm_cmdGetServerTime(cl, body, len); break;
    case 0x1C04: handle_vm_cmdOperTMoodInfo(cl, body, len); break;
    case 0x1C06: handle_vm_cmdLook(cl, body, len); break;
    case 0x1C08: handle_vm_cmdEmote(cl, body, len); break;
    case 0x1C09: handle_vm_cmdShopAdEnter(cl, body, len); break;
    case 0x1DF3: handle_vm_msgGsFriendList(cl, body, len); break;
    case 0x1E15: handle_vm_msgGsLogout(cl, body, len); break;
    case 0x1FB6: handle_vm_cmdPartyJobPrivilegeList(cl, body, len); break;
    case 0x1FB7: handle_vm_msgCcsWorldChannel(cl, body, len); break;
    case 0x1FB8: handle_vm_cmdMulGoodsBuy(cl, body, len); break;
    case 0x1FBC: handle_vm_cmdOperDiscoverEvent(cl, body, len); break;
    case 0x1FD0: handle_vm_cmdLearnCooking(cl, body, len); break;
    case 0x1FD2: handle_vm_cmdFetchOtherStore(cl, body, len); break;
    case 0x1FD4: handle_vm_cmdWarcraftRefresh(cl, body, len); break;
    case 0x1FD6: handle_vm_cmdReplyClientDoubtInfo(cl, body, len); break;
    case 0x201C: handle_vm_cmdRequestOpenSystemDlg(cl, body, len); break;
    case 0x2020: handle_vm_cmdLaunchConvenant(cl, body, len); break;
    case 0x2030: handle_vm_cmdQueryPracticeChildren(cl, body, len); break;
    case 0x2032: handle_vm_cmdGetTodayStat(cl, body, len); break;
    case 0x2036: handle_vm_cmdRequestEnterCitywarRoom(cl, body, len); break;
    case 0x203E: handle_vm_cmdFeedPet(cl, body, len); break;
    case 0x2040: handle_vm_cmdRequestCardGameSortDeck(cl, body, len); break;
    case 0x204E: handle_vm_cmdUpgrade(cl, body, len); break;
    case 0x2050: handle_vm_cmdOpenMenu1(cl, body, len); break;
    case 0x2064: handle_vm_cmdOpenMenu0(cl, body, len); break;
    case 0x2068: handle_vm_cmdPartyRelation(cl, body, len); break;
    case 0x206A: handle_vm_cmdSelectCurrentPet(cl, body, len); break;
    case 0x2074: handle_vm_cmdViewTop(cl, body, len); break;
    case 0x207E: handle_vm_cmdMergeMagicPicture(cl, body, len); break;
    case 0x2082: handle_vm_cmdEquipInbornStone(cl, body, len); break;
    case 0x2086: handle_vm_cmdPartywarOper(cl, body, len); break;
    case 0x2088: handle_vm_cmdSignInDraw(cl, body, len); break;
    case 0x20A6: handle_vm_cmdWuDaoDrawRepCard(cl, body, len); break;
    case 0x20AE: handle_vm_cmdUpgradeMagicPicture(cl, body, len); break;
    case 0x20B0: handle_vm_cmdEquipPetDress(cl, body, len); break;
    case 0x20B4: handle_vm_cmdRewriteChildBook(cl, body, len); break;
    case 0x20B8: handle_vm_cmdGetPartyLog(cl, body, len); break;
    case 0x20BC: handle_vm_cmdFriendTellAuth(cl, body, len); break;
    case 0x20D3: handle_vm_cmdGuardsRecover(cl, body, len); break;
    case 0x20D4: handle_vm_cmdRequestBuyCardPackage(cl, body, len); break;
    case 0x20DA: handle_vm_cmdPartyInfo(cl, body, len); break;
    case 0x20DB: handle_vm_cmdShopVendueShop(cl, body, len); break;
    case 0x20E0: handle_vm_cmdGuessOk(cl, body, len); break;
    case 0x20EE: handle_vm_cmdLookPlayer(cl, body, len); break;
    case 0x20F0: handle_vm_cmdPreAssignChildAttrib(cl, body, len); break;
    case 0x20F2: handle_vm_cmdPresentMusic(cl, body, len); break;
    case 0x20F4: handle_vm_cmdTradeGetRecord(cl, body, len); break;
    case 0x20F8: handle_vm_cmdPurchaseGetObjProp(cl, body, len); break;
    case 0x2100: handle_vm_cmdLIsConvertBotton(cl, body, len); break;
    case 0x2110: handle_vm_cmdSwitchBackEquip(cl, body, len); break;
    case 0x2112: handle_vm_cmdWudaoConvertExtraAttrib(cl, body, len); break;
    case 0x2116: handle_vm_cmdSetUserMood(cl, body, len); break;
    case 0x2118: handle_vm_cmdFishingApplyMultiCard(cl, body, len); break;
    case 0x2123: handle_vm_cmdDungeonViewTasks(cl, body, len); break;
    case 0x2132: handle_vm_cmdFeedChild(cl, body, len); break;
    case 0x2139: handle_vm_msgLConvertSilverCoin(cl, body, len); break;
    case 0x2140: handle_vm_cmdRequestFriendVerify(cl, body, len); break;
    case 0x2147: handle_vm_msgCcsFingerRes(cl, body, len); break;
    case 0x2150: handle_vm_cmdGather(cl, body, len); break;
    case 0x2152: handle_vm_cmdOpenConfirmDlg(cl, body, len); break;
    case 0x2156: handle_vm_cmdNewbiePlayedDrama(cl, body, len); break;
    case 0x2159: handle_vm_msgArenaOper(cl, body, len); break;
    case 0x215A: handle_vm_cmdRequestRecharge(cl, body, len); break;
    case 0x215C: handle_vm_cmdSetSafeLoginTime(cl, body, len); break;
    case 0x215E: handle_vm_cmdSelectMenuItem2(cl, body, len); break;
    case 0x2160: handle_vm_cmdTeamSeekMember(cl, body, len); break;
    case 0x2161: handle_vm_msgChannelOper(cl, body, len); break;
    case 0x2162: handle_vm_cmdLearnSkill(cl, body, len); break;
    case 0x2166: handle_vm_cmdRequestCardActivityList(cl, body, len); break;
    case 0x2168: handle_vm_cmdOtherMoveTo(cl, body, len); break;
    case 0x2170: handle_vm_cmdLocalEncryptRequest(cl, body, len); break;
    case 0x2172: handle_vm_cmdoperstrategywar(cl, body, len); break;
    case 0x2179: handle_vm_cmdRequestItemInfo(cl, body, len); break;
    case 0x2180: handle_vm_cmdGetDailyLoop(cl, body, len); break;
    case 0x2181: handle_vm_cmdVendueManage(cl, body, len); break;
    case 0x2182: handle_vm_cmdNextQuestion(cl, body, len); break;
    case 0x2186: handle_vm_cmdMoveOnCarpet(cl, body, len); break;
    case 0x2188: handle_vm_cmdSetStallGoods(cl, body, len); break;
    case 0x218C: handle_vm_cmdFriendDrinkInvite(cl, body, len); break;
    case 0x2192: handle_vm_cmdTellEx(cl, body, len); break;
    case 0x2194: handle_vm_cmdSamsungOper(cl, body, len); break;
    case 0x2195: handle_vm_cmdShopSearch(cl, body, len); break;
    case 0x2196: handle_vm_cmdReceiveAskTaoShowGift(cl, body, len); break;
    case 0x2198: handle_vm_cmdRequestAddStallPage(cl, body, len); break;
    case 0x219A: handle_vm_cmdSetAchieveTrace(cl, body, len); break;
    case 0x21A0: handle_vm_cmdSetWeddingPacket(cl, body, len); break;
    case 0x21B0: handle_vm_cmdOpenSafeSetting(cl, body, len); break;
    case 0x21B2: handle_vm_cmdWorkshopQueryOrderRate(cl, body, len); break;
    case 0x21B6: handle_vm_cmdRequestMyCardPackages(cl, body, len); break;
    case 0x21B8: handle_vm_cmdWuDaoPreAddAttribRep(cl, body, len); break;
    case 0x21BA: handle_vm_cmdViewPartyWarHistory(cl, body, len); break;
    case 0x21BD: handle_vm_cmdVendueOffer(cl, body, len); break;
    case 0x21C0: handle_vm_cmdGetTaskItem(cl, body, len); break;
    case 0x21C2: handle_vm_cmdExchangeChildAppellation(cl, body, len); break;
    case 0x21C4: handle_vm_cmdReplaceLogin(cl, body, len); break;
    case 0x21CA: handle_vm_cmdWithdraw(cl, body, len); break;
    case 0x21CC: handle_vm_cmdStartCardActivity(cl, body, len); break;
    case 0x21DB: handle_vm_cmdGuessCheckLocked(cl, body, len); break;
    case 0x2202: handle_vm_cmdWuDaoBuyRepCard(cl, body, len); break;
    case 0x2208: handle_vm_cmdQueryUpgradeCash(cl, body, len); break;
    case 0x220A: handle_vm_cmdRefreshpartyFragment(cl, body, len); break;
    case 0x2210: handle_vm_cmdGbuyingFetchCoin(cl, body, len); break;
    case 0x2212: handle_vm_cmdEnterRoom2(cl, body, len); break;
    case 0x2214: handle_vm_cmdWishingOpen(cl, body, len); break;
    case 0x2220: handle_vm_cmdDraftRoll(cl, body, len); break;
    case 0x2224: handle_vm_cmdRefresh(cl, body, len); break;
    case 0x2226: handle_vm_cmdViewAchieve(cl, body, len); break;
    case 0x2228: handle_vm_cmdTrain(cl, body, len); break;
    case 0x2230: handle_vm_cmdShouWuZuDao(cl, body, len); break;
    case 0x2232: handle_vm_cmdXSetSetting(cl, body, len); break;
    case 0x2234: handle_vm_cmdViewCsMatchesSummary(cl, body, len); break;
    case 0x2236: handle_vm_cmdDrop(cl, body, len); break;
    case 0x2238: handle_vm_cmdSetTaskTrace(cl, body, len); break;
    case 0x2244: handle_vm_cmdDrawCardRankBonus(cl, body, len); break;
    case 0x2246: handle_vm_cmdTeamSeekLeader(cl, body, len); break;
    case 0x2254: handle_vm_cmdOperSortTask(cl, body, len); break;
    case 0x2256: handle_vm_cmdSignInOpen(cl, body, len); break;
    case 0x2258: handle_vm_cmdOpenAskTaoShowMall(cl, body, len); break;
    case 0x225A: handle_vm_cmdOperEnvelope(cl, body, len); break;
    case 0x225C: handle_vm_cmdDraftRequestStageInfo(cl, body, len); break;
    case 0x225E: handle_vm_cmdDoucdhChallenge(cl, body, len); break;
    case 0x2262: handle_vm_cmdSubmitPasspodOrder(cl, body, len); break;
    case 0x2264: handle_vm_cmdPreAssignAttrib(cl, body, len); break;
    case 0x2266: handle_vm_cmdSetHotkeys(cl, body, len); break;
    case 0x2268: handle_vm_cmdJoinChatGroup(cl, body, len); break;
    case 0x226B: handle_vm_msgGsEmoteData(cl, body, len); break;
    case 0x226C: handle_vm_cmdOpenMountInbornSkills(cl, body, len); break;
    case 0x2271: handle_vm_cmdShopManageCounter(cl, body, len); break;
    case 0x227A: handle_vm_cmdSelectVisibleChild(cl, body, len); break;
    case 0x227C: handle_vm_cmdRefreshTaskLog(cl, body, len); break;
    case 0x2280: handle_vm_cmdFetchGbuyingCoin(cl, body, len); break;
    case 0x2282: handle_vm_cmdPurchaseCancleOrder(cl, body, len); break;
    case 0x2283: handle_vm_cmdShopManageMoney(cl, body, len); break;
    case 0x2292: handle_vm_cmdRecoverChildBook(cl, body, len); break;
    case 0x2294: handle_vm_cmdTryPlacePattern(cl, body, len); break;
    case 0x2296: handle_vm_cmdRequestToBeTeamLeader(cl, body, len); break;
    case 0x2300: handle_vm_cmdQueryPracticePets(cl, body, len); break;
    case 0x2302: handle_vm_cmdOperCasualGame(cl, body, len); break;
    case 0x2304: handle_vm_cmdIdentifyEquip(cl, body, len); break;
    case 0x2306: handle_vm_cmdFriendTell(cl, body, len); break;
    case 0x2307: handle_vm_msgIntersvrChannelMsg(cl, body, len); break;
    case 0x230A: handle_vm_cmdViewCsPwFixtures(cl, body, len); break;
    case 0x2310: handle_vm_cmdInvite(cl, body, len); break;
    case 0x2316: handle_vm_cmdLocalEncryptBorrow(cl, body, len); break;
    case 0x2320: handle_vm_cmdGetImage(cl, body, len); break;
    case 0x2324: handle_vm_cmdRaceRequestBonus(cl, body, len); break;
    case 0x2326: handle_vm_cmdFishingBuyWeapon(cl, body, len); break;
    case 0x2328: handle_vm_cmdGet(cl, body, len); break;
    case 0x2329: handle_vm_cmdGuessEnd(cl, body, len); break;
    case 0x2334: handle_vm_cmdSetFriendRemark(cl, body, len); break;
    case 0x2340: handle_vm_cmdPartyModifyMember(cl, body, len); break;
    case 0x2346: handle_vm_cmdFishingFireBullet(cl, body, len); break;
    case 0x2347: handle_vm_msgCcsOnlineFriend(cl, body, len); break;
    case 0x2348: handle_vm_cmdSetSetting(cl, body, len); break;
    case 0x234A: handle_vm_cmdEnterRoom1(cl, body, len); break;
    case 0x234C: handle_vm_cmdFishingOpenRemoteStore(cl, body, len); break;
    case 0x2353: handle_vm_cmdShopBuyProduct(cl, body, len); break;
    case 0x2354: handle_vm_cmdSDDHOper(cl, body, len); break;
    case 0x2358: handle_vm_cmdLAgentConnectServer(cl, body, len); break;
    case 0x235B: handle_vm_msgCfgDistList(cl, body, len); break;
    case 0x235D: handle_vm_cmdShopEnterShop(cl, body, len); break;
    case 0x2363: handle_vm_msgCfgDdbList(cl, body, len); break;
    case 0x2367: handle_vm_msgCfgCcsInfo(cl, body, len); break;
    case 0x236C: handle_vm_cmdRequestNewDeck(cl, body, len); break;
    case 0x237D: handle_vm_msgLBuyCard(cl, body, len); break;
    case 0x2380: handle_vm_cmdIgnoreConvenant(cl, body, len); break;
    case 0x2383: handle_vm_msgCcsEmoteData(cl, body, len); break;
    case 0x2384: handle_vm_cmdShopSubmit(cl, body, len); break;
    case 0x238B: handle_vm_msgLQueryAccountState(cl, body, len); break;
    case 0x238F: handle_vm_msgLQueryAddiction(cl, body, len); break;
    case 0x2390: handle_vm_msgLGetSafeCtrlInfo(cl, body, len); break;
    case 0x2391: handle_vm_msgLUpdateAddiction(cl, body, len); break;
    case 0x2393: handle_vm_msgLQueryIpArea(cl, body, len); break;
    case 0x239C: handle_vm_msgLSetSafeCtrlStr(cl, body, len); break;
    case 0x239D: handle_vm_msgLSetSafeCtrlVar(cl, body, len); break;
    case 0x23A1: handle_vm_msgLQueryCitys(cl, body, len); break;
    case 0x23A4: handle_vm_cmdPurchaseFetchCash(cl, body, len); break;
    case 0x23A8: handle_vm_cmdRequestFinishCardCompete(cl, body, len); break;
    case 0x23AA: handle_vm_cmdUnequipPetDress(cl, body, len); break;
    case 0x23AB: handle_vm_msgXGameOper(cl, body, len); break;
    case 0x23AC: handle_vm_cmdDraftRequestStageRank(cl, body, len); break;
    case 0x23B0: handle_vm_cmdRequestAskTaoShow(cl, body, len); break;
    case 0x2400: handle_vm_cmdLPendShop(cl, body, len); break;
    case 0x2401: handle_vm_msgLQueryCoupon(cl, body, len); break;
    case 0x2415: handle_vm_cmdShopReqVendues(cl, body, len); break;
    case 0x2422: handle_vm_cmdLocalEncryptLockFailed(cl, body, len); break;
    case 0x2424: handle_vm_cmdTradeContinueSale(cl, body, len); break;
    case 0x2426: handle_vm_cmdLocalEncryptUnlock(cl, body, len); break;
    case 0x242D: handle_vm_cmdLWhenBlockAccount(cl, body, len); break;
    case 0x2440: handle_vm_cmdRequestCardTodayStat(cl, body, len); break;
    case 0x2444: handle_vm_cmdOperMaster(cl, body, len); break;
    case 0x2446: handle_vm_cmdWarcraftCancel(cl, body, len); break;
    case 0x2471: handle_vm_msgCfgEtcPageData(cl, body, len); break;
    case 0x2473: handle_vm_msgCfgReadEtc(cl, body, len); break;
    case 0x2475: handle_vm_msgCfgAHServerInfo(cl, body, len); break;
    case 0x2479: handle_vm_msgCfgAaaList(cl, body, len); break;
    case 0x247B: handle_vm_msgCfgWriteEtc(cl, body, len); break;
    case 0x247D: handle_vm_msgCfgCsaInfo(cl, body, len); break;
    case 0x247F: handle_vm_msgCfgTtsInfo(cl, body, len); break;
    case 0x2480: handle_vm_cmdLRecycleStall(cl, body, len); break;
    case 0x2482: handle_vm_cmdLBlockShareStore(cl, body, len); break;
    case 0x2484: handle_vm_cmdLWhenBatchBlockAccount(cl, body, len); break;
    case 0x2502: handle_vm_cmdMusicList(cl, body, len); break;
    case 0x2510: handle_vm_cmdoperlockexp(cl, body, len); break;
    case 0x2518: handle_vm_cmdSetWeiboInfo(cl, body, len); break;
    case 0x251A: handle_vm_cmdTakeUp(cl, body, len); break;
    case 0x2602: handle_vm_cmdWuDaoStageInfo(cl, body, len); break;
    case 0x2608: handle_vm_cmdNewbieFirstAction(cl, body, len); break;
    case 0x260A: handle_vm_cmdStartBoxAdventrue(cl, body, len); break;
    case 0x260C: handle_vm_cmdPreviewPetPracticeBonus(cl, body, len); break;
    case 0x2712: handle_vm_cmdEnterArenaRoom(cl, body, len); break;
    case 0x2714: handle_vm_cmdLookOn(cl, body, len); break;
    case 0x271A: handle_vm_cmdPasswordDlg(cl, body, len); break;
    case 0x2720: handle_vm_cmdLotterySelect(cl, body, len); break;
    case 0x2724: handle_vm_cmdSignIn(cl, body, len); break;
    case 0x2726: handle_vm_cmdOperLotteryTicket(cl, body, len); break;
    case 0x2728: handle_vm_cmdGetAwardInfo(cl, body, len); break;
    case 0x272A: handle_vm_cmdRequestStartCardCompete(cl, body, len); break;
    case 0x2803: handle_vm_cmdShopViewOwnShop(cl, body, len); break;
    case 0x2804: handle_vm_cmdChannelOper(cl, body, len); break;
    case 0x2805: handle_vm_msgCcsFriendTell(cl, body, len); break;
    case 0x2806: handle_vm_cmdPlaceFurniture(cl, body, len); break;
    case 0x2808: handle_vm_cmdWho(cl, body, len); break;
    case 0x2812: handle_vm_cmdTradeChangePrice(cl, body, len); break;
    case 0x2814: handle_vm_cmdRequestReceiveCityTask(cl, body, len); break;
    case 0x2903: handle_vm_cmdVendueStoreClosed(cl, body, len); break;
    case 0x290F: handle_vm_cmdGetClientPhyConfig(cl, body, len); break;
    case 0x29C7: handle_vm_cmdSetUserState(cl, body, len); break;
    case 0x2A00: handle_vm_cmdFriendTellEx(cl, body, len); break;
    case 0x2AF2: handle_vm_cmdTradeIntroduce(cl, body, len); break;
    case 0x2C04: handle_vm_msgGotServerTime(cl, body, len); break;
    case 0x2DD1: handle_vm_cmdDraftOper(cl, body, len); break;
    case 0x2DD5: handle_vm_cmdVendueRefresh(cl, body, len); break;
    case 0x2E37: handle_vm_msgGetSysEnvelope(cl, body, len); break;
    case 0x2E3A: handle_vm_cmdInviteChatGroupMember(cl, body, len); break;
    case 0x2EF1: handle_vm_cmdRaidKickoutMember(cl, body, len); break;
    case 0x2EF2: handle_vm_cmdPurchaseAddOrder(cl, body, len); break;
    case 0x2EF7: handle_vm_cmdModifySchedule(cl, body, len); break;
    case 0x2EFC: handle_vm_cmdGbuyingOpenPage(cl, body, len); break;
    case 0x2EFE: handle_vm_cmdTakeoutItem(cl, body, len); break;
    case 0x2FD3: handle_vm_cmdLSetSafeCtrlContent(cl, body, len); break;
    case 0x3012: handle_vm_cmdDraftPresentFlower(cl, body, len); break;
    case 0x3014: handle_vm_cmdGbuyingBuyGoods(cl, body, len); break;
    case 0x3015: handle_vm_cmdMovToQiankundai(cl, body, len); break;
    case 0x3016: handle_vm_cmdQuitLookOn(cl, body, len); break;
    case 0x301B: handle_vm_cmdGuardsRefresh(cl, body, len); break;
    case 0x3038: handle_vm_cmdLookonBroadcastCombat(cl, body, len); break;
    case 0x3046: handle_vm_cmdPurchaseGetOrderList(cl, body, len); break;
    case 0x304C: handle_vm_cmdArenaOper(cl, body, len); break;
    case 0x304E: handle_vm_cmdWishingBuyItem(cl, body, len); break;
    case 0x3052: handle_vm_cmdSwitchChildItem(cl, body, len); break;
    case 0x3054: handle_vm_cmdTeamOpenPlatform(cl, body, len); break;
    case 0x3056: handle_vm_cmdLeaveMagicTower(cl, body, len); break;
    case 0x3060: handle_vm_cmdSwitchInStore(cl, body, len); break;
    case 0x3064: handle_vm_cmdRefreshPetGodbookSkills(cl, body, len); break;
    case 0x3068: handle_vm_cmdTakeStoringCash(cl, body, len); break;
    case 0x306A: handle_vm_cmdWorkshopQueryOrderCash(cl, body, len); break;
    case 0x306C: handle_vm_cmdRequestCityInfo(cl, body, len); break;
    case 0x3070: handle_vm_cmdRequestLevelUp(cl, body, len); break;
    case 0x307A: handle_vm_cmdTempNpcEvent(cl, body, len); break;
    case 0x30AC: handle_vm_cmdXGameOper(cl, body, len); break;
    case 0x30CA: handle_vm_cmdSelectMenuItem1(cl, body, len); break;
    case 0x30CC: handle_vm_cmdTakeoutCard(cl, body, len); break;
    case 0x30D0: handle_vm_cmdReportIllegalTool(cl, body, len); break;
    case 0x30D1: handle_vm_msgDataMining(cl, body, len); break;
    case 0x3101: handle_vm_msgLeagueOper(cl, body, len); break;
    case 0x3102: handle_vm_cmdLeagueOper(cl, body, len); break;
    case 0x3103: handle_vm_msgBottleOper(cl, body, len); break;
    case 0x3104: handle_vm_cmdBottleOper(cl, body, len); break;
    case 0x3202: handle_vm_cmdOpenArenaRoomSeat(cl, body, len); break;
    case 0x3208: handle_vm_cmdTowerSelected(cl, body, len); break;
    case 0x3300: handle_vm_cmdDropPet(cl, body, len); break;
    case 0x3302: handle_vm_cmdHsslStartChallenge(cl, body, len); break;
    case 0x3317: handle_vm_msgLChangeCoupon(cl, body, len); break;
    case 0x3319: handle_vm_msgLQueryRandomCoupon(cl, body, len); break;
    case 0x3325: handle_vm_msgDoRecharge(cl, body, len); break;
    case 0x3327: handle_vm_msgDoInquiry(cl, body, len); break;
    case 0x3328: handle_vm_operHellInstruction(cl, body, len); break;
    case 0x332A: handle_vm_cmdTaskScoreExchangeBonus(cl, body, len); break;
    case 0x3369: handle_vm_msgLVerifyPassword(cl, body, len); break;
    case 0x3379: handle_vm_msgLLockCard(cl, body, len); break;
    case 0x337B: handle_vm_msgLUnlockCard(cl, body, len); break;
    case 0x3380: handle_vm_msgLGetSsbCombatRes(cl, body, len); break;
    case 0x3381: handle_vm_msgLSsbUpdateAccount(cl, body, len); break;
    case 0x3382: handle_vm_msgLSsbCopyAccount(cl, body, len); break;
    case 0x3383: handle_vm_msgLGetSsbCombatConf(cl, body, len); break;
    case 0x3384: handle_vm_msgLGetSsbFinishFlag(cl, body, len); break;
    case 0x3385: handle_vm_msgLGetSsbPromptMsg(cl, body, len); break;
    case 0x3386: handle_vm_msgLCloneAccountResult(cl, body, len); break;
    case 0x3389: handle_vm_msgLLockAccount(cl, body, len); break;
    case 0x338D: handle_vm_msgLEnableAutoLock(cl, body, len); break;
    case 0x3390: handle_vm_cmdWudaoSetAttribStatus(cl, body, len); break;
    case 0x3391: handle_vm_cmdShopBuyShop(cl, body, len); break;
    case 0x3392: handle_vm_cmdRandomUserName(cl, body, len); break;
    case 0x3402: handle_vm_cmdSetPassword(cl, body, len); break;
    case 0x3406: handle_vm_cmdRequestCancelMatchCardGame(cl, body, len); break;
    case 0x3421: handle_vm_msgXgCreateName(cl, body, len); break;
    case 0x3423: handle_vm_msgXgGetAccount(cl, body, len); break;
    case 0x3425: handle_vm_msgXgGetName(cl, body, len); break;
    case 0x3502: handle_vm_cmdWarcraftStart(cl, body, len); break;
    case 0x3504: handle_vm_cmdTake(cl, body, len); break;
    case 0x3506: handle_vm_cmdGetPartyChannelDenyList(cl, body, len); break;
    case 0x3508: handle_vm_operGardenInstruction(cl, body, len); break;
    case 0x350A: handle_vm_cmdRequestRenameDeck(cl, body, len); break;
    case 0x350E: handle_vm_cmdRepair(cl, body, len); break;
    case 0x3702: handle_vm_cmdKickout(cl, body, len); break;
    case 0x3704: handle_vm_cmdRequestMineralsInMap(cl, body, len); break;
    case 0x3706: handle_vm_cmdExchangeOpen(cl, body, len); break;
    case 0x3708: handle_vm_cmdListenMusic(cl, body, len); break;
    case 0x370A: handle_vm_cmdTradeGetData(cl, body, len); break;
    case 0x3801: handle_vm_msgGsFriendTell(cl, body, len); break;
    case 0x3802: handle_vm_cmdGeneralRequest(cl, body, len); break;
    case 0x3804: handle_vm_cmdRecoverMagicPennant(cl, body, len); break;
    case 0x39D1: handle_vm_msgCsaCommonOper(cl, body, len); break;
    case 0x3A02: handle_vm_msgInternalAuth(cl, body, len); break;
    case 0x3A14: handle_vm_cmdRequestJoinRecruitment(cl, body, len); break;
    case 0x3AEE: handle_vm_cmdSetDoucdhPetPos(cl, body, len); break;
    case 0x3E00: handle_vm_cmdQueryReportInfo(cl, body, len); break;
    case 0x3E05: handle_vm_cmdGuardsDropGuard(cl, body, len); break;
    case 0x3E12: handle_vm_cmdDraftReport(cl, body, len); break;
    case 0x3E14: handle_vm_cmdReplyClientProcessInfo(cl, body, len); break;
    case 0x3E20: handle_vm_cmdRequestDeckInfo(cl, body, len); break;
    case 0x3E22: handle_vm_cmdHsslRequestInfo(cl, body, len); break;
    case 0x3E24: handle_vm_cmdOpenOnlineMall(cl, body, len); break;
    case 0x3E26: handle_vm_cmdOperSecurity(cl, body, len); break;
    case 0x3E28: handle_vm_cmdModifyCustomNpcList(cl, body, len); break;
    case 0x3E2A: handle_vm_cmdLogout(cl, body, len); break;
    case 0x3E2C: handle_vm_cmdRequestStallList(cl, body, len); break;
    case 0x3E32: handle_vm_cmdWLResetSeal(cl, body, len); break;
    case 0x3E34: handle_vm_cmdWLSealInfo(cl, body, len); break;
    case 0x4002: handle_vm_cmdTradeCancelSale(cl, body, len); break;
    case 0x4004: handle_vm_cmdReselectChar(cl, body, len); break;
    case 0x4006: handle_vm_cmdDropChild(cl, body, len); break;
    case 0x400A: handle_vm_cmdoperpackeditem(cl, body, len); break;
    case 0x4010: handle_vm_cmdRequestCardList(cl, body, len); break;
    case 0x4012: handle_vm_cmdGetAchieveBonus(cl, body, len); break;
    case 0x4014: handle_vm_cmdViewCsFixtures(cl, body, len); break;
    case 0x401C: handle_vm_cmdopersixanniinfo(cl, body, len); break;
    case 0x402E: handle_vm_cmdRepairFurniture(cl, body, len); break;
    case 0x4058: handle_vm_cmdExchangeCancel(cl, body, len); break;
    case 0x4062: handle_vm_cmdWishing(cl, body, len); break;
    case 0x4076: handle_vm_cmdReject(cl, body, len); break;
    case 0x4078: handle_vm_cmdDropTask(cl, body, len); break;
    case 0x408A: handle_vm_cmdQuitChatGroup(cl, body, len); break;
    case 0x40BA: handle_vm_cmdManageCity(cl, body, len); break;
    case 0x40C6: handle_vm_cmdHellDealer(cl, body, len); break;
    case 0x40CE: handle_vm_cmdSetXinfaSkillStatus(cl, body, len); break;
    case 0x40D2: handle_vm_cmdFishSwallowFinish(cl, body, len); break;
    case 0x40D4: handle_vm_cmdOpenBureau(cl, body, len); break;
    case 0x40E4: handle_vm_cmdRequestBuildDeck(cl, body, len); break;
    case 0x40F6: handle_vm_cmdMaOper(cl, body, len); break;
    case 0x40F8: handle_vm_cmdMaSet(cl, body, len); break;
    case 0x4102: handle_vm_cmdTakeoutGodbook(cl, body, len); break;
    case 0x4105: handle_vm_msgSDDHOper(cl, body, len); break;
    case 0x4106: handle_vm_cmdRequestPropertyBind(cl, body, len); break;
    case 0x410A: handle_vm_cmdTell(cl, body, len); break;
    case 0x4116: handle_vm_cmdSubmitConfirmDlg(cl, body, len); break;
    case 0x4118: handle_vm_cmdTradeFavorite(cl, body, len); break;
    case 0x4120: handle_vm_cmdPromptBeforeLeave(cl, body, len); break;
    case 0x4124: handle_vm_cmdLogin(cl, body, len); break;
    case 0x4126: handle_vm_cmdQueryUpgradeCashEx(cl, body, len); break;
    case 0x4130: handle_vm_cmdMoveTo(cl, body, len); break;
    case 0x4132: handle_vm_cmdLocalEncryptLocked(cl, body, len); break;
    case 0x4226: handle_vm_cmdLearnZhenfa(cl, body, len); break;
    case 0x4228: handle_vm_cmdSwitchGroupEx(cl, body, len); break;
    case 0x4230: handle_vm_cmdFishingChangeWeapon(cl, body, len); break;
    case 0x4367: handle_vm_msgVerifyCodeResult(cl, body, len); break;
    case 0x4381: handle_vm_msgLQueryFillInfo(cl, body, len); break;
    case 0x4385: handle_vm_msgLAddFillInfo(cl, body, len); break;
    case 0x4387: handle_vm_msgLEcardBusiness(cl, body, len); break;
    case 0x4420: handle_vm_cmdOpenAuthAllProtect(cl, body, len); break;
    case 0x4423: handle_vm_msgTreasureTradingOper(cl, body, len); break;
    case 0x4AF2: handle_vm_cmdSetSafeState(cl, body, len); break;
    case 0x4AF4: handle_vm_cmdNotifySendWeibo(cl, body, len); break;
    case 0x5000: handle_vm_cmdOperatePetStore(cl, body, len); break;
    case 0x5002: handle_vm_cmdViewStall(cl, body, len); break;
    case 0x5004: handle_vm_cmdExchangeOk(cl, body, len); break;
    case 0x5005: handle_vm_cmdGetOrderRank(cl, body, len); break;
    case 0x5006: handle_vm_cmdReselectVerifyCode(cl, body, len); break;
    case 0x500A: handle_vm_cmdOpenMusic(cl, body, len); break;
    case 0x500C: handle_vm_cmdStartTaotaotaoRun(cl, body, len); break;
    case 0x500E: handle_vm_cmdOpenBoxAdventrueInterface(cl, body, len); break;
    case 0x5010: handle_vm_cmdPurchaseFetchItem(cl, body, len); break;
    case 0x5016: handle_vm_cmdPlaceBox(cl, body, len); break;
    case 0x5018: handle_vm_getInstructionBonus(cl, body, len); break;
    case 0x508A: handle_vm_cmdViewCsMatches(cl, body, len); break;
    case 0x508C: handle_vm_cmdGoodsSell(cl, body, len); break;
    case 0x50A6: handle_vm_cmdSysPurchaseViewHistory(cl, body, len); break;
    case 0x50A8: handle_vm_cmdActivityOper(cl, body, len); break;
    case 0x510A: handle_vm_cmdSetSafeMechanism(cl, body, len); break;
    case 0x5112: handle_vm_cmdTradeAddGoods(cl, body, len); break;
    case 0x5202: handle_vm_cmdWuDaoAddAttribRep(cl, body, len); break;
    case 0x5361: handle_vm_msgLChargeResult(cl, body, len); break;
    case 0x536B: handle_vm_msgLRequestIpCard(cl, body, len); break;
    case 0x536D: handle_vm_msgLRequestCoupon(cl, body, len); break;
    case 0x537F: handle_vm_msgLQueryCard(cl, body, len); break;
    case 0x5383: handle_vm_msgLCostFillInfo(cl, body, len); break;
    case 0x5501: handle_vm_msgLOperProtectRange(cl, body, len); break;
    case 0x5503: handle_vm_msgLAuthAllProtect(cl, body, len); break;
    case 0x5505: handle_vm_cmdShopAdFill(cl, body, len); break;
    case 0x5506: handle_vm_cmdMakePill(cl, body, len); break;
    case 0x5E21: handle_vm_cmdShopDoBusiness(cl, body, len); break;
    case 0x6004: handle_vm_cmdShopQueryCounter(cl, body, len); break;
    case 0x6005: handle_vm_cmdVendueCanVendue(cl, body, len); break;
    case 0x6010: handle_vm_cmdSetpass(cl, body, len); break;
    case 0x6373: handle_vm_msgLQueryCoin(cl, body, len); break;
    case 0x6375: handle_vm_msgLCostCoin(cl, body, len); break;
    case 0x6377: handle_vm_msgLPrecostCoin(cl, body, len); break;
    case 0x6379: handle_vm_msgLAddCoin(cl, body, len); break;
    case 0x637B: handle_vm_msgLManageVipSms(cl, body, len); break;
    case 0x637D: handle_vm_msgLSendVipSms(cl, body, len); break;
    case 0x6381: handle_vm_msgLDoAlipayRecharge(cl, body, len); break;
    case 0x6382: handle_vm_cmdWindowSizeChange(cl, body, len); break;
    case 0x6384: handle_vm_cmdLcDoAction(cl, body, len); break;
    case 0x6387: handle_vm_msgLManageMpDevice(cl, body, len); break;
    case 0x638F: handle_vm_msgLGetCreditCoins(cl, body, len); break;
    case 0x7002: handle_vm_cmdShowSaleLog(cl, body, len); break;
    case 0x7004: handle_vm_cmdRequestDrawCardCompeteBonus(cl, body, len); break;
    case 0x7006: handle_vm_cmdSetChildName(cl, body, len); break;
    case 0x700C: handle_vm_cmdTakeChildItemUp(cl, body, len); break;
    case 0x7012: handle_vm_cmdOpenMenu2(cl, body, len); break;
    case 0x7014: handle_vm_cmdSetRecommendAttrib(cl, body, len); break;
    case 0x7016: handle_vm_cmdResetAppellationTaskMode(cl, body, len); break;
    case 0x7018: handle_vm_cmdRequestTakeProperty(cl, body, len); break;
    case 0x701C: handle_vm_cmdDraftVote(cl, body, len); break;
    case 0x701E: handle_vm_cmdSetDecoration(cl, body, len); break;
    case 0x70A8: handle_vm_cmdPartyRequestList(cl, body, len); break;
    case 0x70AC: handle_vm_cmdRequestComposeCard(cl, body, len); break;
    case 0x70AE: handle_vm_cmdTeamRequestTask(cl, body, len); break;
    case 0x7E02: handle_vm_cmdRequestCombatDeckList(cl, body, len); break;
    case 0x7E04: handle_vm_cmdSysPurchaseViewOrders(cl, body, len); break;
    case 0x7E08: handle_vm_cmdShift(cl, body, len); break;
    case 0x7E0A: handle_vm_cmdTeamMatchSeek(cl, body, len); break;
    case 0xCF04: handle_vm_cmdEndChallenge(cl, body, len); break;
    case 0xCF08: handle_vm_cmdPartySendMessage(cl, body, len); break;
    case 0xCF0A: handle_vm_cmdReplyFriendVerify(cl, body, len); break;
    case 0xCF0C: handle_vm_cmdFinishInterfaceGuide(cl, body, len); break;
    case 0xE001: handle_vm_cmdSetImage(cl, body, len); break;
    case 0xE002: handle_vm_cmdViewCouponShow(cl, body, len); break;
    case 0xE004: handle_vm_cmdShowUpgradeEquipItems(cl, body, len); break;
    case 0xEEEE: handle_vm_cmdMountInbornStoneList(cl, body, len); break;
    case 0xF077: handle_vm_cmdSetAutoMsg(cl, body, len); break;
    case 0xF0AF: handle_vm_msgIntersvrRpcRet(cl, body, len); break;
    case 0xF0B5: handle_vm_msgGsWorldChannel(cl, body, len); break;
    case 0xF0C4: handle_vm_cmdrefreshmountattrib(cl, body, len); break;
    case 0xF0C8: handle_vm_cmdWorkshopBuyOrder(cl, body, len); break;
    case 0xF0CA: handle_vm_cmdRequestChatGroupInfo(cl, body, len); break;
    case 0xF0CC: handle_vm_cmdMobileOperTask(cl, body, len); break;
    case 0xF0D0: handle_vm_cmdSysPurchaseSell(cl, body, len); break;
    case 0xF0D2: handle_vm_cmdGetRandomBoxAward(cl, body, len); break;
    case 0xF0DB: handle_vm_cmdSetRecPerson(cl, body, len); break;
    case 0xF370: handle_vm_cmdFinishCombatGuide(cl, body, len); break;
    case 0xF372: handle_vm_cmdStallInputPrice(cl, body, len); break;
    case 0xF376: handle_vm_cmdStore(cl, body, len); break;
    case 0xF504: handle_vm_msgLGetVoteResult(cl, body, len); break;
    case 0xF510: handle_vm_cmdPurchaseMatchOrder(cl, body, len); break;
    case 0xF512: handle_vm_cmdRequestEnterXGame(cl, body, len); break;
    case 0xF903: handle_vm_cmdCtaToServer(cl, body, len); break;
    case 0xF906: handle_vm_cmdSysPurchaseBuy(cl, body, len); break;
    case 0xF9C1: handle_vm_cmdSubmitIcon(cl, body, len); break;
    case 0xFA09: handle_vm_cmdVendueTryVendue(cl, body, len); break;
    case 0xFA0D: handle_vm_cmdGetClientPerformance(cl, body, len); break;
    case 0xFA11: handle_vm_cmdExchangeOpenEx(cl, body, len); break;
    case 0xFA13: handle_vm_cmdShopReqRunnings(cl, body, len); break;
    case 0xFA15: handle_vm_msgCitywarOper(cl, body, len); break;
    case 0xFA21: handle_vm_cmdShopMoveMerch(cl, body, len); break;
    case 0xFA37: handle_vm_msgActivityOper(cl, body, len); break;
    case 0xFA3C: handle_vm_cmdPutInItem(cl, body, len); break;
    case 0xFA42: handle_vm_cmdSendEnvelope(cl, body, len); break;
    case 0xFA43: handle_vm_cmdShopChangeInfo(cl, body, len); break;
    case 0xFA46: handle_vm_cmdLeaveArenaRoom(cl, body, len); break;
    case 0xFA48: handle_vm_cmdTradeViewLog(cl, body, len); break;
    case 0xFA4C: handle_vm_cmdRequestDropCardTask(cl, body, len); break;
    case 0xFAE0: handle_vm_cmdRefreshServiceLog(cl, body, len); break;
    case 0xFAFA: handle_vm_cmdLgxyChallenge(cl, body, len); break;
    case 0xFAFC: handle_vm_cmdTradeSale(cl, body, len); break;
    case 0xFAFD: handle_vm_cmdSetSignature(cl, body, len); break;
    case 0xFD09: handle_vm_cmdVendueCancel(cl, body, len); break;
    case 0xFD0A: handle_vm_cmdUnequip(cl, body, len); break;
    case 0xFD12: handle_vm_cmdManageMpDevice(cl, body, len); break;
    case 0xFD14: handle_vm_cmdModifyItemRemark(cl, body, len); break;
    case 0xFD15: handle_vm_msgGsSystemChannel(cl, body, len); break;
    case 0xFD16: handle_vm_cmdOperMagicPicture(cl, body, len); break;
    case 0xFD17: handle_vm_msgCcsRebootResult(cl, body, len); break;
    case 0xFD1A: handle_vm_cmdRefreshRecruitmentInfo(cl, body, len); break;
    case 0xFD1B: handle_vm_msgPartywarOper(cl, body, len); break;
    case 0xFD1C: handle_vm_cmdMoveChildItem(cl, body, len); break;
    case 0xFD1E: handle_vm_doubleBonusTime(cl, body, len); break;
    case 0xFD26: handle_vm_cmdStartPetPractice(cl, body, len); break;
    case 0xFD28: handle_vm_cmdEndRecruitment(cl, body, len); break;
    case 0xFD29: handle_vm_cmdGuardsChangeName(cl, body, len); break;
    case 0xFD30: handle_vm_cmdGetRaiseBonus(cl, body, len); break;
    case 0xFD33: handle_vm_msgDraftOper(cl, body, len); break;
    case 0xFD34: handle_vm_cmdDropRandomBoxAward(cl, body, len); break;
    case 0xFD38: handle_vm_cmdSearchStallGoods(cl, body, len); break;
    case 0xFD40: handle_vm_cmdQueryActivity(cl, body, len); break;
    case 0xFD42: handle_vm_cmdKill(cl, body, len); break;
    case 0xFD43: handle_vm_cmdVendueFetch(cl, body, len); break;
    case 0xFD4A: handle_vm_cmdRenameStall(cl, body, len); break;
    case 0xFD4B: handle_vm_cmdTeleportEx(cl, body, len); break;
    case 0xFD4E: handle_vm_cmdFinishQuestion(cl, body, len); break;
    case 0xFD51: handle_vm_msgGetGoodsFromTts(cl, body, len); break;
    case 0xFD56: handle_vm_cmdSetPetName(cl, body, len); break;
    case 0xFD57: handle_vm_msgSellTreasure(cl, body, len); break;
    case 0xFD62: handle_vm_cmdPutChildItemDown(cl, body, len); break;
    case 0xFD64: handle_vm_cmdGeneralNotify(cl, body, len); break;
    case 0xFD68: handle_vm_cmdTestCsLogin(cl, body, len); break;
    case 0xFD6A: handle_vm_cmdFinishInterfaceStatus(cl, body, len); break;
    case 0xFD6C: handle_vm_cmdRevokeConvenant(cl, body, len); break;
    case 0xFD70: handle_vm_cmdUpgradeByAmount(cl, body, len); break;
    case 0xFD72: handle_vm_cmdMultiMoveTo(cl, body, len); break;
    case 0xFD79: handle_vm_msgUpdateRechargeReturnCredit(cl, body, len); break;
    case 0xFD82: handle_vm_cmdLuckySign(cl, body, len); break;
    case 0xFD84: handle_vm_cmdPrompt(cl, body, len); break;
    case 0xFD86: handle_vm_cmdExchangeAppraise(cl, body, len); break;
    case 0xFD99: handle_vm_msgUpdateFirstCreditBonus(cl, body, len); break;
    case 0xFD9B: handle_vm_msgSetPhahBonusInfo(cl, body, len); break;
    case 0xFDA2: handle_vm_cmdGetNameByGid(cl, body, len); break;
    case 0xFDA5: handle_vm_msgCheckRoleOnSale(cl, body, len); break;
    case 0xFDAA: handle_vm_cmdEndRaise(cl, body, len); break;
    case 0xFDAC: handle_vm_cmdSetRegion(cl, body, len); break;
    case 0xFDD1: handle_vm_msgCcsSystemChannel(cl, body, len); break;
    case 0xFDDF: handle_vm_cmdShopFavorite(cl, body, len); break;
    case 0xFFDE: handle_vm_cmdStartCardInstruction(cl, body, len); break;
    case 0xFFF1: handle_vm_cmdLGetSafeCtrlContent(cl, body, len); break;
    case 0x4401: handleMsgReadContent(cl, body, len); break;
    case 0x2403: handleMsgWrittenContent(cl, body, len); break;
    case 0x3411: handleMsgCreatedGid(cl, body, len); break;
    case 0x3415: handleMsgGotGid(cl, body, len); break;
    case 0x4419: handleMsgGotGidArr(cl, body, len); break;
    case 0x4417: handleMsgGotGidInfo(cl, body, len); break;
    default:
        if (std::getenv("LPCVM_TRACE"))
            std::cerr << "[game] cmd 0x" << std::hex << cmdNo << std::dec
                      << " handler not implemented\n";
        break;
    }
}

// Feed bytes into the client's packet state machine (vm_clientSocketCallback).
// Status values mirror the original: 1=LOST_SYNC 2=SYNCED_1 3=WAIT_HEADER
// 4=WAIT_BODY.  The receiver reads the 12-byte header (Code/checkSum/tickCount/
// msgLen/cmdNo), then msgLen-12 body bytes, then dispatches.
void feedClient(GameClient& cl, const char* data, size_t n) {
    if (std::getenv("LPCVM_TRACE"))
        std::cerr << "[game] feed client=" << cl.clientNo << " n=" << n
                  << " status=" << static_cast<int>(cl.status) << "\n";
    size_t off = 0;
    while (off < n) {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(data) + off;
        size_t avail = n - off;
        if (cl.status == 1 || cl.status == 2) {
            // LOST_SYNC / SYNCED_1: scan for the MZ sync code.
            size_t i = 0;
            bool found = false;
            while (i < avail) {
                if (p[i] == 'M') {
                    if (i + 1 < avail) {
                        if (p[i + 1] == 'Z') { found = true; break; }
                        // 'M' but not 'MZ': advance one (the 'M' might be part
                        // of a later code after the next byte).
                        i += 1;
                        continue;
                    }
                    // 'M' is the last byte; keep it for the next chunk.
                    cl.hdrBuf.assign(1, 'M');
                    cl.status = 2;
                    off += avail;
                    break;
                }
                i += 1;
            }
            if (!found) {
                if (cl.status == 2 && cl.hdrBuf.size() == 1 && avail > 0) {
                    // previous chunk ended with 'M'; check this chunk's first byte
                    if (p[0] == 'Z') { found = true; i = 0; cl.hdrBuf.clear(); }
                    else { cl.hdrBuf.clear(); cl.status = 1; }
                    if (!found) { off += avail; break; }
                } else {
                    off += avail;
                    break;
                }
            }
            if (found) {
                off += i;  // consume up to (not including) 'M'
                cl.status = 3;
                cl.hdrBuf.clear();
                continue;
            }
            break;
        }
        if (cl.status == 3) {
            // WAIT_HEADER: accumulate 12 bytes.
            size_t need = 12 - cl.hdrBuf.size();
            size_t take = avail < need ? avail : need;
            cl.hdrBuf.append(data + off, take);
            off += take;
            if (cl.hdrBuf.size() < 12) break;
            const uint8_t* h = reinterpret_cast<const uint8_t*>(cl.hdrBuf.data());
            if (h[0] != 'M' || h[1] != 'Z') {
                cl.status = 1;
                cl.hdrBuf.clear();
                ++cl.recvBadPkts;
                continue;
            }
            cl.pendingLen = rd16be(h + 8);
            cl.pendingCmd = rd16be(h + 10);
            cl.bodyGot = 0;
            cl.body.clear();
            // Original vm_clientSocketCallback: len = ROR2(msgLen) - 2.
            // msgLen counts from the cmdNo field (2 bytes) through the body,
            // so bodyLen = msgLen - 2 (NOT msgLen - 12).  Total wire packet =
            // 12-byte header + (msgLen - 2) body bytes.
            if (cl.pendingLen <= 2) {
                // No body: dispatch the command now.
                dispatchCommand(cl, cl.pendingCmd, nullptr, 0);
                cl.status = 3;
                cl.hdrBuf.clear();
                continue;
            }
            cl.status = 4;
            cl.body.reserve(cl.pendingLen - 2);
            continue;
        }
        if (cl.status == 4) {
            // WAIT_BODY
            size_t need = cl.pendingLen - 2 - cl.bodyGot;
            size_t take = avail < need ? avail : need;
            cl.body.insert(cl.body.end(), p, p + take);
            cl.bodyGot += take;
            off += take;
            if (cl.bodyGot >= cl.pendingLen - 2) {
                dispatchCommand(cl, cl.pendingCmd, cl.body.data(), cl.body.size());
                cl.status = 3;
                cl.hdrBuf.clear();
                // More packets may already be buffered in this recv() chunk
                // (dba/gs/cta batch several cmd_cfg_get_* requests in one
                // write).  Continue the loop instead of dropping the rest.
                continue;
            }
            break;
        }
        break;
    }
}

} // namespace

int startGameService(int port) {
    ensureWsaGame();
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return 0;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<u_short>(port));
    if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        closesocket(s);
        return 0;
    }
    if (listen(s, 2104) == SOCKET_ERROR) {
        closesocket(s);
        return 0;
    }
    std::thread([s, port]() {
        std::cout << "Service started on port " << port << "\n";
        while (true) {
            SOCKET c = accept(s, nullptr, nullptr);
            if (c == INVALID_SOCKET) {
                if (WSAGetLastError() == WSAEINTR) continue;
                break;
            }
            std::thread([c]() {
                int clientNo = nextGameClientNo().fetch_add(1);
                if (std::getenv("LPCVM_TRACE"))
                    std::cerr << "[game] accepted client no=" << clientNo << "\n";
                GameClient cl;
                cl.fd = static_cast<int>(c);
                cl.clientNo = clientNo;
                cl.attrib = ATTRIB_CLIENT_USER;  // CLIENT_USER initially
                cl.status = 1;
                cl.timeStamp = time(nullptr);
                cl.timeRecvEcho = cl.timeStamp;
                {
                    std::lock_guard<std::mutex> lk(gameMutex());
                    gameClients()[clientNo] = std::move(cl);
                }
                // vm_gameListenSocketCallback: after binding the client slot,
                // notify the LPC master: vm_apply("cmd_new_connection", clientNo)
                // -> cmd_new_connection -> threadd dispatch -> post_new_connection
                // (creates/registers the client agent).
                try {
                    callMasterProgramFunction("cmd_new_connection", {make_int(clientNo)},
                                              lpc::masterSearchRoot());
                } catch (...) {
                }
                // The original driver is a cooperative single-threaded
                // scheduler: the new-connection callback runs to completion
                // (agent cloned + set_client_agent registered) BEFORE the
                // socket is read.  Our port runs the callback on the
                // connectiond queue thread, so wait here until the agent for
                // this client exists before feeding any packet - otherwise the
                // first packet's process_command -> find_agent_by_client races
                // the registration and the command is silently dropped.
                {
                    auto root = lpc::masterSearchRoot();
                    for (int tries = 0; tries < 200; ++tries) {
                        Value agent = callMasterProgramFunction("find_agent_by_client",
                                                               {make_int(clientNo)}, root);
                        if (agent.index() == 7) break;   // object registered
                        if (std::getenv("LPCVM_TRACE"))
                            std::cerr << "[game] waiting agent client=" << clientNo
                                      << " try=" << tries << "\n";
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    }
                }
                {
                    auto root = lpc::masterSearchRoot();
                    Value agent = callMasterProgramFunction("find_agent_by_client",
                                                           {make_int(clientNo)}, root);
                    std::string peer = "?";
                    sockaddr_in sin{};
                    int slen = static_cast<int>(sizeof(sin));
                    if (getpeername(c, reinterpret_cast<sockaddr*>(&sin), &slen) == 0) {
                        char pbuf[64];
                        snprintf(pbuf, sizeof(pbuf), "%s:%d", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
                        peer = pbuf;
                    }
                    std::cerr << "[accept] client=" << clientNo << " agent-type="
                              << static_cast<int>(agent.index()) << " peer=" << peer << "\n";
                }
                char buf[4096];
                for (;;) {
                    int n = recv(c, buf, sizeof(buf), 0);
                    {
                        std::string hex;
                        static const char* hx = "0123456789ABCDEF";
                        for (int i = 0; i < n && i < 48; ++i) { hex += hx[(unsigned char)buf[i] >> 4]; hex += hx[(unsigned char)buf[i] & 15]; }
                        std::cerr << "[recv-accept] client=" << clientNo << " n=" << n << " " << hex << "\n";
                    }
                    if (n <= 0) break;
                    GameClient* cur;
                    {
                        std::lock_guard<std::mutex> lk(gameMutex());
                        auto it = gameClients().find(clientNo);
                        if (it == gameClients().end()) break;
                        cur = &it->second;
                    }
                    // The original driver is cooperative: a dispatched service
                    // (e.g. _check_client_data) runs to completion - including
                    // its lite_set_temp("antibot_answer") AFTER the reply - before
                    // the client's NEXT packet is read.  Wait here until the
                    // threadd service queue drains so the account packet doesn't
                    // race that tail and see antibot_answer still unset.
                    if (cur->attrib & ATTRIB_CLIENT_USER) {
                        for (int tries = 0; tries < 200 && !lpc::serviceQueueIdle(); ++tries)
                            std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    }
                    feedClient(*cur, buf, static_cast<size_t>(n));
                }
                closesocket(c);
                {
                    std::lock_guard<std::mutex> lk(gameMutex());
                    gameClients().erase(clientNo);
                }
            }).detach();
        }
        closesocket(s);
    }).detach();
    return 1;
}

void gameServiceFeed(int clientNo, const char* data, size_t len) {
    std::lock_guard<std::mutex> lk(gameMutex());
    auto it = gameClients().find(clientNo);
    if (it == gameClients().end()) return;
    feedClient(it->second, data, len);
}

void gameServiceDisconnect(int clientNo) {
    std::lock_guard<std::mutex> lk(gameMutex());
    auto it = gameClients().find(clientNo);
    if (it == gameClients().end()) return;
    if (it->second.fd >= 0) closesocket(static_cast<SOCKET>(it->second.fd));
    gameClients().erase(it);
}

void gameServiceSetAttrib(int clientNo, uint32_t attribOrMask, int maxBodySize, int peekMsgNum) {
    std::lock_guard<std::mutex> lk(gameMutex());
    auto it = gameClients().find(clientNo);
    if (it == gameClients().end()) return;
    it->second.attrib |= attribOrMask;
    if (maxBodySize > 0) it->second.maxBodySize = static_cast<uint32_t>(maxBodySize);
    it->second.peekMsgNum = peekMsgNum;
}

bool gameServiceSend(int clientNo, const std::string& data) {
    // vm_sendFullPacket: write the complete wire frame to the client socket.
    // A partial write counts as failure (vm_writeDataToSocket returning -3).
    int fd = -1;
    {
        std::lock_guard<std::mutex> lk(gameMutex());
        auto it = gameClients().find(clientNo);
        if (it == gameClients().end()) { if (std::getenv("LPCVM_TRACE")) std::cerr << "[send] cno=" << clientNo << " not-found\n"; return false; }
        fd = it->second.fd;
    }
    if (fd < 0) { if (std::getenv("LPCVM_TRACE")) std::cerr << "[send] cno=" << clientNo << " bad-fd\n"; return false; }
    int sent = 0;
    while (sent < static_cast<int>(data.size())) {
        int n = send(static_cast<SOCKET>(fd), data.data() + sent,
                     static_cast<int>(data.size()) - sent, 0);
        if (n <= 0) {
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK || err == WSAEINTR) {   // non-blocking socket
                Sleep(1);
                continue;
            }
            if (std::getenv("LPCVM_TRACE")) std::cerr << "[send] cno=" << clientNo << " fd=" << fd << " fail n=" << n << " err=" << err << "\n";
            return false;
        }
        sent += n;
    }
    if (std::getenv("LPCVM_TRACE")) std::cerr << "[send] cno=" << clientNo << " fd=" << fd << " sent=" << sent << "\n";
    return true;
}

int gameServiceBindClient(int fd, uint32_t clientType) {
    // vm_bindToClient (0x804eccc): register a connected socket as a game
    // client and start its receive loop.  The original driver adds the socket
    // to its global scanner; we spawn a dedicated recv thread per connection.
    int clientNo = nextGameClientNo().fetch_add(1);
    {
        sockaddr_in sin{};
        int slen = static_cast<int>(sizeof(sin));
        std::string peer = "?";
        std::string local = "?";
        if (getpeername(static_cast<SOCKET>(fd), reinterpret_cast<sockaddr*>(&sin), &slen) == 0) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%s:%d", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
            peer = buf;
        }
        sockaddr_in ls{};
        int llen = static_cast<int>(sizeof(ls));
        if (getsockname(static_cast<SOCKET>(fd), reinterpret_cast<sockaddr*>(&ls), &llen) == 0) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%s:%d", inet_ntoa(ls.sin_addr), ntohs(ls.sin_port));
            local = buf;
        }
        std::cerr << "[bind] fd=" << fd << " cno=" << clientNo << " type=" << clientType
                  << " peer=" << peer << " local=" << local << "\n";
    }
    {
        GameClient cl;
        cl.fd = fd;
        cl.clientNo = clientNo;
        cl.attrib = clientType;
        cl.status = 1;
        cl.timeStamp = time(nullptr);
        cl.timeRecvEcho = cl.timeStamp;
        std::lock_guard<std::mutex> lk(gameMutex());
        gameClients()[clientNo] = std::move(cl);
    }
    std::thread([fd, clientNo]() {
        char buf[4096];
        int64_t thrRecvBytes = 0;
        int64_t thrRecvReads = 0;
        int64_t thrNoData = 0;
        for (;;) {
            int n = recv(static_cast<SOCKET>(fd), buf, sizeof(buf), 0);
            if (n == SOCKET_ERROR) {
                int err = WSAGetLastError();
                // Non-blocking socket (created by xdbcConnectAsync / FIONBIO
                // in the virtual socket layer): no data yet is NOT an error -
                // retry after a short sleep instead of killing the connection.
                if (err == WSAEWOULDBLOCK || err == WSAEINTR) {
                    if (++thrNoData <= 1 || (thrNoData % 5000) == 0)
                        std::cerr << "[recv-thr] cno=" << clientNo << " fd=" << fd
                                  << " no-data spins=" << thrNoData << "\n";
                    Sleep(1);
                    continue;
                }
                std::cerr << "[recv-thr] cno=" << clientNo << " fd=" << fd
                          << " EXIT recv-err err=" << err
                          << " reads=" << thrRecvReads << " bytes=" << thrRecvBytes << "\n";
                break;
            }
            if (n == 0) {
                std::cerr << "[recv-thr] cno=" << clientNo << " fd=" << fd
                          << " EXIT closed-by-peer"
                          << " reads=" << thrRecvReads << " bytes=" << thrRecvBytes << "\n";
                break;
            }
            ++thrRecvReads;
            thrRecvBytes += n;
            if (thrRecvReads <= 50 || (thrRecvReads % 200) == 0)
                std::cerr << "[recv-thr] cno=" << clientNo << " fd=" << fd
                          << " read n=" << n << " reads=" << thrRecvReads
                          << " bytes=" << thrRecvBytes << "\n";
            GameClient* cur;
            {
                std::lock_guard<std::mutex> lk(gameMutex());
                auto it = gameClients().find(clientNo);
                if (it == gameClients().end()) break;
                cur = &it->second;
            }
            // Preserve the cooperative scheduler's reply-then-cleanup ordering
            // (see the accept loop for details): only the CLIENT_USER path
            // (account login) depends on the threadd service queue draining
            // before the next packet is read.  Internal comm connections
            // (aaa/dba/gs) dispatch replies synchronously into their target
            // objects (wait_event/raise_event); blocking here on a leaked
            // pending-service counter starved the socket - ccs's dba reply
            // (0x4401) was never fed, so filed's do_operate waited forever.
            if (cur->attrib & ATTRIB_CLIENT_USER) {
                for (int tries = 0; tries < 200 && !lpc::serviceQueueIdle(); ++tries)
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            feedClient(*cur, buf, static_cast<size_t>(n));
        }
        closesocket(static_cast<SOCKET>(fd));
        {
            std::lock_guard<std::mutex> lk(gameMutex());
            gameClients().erase(clientNo);
        }
    }).detach();
    return clientNo;
}

Value gameGetClientInfo(int clientNo) {
    // vm_f_get_client_info (0x804d478): mapping with id/client_no/alive/peek/
    // time_stamp/.../local_address/local_port/remote_address/remote_port/
    // remain_send_bytes; empty mapping when the client slot does not exist.
    auto mp = std::make_shared<MappingValue>();
    GameClient cl;
    bool found = false;
    {
        std::lock_guard<std::mutex> lk(gameMutex());
        auto it = gameClients().find(clientNo);
        if (it != gameClients().end()) { cl = it->second; found = true; }
    }
    if (!found) return mp;
    auto add = [&](const char* k, const Value& v) { mp->insert(make_string(k), v); };
    add("id", make_int(cl.id));
    add("client_no", make_int(cl.clientNo));
    add("alive", make_int(cl.fd > 0 ? 1 : 0));
    add("peek", make_int(cl.peekMsgNum));
    add("time_stamp", make_int(cl.timeStamp));
    add("time_receive_echo", make_int(cl.timeRecvEcho));
    add("total_receive", make_int(static_cast<int64_t>(cl.recvPkts)));
    add("failed_count", make_int(static_cast<int64_t>(cl.recvBadPkts)));
    add("debug_enabled", make_int(0));
    add("mobile_client", make_int(cl.mobileClient ? 1 : 0));
    if (cl.fd > 0) {
        sockaddr_in sin{};
        int slen = static_cast<int>(sizeof(sin));
        if (getpeername(static_cast<SOCKET>(cl.fd), reinterpret_cast<sockaddr*>(&sin), &slen) == 0) {
            add("remote_address", make_string(inet_ntoa(sin.sin_addr)));
            add("remote_port", make_int(ntohs(sin.sin_port)));
        }
    }
    return mp;
}

void gameServiceDispatchPacket(GameClient& cl) {
    (void)cl;
}

std::vector<int> gameGetAllClientNo() {
    std::vector<int> out;
    std::lock_guard<std::mutex> lk(gameMutex());
    for (auto& kv : gameClients()) out.push_back(kv.first);
    return out;
}

} // namespace lpc
