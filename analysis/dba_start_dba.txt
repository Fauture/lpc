/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: start_dba.o
 * Source: /util/global_init.c
 * Inherits: (none)
 * Constants: 3912 (HEX:2110 INT:421 STR:1160 CODE:204 REF:17)
 * Functions: 416 names, 416 code segments found
 * Strings: 820, Variables: 98
 *********************************************************************/

/* ================================================================
   CONSTANT DEFINITIONS (#define)
   Total: 3912 (HEX:2110 INT:421 STR:1160 CODE:204 REF:17)
   ================================================================ */


// --- Protocol IDs / Hex Constants (2110) ---
#define SCROLL_PARTY_DAILYCHALLENGE                            ##1004729
#define SCROLL_XIANREN_ZHILU                                   ##1021966
#define SCROLL_LUCKY_SIGN                                      ##1048251
#define SCROLL_XINGYUNDASHI                                    ##1048263
#define RECHARGE_RETURN_COUPON_TYPE                            ##1084540
#define SCROLL_CHONGWUQIANGHUA                                 ##956350
#define SCROLL_XIANMOLU                                        ##956351
#define SCROLL_YAOMODAO                                        ##956352
#define SCROLL_LINGSHUANG                                      ##956353
#define SCROLL_MENPAIBIWU                                      ##956354
#define SCROLL_SHIDAODAHUI                                     ##956355
#define SCROLL_LUOBOTAOZI                                      ##956356
#define SCROLL_JIERIJIANGLI                                    ##956357
#define SCROLL_YABIAO                                          ##956358
#define SCROLL_TIANJIANGDAREN                                  ##956360
#define SCROLL_FUBEN                                           ##956361
#define SCROLL_XIANGYAO                                        ##956362
#define SCROLL_CHUBAO                                          ##956363
#define SCROLL_BANGPAI                                         ##956364
#define SCROLL_SHOUHUSHEN                                      ##956365
#define SCROLL_RICHANGRENWU                                    ##956366
#define SCROLL_DATI                                            ##956367
#define SCROLL_TIANGANGDISHA                                   ##956368
#define SCROLL_JIARUBANGPAI                                    ##956369
#define SCROLL_JIASUO                                          ##956370
#define SCROLL_BANGDING                                        ##956371
#define SCROLL_LIANDAN                                         ##956372
#define SCROLL_QIECUO                                          ##956373
#define SCROLL_QIANZHUANG                                      ##956374
#define SCROLL_BAITAN                                          ##956375
#define SCROLL_ZHUOCHONG                                       ##956376
#define SCROLL_CAIGOU                                          ##956377
#define SCROLL_JIEBAI                                          ##956378
#define SCROLL_JIEHUN                                          ##956379
#define SCROLL_BAISHI                                          ##956380
#define MENU_AUTO_COMBAT                                       0x00000001
#define ANIMATION_LOCATION_HEAD                                0x00000001
#define MENU_MAGIC                                             0x00000002
#define ANIMATION_LOCATION_FLOOR                               0x00000002
#define ANIMATION_LOCATION_FOOT                                0x00000004
#define MENU_ITEM                                              0x00000004
#define ANIMATION_LOCATION_ABOVE_ALL_CHAR                      0x00000008
#define MENU_DEFENSE                                           0x00000008
#define MENU_SELECT_PET                                        0x00000010
#define ANIMATION_LOCATION_FULL_SCREEN                         0x00000010
#define ANIMATION_LOCATION_ABOVE_CHAR                          0x00000020
#define MENU_CALLBACK_PET                                      0x00000020
#define ANIMATION_LOCATION_BELOW_CHAR                          0x00000040
#define MENU_PROTECT                                           0x00000040
#define MENU_CATCH_PET                                         0x00000080
#define MENU_FLEE                                              0x00000100
#define MENU_CALLBACK_CHILD                                    0x00000200
#define PROP_SYS_VENDUE                                        0x0001
#define SEPARATE_PATCH_DATA                                    0x0001
#define PROP_SYS_PURCHASE                                      0x0002
#define SEPARATE_CARRY_DATA                                    0x0002
#define CMD_QUERY_UPGRADE_RATE                                 0x0004
#define PROP_SYS_COIN_TRADE                                    0x0004
#define SEPARATE_UPGRADE_DATA                                  0x0004
#define MSG_UPDATE_TEAM_LIST                                   0x0006
#define PROP_SYS_STALL_EX                                      0x0008
#define CMD_C_SELECT_MENU_ITEM                                 0x000A
#define CMD_RETURN_TEAM                                        0x000C
#define CMD_REQUEST_CITY_BUILDING_INFO                         0x000E
#define PROP_SYS_SHOP                                          0x0010
#define CMD_ADMIN_BLOCK_ACCOUNT                                0x0016
#define CMD_CLONE_SPECIAL_EQUIP                                0x001A
#define CMD_RECIVE_INSTRUCTION_TASK                            0x001C
#define MSG_LIUJIN_SUIYUE                                      0x001D
#define CMD_RACE_FEED_NPC                                      0x001E
#define PROP_SYS_GROUP_BUY                                     0x0020
#define CMD_DIVIDE_IN_STORE                                    0x0022
#define CMD_CSC_ADD_SIGN_FEE                                   0x0024
#define CMD_PARTY_MODIFY_ANNOUNCE                              0x0026
#define CMD_INTERSVR_RPC_CALL                                  0x003A
#define CMD_UPGRADE_EX                                         0x003C
#define PROP_SYS_VENDUE_EX                                     0x0040
#define MSG_MASTER_SKILLS                                      0x0053
#define CMD_COMMON_GUESS_DRAW_BONUS                            0x0054
#define MSG_CD_WISHING_RATE                                    0x0055
#define CMD_SELECT_CURRENT_MOUNT                               0x0056
#define MSG_TASK_SCORE_EXCHANGE_LIST                           0x0057
#define CMD_REQUEST_OPEN_CARD_PACKAGE                          0x0058
#define MSG_LC_END_ACTION                                      0x0059
#define CMD_ADMIN_TELEPORT                                     0x005A
#define MSG_CC_DESTRUCT_HAND_CARDS                             0x005B
#define MSG_QUERY_PRACTICE_CHILDREN                            0x005F
#define CMD_CLONE_UPGRADE_STONE                                0x0061
#define MSG_PARTY_ICON                                         0x0092
#define CMD_SWITCH_GROUP                                       0x009E
#define CMD_GET_PHONE_AUTH_BIND_TIME                           0x00B2
#define CMD_COIN_REFRESH_MALL                                  0x00C4
#define MSG_OPEN_LOTTERY_TICKET                                0x00C5
#define MSG_CC_NEW_ROUND                                       0x00C7
#define CMD_PARTY_MEMBERS                                      0x00CC
#define MSG_REQUEST_ITEM_INFO                                  0x00D8
#define CMD_REQUEST_SERVER_STATUS                              0x00DE
#define MSG_FISHING_CHANGE_WEAPON                              0x00EB
#define CMD_OPEN_ASKTAO_SHOW_GIFT_BOX                          0x00EC
#define MSG_SAMSUNG_OPER                                       0x00F3
#define CMD_OPER_XIAOBAO_SONGSHENBING                          0x00F4
#define CMD_ACCEPT                                             0x00F6
#define CMD_AUTH_VERIFY_CODE                                   0x00F8
#define CMD_ADMIN_QUERY_PLAYER                                 0x00FA
#define ANTI_CHEATER_LIMIT_OPER                                0x01
#define BONUS_PET_ATTRIB_ENCHANT                               0x01
#define ANTI_CHEATER_LIMIT_CASH                                0x02
#define BONUS_PET_ATTRIB_ECLOSION                              0x02
#define MSG_VENDUE_REFRESH_PETS                                0x0203
#define CMD_START_CHILD_COMPETE                                0x0206
#define CMD_DO_INTERNAL_COMM                                   0x0300
#define MSG_EVERYONE_PK_OPER                                   0x0329
#define MSG_TELEPORT_LIST                                      0x0331
#define MSG_SPA_BUSY                                           0x0604
#define MSG_CARD_COMPETE_SELECT_CARD                           0x09C9
#define MSG_STORE                                              0x09FD
#define MSG_CONTAINER_INVALID_RANGE                            0x09FF
#define CMD_TRADE_GET_OBJ_PROP                                 0x0A04
#define CMD_GROUP_CHANNEL                                      0x0A0A
#define MSG_C_PET_ANGER_DELTA                                  0x0A0B
#define CMD_ADD_AWARD_TIMES                                    0x0A0C
#define MSG_C_CHAR_OFFLINE                                     0x0A0D
#define MSG_TEMP_NPC_EVENT                                     0x0A0F
#define CMD_START_CARD_FRIEND_PK                               0x0A10
#define MSG_CD_WISHING_GOODS                                   0x0A11
#define CMD_ARENA_GOODS_SELL                                   0x0A12
#define MSG_PK_MATCH_SCORE                                     0x0B01
#define MSG_AUTH_VIP_SMS                                       0x0B03
#define CMD_L_GET_ANTIBOT_QUESTION                             0x0B05
#define MSG_UPDATE_CARD_INFO                                   0x0B07
#define MSG_LC_CATCH_PET                                       0x0B09
#define MSG_C_CALLBACK_CHILD                                   0x0B0B
#define CMD_ASK_PILL                                           0x0B0C
#define MSG_MESSAGE                                            0x0B0D
#define MSG_JINGBING_JIANYUE                                   0x0B0F
#define MSG_SCZL2016_INTERFACE                                 0x0B11
#define MSG_OPEN_ONLINEMALL_REBATE                             0x0B13
#define MSG_ONLINEMALL_REBATE_BONUS                            0x0B15
#define MSG_PET_XINFA_EXP_CARD                                 0x0B17
#define MSG_APPLY_HUFENG_HUANYUFU                              0x0B19
#define MSG_MAP_WEATHER                                        0x0B1B
#define MSG_TRADE_SUIT_PROP                                    0x0B1D
#define MSG_REFRESH_EXQUISITE                                  0x0B21
#define CMD_APPLY_EXQUISITE                                    0x0B22
#define MSG_VIEW_CS_MATCHES_SUMMARY                            0x0DC5
#define MSG_STALL_NAME                                         0x0DFD
#define MSG_SPECIFIED_COMBAT_DECK_LIST                         0x0DFF
#define MSG_TRADE_STORE_PET_RECORD                             0x0FB7
#define MSG_CLONE_SPECIAL_EQUIP                                0x0FB9
#define MSG_DISTRIBUTE_RESULT                                  0x0FEF
#define MSG_STRATEGY_WAR_VS_INFO                               0x1001
#define MSG_LC_ACCEPT_MULTI_HIT                                0x1003
#define MSG_SAFE_LOGIN_TIME                                    0x1005
#define CMD_LOCK_ACCOUNT                                       0x100E
#define CMD_QUIT_TEAM                                          0x1010
#define CMD_SET_ATTENTION                                      0x1012
#define MSG_CHILD_INTIMACY                                     0x1013
#define CMD_WAR_VIEW_CURRENT_INFO                              0x1014
#define MSG_CC_CAST_SKILL                                      0x1017
#define CMD_MUL_ASSIGN_ATTRIB                                  0x1018
#define MSG_DUNGEON_TASK                                       0x1019
#define CMD_CHAT_GROUP_CHANGE_SETTING                          0x101A
#define MSG_UPDATE_CHILD_TEAM_LIST                             0x101B
#define CMD_REQUEST_JOIN                                       0x1020
#define CMD_START_CARD_DUEL                                    0x1024
#define CMD_ENTER_ROOM_0                                       0x1026
#define CMD_COMMON_GUESS_INFO                                  0x1028
#define CMD_DECORATE_MAKE                                      0x1036
#define MSG_ENVELOPE_SENDED                                    0x103B
#define CMD_QUIT_LOOKON_COMBAT                                 0x103C
#define MSG_RELOCATE                                           0x103D
#define CMD_SELECT_MENU_ITEM_0                                 0x1042
#define MSG_CCS_FRIEND_STATUS                                  0x1043
#define CMD_REMOVE_FRIEND                                      0x1044
#define MSG_ECARD_LIST_TRADE                                   0x1045
#define MSG_CLIENT_TASK_TIME                                   0x1047
#define CMD_LEAVE_TEMP_TEAM                                    0x104A
#define MSG_C_UPDATE_STATUS_INFO                               0x104D
#define CMD_SELECT_CURRENT_CHILD                               0x1052
#define MSG_GET_CLIENT_BEHAVIOR                                0x1053
#define CMD_TRADE_OPEN_URL                                     0x1054
#define MSG_GUESS_START                                        0x1057
#define CMD_CARD_COMPETE_SELECT_CARD                           0x105A
#define CMD_LOAD_EXISTED_CHAR                                  0x1060
#define MSG_LC_FLEE                                            0x1063
#define MSG_CASUAL_REFRESH_ROOM_LIST                           0x1065
#define CMD_TURN_DIR                                           0x1070
#define CMD_CHAT_GROUP_SEARCH                                  0x1072
#define MSG_C_ADD_FRIEND                                       0x1075
#define MSG_LOOK_PLAYER                                        0x1077
#define MSG_GATHER                                             0x1079
#define CMD_REQUEST_REFRESH_CITY_TASK                          0x107A
#define CMD_SHOP_ENTER_SHOP_TYPE                               0x107B
#define MSG_UPDATE_FRIEND_LIST_EX                              0x1081
#define MSG_REFRESH_TODAY_STAT                                 0x1083
#define CMD_REPORT_CHEATER                                     0x1084
#define MSG_SHENGSIDOU                                         0x1085
#define CMD_ARENA_ROOM                                         0x1086
#define CMD_SHOW_CHILDREN_INFO                                 0x1088
#define CMD_TRADE_GET_OBJ                                      0x108A
#define MSG_MAGIC_DLG_OPEN                                     0x108E
#define CMD_REMOVE_ASKTAO_SHOW_GIFT                            0x1090
#define CMD_ADMIN_THROW_IN_JAIL                                0x1096
#define CMD_CARE_ITEM                                          0x1098
#define CMD_CSA_COMMON_OPER                                    0x109A
#define CMD_FRIEND_CHILDREN_LIST                               0x109C
#define CMD_CCS_SHUTDOWN_BY_SPA                                0x10A0
#define MSG_MA_VOICE_MSG_SERVER                                0x10A1
#define CMD_REQUEST_DECOMPOSE_CARD                             0x10AA
#define CMD_GET_SAFE_QUESTION                                  0x10AC
#define CMD_START_AUTOMATISM_STALL                             0x10AE
#define MSG_TEAM_SEEK_LIST                                     0x10AF
#define MSG_GUARDS_DROP                                        0x10B0
#define MSG_UPDATE_CARD_HERO                                   0x10B1
#define CMD_WUDAO_ADD_ATTRIB_AUTO                              0x10B2
#define CMD_SHOP_VIEW_SALE_LOG                                 0x10B3
#define MSG_VIEW_CONVENANT                                     0x10B5
#define CMD_ARENA_ROOM_CHANGE_SEAT                             0x10B6
#define MSG_REQUEST_MA_OK                                      0x10B7
#define MSG_C_SET_CUSTOM_MSG                                   0x10B9
#define MSG_REFRESH_HELL_INSTRUCTION                           0x10BB
#define CMD_VENDUE_VENDUE_MERCH                                0x10BD
#define CMD_ANSWER_RESULT                                      0x10BE
#define MSG_DISAPPEAR                                          0x10BF
#define CMD_START_STALL                                        0x10C0
#define MSG_TRADE_TIME                                         0x10C1
#define CMD_GET_PET_EVOLVE_LIST                                0x10C2
#define MSG_ALL_MUSIC_INFO                                     0x10C3
#define MSG_C_QUIT_COMBAT                                      0x10C5
#define MSG_SHOW_UPGRADE_EQUIP_ITEMS                           0x10C7
#define CMD_RESEND_VIP_SMS                                     0x10C8
#define MSG_START_FISHING_GAME                                 0x10C9
#define MSG_CHAR_PRACTICE                                      0x10CB
#define MSG_ARENA_HALL_NAME                                    0x10CD
#define MSG_WEDDING_SET_PACKET                                 0x10CF
#define MSG_OLYMPIC_TOP_INFO                                   0x10D1
#define CMD_SET_PENNANT_SKILL                                  0x10D2
#define MSG_OPEN_SPECIAL_DLG                                   0x10D3
#define CMD_SWITCH_SERVER                                      0x10D4
#define MSG_LC_MENU_LIST                                       0x10D5
#define CMD_APPLY_OFFLINE_STALL                                0x10D6
#define MSG_FISHING_FISH_DISAPPEAR                             0x10D7
#define MSG_GS_FINGER                                          0x10D9
#define CMD_CARD_COMPETE_SELECT_DECK                           0x10DA
#define MSG_CHILD_ENTER_HALL                                   0x10DB
#define CMD_OPER_LONGTENG_SHENGSHI                             0x10DC
#define MSG_C_CHAR_DIED                                        0x10DD
#define CMD_CHARGE_VENDUE_STORE_CLOSED                         0x10DF
#define MSG_ENTER_GAME                                         0x10E1
#define MSG_COMMAND_RESULT                                     0x10E3
#define MSG_GET_SCHEDULE                                       0x10E5
#define MSG_HOUSE                                              0x10E7
#define MSG_REFRESH_MAGIC_STONE_INFO                           0x10E9
#define CMD_PERSENT_ASKTAO_SHOW                                0x10EA
#define MSG_WUDAO_STAGE_INFO                                   0x10EB
#define CMD_VIEW_CS_MATCH_DETAIL                               0x10EC
#define MSG_FISHING_ADD_MULTI_CARD                             0x10ED
#define MSG_UPGRADE_INFO                                       0x10EF
#define CMD_SET_BIRTHDAY                                       0x10F0
#define MSG_START_CHEATER_DETECT                               0x10F1
#define CMD_EXCHANGE_SELECT                                    0x10F2
#define MSG_C_CALLBACK_PET                                     0x10F3
#define CMD_CHAT_GROUP_LOG                                     0x10F4
#define MSG_BUREAU_LIST                                        0x10F5
#define MSG_REFRESH_OLD_NAME                                   0x10F6
#define MSG_VIEW_FIXTURES                                      0x10F7
#define CMD_GUARDS_SET_TACTICS                                 0x10F9
#define MSG_CITY_INFO                                          0x10FB
#define MSG_LOCAL_ENCRYPT_INFO                                 0x1101
#define CMD_CHANGE_TITLE_EFFECT                                0x1102
#define CMD_C_DO_ACTION_2                                      0x1104
#define CMD_SORT_PACK                                          0x1106
#define CMD_C_FLEE                                             0x1108
#define CMD_SEND_FRIEND_CARD                                   0x110A
#define CMD_GET_SYS_ENVELOPE                                   0x110C
#define CMD_ANSWER_SURVEY                                      0x110E
#define MSG_CELEBRITY_OPER                                     0x110F
#define MSG_REFRESH_SEARCH_STALL_RESULT                        0x1110
#define MSG_EXCHANGE_CONFIRM                                   0x1111
#define CMD_UNEQUIP_INBORN_STONE                               0x1112
#define MSG_ONLINE_MALL_REBATE                                 0x1113
#define CMD_CASUAL_GAME_DO_ACTION                              0x1114
#define CMD_REPLY_REQUEST_ASKTAO_SHOW                          0x1115
#define MSG_ARENA_ROOM_BTN_STAT                                0x1117
#define CMD_GET_CLIENT_ACTION                                  0x1119
#define CMD_BOX_ADVENTRUE_OPEN_BOX                             0x111A
#define MSG_C_ADD_CHILD                                        0x111B
#define CMD_DIG_PARTY_FRAGMENT                                 0x111C
#define CMD_REQUEST_CARD_GAME_SORT_LIST                        0x111E
#define MSG_ARENA_ROOM_SEAT_INFO                               0x1121
#define MSG_LC_ACTION                                          0x1123
#define CMD_FISHING_AUTO_BUY_WEAPON                            0x1124
#define MSG_X_LOGIN_DONE                                       0x1125
#define CMD_ARENA_HALL_EXIT                                    0x1126
#define MSG_MAP_FRAGMENT                                       0x1127
#define MSG_REPLACE_LOGIN                                      0x1129
#define CMD_KILL_EX                                            0x112D
#define MSG_BAOTU_TIP                                          0x112F
#define CMD_SET_CARD_TOPMOST                                   0x1130
#define MSG_PARTY_INFO                                         0x1131
#define CMD_RESET_APPELLATION_TASK_DUE                         0x1132
#define MSG_CITY_BUILDING_INFO_COMPETE                         0x1133
#define CMD_CHAT_GROUP_REMOVE_MEMBER                           0x1134
#define MSG_FISHING_START                                      0x1135
#define CMD_CLIENT_BEHAVIOR                                    0x1136
#define MSG_TRADE_CASH_PROP                                    0x1137
#define CMD_C_DO_ACTION_1                                      0x1138
#define MSG_TRADE_EQUIP_PROP                                   0x1139
#define MSG_CLOSE_KIND_STORE                                   0x113A
#define MSG_TRADE_ITEM_PROP                                    0x113B
#define CMD_ARENA_GOOD_LIST                                    0x113C
#define MSG_TRADE_PET_PROP                                     0x113D
#define CMD_CLOSE_BUREAU                                       0x113E
#define MSG_TARDE_ROLE_PROP                                    0x113F
#define CMD_DIVIDE                                             0x1140
#define MSG_UPDATE_APPELLATION_TASK                            0x1141
#define CMD_REQUEST_SHILING                                    0x1142
#define MSG_REQUEST_RECHARGE                                   0x1143
#define CMD_TAKE_STALL_CASH                                    0x1144
#define CMD_DRAFT_MODIFY_MY_INFO                               0x1145
#define CMD_REFRESH_FRIEND                                     0x1146
#define MSG_TRADE_ROLE_EQUIP_PROP                              0x1147
#define CMD_REQUEST_INFO                                       0x1148
#define MSG_TARDE_ROLE_ITEM_PROP                               0x1149
#define MSG_TRADE_ROLE_PET_PROP                                0x114B
#define MSG_TRADE_ROLE_UPGRADE_PROP                            0x114D
#define MSG_TRADE_ROLE_GUARD_PROP                              0x114F
#define MSG_CHILD_START_FIGHT                                  0x1151
#define MSG_TRADE_ROLE_UPGRADE_EQUIP                           0x1153
#define CMD_SWITCH_ITEM                                        0x1154
#define MSG_ENTER_ROOM                                         0x1155
#define CMD_CAST                                               0x1156
#define MSG_TRADE_ROLE_CHILD_PROP                              0x1157
#define CMD_TRADE_BUY_SERVICE                                  0x1158
#define MSG_SET_CURRENT_MOUNT                                  0x1159
#define CMD_CELEBRITY_OPER                                     0x115A
#define MSG_C_ACCEPT_HIT                                       0x115B
#define CMD_SET_FIREWORKS_RELEASE_TIME                         0x115C
#define MSG_SPECIAL_MOVE_EVENT                                 0x115D
#define CMD_MANAGE_HOUSE                                       0x115E
#define MSG_ADMIN_QUERY_MAP                                    0x115F
#define CMD_WORKSHOP_RELEASE_ORDER                             0x1160
#define MSG_ADMIN_GLOBAL_BONUS_INFO                            0x1161
#define CMD_OPER_TELEPORT_ITEM                                 0x1162
#define MSG_VIEW_EC_GUESS                                      0x1163
#define CMD_VERIFY_CHAR_NAME                                   0x1164
#define MSG_VIEW_CS_FIXTURES                                   0x1165
#define CMD_BUY_FROM_ONLINE_MALL                               0x1166
#define MSG_FISHING_AUTO_BUY_WEAPON                            0x1167
#define CMD_FISH_SWALLOW_DIE                                   0x1168
#define MSG_FISHING_OPEN_REMOTE_STORE                          0x1169
#define CMD_FEED_PET_XINFA_EXP                                 0x116A
#define MSG_ARENA_ROOM_INFO                                    0x116B
#define MSG_ADMIN_QUERY_GID                                    0x116D
#define MSG_CHAT_GROUP_SEARCH                                  0x116F
#define CMD_ASKTAO_SOUND                                       0x1171
#define MSG_NOTICE_APPELLATION_COLOR                           0x1175
#define MSG_JIULI_XIANGYAO                                     0x1177
#define MSG_YAOYAOLE_OPEN                                      0x1179
#define MSG_SORT_TASK_AWARD_INFO                               0x117B
#define MSG_TIME_LUCK_INFO                                     0x117D
#define MSG_COMPACT_TIME_LUCK_INFO                             0x117F
#define MSG_ZODIAC_PLATE_INFO                                  0x1181
#define MSG_TRADE_STORE_PET_INFO                               0x11E1
#define MSG_IDENTIFY_EQUIP                                     0x11E3
#define MSG_CUR_SYSTEM_DATE                                    0x11E5
#define MSG_SEND_NOTICE_CHANNEL                                0x11E7
#define MSG_ADMIN_SET_ATTRIB                                   0x11E9
#define CMD_REPURCHASE                                         0x1202
#define MSG_LC_ADD_FRIEND                                      0x1203
#define CMD_SHOUT                                              0x1204
#define MSG_IS_CONVERT_BOTTON                                  0x1205
#define CMD_DRAFT_REQUEST_MY_INFO                              0x1206
#define MSG_SHOP_ACK_RUNNINGS                                  0x1207
#define CMD_SET_GCSL_ATTRIBS                                   0x1208
#define MSG_REFESH_STALL_LIST                                  0x1209
#define CMD_C_SET_POS                                          0x120C
#define CMD_GET_UPDATE_NOTICE                                  0x120E
#define CMD_QUERY_UPGRADE_COIN                                 0x1212
#define CMD_START_RAISE                                        0x1216
#define CMD_FRIEND_DRINK_REFRESH                               0x1218
#define CMD_REQUEST_MATCH_CARD_GAME                            0x121A
#define MSG_OPEN_PET_PRACTICE                                  0x121B
#define CMD_REPORT_ILLEGAL                                     0x121C
#define MSG_LC_COMMAND_ACCEPTED                                0x121D
#define CMD_GUESS_BONUS                                        0x121E
#define CMD_REMOVE_BUREAU_ITEM                                 0x121F
#define CMD_TOWER_CLICKED                                      0x1220
#define MSG_L_GET_PROTECT_INFO                                 0x1221
#define CMD_L_GET_PROTECT_INFO                                 0x1222
#define CMD_L_SET_PROTECT_BONUS                                0x1224
#define MSG_CC_QUIT_COMBAT                                     0x1225
#define CMD_PUT_DOWN                                           0x1226
#define MSG_SET_TOOL_STATE                                     0x1227
#define CMD_EQUIP                                              0x1228
#define CMD_OPER_ELIXIR                                        0x122A
#define CMD_TXSS_START_AWARD                                   0x122C
#define CMD_TXSS_RESET_BINGO                                   0x122E
#define CMD_REQUEST_CITY_LIST                                  0x1230
#define MSG_CC_UPDATE_STATUS                                   0x1231
#define CMD_C_CHANGE_ZHENFA                                    0x1232
#define MSG_C_SANDGLASS                                        0x1233
#define CMD_REFRESH_SAFE_LOGIN                                 0x123E
#define MSG_PURCHASE_CASH_NUM                                  0x1241
#define MSG_REFRESH_CARD_TODAY_STAT                            0x1243
#define MSG_REQUEST_REPURCHASE                                 0x1245
#define CMD_SHOP_SALE                                          0x1247
#define MSG_GS_LOGIN                                           0x1249
#define MSG_C_ACCEPT_MULTI_HIT                                 0x1251
#define CMD_MOV_FROM_QIANKUNDAI                                0x1252
#define MSG_APPELLATION_EFFECT_DESC                            0x1253
#define CMD_WUDAO_BATCH_ADD_REP                                0x1254
#define MSG_CUSTOMIZE_PACK_LIST                                0x1255
#define CMD_RACE_HIT_NPC                                       0x1256
#define MSG_RAID_MEMBER_UPDATE                                 0x1257
#define CMD_FIGHT                                              0x1258
#define MSG_LIGHT_LANTERN                                      0x1259
#define CMD_DELETE_EXISTED_CHAR                                0x125A
#define MSG_ADMIN_QUERY_SKILL                                  0x125B
#define CMD_RECHARGE                                           0x125C
#define MSG_SHOW_XIAOBAO_SONGSHENBING                          0x125D
#define CMD_ADD_SKILL_ESSENCE                                  0x125E
#define MSG_ENVELOPE_REMOVE                                    0x125F
#define CMD_REQUEST_TOP_TAO_PAGE                               0x1260
#define MSG_CARD_COMPETE_RECORD                                0x1261
#define CMD_CANCEL_AUTH_VIP_SMS                                0x1262
#define MSG_C_OPPONENT_INFO                                    0x1263
#define CMD_REQUEST_ICON                                       0x1265
#define CMD_RAID_LEAVE_RAID                                    0x1347
#define MSG_OPEN_SORT_TASK_INTERFACE_SF                        0x1351
#define CMD_L_TEST_ACC                                         0x1352
#define MSG_L_TEST_ACC                                         0x1353
#define MSG_L_CREATE_ACC                                       0x1354
#define CMD_AAA_SHUTDOWN                                       0x1356
#define CMD_DBA_SHUTDOWN                                       0x1358
#define MSG_L_CHANGE_PASSWORD                                  0x135B
#define MSG_CFG_MY_DIST                                        0x135D
#define MSG_L_CHANGE_PROTECT                                   0x135F
#define CMD_FISHING_QUIT                                       0x1360
#define MSG_FRIEND_REMOVE_CHAR                                 0x1361
#define CMD_RECOVER_ASKTAO_SHOW                                0x1362
#define MSG_L_CHANGE_ACC_INFO                                  0x1363
#define CMD_ADMIN_SET_ATTRIB                                   0x1364
#define MSG_UPDATE_CHILD_ATTRIB                                0x1365
#define CMD_ASK_SKILL_DESC                                     0x1366
#define CMD_SUBMIT_IMAGE                                       0x1367
#define CMD_AGENT_CLOSE_CLIENT                                 0x136A
#define MSG_RESOLVE_IP_DONE                                    0x136C
#define CMD_L_GET_QR_CODE_INFO                                 0x137A
#define MSG_L_AUTH_MATRIX                                      0x137B
#define CMD_L_LOGIN_BY_QR_CODE                                 0x137C
#define MSG_L_GET_QR_CODE_INFO                                 0x137D
#define MSG_L_LOGIN_BY_QR_CODE                                 0x137F
#define CMD_BOTTLE_THROW                                       0x1380
#define MSG_MUD_COMBAT_UPDATE                                  0x1381
#define MSG_SAFE_STATE                                         0x1382
#define MSG_RECHARGE                                           0x1383
#define CMD_REQUEST_DECK_LIST                                  0x1384
#define CMD_GBUYING_FETCH_GOODS                                0x1386
#define CMD_L_LOCK_ACCOUNT                                     0x138A
#define CMD_L_QUERY_ACCOUNT_STATE                              0x138C
#define CMD_L_SET_ACCOUNT_INFO                                 0x138E
#define CMD_L_QUERY_ADDICTION                                  0x1390
#define CMD_GBUYING_INJECT_COIN                                0x1392
#define MSG_MOVED                                              0x1393
#define CMD_L_GET_LOCAL_ENCRYPT_INFO                           0x1394
#define MSG_L_GET_LOCAL_ENCRYPT_INFO                           0x1395
#define CMD_L_SET_LOCAL_ENCRYPT_INFO                           0x1396
#define MSG_L_SET_LOCAL_ENCRYPT_INFO                           0x1397
#define CMD_REQUEST_DELETE_DECK                                0x1398
#define MSG_CC_UPDATE_ONE_CARD                                 0x1399
#define CMD_GET_SAFE_STATE                                     0x13A0
#define MSG_L_LOCAL_ENCRYPT_POS                                0x13A1
#define CMD_L_AUTH_LOCAL_ENCRYPT                               0x13A2
#define MSG_L_GET_ACC_COMP_CHAR                                0x13A3
#define CMD_WEAR_ASKTAO_SHOW                                   0x13A4
#define MSG_C_ACCEPT_MAGIC_HIT                                 0x13A5
#define CMD_L_GET_ACC_COMP_CHAR                                0x13A6
#define MSG_CHAT_GROUP_INVITE_MEMBER                           0x13A7
#define CMD_L_SET_ACC_COMP_CHAR                                0x13A8
#define MSG_AUTO_STEP_MOVE_START                               0x13A9
#define CMD_CHAR_PRACTICE                                      0x13AA
#define CMD_SKILL_LEVEL_DOWN                                   0x13AC
#define CMD_FINISH_STALL                                       0x13AE
#define CMD_L_BORROW_LOCAL_ENCRYPT                             0x13AE
#define CMD_PURCHASE_MULTI_MATCH                               0x13B0
#define MSG_L_BORROW_LOCAL_ENCRYPT                             0x13B1
#define MSG_WANGWEI_ZHENDUO_RANK                               0x1413
#define MSG_OPEN_EXCHANGE                                      0x1415
#define CMD_MOBILE_REQUEST_NPC_LIST                            0x1416
#define MSG_RPC_SERVER_NAME                                    0x1417
#define CMD_SELECT_SPECIFIED_COMBAT_DECK                       0x1418
#define MSG_GET_ACTIVITY_DATES                                 0x1419
#define CMD_SPECIAL_MOVE_EVENT                                 0x141A
#define MSG_C_MENU_LIST_EX                                     0x141B
#define CMD_VIEW_ACHIEVE_OVERVIEW                              0x141C
#define MSG_LEVEL_UP                                           0x1421
#define MSG_VERIFY_MENU_CLOSED                                 0x19D9
#define MSG_TRADE_ASK_FIELDS                                   0x19DF
#define MSG_SIGN_IN_INFO                                       0x19E7
#define MSG_REPORT_ILLEGAL_TOOL                                0x19EB
#define MSG_C_ANGER_DELTA                                      0x19F3
#define CMD_QUERY_RECRUIT_INFO                                 0x1A02
#define MSG_PERMISSION                                         0x1A03
#define CMD_PARTY_BLACK_LIST                                   0x1A04
#define CMD_CATCH_PKER                                         0x1A06
#define CMD_WAR_VIEW_HISTORY_INFO                              0x1A08
#define CMD_CLEAN_REQUEST                                      0x1A0C
#define MSG_SYNC_MESSAGE                                       0x1A29
#define CMD_TEAM_CANCEL_SEEK                                   0x1AE6
#define CMD_SELECT_VISIBLE_PET                                 0x1AEA
#define CMD_GOODS_BUY                                          0x1AEC
#define CMD_JOIN_SYSTEM_GROUP                                  0x1AEE
#define CMD_FINGER                                             0x1AF6
#define CMD_OPER_VISUAL_PACKS                                  0x1AF8
#define CMD_CHILD_USE_COMBAT                                   0x1AFA
#define CMD_ADMIN_QUERY_MAP                                    0x1AFC
#define CMD_WISHING_SALE_ITEM                                  0x1AFE
#define CMD_APEX                                               0x1B01
#define MSG_APEX                                               0x1B02
#define CMD_L_CHECK_USER_DATA                                  0x1B03
#define MSG_L_ANTIBOT_QUESTION                                 0x1B06
#define CMD_L_AUTH_PIC                                         0x1B07
#define MSG_L_AUTH_PIC                                         0x1B08
#define CMD_CITYWAR_OPER                                       0x1B0A
#define CMD_CARD_COMPETE_SELECT_HERO                           0x1B0C
#define CMD_GUARDS_CHEER                                       0x1B0D
#define CMD_WEDDING_SET_LIST                                   0x1B0E
#define MSG_CHECK_BIND_OK                                      0x1B0F
#define CMD_GET_SERVER_TIME                                    0x1C03
#define CMD_OPER_MOOD_INFO                                     0x1C04
#define CMD_LOOK                                               0x1C06
#define MSG_ADMIN_DOUBT_INFO                                   0x1C07
#define CMD_EMOTE                                              0x1C08
#define CMD_SHOP_AD_ENTER                                      0x1C09
#define MSG_ARENA_HALL_ROOM_DETAIL                             0x1DD7
#define MSG_FISH_SWALLOW_RESULT                                0x1DDB
#define MSG_C_OPPONENTS                                        0x1DE5
#define MSG_C_START_SEQUENCE                                   0x1DE7
#define MSG_PLAYER_INFO_LIST                                   0x1DEB
#define MSG_GS_FRIEND_LIST                                     0x1DF3
#define MSG_VENDUE_REFRESH_ITEMS                               0x1DF5
#define MSG_C_FLEE                                             0x1DF7
#define MSG_RAID_MEMBERS                                       0x1E07
#define MSG_CARD_COMPETE_SELECT_DECK                           0x1E09
#define MSG_GS_LOGOUT                                          0x1E15
#define MSG_POPUP_SAFE_TIME                                    0x1E17
#define MSG_ACK_INFO                                           0x1E19
#define MSG_END_MAKE_PILL                                      0x1E1B
#define CMD_CLEAR_BAG                                          0x1FB4
#define MSG_FISHING_MEMBER_INFO                                0x1FB5
#define CMD_PARTY_JOB_PRIVILEGE_LIST                           0x1FB6
#define MSG_CCS_WORLD_CHANNEL                                  0x1FB7
#define CMD_MUL_GOODS_BUY                                      0x1FB8
#define MSG_UPDATE_MOVE_SPEED                                  0x1FB9
#define MSG_MY_CARD_PACKAGES                                   0x1FBB
#define CMD_OPER_DISCOVER_EVENT                                0x1FBC
#define MSG_LC_CHAR_REVIVE                                     0x1FC1
#define MSG_ARENA_SERVER_LIST                                  0x1FC3
#define MSG_ANSWER_SKILL_DESC                                  0x1FC5
#define MSG_ADMIN_QUERY_NET_INFO                               0x1FC7
#define CMD_ARENA_GOOD_BUY                                     0x1FC8
#define CMD_LEARN_COOKING                                      0x1FD0
#define MSG_SHOP_ENTER_SHOP                                    0x1FD1
#define CMD_SPECIAL_DLG                                        0x1FD2
#define MSG_ADMIN_REQUEST_CURRENT_TIME                         0x1FD3
#define CMD_WARCRAFT_REFRESH                                   0x1FD4
#define MSG_SAFE_LOGIN_STATE                                   0x1FD5
#define CMD_REPLY_CLIENT_DOUBT_INFO                            0x1FD6
#define MSG_WEDDING_QUERY_LIST                                 0x1FDD
#define MSG_DIALOG_OK                                          0x1FE5
#define MSG_SHOW_SALE_LOG                                      0x1FFF
#define CMD_REQUEST_OPEN_SYSTEM_DLG                            0x201C
#define MSG_DISAPPEAR_FROM_SMALL_MAP                           0x201E
#define CMD_LAUNCH_CONVENANT                                   0x2020
#define CMD_ARENA_MATCH_FIXTURES                               0x2022
#define MSG_ARENA_MATCH_FIXTURES                               0x2023
#define CMD_ARENA_MATCH_ROOMS                                  0x2024
#define MSG_ARENA_MATCH_ROOMS                                  0x2025
#define CMD_ARENA_MATCH_ROOM_DETAIL                            0x2026
#define MSG_ARENA_MATCH_LIST                                   0x2027
#define CMD_ARENA_MATCH_JOIN_ROOM                              0x2028
#define CMD_ARENA_ROOM_ACTION                                  0x202A
#define CMD_ARENA_HALL_REQUEST_RL                              0x202C
#define CMD_QUERY_PRACTICE_CHILDREN                            0x2030
#define CMD_GET_TODAY_STAT                                     0x2032
#define MSG_CASUAL_GAME_INFO                                   0x2034
#define CMD_REQUEST_ENTER_CITYWAR_ROOM                         0x2036
#define MSG_MENU_LIST                                          0x2037
#define CMD_ARENA_MATCH_LIST                                   0x2038
#define MSG_PARTY_CHANNEL_DENY_LIST                            0x2039
#define MSG_TEAM_INIT_PLATFORM                                 0x203B
#define MSG_C_UPDATE                                           0x203D
#define CMD_FEED_PET                                           0x203E
#define CMD_REQUEST_CARD_GAME_SORT_DECK                        0x2040
#define CMD_BOTTLE_THROW_BACK                                  0x2048
#define CMD_UPGRADE                                            0x204E
#define CMD_OPEN_MENU_1                                        0x2050
#define MSG_COIN_DEAL_ORDER                                    0x205B
#define CMD_CREATE_NEW_CHAR                                    0x205C
#define CMD_MAP_CHILD_LIST                                     0x205E
#define CMD_REFRESH_REQUEST                                    0x2062
#define CMD_OPEN_MENU_0                                        0x2064
#define CMD_DEPOSIT                                            0x2066
#define CMD_PARTY_RELATION                                     0x2068
#define CMD_SELECT_CURRENT_PET                                 0x206A
#define CMD_APPLY_ON_ITEM                                      0x206C
#define MSG_CC_ATTACH_SKILL_EFFECT                             0x206D
#define CMD_VIEW_TOP                                           0x2074
#define MSG_SHOP_SHOW_ITEM_COUNTER                             0x2077
#define MSG_XQSTY_INFO                                         0x207D
#define CMD_MERGE_MAGIC_PICTURE                                0x207E
#define MSG_NAME_BY_GID                                        0x207F
#define CMD_CLOSE_TAOTAOTAO                                    0x2080
#define MSG_LC_INIT_STATUS                                     0x2081
#define CMD_EQUIP_INBORN_STONE                                 0x2082
#define MSG_DRAFT_OPEN_ROLL                                    0x2083
#define MSG_SUBMIT_ICON                                        0x2084
#define MSG_ADMIN_MAINTAIN_MODULE                              0x2085
#define CMD_PARTYWAR_OPER                                      0x2086
#define CMD_SIGN_IN_DRAW                                       0x2088
#define CMD_CD_WISHING_GOODS                                   0x2094
#define MSG_LC_UPDATE_IMPROVEMENT                              0x20A1
#define MSG_VIEW_TOP                                           0x20A3
#define CMD_ARENA_SWITCH_SERVER                                0x20A4
#define MSG_CASUAL_REFRESH_GAME_LIST                           0x20A5
#define CMD_WUDAO_DRAW_REP_CARD                                0x20A6
#define MSG_GBUYING_OPEN_PAGE                                  0x20A7
#define MSG_REQUEST_FRIEND_VERIFY                              0x20A9
#define MSG_CLIENT_DIALOG_NPC_DISAPPEAR                        0x20AB
#define MSG_FISHING_PLAY_EFFECT                                0x20AD
#define CMD_UPGRADE_MAGIC_PICTURE                              0x20AE
#define MSG_PLAY_EFFECT_SCRIPT                                 0x20AF
#define CMD_EQUIP_PET_DRESS                                    0x20B0
#define MSG_PARTY_KICKOUT_MEMBER                               0x20B1
#define MSG_FINISH_AWARD                                       0x20B3
#define CMD_REWRITE_CHILD_BOOK                                 0x20B4
#define MSG_C_REFRESH_ZHENFA                                   0x20B5
#define MSG_PROMPT_BEFORE_LEAVE                                0x20B7
#define CMD_GET_PARTY_LOG                                      0x20B8
#define MSG_CC_DO_HIT                                          0x20B9
#define MSG_STALL_ITEM_INFO                                    0x20BB
#define CMD_FRIEND_TELL_AUTH                                   0x20BC
#define MSG_SET_CURRENT_CHILD                                  0x20BD
#define CMD_ADMIN_THROW_IN_JAIL_EX                             0x20BE
#define MSG_OLYMPIC_GUESS_INFO                                 0x20BF
#define MSG_PLAY_LIGHT_EFFECT_EX                               0x20C1
#define MSG_APPRENTICE_LIST                                    0x20C3
#define MSG_EXCHANGE_CHILD_APPELLATION                         0x20C5
#define MSG_AUTH_MATRIX_FOR_GS                                 0x20C7
#define MSG_FISHING_CATCHED_FISH                               0x20C9
#define MSG_LEARN_ZHENFA                                       0x20CB
#define MSG_EQUIPPED                                           0x20CD
#define MSG_UPDATE_PETS                                        0x20CF
#define MSG_TALK_DRAMA_LIST                                    0x20D1
#define CMD_ECHO                                               0x20D2
#define CMD_GUARDS_RECOVER                                     0x20D3
#define CMD_REQUEST_BUY_CARD_PACKAGE                           0x20D4
#define MSG_SWITCH_SERVER                                      0x20D5
#define MSG_SWITCH_SERVER_EX                                   0x20D6
#define MSG_ADMIN_QUERY_NPC                                    0x20D7
#define MSG_CONNECTED                                          0x20D9
#define CMD_PARTY_INFO                                         0x20DA
#define CMD_SHOP_VENDUE_SHOP                                   0x20DB
#define CMD_ARENA_QUERY_RANK_BY_NAME                           0x20DC
#define MSG_CC_SWITCH_CARD                                     0x20DD
#define MSG_OPEN_CONFIRM_DIALOG                                0x20DF
#define CMD_GUESS_OK                                           0x20E0
#define MSG_CHARGE_VENDUE_OPEN_STORE                           0x20E1
#define MSG_BUTTON_DOUBLE_BONUS                                0x20E3
#define MSG_GBUYING_OPEN_HISTORY                               0x20E5
#define MSG_SET_INTERFACE_GUIDE                                0x20E7
#define MSG_ADMIN_VIEW_PLAYER                                  0x20E9
#define MSG_UPDATE_ZHENFA                                      0x20EB
#define MSG_SET_VISIBLE_CHILD                                  0x20ED
#define CMD_LOOK_PLAYER                                        0x20EE
#define MSG_PARTY_LIST                                         0x20EF
#define CMD_PRE_ASSIGN_CHILD_ATTRIB                            0x20F0
#define MSG_FIGHT_HALL_CHILD_LIST                              0x20F1
#define CMD_PRESENT_MUSIC                                      0x20F2
#define MSG_QUERY_UPGRADE_COIN                                 0x20F3
#define CMD_TRADE_GET_RECORD                                   0x20F4
#define MSG_CITY_BUILDING_INFO                                 0x20F5
#define CMD_ADMIN_QUERY_NET_INFO                               0x20F6
#define MSG_PARTY_WAR_INFO                                     0x20F7
#define CMD_PURCHASE_GET_OBJ_PROP                              0x20F8
#define MSG_ARENA_RANK_LIST                                    0x20F9
#define MSG_OPEN_MULTI_BONUS                                   0x20FB
#define MSG_C_BATTLE_ARRAY                                     0x20FD
#define CMD_IS_CONVERT_BOTTON                                  0x2100
#define MSG_OPEN_MOUNT_INBORN_SKILLS                           0x2101
#define MSG_BOTTLE_OPEN_INTERFACE                              0x2105
#define MSG_PARTY_QUERY_MEMBER                                 0x2107
#define MSG_PREVIEW_PET_PRACTICE_BONUS                         0x2109
#define CMD_SWITCH_BACK_EQUIP                                  0x2110
#define MSG_C_MENU_LIST                                        0x2111
#define CMD_WUDAO_CONVERT_EXTRA_ATTRIB                         0x2112
#define MSG_CLOSE_GAME                                         0x2113
#define CMD_ARENA_HALL_REQUEST_PL                              0x2114
#define MSG_REFRESH_STONE_INFO                                 0x2115
#define CMD_SET_USER_MOOD                                      0x2116
#define MSG_REFRESH_MAGIC_STONE_STAR                           0x2117
#define CMD_FISHING_APPLY_MULTI_CARD                           0x2118
#define MSG_OPEN_MENU                                          0x2119
#define CMD_C_USE_STUN_CHILD_BOOK                              0x211A
#define MSG_ANSWER_FIELDS                                      0x211B
#define CMD_CHANGE_ITEM_ATTRIB                                 0x211C
#define MSG_C_MESSAGE                                          0x211D
#define MSG_ACCOUNT_STATE                                      0x211F
#define MSG_BOX_ADVENTRUE_UPDATE                               0x2121
#define CMD_DUNGEON_VIEW_TASKS                                 0x2123
#define MSG_CC_ADD_MINION                                      0x2125
#define MSG_SEARCH_GOODS                                       0x2127
#define MSG_EXCHANGE_CLOSE                                     0x2129
#define CMD_CREATE_NEW_RAID                                    0x2130
#define MSG_ACHIEVE_CONFIG                                     0x2131
#define CMD_FEED_CHILD                                         0x2132
#define MSG_DRAFT_MY_INFO                                      0x2133
#define CMD_L_CONVERT_SILVER_COIN                              0x2134
#define MSG_WARCRAFT_CONFIRM                                   0x2135
#define MSG_AUTO_UPGRADE                                       0x2137
#define MSG_L_CONVERT_SILVER_COIN                              0x2139
#define CMD_REQUEST_FRIEND_VERIFY                              0x2140
#define MSG_OPEN_DIALOG                                        0x2141
#define MSG_MUSIC_LIST                                         0x2143
#define MSG_CC_END_COMBAT                                      0x2145
#define MSG_CCS_FINGER_RES                                     0x2147
#define MSG_FISHING_UPDATE_WEAPON                              0x2149
#define MSG_RAISE_MAP_INFO                                     0x214B
#define MSG_CC_DROP_HAND_CARDS                                 0x214D
#define CMD_GATHER                                             0x2150
#define MSG_ACHIEVE_EFFECT                                     0x2151
#define CMD_OPEN_CONFIRM_DLG                                   0x2152
#define MSG_OPEN_CS_EXCHANGE                                   0x2153
#define CMD_ADD_MOUNT_ITEM                                     0x2154
#define MSG_UPDATE_SKILL_POINT                                 0x2155
#define CMD_NEWBIE_PLAYED_DRAMA                                0x2156
#define MSG_FISHING_UPDATE_MULTI_CARD                          0x2157
#define CMD_CHAT_GROUP_ACCEPT_JOIN                             0x2158
#define MSG_ARENA_OPER                                         0x2159
#define CMD_REQUEST_RECHARGE                                   0x215A
#define MSG_COLLECT_CARD_INFO                                  0x215B
#define CMD_SET_SAFE_LOGIN_TIME                                0x215C
#define CMD_SELECT_MENU_ITEM_2                                 0x215E
#define CMD_TEAM_SEEK_MEMBER                                   0x2160
#define MSG_CHANNEL_OPER                                       0x2161
#define CMD_LEARN_SKILL                                        0x2162
#define MSG_C_CATCH_PET                                        0x2163
#define CMD_CHAT_GROUP_ACCEPT_INVITE                           0x2164
#define MSG_CHARGE_VENDUE_REFRESH                              0x2165
#define CMD_REQUEST_CARD_ACTIVITY_LIST                         0x2166
#define CMD_OTHER_MOVE_TO                                      0x2168
#define CMD_LOCAL_ENCRYPT_REQUEST                              0x2170
#define MSG_DISCOVER_REFRESH_MAP_INFO                          0x2171
#define CMD_OPER_STRATEGY_WAR                                  0x2172
#define MSG_GOODS_LIST                                         0x2173
#define MSG_AUTH_MATRIX_CLOSE                                  0x2174
#define MSG_ARENA_HALL_PLAYERS_LIST                            0x2175
#define CMD_ADMIN_OPER_SYS_BROADCAST                           0x2176
#define MSG_C_SET_FIGHT_CHILD                                  0x2177
#define CMD_CHALLENGE_ANSWER                                   0x2178
#define CMD_REQUEST_ITEM_INFO                                  0x2179
#define CMD_C_DO_ACTION_0                                      0x217A
#define MSG_ATTENTION_STATUS                                   0x217B
#define MSG_GET_CLIENT_PERFORMANCE                             0x217C
#define MSG_EXCHANGE_RECONFIRM                                 0x217D
#define CMD_BROADCAST_COMBAT_LIST                              0x217E
#define MSG_VENDUE_GOT_IT                                      0x217F
#define CMD_GET_DAILY_LOOP                                     0x2180
#define CMD_VENDUE_MANAGE                                      0x2181
#define CMD_NEXT_QUESTION                                      0x2182
#define MSG_ARENA_HALL_ROOMS_LIST                              0x2183
#define CMD_COLLECT_DATA                                       0x2184
#define MSG_MAP_CHILD_LIST                                     0x2185
#define CMD_MOVE_ON_CARPET                                     0x2186
#define MSG_AWARD_OPEN                                         0x2187
#define CMD_SET_STALL_GOODS                                    0x2188
#define MSG_CUSTOM_NPC_LIST                                    0x2189
#define CMD_CALC_MOUNT_ATTRIB                                  0x218A
#define MSG_VENDUE_CAN_VENDUE                                  0x218B
#define CMD_FRIEND_DRINK_INVITE                                0x218C
#define MSG_VIEW_TOP_EXTRA                                     0x218D
#define CMD_CHILD_START_FIGHT                                  0x218E
#define MSG_PARTY_TOWER_INFO                                   0x218F
#define CMD_BOTTLE_OPEN_INTERFACE                              0x2190
#define MSG_X_ADMIN_QUERY_ACCOUNT                              0x2191
#define CMD_TELL_EX                                            0x2192
#define MSG_COMMON_GUESS_INFO                                  0x2193
#define CMD_SAMSUNG_OPER                                       0x2194
#define CMD_SHOP_SEARCH                                        0x2195
#define CMD_RECEIVE_ASKTAO_SHOW_GIFT                           0x2196
#define MSG_ADMIN_MASTER_LIST                                  0x2197
#define CMD_REQUEST_ADD_STALL_PAGE                             0x2198
#define MSG_NOT_RED_LIST                                       0x2199
#define CMD_SET_ACHIEVE_TRACE                                  0x219A
#define MSG_DIALOG                                             0x219B
#define CMD_WEDDING_SET_PACKET                                 0x21A0
#define MSG_VIEW_FRIEND_ACHIEVE                                0x21A1
#define MSG_C_DIRECT_OPPONENT_INFO                             0x21A3
#define MSG_VIEW_ACHIEVE                                       0x21A5
#define MSG_REQUEST_STALL_LIST                                 0x21A7
#define MSG_C_END_COMBAT                                       0x21A9
#define MSG_WINTER_VACATION_2014_APP                           0x21AB
#define MSG_DEVELOP_SKILLS                                     0x21AD
#define MSG_QUERY_PRACTICE_PETS                                0x21AF
#define CMD_OPEN_SAFE_SETTING                                  0x21B0
#define MSG_CHECK_SERVER                                       0x21B1
#define CMD_WORKSHOP_QUERY_ORDER_RATE                          0x21B2
#define MSG_DECORATE_VIEW                                      0x21B3
#define CMD_CHANGE_TEAM_LEADER                                 0x21B4
#define MSG_GET_ORDER_RANK                                     0x21B5
#define CMD_REQUEST_MY_CARD_PACKAGES                           0x21B6
#define MSG_VIEW_PACKED_ITEM                                   0x21B7
#define CMD_WUDAO_PRE_ADD_ATTRIB_REP                           0x21B8
#define MSG_CALC_USER_ATTRIBS                                  0x21B9
#define CMD_VIEW_PARTY_WAR_HISTORY                             0x21BA
#define MSG_ADMIN_VIEW_MACS                                    0x21BB
#define CMD_CANCEL_RELEASE_SAFE_LOGIN                          0x21BC
#define CMD_VENDUE_OFFER                                       0x21BD
#define CMD_START_AWARD                                        0x21BE
#define MSG_MANAGE_MP_DEVICE                                   0x21BF
#define CMD_GET_TASK_ITEM                                      0x21C0
#define MSG_REFRESH_PENNANT_SKILLS                             0x21C1
#define CMD_EXCHANGE_CHILD_APPELLATION                         0x21C2
#define MSG_DRAFT_OPEN                                         0x21C3
#define CMD_REPLACE_LOGIN                                      0x21C4
#define MSG_SHIDAO_GLORY_HISTORY                               0x21C5
#define CMD_ACCEPT_COMMON_CONVENANT                            0x21C6
#define MSG_CD_WISHING_LIST                                    0x21C7
#define CMD_BUY_FROM_ASKTAO_SHOW_MALL                          0x21C8
#define MSG_SET_MAGIC_TOWER                                    0x21C9
#define CMD_WITHDRAW                                           0x21CA
#define MSG_ADMIN_TORCH_INFO                                   0x21CB
#define CMD_START_CARD_ACTIVITY                                0x21CC
#define MSG_CALC_PET_GROW                                      0x21CD
#define MSG_SEARCH_RESULT                                      0x21D1
#define MSG_APPEAR_IN_SMALL_MAP                                0x21D3
#define MSG_VIEW_STALL_ACCOUNT                                 0x21D5
#define MSG_RAID_LIST                                          0x21D7
#define MSG_ADMIN_REQUEST_MAP_LIST                             0x21D9
#define CMD_GUESS_CHECK_LOCKED                                 0x21DB
#define MSG_C_DIALOG_OK                                        0x21DD
#define MSG_ZHONGZHOU_SHIPU                                    0x2201
#define CMD_WUDAO_BUY_REP_CARD                                 0x2202
#define MSG_SEND_IMAGE                                         0x2203
#define CMD_ADMIN_TRANSPORT                                    0x2204
#define MSG_SHOP_ENTER_SHOP_TYPE                               0x2205
#define MSG_CC_UPDATE_DECK                                     0x2207
#define CMD_QUERY_UPGRADE_CASH                                 0x2208
#define MSG_VIEW_CS_SUMMARY                                    0x2209
#define CMD_REFRESH_PARTY_FRAGMENT                             0x220A
#define CMD_APPLY                                              0x220C
#define CMD_GBUYING_FETCH_COIN                                 0x2210
#define MSG_ASKTAO_SHOW_MALL_LIST                              0x2211
#define CMD_ENTER_ROOM_2                                       0x2212
#define CMD_WISHING_OPEN                                       0x2214
#define CMD_CALC_PET_GROW                                      0x2216
#define MSG_KIND_GOODS_LIST                                    0x2218
#define CMD_DRAFT_ROLL                                         0x2220
#define CMD_COMMON_GUESS_LIST                                  0x2222
#define MSG_C_CHILD_EFFECT                                     0x2223
#define CMD_REFRESH                                            0x2224
#define MSG_PASSWORD_DLG                                       0x2225
#define CMD_VIEW_ACHIEVE                                       0x2226
#define MSG_SET_CARRY_CHILD                                    0x2227
#define CMD_TRAIN                                              0x2228
#define MSG_ADMIN_REQUEST_PLAYER_LIST                          0x2229
#define CMD_SHOUWU_ZUDAO                                       0x2230
#define MSG_C_UPDATE_IMPROVEMENT                               0x2231
#define CMD_X_SET_SETTING                                      0x2232
#define CMD_CLICKED_LINKER                                     0x2233
#define CMD_VIEW_CS_MATCHES_SUMMARY                            0x2234
#define MSG_ADMIN_QUERY_PLAYER                                 0x2235
#define CMD_DROP                                               0x2236
#define MSG_FINISH_QUESTION                                    0x2237
#define CMD_SET_TASK_TRACE                                     0x2238
#define MSG_START_COMBAT_GUIDE                                 0x2239
#define CMD_CC_FINISH_ROUN                                     0x2240
#define CMD_ACCELERATE_MOVE_TO                                 0x2242
#define CMD_DRAW_CARD_RANK_BONUS                               0x2244
#define CMD_TEAM_SEEK_LEADER                                   0x2246
#define CMD_ADMIN_SRMOUSE_CLICK                                0x2248
#define CMD_ADMIN_REQUEST_PLAYER_LIST                          0x2250
#define CMD_ASSIGN_RESIST                                      0x2252
#define CMD_OPER_SORT_TASK                                     0x2254
#define MSG_RANDOM_BOX_AWARD_INFO                              0x2255
#define CMD_SIGN_IN_OPEN                                       0x2256
#define MSG_UPGRADE_MATERIAL                                   0x2257
#define CMD_OPEN_ASKTAO_SHOW_MALL                              0x2258
#define CMD_OPER_ENVELOPE                                      0x225A
#define CMD_DRAFT_REQUEST_STAGE_INFO                           0x225C
#define CMD_DOUCDH_CHALLENGE                                   0x225E
#define CMD_REQUEST_ARENA_RANK_LIST                            0x2260
#define CMD_SUBMIT_PASSPOD_ORDER                               0x2262
#define CMD_PRE_ASSIGN_ATTRIB                                  0x2264
#define CMD_SET_HOTKEYS                                        0x2266
#define MSG_TRADE_SHELF_PET_INFO                               0x2267
#define CMD_CHAT_GROUP_JOIN                                    0x2268
#define MSG_DECK_LIST                                          0x2269
#define CMD_C_CATCH_PET                                        0x226A
#define MSG_GS_EMOTE_DATA                                      0x226B
#define CMD_OPEN_MOUNT_INBORN_SKILLS                           0x226C
#define MSG_OPEN_PLACE_PATTERN                                 0x226D
#define MSG_SET_CURRENT_PET                                    0x226F
#define CMD_SHOP_MANAGE_COUNTER                                0x2271
#define CMD_CHAT                                               0x2276
#define MSG_REQUEST_TOP_TAO_PAGE                               0x2277
#define CMD_ADMIN_SUMMON                                       0x2278
#define CMD_SELECT_VISIBLE_CHILD                               0x227A
#define CMD_REFRESH_TASK_LOG                                   0x227C
#define CMD_FETCH_GBUYING_COIN                                 0x2280
#define MSG_ACHIEVE_TRACE                                      0x2281
#define CMD_PURCHASE_CANCLE_ORDER                              0x2282
#define CMD_SHOP_MANAGE_MONEY                                  0x2283
#define CMD_ADD_MATERIAL                                       0x2284
#define MSG_COIN_REFRESH_ORDER                                 0x2285
#define MSG_TXSS_OPEN                                          0x2287
#define MSG_TXSS_ACTIVATE                                      0x2289
#define MSG_CANGBAOGE_OPEN                                     0x228B
#define MSG_CANGBAOGE_BONUS                                    0x228D
#define CMD_CHECK_SAFE_ANSWER                                  0x2290
#define MSG_LC_START_LOOKON                                    0x2291
#define CMD_RECOVER_CHILD_BOOK                                 0x2292
#define MSG_CC_UPDATE_WEAPON                                   0x2293
#define CMD_TRY_PLACE_PATTERN                                  0x2294
#define MSG_REFRESH_MELT_MATERIAL                              0x2295
#define CMD_REQUEST_TO_BE_TEAM_LEADER                          0x2296
#define MSG_OPEN_SAFE_SETTING                                  0x2297
#define CMD_QUERY_PRACTICE_PETS                                0x2300
#define MSG_GUESS_RESULT                                       0x2301
#define CMD_OPER_CASUAL_GAME                                   0x2302
#define MSG_DECORATE_SET                                       0x2303
#define CMD_IDENTIFY_EQUIP                                     0x2304
#define MSG_CHILD_FIGHT_REFRESH                                0x2305
#define CMD_FRIEND_TELL                                        0x2306
#define MSG_INTERSVR_CHANNEL_MSG                               0x2307
#define CMD_CLOSE_TALK_DRAMA                                   0x2308
#define MSG_X_ENTER_GAME                                       0x2309
#define CMD_VIEW_CS_PW_FIXTURES                                0x230A
#define CMD_INVITE                                             0x2310
#define MSG_ENVELOPE_DETAIL                                    0x2311
#define CMD_BUY_CUSTOMIZE_PACKAGE                              0x2312
#define MSG_PARTY_MEMBERS                                      0x2313
#define CMD_ADMIN_COMMON_COMMAND                               0x2314
#define MSG_C_UPDATE_APPEARANCE                                0x2315
#define CMD_LOCAL_ENCRYPT_BORROW                               0x2316
#define MSG_COMMON_GUESS_LIST                                  0x2317
#define CMD_GET_IMAGE                                          0x2320
#define MSG_HOUSE_ID_LIST                                      0x2321
#define MSG_TRADE_SHELF_PET_RECORD                             0x2323
#define CMD_RACE_SHOW_BONUS                                    0x2324
#define MSG_TRADE_INTRODUCE                                    0x2325
#define CMD_FISHING_BUY_WEAPON                                 0x2326
#define MSG_C_WAIT_ALL_END                                     0x2327
#define CMD_GET                                                0x2328
#define CMD_GUESS_END                                          0x2329
#define MSG_LC_ACCEPT_MAGIC_HIT                                0x2333
#define CMD_SET_FRIEND_REMARK                                  0x2334
#define MSG_SECURITY_SETTING                                   0x2335
#define CMD_CHANGE_FURNITURE_DIR                               0x2336
#define MSG_BUY_SERVICE                                        0x2337
#define CMD_PARTY_MODIFY_MEMBER                                0x2340
#define MSG_FILL_PASSPOD_ORDER                                 0x2341
#define CMD_OPEN_ARENA_RANK                                    0x2342
#define MSG_STORE_BOXES                                        0x2343
#define CMD_ADMIN_BROADCAST                                    0x2344
#define MSG_CC_LIFE_DELTA                                      0x2345
#define CMD_FISHING_FIRE_BULLET                                0x2346
#define MSG_CCS_ONLINE_FRIEND                                  0x2347
#define CMD_SET_SETTING                                        0x2348
#define MSG_DRAFT_STAGE_INFO                                   0x2349
#define CMD_ENTER_ROOM_1                                       0x234A
#define MSG_TEAM_MOVED                                         0x234B
#define CMD_FISHING_OPEN_REMOTE_STORE                          0x234C
#define MSG_LC_START_SEQUENCE                                  0x234D
#define CMD_L_ACCOUNT                                          0x2350
#define MSG_UPDATE_MY_ASKTAO_SHOW                              0x2351
#define CMD_L_GET_DIST_LIST                                    0x2352
#define CMD_SHOP_BUY_PRODUCT                                   0x2353
#define CMD_SDDH_OPER                                          0x2354
#define MSG_SHIZN_SHINA_JIUWEN                                 0x2355
#define MSG_LC_WAIT_ALL_END                                    0x2357
#define CMD_L_AGENT_CONNECT_SERVER                             0x2358
#define MSG_ADMIN_VIEW_IPS                                     0x2359
#define CMD_CFG_GET_DIST_LIST                                  0x235A
#define MSG_CFG_DIST_LIST                                      0x235B
#define CMD_CFG_GET_MY_DIST                                    0x235C
#define CMD_SHOP_ENTER_SHOP                                    0x235D
#define CMD_CFG_GET_SERVER_LIST                                0x235E
#define CMD_CFG_GET_DBA_LIST                                   0x2360
#define MSG_EXCHANGE_ADD_GOODS                                 0x2361
#define CMD_CFG_GET_DDB_LIST                                   0x2362
#define MSG_CFG_DDB_LIST                                       0x2363
#define CMD_CFG_GET_MY_INFO                                    0x2364
#define MSG_ANTI_HACK_COLLECT                                  0x2365
#define CMD_CFG_GET_CCS_INFO                                   0x2366
#define MSG_CFG_CCS_INFO                                       0x2367
#define CMD_CFG_GET_ADB_INFO                                   0x236A
#define MSG_CFG_ADB_INFO                                       0x236B
#define CMD_REQUEST_NEW_DECK                                   0x236C
#define MSG_CFG_CCS_LIST                                       0x236D
#define CMD_CFG_GET_LDBA_LIST                                  0x236E
#define MSG_CFG_CTA_LIST                                       0x236F
#define MSG_L_ADD_POINTS                                       0x2371
#define CMD_L_EXCHANGE_POINTS                                  0x2372
#define MSG_L_EXCHANGE_POINTS                                  0x2373
#define CMD_L_GET_CARD_INFO                                    0x2374
#define MSG_REFRESH_MAGIC_PICTURE                              0x2375
#define CMD_L_GET_CHARGE_INFO                                  0x2376
#define MSG_OPEN_AUTH_ALL_PROTECT                              0x2377
#define CMD_L_LOCK_CARD                                        0x2378
#define CMD_L_AUTH_MATRIX                                      0x237A
#define MSG_L_BUY_CARD                                         0x237D
#define CMD_IGNORE_CONVENANT                                   0x2380
#define MSG_PICTURE_DIALOG                                     0x2381
#define MSG_GET_CLIENT_ACTION                                  0x2382
#define MSG_CCS_EMOTE_DATA                                     0x2383
#define CMD_SHOP_SUBMIT                                        0x2384
#define MSG_L_QUERY_ACCOUNT_STATE                              0x238B
#define CMD_L_ENABLE_AUTO_LOCK                                 0x238E
#define MSG_L_QUERY_ADDICTION                                  0x238F
#define MSG_L_GET_SAFE_CTRL_INFO                               0x2390
#define MSG_L_UPDATE_ADDICTION                                 0x2391
#define CMD_L_UPDATE_ADDICTION                                 0x2392
#define MSG_L_QUERY_IP_AREA                                    0x2393
#define CMD_L_QUERY_IP_AREA                                    0x2395
#define CMD_L_SSB_COPY_ACCOUNT                                 0x2396
#define CMD_L_SSB_UPDATE_ACCOUNT                               0x2397
#define CMD_L_GET_SSB_COMBATD_CONF                             0x2398
#define CMD_L_GET_SSB_FINISH_FLAG                              0x2399
#define CMD_L_SET_SAFE_CTRL_STR                                0x239A
#define CMD_L_SET_SAFE_CTRL_VAR                                0x239B
#define MSG_L_SET_SAFE_CTRL_STR                                0x239C
#define MSG_L_SET_SAFE_CTRL_VAR                                0x239D
#define CMD_L_GET_SAFE_CTRL_INFO                               0x239E
#define CMD_L_QUERY_COUPON                                     0x239F
#define CMD_L_QUERY_CITYS                                      0x23A0
#define MSG_L_QUERY_CITYS                                      0x23A1
#define CMD_ASSIGN_POLAR                                       0x23A2
#define MSG_VIEW_CS_MATCHES                                    0x23A3
#define CMD_PURCHASE_FETCH_CASH                                0x23A4
#define MSG_X_VIEW_ACHIEVE                                     0x23A5
#define CMD_CARD_COMPETE_FINISH                                0x23A8
#define MSG_WUDAO_PRE_ADD_ATTRIB_REP                           0x23A9
#define CMD_UNEQUIP_PET_DRESS                                  0x23AA
#define MSG_XGAME_OPER                                         0x23AB
#define CMD_DRAFT_REQUEST_STAGE_RANK                           0x23AC
#define MSG_FRIEND_CHILDREN_LIST                               0x23AD
#define MSG_TEMP_NPC_GUIDE                                     0x23AE
#define MSG_PRE_ASSIGN_CHILD_ATTRIB                            0x23AF
#define CMD_REQUEST_ASKTAO_SHOW                                0x23B0
#define MSG_ARENA_ROOM_FLEE                                    0x23B1
#define CMD_CHARGE_VENDUE_OFFER_PRICE                          0x23B3
#define MSG_FISH_SWALLOW_START                                 0x23B5
#define CMD_CHILD_ENTER_MAP                                    0x23B6
#define MSG_VIEW_PKERS                                         0x23B7
#define CMD_L_PEND_SHOP                                        0x2400
#define MSG_L_QUERY_COUPON                                     0x2401
#define MSG_WRITTEN_CONTENT                                    0x2403
#define CMD_DELETE_CONTENT                                     0x2404
#define MSG_DELETED_CONTENT                                    0x2405
#define MSG_CREATED_CONTENT                                    0x2407
#define MSG_GOT_CATALOG                                        0x2409
#define MSG_GOT_PAGE_DATA                                      0x240B
#define MSG_GOT_PAGE_DATA_EX                                   0x240D
#define MSG_GOT_CATALOG_EX                                     0x240F
#define MSG_DELETED_GID                                        0x2411
#define MSG_UPDATED_GID                                        0x2413
#define CMD_SHOP_REQ_VENDUES                                   0x2415
#define MSG_READ_LOG                                           0x241B
#define MSG_UPDATE_APPEARANCE                                  0x2421
#define CMD_LOCAL_ENCRYPT_LOCK_FAILED                          0x2422
#define MSG_RAISE_MESSAGE                                      0x2423
#define CMD_TRADE_CONTINUE_SALE                                0x2424
#define MSG_VIEW_ACHIEVE_OVERVIEW                              0x2425
#define CMD_LOCAL_ENCRYPT_UNLOCK                               0x2426
#define MSG_LC_OPPONENTS                                       0x2427
#define CMD_ARENA_HALL_JOIN_ROOM                               0x2428
#define CMD_LOG_IMPORTANT_ACTION                               0x242A
#define CMD_LOG_ANTI_CHEATER                                   0x242C
#define CMD_L_WHEN_BLOCK_ACCOUNT                               0x242D
#define CMD_LOG_IP_TASK                                        0x242E
#define CMD_LOG_USER_REPORT_IP                                 0x2430
#define CMD_REQUEST_CARD_TODAY_STAT                            0x2440
#define MSG_SET_DOUCDH_PET_POS                                 0x2441
#define CMD_ADMIN_REBOOT                                       0x2442
#define CMD_OPER_MASTER                                        0x2444
#define CMD_WARCRAFT_CANCEL                                    0x2446
#define CMD_CFG_GET_CCS_LIST                                   0x246C
#define CMD_CFG_GET_CTA_LIST                                   0x246E
#define MSG_CFG_ETC_PAGE_DATA                                  0x2471
#define CMD_CFG_GET_AHSERVER_INFO                              0x2472
#define MSG_CFG_READ_ETC                                       0x2473
#define MSG_CFG_AHSERVER_INFO                                  0x2475
#define MSG_CFG_DDB_LIST_EX                                    0x2477
#define CMD_CFG_GET_AAA_LIST                                   0x2478
#define MSG_CFG_AAA_LIST                                       0x2479
#define CMD_CFG_GET_CSA_INFO                                   0x247A
#define MSG_CFG_WRITE_ETC                                      0x247B
#define MSG_CFG_CSA_INFO                                       0x247D
#define CMD_CFG_GET_TTS_INFO                                   0x247E
#define MSG_CFG_TTS_INFO                                       0x247F
#define CMD_L_RECYCLE_STALL                                    0x2480
#define CMD_L_BLOCK_SHARE_STORE                                0x2482
#define CMD_L_WHEN_BATCH_BLOCK_ACCOUNT                         0x2484
#define MSG_SHOP_AD_OPEN                                       0x2501
#define CMD_MUSIC_LIST                                         0x2502
#define MSG_LC_UPDATE                                          0x2503
#define MSG_START_QUESTION                                     0x2505
#define MSG_ADD_ANIMATION                                      0x2507
#define CMD_OPER_LOCK_EXP                                      0x2510
#define MSG_CLIENT_NPC_BASIC_INFO                              0x2511
#define MSG_SKILL_FROZEN_LIST                                  0x2513
#define MSG_GRAY_COMBAT_MENU                                   0x2515
#define MSG_PILL_RESULT                                        0x2517
#define CMD_SET_WEIBO_INFO                                     0x2518
#define MSG_ARENA_ROOM_SEAT_STATUS                             0x2519
#define CMD_TAKE_UP                                            0x251A
#define MSG_C_UPDATE_IMPROVEMENT_EX                            0x251B
#define MSG_INVENTORY_ITEM                                     0x2601
#define CMD_WUDAO_STAGE_INFO                                   0x2602
#define MSG_TASK_PROMPT                                        0x2603
#define MSG_ATTACH_SKILL_LIGHT_EFFECT                          0x2604
#define CMD_APPLY_OTHER                                        0x2606
#define CMD_NEWBIE_FIRST_ACTION                                0x2608
#define CMD_BOX_ADVENTRUE_START                                0x260A
#define CMD_PREVIEW_PET_PRACTICE_BONUS                         0x260C
#define CMD_MA_L_ACCOUNT_FROM_SPA                              0x2704
#define MSG_MA_CHANNEL_MESSAGE                                 0x270A
#define MSG_CANCEL_MATCH_CARD_GAME                             0x270D
#define MSG_MA_REPORT_ILLEGAL_TOOL                             0x270F
#define MSG_VIEW_VISUAL_PACKS                                  0x2711
#define CMD_ARENA_ROOM_ENTER                                   0x2712
#define MSG_L_SHOW_VERIFY_CODE                                 0x2713
#define CMD_LOOK_ON                                            0x2714
#define MSG_TEMP_ITEM_DESC                                     0x2715
#define CMD_L_AUTH_VERIFY_CODE                                 0x2716
#define CMD_L_RESELECT_VERIFY_CODE                             0x2718
#define CMD_PASSWORD_DLG                                       0x271A
#define CMD_LOTTERY_SELECT                                     0x2720
#define MSG_CUR_SCENE                                          0x2721
#define CMD_ARENA_ROOM_KICK_OUT                                0x2722
#define MSG_CC_UPDATE                                          0x2723
#define CMD_SIGN_IN                                            0x2724
#define CMD_OPER_LOTTERY_TICKET                                0x2726
#define CMD_GET_AWARD_INFO                                     0x2728
#define CMD_CARD_COMPETE_START                                 0x272A
#define CMD_CS_EXCHANGE                                        0x272C
#define CMD_ADMIN_QUERY_SKILL                                  0x272E
#define CMD_CHILD_USE_SKILL                                    0x2800
#define MSG_ADMIN_SMBZ_INFO                                    0x2801
#define CMD_APPLY_0N_FRIEND                                    0x2802
#define CMD_SHOP_VIEW_OWN_SHOP                                 0x2803
#define CMD_CHANNEL_OPER                                       0x2804
#define MSG_CCS_FRIEND_TELL                                    0x2805
#define CMD_PLACE_FURNITURE                                    0x2806
#define MSG_LC_LIFE_DELTA                                      0x2807
#define CMD_WHO                                                0x2808
#define MSG_ADMIN_SERVER_LIST                                  0x2809
#define CMD_COIN_OPEN_MALL                                     0x280A
#define MSG_SET_ACHIEVE_BONUSED                                0x280B
#define CMD_CHILD_ENTER_FIGHT_HALL                             0x280C
#define MSG_ADMIN_VIEW_IMPROVEMENT                             0x280D
#define CMD_ARENA_HALL_REQUEST_CI                              0x280E
#define MSG_OPEN_SORT_TASK_INTERFACE                           0x280F
#define MSG_UPGRADE_OPEN                                       0x2811
#define CMD_TRADE_CHANGE_PRICE                                 0x2812
#define CMD_REQUEST_RECEIVE_CITY_TASK                          0x2814
#define CMD_CALC_USER_ATTRIBS                                  0x2816
#define CMD_ADMIN_SET_SKILL                                    0x2818
#define CMD_REQUEST_ONLINEMALL_CARD_PACKAGES                   0x281A
#define CMD_BUY_SERVICE                                        0x281C
#define CMD_DELETE_MUSIC                                       0x281E
#define MSG_UPDATE_SKILLS                                      0x2821
#define MSG_SET_VISIBLE_PET                                    0x2901
#define CMD_VENDUE_STORE_CLOSED                                0x2903
#define MSG_WUZHUANGYAUN_RANK                                  0x2905
#define MSG_REFRESH_BOX_INFO                                   0x2907
#define MSG_ADMIN_GRANT_CMD_LIST                               0x2909
#define MSG_LUCKY_SIGN                                         0x290B
#define MSG_MOBILE_TASK                                        0x290D
#define CMD_GET_CLIENT_PHY_CONFIG                              0x290F
#define MSG_CHAT_GROUP_REQUEST_MEMBER                          0x29C3
#define CMD_SET_USER_STATE                                     0x29C7
#define MSG_GET_UPDATE_NOTICE                                  0x29CB
#define MSG_DELETE_MUSIC                                       0x29CD
#define MSG_LC_MENU_SELECTED                                   0x29E5
#define MSG_C_MENU_SELECTED                                    0x29ED
#define MSG_PLACE_PATTERN_RESULT                               0x29F5
#define CMD_FRIEND_TELL_EX                                     0x2A00
#define CMD_COIN_CANCEL_ORDER                                  0x2A02
#define MSG_ASKTAO_SOUND                                       0x2A0E
#define CMD_CHANGE_TITLE                                       0x2AE8
#define CMD_TRADE_INTRODUCE                                    0x2AF2
#define MSG_L_CHECK_USER_DATA                                  0x2B04
#define MSG_GOT_SERVER_TIME                                    0x2C04
#define MSG_SEND_RECOMMEND_ATTRIB                              0x2DBF
#define MSG_END_MULTI_BONUS                                    0x2DC1
#define MSG_ANSWER_RESULT                                      0x2DC3
#define MSG_UPGRADE_CLOSE                                      0x2DC5
#define MSG_TRADE_SHELF_ITEM_RECORD                            0x2DC7
#define MSG_SEND_PARTY_LOG                                     0x2DC9
#define CMD_CHAT_GROUP_DISMISS                                 0x2DCA
#define MSG_STORING_CASH_INFO                                  0x2DCB
#define MSG_SUBMIT_CONFIRM_DLG_CLOSED                          0x2DCD
#define MSG_OPEN_FIREWORKS_DLG                                 0x2DCF
#define CMD_DRAFT_OPER                                         0x2DD1
#define MSG_MENU_CLOSED                                        0x2DD3
#define CMD_VENDUE_REFRESH                                     0x2DD5
#define MSG_SHOUWU_ZUDAO                                       0x2DED
#define MSG_START_CARD_INSTRUCTION                             0x2E05
#define MSG_CITY_LIST                                          0x2E0B
#define MSG_SHOP_SHOW_SUBMIT                                   0x2E0D
#define MSG_VIEW_CS_MATCH_DETAIL                               0x2E0F
#define MSG_UPGRADE_STATUS                                     0x2E11
#define MSG_GET_SYS_ENVELOPE                                   0x2E37
#define MSG_MOOD_INFO                                          0x2E39
#define CMD_CHAT_GROUP_INVITE_MEMBER                           0x2E3A
#define CMD_GO                                                 0x2E3C
#define CMD_ADMIN_FETCH_PICS                                   0x2EEE
#define CMD_CONTROL_PARTY_CHANNEL                              0x2EF0
#define CMD_RAID_KICKOUT_MEMBER                                0x2EF1
#define CMD_PURCHASE_ADD_ORDER                                 0x2EF2
#define MSG_SHOW_MATCH_SCHEDULE                                0x2EF3
#define CMD_ADMIN_VIEW                                         0x2EF4
#define MSG_C_LIGHT_EFFECT                                     0x2EF5
#define CMD_MODIFY_SCHEDULE                                    0x2EF7
#define MSG_REFRESH_INSTRUCTION_INFO                           0x2EF9
#define MSG_CARD_LIST                                          0x2EFB
#define CMD_GBUYING_OPEN_PAGE                                  0x2EFC
#define MSG_ADMIN_LZDSJ_INFO                                   0x2EFD
#define CMD_TAKEOUT_ITEM                                       0x2EFE
#define MSG_RAID_MEMBER_STATE                                  0x2FBB
#define MSG_ITEM_DISAPPEAR                                     0x2FBD
#define CMD_SET_SAFE_CONTROL_CONTENT                           0x2FD3
#define MSG_CALC_MOUNT_ATTRIB                                  0x2FE9
#define MSG_STALL_INPUT_PRICE                                  0x2FED
#define MSG_FRIEND_GROUP_OPERATION                             0x2FEF
#define MSG_SEND_PET_EVOLVE_LIST                               0x2FFD
#define MSG_UPDATE_WITHOUT_LEAVE_TEMP_TEAM_LIST                0x2FFF
#define MSG_ADMIN_COMMON_BLOCK_REASON                          0x3001
#define MSG_CC_UPDATE_HAND_CARDS                               0x3005
#define CMD_ARENA_ROOM_FLEE                                    0x3010
#define MSG_CHALLENGE_MATCH                                    0x3011
#define CMD_DRAFT_PRESENT_FLOWER                               0x3012
#define MSG_DECK_INFO                                          0x3013
#define CMD_GBUYING_BUY_GOODS                                  0x3014
#define CMD_MOV_TO_QIANKUNDAI                                  0x3015
#define CMD_QUIT_LOOK_ON                                       0x3016
#define CMD_AUTH_REFRESH_PIC_FOR_GS                            0x3018
#define CMD_LOG_GIFT_ITEM                                      0x301A
#define CMD_GUARDS_REFRESH                                     0x301B
#define CMD_ADMIN_QUERY_NPC                                    0x301C
#define CMD_LOOKON_BROADCAST_COMBAT                            0x3038
#define CMD_TTS_SHUTDOWN                                       0x3042
#define CMD_DISTRIBUTE_RECRUITMENT                             0x3044
#define CMD_PURCHASE_GET_ORDER_LIST                            0x3046
#define CMD_LOG_CSP_COMBAT                                     0x3048
#define CMD_LOG_CSP_MATCH                                      0x304A
#define MSG_CARD_GAME_SORT_DECK                                0x304B
#define CMD_ARENA_OPER                                         0x304C
#define MSG_WISHING                                            0x304D
#define CMD_WISHING_BUY_ITEM                                   0x304E
#define CMD_CHILD_INVITE_FIGHT                                 0x3050
#define CMD_SWITCH_CHILD_ITEM                                  0x3052
#define CMD_TEAM_OPEN_PLATFORM                                 0x3054
#define CMD_LEAVE_MAGIC_TOWER                                  0x3056
#define MSG_TEAM_FESTIVAL_TASK                                 0x3057
#define CMD_ADD_FRIEND                                         0x3058
#define MSG_AWARD_INFO_EX                                      0x3059
#define CMD_SWITCH_IN_STORE                                    0x3060
#define MSG_REQUEST_SHILING                                    0x3061
#define CMD_CHANGE_ZHENFA                                      0x3062
#define CMD_REFRESH_PET_GODBOOK_SKILLS                         0x3064
#define MSG_ADMIN_ONLINE_GM_LIST                               0x3065
#define MSG_SHOP_VIEW_OWN                                      0x3067
#define CMD_TAKE_STORING_CASH                                  0x3068
#define CMD_WORKSHOP_QUERY_ORDER_CASH                          0x306A
#define MSG_TAOTAOTAO_CLOSE                                    0x306B
#define CMD_REQUESY_CITY_INFO                                  0x306C
#define CMD_CHAT_GROUP_CREATE                                  0x306E
#define CMD_REQUEST_LEVELUP                                    0x3070
#define CMD_TEMP_NPC_EVENT                                     0x307A
#define MSG_RECORD_COMMAND                                     0x30A9
#define CMD_XGAME_OPER                                         0x30AC
#define CMD_SELECT_MENU_ITEM_1                                 0x30CA
#define CMD_TAKEOUT_CARD                                       0x30CC
#define CMD_REPORT_ILLEGAL_TOOL                                0x30D0
#define MSG_DATA_MINING                                        0x30D1
#define MSG_LEAGUE_OPER                                        0x3101
#define CMD_LEAGUE_OPER                                        0x3102
#define MSG_BOTTLE_OPER                                        0x3103
#define CMD_BOTTLE_OPER                                        0x3104
#define CMD_ARENA_ROOM_OPEN_SEAT                               0x3202
#define CMD_ARENA_REQUEST_SERVER_LIST                          0x3204
#define CMD_COMMON_GUESS_START_GUESS                           0x3206
#define CMD_TOWER_SELECTED                                     0x3208
#define MSG_ADMIN_PRINT_SCREEN                                 0x3209
#define CMD_DROP_PET                                           0x3300
#define MSG_LC_UPDATE_STATUS                                   0x3301
#define CMD_HSSL_START_CHALLENGE                               0x3302
#define MSG_C_DIRECT_UPDATE                                    0x3303
#define MSG_L_CHANGE_COUPON                                    0x3317
#define CMD_L_CHANGE_COUPON                                    0x3318
#define MSG_L_QUERY_RANDOM_COUPON                              0x3319
#define CMD_L_QUERY_RANDOM_COUPON                              0x3320
#define CMD_L_ADD_ENCOURAGE_COUPON                             0x3322
#define CMD_L_DO_RECHARGE                                      0x3324
#define MSG_L_DO_RECHARGE                                      0x3325
#define CMD_L_DO_INQUIRY                                       0x3326
#define MSG_L_DO_INQUIRY                                       0x3327
#define CMD_OPER_HELL_INSTRUCTION                              0x3328
#define MSG_LC_LIGHT_EFFECT                                    0x3329
#define CMD_TASK_SCORE_EXCHANGE_BONUS                          0x332A
#define MSG_REFRESH_MELT_PET                                   0x332B
#define MSG_L_DIST_LIST                                        0x3353
#define CMD_L_GET_SERVER_LIST                                  0x3354
#define CMD_L_CLIENT_CONNECT_AGENT                             0x3356
#define MSG_L_AGENT_RESULT                                     0x3357
#define CMD_L_CHARGE_ACCOUNT                                   0x335A
#define CMD_L_BLOCK_ACCOUNT                                    0x335E
#define CMD_L_CHANGE_PROTECT                                   0x3360
#define MSG_CFG_DBA_LIST                                       0x3361
#define MSG_CFG_LDBA_LIST                                      0x3363
#define MSG_CFG_MY_INFO                                        0x3365
#define CMD_L_GET_ACC_INFO                                     0x3366
#define CMD_VERIFY_CODE                                        0x3368
#define MSG_L_VERIFY_PASSWORD                                  0x3369
#define CMD_L_ADD_POINTS                                       0x3370
#define CMD_L_CHARGE_HISTORY                                   0x3378
#define MSG_L_LOCK_CARD                                        0x3379
#define CMD_L_UNLOCK_CARD                                      0x337A
#define MSG_L_UNLOCK_CARD                                      0x337B
#define CMD_L_BUY_CARD                                         0x337C
#define CMD_L_QUERY_CARD                                       0x337E
#define CMD_L_FORBID_ACCOUNT                                   0x3380
#define MSG_L_GET_SSB_COMBATD_RES                              0x3380
#define MSG_L_SSB_UPDATE_ACCOUNT                               0x3381
#define MSG_L_SSB_COPY_ACCOUNT                                 0x3382
#define MSG_L_GET_SSB_COMBATD_CONF                             0x3383
#define MSG_L_GET_SSB_FINISH_FLAG                              0x3384
#define MSG_L_GET_SSB_PROMPT_MSG                               0x3385
#define MSG_L_CLONE_ACCOUNT_RESULT                             0x3386
#define CMD_L_CLONE_ACCOUNT                                    0x3387
#define MSG_L_LOCK_ACCOUNT                                     0x3389
#define MSG_L_ENABLE_AUTO_LOCK                                 0x338D
#define CMD_WUDAO_SET_ATTRIB_STATUS                            0x3390
#define CMD_SHOP_BUY_SHOP                                      0x3391
#define CMD_RANDOM_USER_NAME                                   0x3392
#define MSG_BANNER                                             0x3393
#define MSG_SAFE_QUESTIONS                                     0x3394
#define MSG_ARENA_GOOD_LIST                                    0x3395
#define CMD_READ_CONTENT                                       0x3400
#define MSG_CARD_GAME_SORT_LIST                                0x3401
#define CMD_SET_PASSWORD                                       0x3402
#define MSG_ENVELOPE_RECEIVED                                  0x3403
#define MSG_GUARDS_VISIBLE                                     0x3404
#define MSG_C_UPDATE_BACKGROUND                                0x3405
#define CMD_REQUEST_CANCEL_CARD_GAME                           0x3406
#define MSG_WORKSHOP_ORDER_LIST                                0x3407
#define CMD_CITYWAR_GIVE_ORDERS                                0x3408
#define MSG_CC_UPDATE_IMPROVEMENT                              0x3409
#define MSG_COMPOSE_CARD_SUCCEED                               0x340B
#define MSG_CREATED_GID                                        0x3411
#define MSG_GOT_GID                                            0x3415
#define CMD_XG_CREATE_NAME                                     0x3420
#define MSG_XG_CREATE_NAME                                     0x3421
#define CMD_XG_GET_ACCOUNT                                     0x3422
#define MSG_XG_GET_ACCOUNT                                     0x3423
#define CMD_XG_GET_NAME                                        0x3424
#define MSG_XG_GET_NAME                                        0x3425
#define CMD_CHARGE_VENDUE_REFRESH                              0x3501
#define CMD_WARCRAFT_START                                     0x3502
#define MSG_CC_ADD_HAND_CARDS                                  0x3503
#define CMD_TAKE                                               0x3504
#define MSG_FISHING_END                                        0x3505
#define CMD_GET_PARTY_CHANNEL_DENY_LIST                        0x3506
#define MSG_ZHENFA_INFO                                        0x3507
#define CMD_OPER_GARDEN_INSTRUCTION                            0x3508
#define MSG_SYS_LIGHT_EFFECT                                   0x3509
#define CMD_REQUEST_RENAME_DECK                                0x350A
#define MSG_LC_QUIT_COMBAT                                     0x350B
#define CMD_BUY_FROM_STALL                                     0x350C
#define MSG_C_GENERAL_NOTIFY                                   0x350D
#define CMD_REPAIR                                             0x350E
#define MSG_FRIEND_NOTIFICATION                                0x3601
#define MSG_NEW_VERSION_GUIDE                                  0x3603
#define MSG_STATE_INFO                                         0x3701
#define CMD_KICKOUT                                            0x3702
#define MSG_CARD_INFO                                          0x3703
#define CMD_REQUEST_MINERALS_IN_MAP                            0x3704
#define MSG_AUTO_STEP_MOVE_END                                 0x3705
#define CMD_EXCHANGE_OPEN                                      0x3706
#define CMD_LISTEN_MUSIC                                       0x3708
#define CMD_TRADE_GET_DATA                                     0x370A
#define MSG_GS_FRIEND_TELL                                     0x3801
#define CMD_GENERAL_REQUEST                                    0x3802
#define MSG_NPC_GUIDE                                          0x3803
#define CMD_RECOVER_MAGIC_PENNANT                              0x3804
#define CMD_COIN_FETCH_STORE                                   0x3806
#define MSG_CSA_COMMON_OPER                                    0x39D1
#define MSG_OPEN_CONFIRM_DLG                                   0x39E3
#define MSG_MOBILE_NPC_LIST                                    0x39E9
#define MSG_FRIEND_TELL_AUTH                                   0x39EB
#define MSG_SHOW_CHILDREN_INFO                                 0x39EF
#define MSG_UPDATE                                             0x39F1
#define CMD_INTERNAL_AUTH                                      0x3A01
#define MSG_INTERNAL_AUTH                                      0x3A02
#define CMD_INTERNAL_AUTH_EX                                   0x3A04
#define MSG_UPGRADE_STATE                                      0x3A0F
#define CMD_APPLY_QUESTION_ITEM                                0x3A10
#define MSG_COIN_OPEN_MALL                                     0x3A11
#define CMD_ADMIN_PRINT_SCREEN                                 0x3A12
#define CMD_REQUEST_JOIN_RECRUITMENT                           0x3A14
#define CMD_CHAT_EX                                            0x3AE4
#define CMD_SET_DOUCDH_PET_POS                                 0x3AEE
#define MSG_CHARGE_VENDUE_FETCHED                              0x3DCD
#define MSG_CS_PW_VIEW_SUMMARY                                 0x3DCF
#define MSG_REFRESH_RECRUITMENT_INFO                           0x3DD9
#define MSG_VENDUE_OPEN_STORE                                  0x3DEF
#define MSG_TRADE_STORE_ITEM_RECORD                            0x3DF1
#define MSG_GENERAL_NOTIFY                                     0x3DF3
#define CMD_ADMIN_QUERY_REPORT_INFO                            0x3E00
#define MSG_SHOW_SIX_ANNI_INFO                                 0x3E03
#define CMD_GUARDS_DROP_GUARD                                  0x3E05
#define MSG_SET_OWNER_CHILD                                    0x3E07
#define MSG_PLAY_LIGHT_EFFECT                                  0x3E09
#define MSG_FIRST_ACTIONS_STATUS                               0x3E0B
#define CMD_CC_DO_ACTION                                       0x3E10
#define MSG_CHONGZHI_FANLI                                     0x3E11
#define CMD_DRAFT_REPORT                                       0x3E12
#define MSG_WISHING_RESULT                                     0x3E13
#define CMD_REPLY_CLIENT_PROCESS_INFO                          0x3E14
#define CMD_REQUEST_DECK_INFO                                  0x3E20
#define MSG_LC_LEAVE_AT_ONCE                                   0x3E21
#define CMD_HSSL_REQUEST_INFO                                  0x3E22
#define MSG_ADMIN_REQUEST_CURRENT_VER                          0x3E23
#define CMD_OPEN_ONLINE_MALL                                   0x3E24
#define MSG_START_MULTI_BONUS                                  0x3E25
#define CMD_OPER_SECURITY                                      0x3E26
#define MSG_COLLECT_DATA                                       0x3E27
#define CMD_MODIFY_CUSTOM_NPC_LIST                             0x3E28
#define MSG_CONVENANT_LIST                                     0x3E29
#define CMD_LOGOUT                                             0x3E2A
#define CMD_REQUEST_STALL_LIST                                 0x3E2C
#define CMD_CONFIRM_TAKE_PROPERTY                              0x3E30
#define MSG_WORLD_LEVEL_RS_INFO                                0x3E31
#define CMD_WORLD_LEVEL_RESET_SEAL                             0x3E32
#define MSG_SCORE_WISHING_INFO                                 0x3E33
#define CMD_WORLD_LEVEL_SEAL_INFO                              0x3E34
#define MSG_SCORE_WISHING_SORT                                 0x3E35
#define MSG_SCORE_WISHING_LIST                                 0x3E37
#define MSG_UPDATE_CHILD_SKILLS                                0x3FBD
#define MSG_MESSAGE_EX                                         0x3FFF
#define CMD_TRADE_CANCEL_SALE                                  0x4002
#define CMD_RESELECT_CHAR                                      0x4004
#define MSG_SHOP_CLOSE_MENU                                    0x4005
#define CMD_DROP_CHILD                                         0x4006
#define CMD_BOTTLE_REPLY                                       0x4008
#define CMD_OPER_PACKED_ITEM                                   0x400A
#define CMD_REQUEST_CARD_LIST                                  0x4010
#define CMD_GET_ACHIEVE_BONUS                                  0x4012
#define CMD_VIEW_CS_FIXTURES                                   0x4014
#define CMD_OPER_SIX_ANNI_INFO                                 0x401C
#define MSG_CC_PLAY_CARD                                       0x402D
#define CMD_REPAIR_FURNITURE                                   0x402E
#define MSG_EXCHANGE_ADD_PET                                   0x402F
#define CMD_EXCHANGE_CANCEL                                    0x4058
#define CMD_WISHING                                            0x4062
#define CMD_REJECT                                             0x4076
#define CMD_DROP_TASK                                          0x4078
#define CMD_CHANGE_UPGRADE_STATE                               0x407C
#define CMD_CHAT_GROUP_QUIT                                    0x408A
#define CMD_CHILD_CANCEL_FIGHT                                 0x40AE
#define CMD_MANAGE_CITY                                        0x40BA
#define CMD_HELL_DEALER                                        0x40C6
#define CMD_SET_XINFA_SKILL_STATUS                             0x40CE
#define MSG_EMOTE                                              0x40D1
#define CMD_FISH_SWALLOW_FINISH                                0x40D2
#define MSG_TRADE_DATA                                         0x40D3
#define CMD_OPEN_BUREAU                                        0x40D4
#define CMD_ARENA_HALL_FIGHT                                   0x40D6
#define MSG_CARD_COMPETE_SELECT_HERO                           0x40D7
#define CMD_REQUEST_BIND_SHITU_RELATION                        0x40D8
#define MSG_MAP_EFFECT_TIME                                    0x40D9
#define CMD_ASSIGN_ATTRIB                                      0x40E2
#define MSG_TOP_USER                                           0x40E3
#define CMD_REQUEST_BUILD_DECK                                 0x40E4
#define MSG_C_LEAVE_AT_ONCE                                    0x40E5
#define MSG_AUTO_COMBAT_TIME                                   0x40E6
#define MSG_C_REFRESH_PET_LIST                                 0x40E7
#define MSG_QISHAZHEN_XIANJIN                                  0x40E9
#define CMD_MA_L_ACCOUNT                                       0x40F0
#define MSG_CLIENT_CONNECTED                                   0x40F1
#define CMD_CFG_GET_SPA_INFO                                   0x40F2
#define MSG_CLIENT_DISCONNECTED                                0x40F3
#define CMD_MA_QUERY                                           0x40F4
#define MSG_CFG_SPA_INFO                                       0x40F5
#define CMD_MA_OPER                                            0x40F6
#define MSG_MA_L_AUTH                                          0x40F7
#define CMD_MA_SET                                             0x40F8
#define MSG_MA_QUERY_RESULT_PAGE                               0x40F9
#define MSG_MA_QUERY_EXPORT_DATA                               0x40FB
#define MSG_MA_QUERY_FIELD_NAMES                               0x40FD
#define MSG_MA_QUERY_RESULT_END                                0x40FF
#define CMD_ENTER_ARENA                                        0x4100
#define MSG_SHOP_VIEW_SALE_LOG                                 0x4101
#define CMD_TAKEOUT_GODBOOK                                    0x4102
#define MSG_FRIEND_UPDATE_PARTIAL                              0x4103
#define CMD_C_END_ANIMATE                                      0x4104
#define MSG_SDDH_OPER                                          0x4105
#define CMD_REQUEST_PROPERTY_BIND                              0x4106
#define MSG_C_UPDATE_STATUS                                    0x4107
#define CMD_CLOSE_REMOTE_STORE                                 0x4108
#define MSG_GET_ACTIVITY_CONTENT                               0x4109
#define CMD_TELL                                               0x410A
#define CMD_BUY_ECARD                                          0x4114
#define CMD_SUBMIT_CONFIRM_DLG                                 0x4116
#define CMD_TRADE_FAVORITE                                     0x4118
#define CMD_PROMPT_BEFORE_LEAVE                                0x4120
#define MSG_C_ACCEPTED_COMMAND                                 0x4121
#define CMD_CHILD_ENTER_HALL                                   0x4122
#define MSG_TRADE_VIEW_LOG                                     0x4123
#define CMD_LOGIN                                              0x4124
#define MSG_RECRUIT_INFO                                       0x4125
#define CMD_QUERY_UPGRADE_CASH_EX                              0x4126
#define MSG_FISHING_UPDATE_SUPER_WEAPON                        0x4127
#define MSG_COMMON_CONVENANT_LIST                              0x4129
#define MSG_FISHING_GAME_RESULT                                0x412B
#define CMD_MOVE_TO                                            0x4130
#define CMD_LOCAL_ENCRYPT_LOCKED                               0x4132
#define MSG_MA_OPER_RESULT                                     0x41F1
#define MSG_MA_REPORT_XIUXING                                  0x41F3
#define MSG_MA_REPORT_FUMO                                     0x41F5
#define MSG_MA_REPORT_SIGNATURE                                0x41F7
#define MSG_MA_REPORT_DOUBT_ACCOUNT                            0x41F9
#define MSG_REPORT_ILLEGAL_MESSAGE                             0x41FB
#define MSG_REPORT_PROCESS_INFO                                0x41FD
#define MSG_MAP_EFFECT_TIME_EX                                 0x4225
#define CMD_LEARN_ZHENFA                                       0x4226
#define MSG_LC_SANDGLASS                                       0x4227
#define CMD_SWITCH_GROUP_EX                                    0x4228
#define MSG_GET_DAILY_LOOP                                     0x4229
#define CMD_FISHING_CHANGE_WEAPON                              0x4230
#define MSG_L_SERVER_LIST                                      0x4355
#define MSG_L_SERVER_RESULT                                    0x4359
#define MSG_CFG_SERVER_LIST                                    0x435F
#define CMD_L_GET_CHARGE_RESULT                                0x4362
#define MSG_VERIFY_CODE_RESULT                                 0x4367
#define CMD_L_REQUEST_IP_CARD                                  0x436C
#define MSG_L_CHARGE_INFO                                      0x4377
#define MSG_L_CHARGE_HISTORY                                   0x4379
#define MSG_L_QUERY_FILL_INFO                                  0x4381
#define CMD_L_QUERY_FILL_INFO                                  0x4382
#define CMD_L_COST_FILL_INFO                                   0x4384
#define MSG_L_ADD_FILL_INFO                                    0x4385
#define CMD_L_ADD_FILL_INFO                                    0x4386
#define MSG_L_ECARD_BUSINESS                                   0x4387
#define CMD_L_ECARD_BUSINESS                                   0x4388
#define MSG_READ_CONTENT                                       0x4401
#define MSG_GET_COUNT                                          0x4402
#define CMD_GET_COUNT                                          0x4403
#define MSG_MUD_COMBAT_END                                     0x4405
#define MSG_CFMDLG_OPEN                                        0x4407
#define CMD_GET_CATALOG                                        0x4408
#define CMD_CREATE_GID                                         0x440A
#define CMD_GET_GID                                            0x440C
#define CMD_UPDATE_GID                                         0x440E
#define CMD_GET_GID_INFO                                       0x4410
#define CMD_GET_CATALOG_EX                                     0x4412
#define CMD_DELETE_GID                                         0x4414
#define MSG_GOT_GID_INFO                                       0x4417
#define CMD_GET_GID_ARR                                        0x4418
#define MSG_GOT_GID_ARR                                        0x4419
#define CMD_READ_LOG                                           0x441A
#define CMD_OPEN_AUTH_ALL_PROTECT                              0x4420
#define MSG_TREASURE_TRADING_OPER                              0x4423
#define CMD_CFG_READ_ETC                                       0x4470
#define CMD_CFG_WRITE_ETC                                      0x4474
#define MSG_MANAGE_HOUSE                                       0x49F7
#define CMD_SET_SAFE_STATE                                     0x4AF2
#define CMD_NOTIFY_SEND_WEIBO                                  0x4AF4
#define MSG_VOTE_KICKOUT_RAID                                  0x4DE9
#define MSG_QUERY_UPGRADE_RATE                                 0x4DEB
#define MSG_ADMIN_VIEW                                         0x4DF7
#define MSG_BOTTLE_REFRESH                                     0x4E01
#define MSG_ADMIN_CARD_BONUS_INFO                              0x4E07
#define MSG_GET_SAFE_CONTROL_CONTENT                           0x4E08
#define MSG_LC_ACCEPT_HIT                                      0x4E11
#define MSG_CC_UPDATE_HERO_INFO                                0x4E13
#define MSG_GET_FRIEND_VERIFY                                  0x4E1D
#define MSG_ADMIN_QUERY_ACCOUNT                                0x4E1F
#define MSG_LOCAL_ENCRYPT_RESULT                               0x4E23
#define MSG_C_END_ACTION                                       0x4E35
#define MSG_USER_HOTKEYS                                       0x4FF3
#define CMD_OPERATE_PET_STORE                                  0x5000
#define MSG_LC_CHAR_DIED                                       0x5001
#define CMD_VIEW_STALL                                         0x5002
#define MSG_START_MATCH_CARD_GAME                              0x5003
#define CMD_EXCHANGE_OK                                        0x5004
#define CMD_GET_ORDER_RANK                                     0x5005
#define CMD_RESELECT_VERIFY_CODE                               0x5006
#define MSG_CHALLENGE_MATCH_TOP                                0x5007
#define CMD_CHILD_SKILL_LEVEL_UP                               0x5008
#define MSG_RESELECT_CHAR                                      0x5009
#define CMD_OPEN_MUSIC                                         0x500A
#define CMD_START_TAOTAOTAO_RUN                                0x500C
#define CMD_BOX_ADVENTRUE_OPEN_INTERFACE                       0x500E
#define CMD_PURCHASE_FETCH_ITEM                                0x5010
#define CMD_RECRUIT_APPRENTICE                                 0x5012
#define CMD_EVERYONE_PK_OPER                                   0x5014
#define CMD_PLACE_BOX                                          0x5016
#define CMD_GET_INSTRUCTION_BONUS                              0x5018
#define CMD_BUY_CARD                                           0x506E
#define CMD_TREASURE_TRADING_OPER                              0x5088
#define CMD_VIEW_CS_MATCHES                                    0x508A
#define CMD_GOODS_SELL                                         0x508C
#define CMD_ADD_BUREAU_PAGE                                    0x50A2
#define CMD_CASUAL_OPER_GAME                                   0x50A4
#define CMD_SYS_PURCHASE_VIEW_HISTORY                          0x50A6
#define CMD_ACTIVITY_OPER                                      0x50A8
#define MSG_NOTIFICATION                                       0x5101
#define MSG_INVENTORY                                          0x5103
#define MSG_CLIENT_MOVE_NPC_POS_INFO                           0x5105
#define CMD_CD_WISHING_START                                   0x5106
#define CMD_CHECK_SERVER                                       0x5108
#define MSG_OPEN_ONLINE_MALL                                   0x5109
#define CMD_SET_SAFE_MECHANISM                                 0x510A
#define CMD_TRADE_ADD_GOODS                                    0x5112
#define CMD_WUDAO_ADD_ATTRIB_REP                               0x5202
#define MSG_C_WAIT_COMMAND                                     0x5203
#define MSG_L_AUTH                                             0x5351
#define CMD_L_CHANGE_PASSWORD                                  0x535C
#define MSG_L_CHARGE_RESULT                                    0x5361
#define CMD_L_VERIFY_PASSWORD                                  0x536A
#define MSG_L_REQUEST_IP_CARD                                  0x536B
#define MSG_L_REQUEST_COUPON                                   0x536D
#define CMD_L_REQUEST_COUPON                                   0x536E
#define CMD_L_COST_COUPON                                      0x5370
#define MSG_L_CARD_INFO                                        0x5375
#define MSG_L_QUERY_CARD                                       0x537F
#define MSG_L_COST_FILL_INFO                                   0x5383
#define CMD_READ_PET_DEPOSIT                                   0x5400
#define MSG_READ_PET_DEPOSIT                                   0x5401
#define CMD_WRITE_CONTENT                                      0x5402
#define CMD_WRITE_PET_DEPOSIT                                  0x5404
#define MSG_WRITE_PET_DEPOSIT                                  0x5405
#define CMD_CREATE_CONTENT                                     0x5406
#define CMD_CREATE_PET_DEPOSIT                                 0x5408
#define MSG_CREATE_PET_DEPOSIT                                 0x5409
#define CMD_MOVE_CONTENT                                       0x54A0
#define MSG_MOVED_CONTENT                                      0x54A1
#define CMD_FORCE_WRITE_LOG                                    0x54A2
#define MSG_L_OPER_PROTECT_RANGE                               0x5501
#define CMD_L_OPER_PROTECT_RANGE                               0x5502
#define MSG_L_AUTH_ALL_PROTECT                                 0x5503
#define CMD_L_AUTH_ALL_PROTECT                                 0x5504
#define CMD_SHOP_AD_FILL                                       0x5505
#define CMD_MAKE_PILL                                          0x5506
#define CMD_LOG_PURCHASE                                       0x5510
#define CMD_SHOP_DO_BUSINESS                                   0x5E21
#define MSG_TAOTAOTAO_OPEN                                     0x5FB9
#define MSG_NOTICE_ADD_ATTRIB                                  0x6001
#define CMD_ADMIN_TIME_BROADCAST                               0x6002
#define MSG_GET_CLIENT_PROCESS_INFO                            0x6003
#define CMD_SHOP_QUERY_COUNTER                                 0x6004
#define CMD_VENDUE_CAN_VENDUE                                  0x6005
#define MSG_CLOSE_STORE                                        0x6007
#define MSG_IMAGE_TIPS                                         0x6009
#define MSG_GUARDS_REFRESH                                     0x600A
#define MSG_WARCRAFT_REFRESH                                   0x600B
#define MSG_CHAT_GROUP_LOG                                     0x600D
#define CMD_SET_PASS                                           0x6010
#define MSG_MOUNT_INBORN_STONE_LIST                            0x6011
#define CMD_L_QUERY_COIN                                       0x6372
#define MSG_L_QUERY_COIN                                       0x6373
#define CMD_L_COST_COIN                                        0x6374
#define MSG_L_COST_COIN                                        0x6375
#define CMD_L_PRECOST_COIN                                     0x6376
#define MSG_L_PRECOST_COIN                                     0x6377
#define CMD_L_ADD_COIN                                         0x6378
#define MSG_L_ADD_COIN                                         0x6379
#define CMD_L_LOG_COST_COIN                                    0x637A
#define MSG_L_MANAGE_VIP_SMS                                   0x637B
#define CMD_L_MANAGE_VIP_SMS                                   0x637C
#define MSG_L_SEND_VIP_SMS                                     0x637D
#define CMD_L_SEND_VIP_SMS                                     0x637E
#define CMD_L_DO_ALIPAY_RECHARGE                               0x6380
#define MSG_L_DO_ALIPAY_RECHARGE                               0x6381
#define CMD_WINDOW_SIZE_CHANGE                                 0x6382
#define CMD_LC_DO_ACTION                                       0x6384
#define CMD_L_MANAGE_MP_DEVICE                                 0x6386
#define MSG_L_MANAGE_MP_DEVICE                                 0x6387
#define CMD_L_NOTICE_LEVEL_UP                                  0x6388
#define CMD_LOG_WISHING                                        0x638A
#define CMD_L_NOTICE_LOGIN                                     0x638C
#define CMD_L_GET_CREDIT_COINS                                 0x638E
#define MSG_L_GET_CREDIT_COINS                                 0x638F
#define CMD_ACCEPT_CONVENANT                                   0x6400
#define MSG_CLOSE_EXCHANGE                                     0x7001
#define CMD_SHOW_SALE_LOG                                      0x7002
#define MSG_C_STATUS_IMPROVEMENT                               0x7003
#define CMD_CARD_COMPETE_DRAW_BONUS                            0x7004
#define MSG_DOUCDH_INFO                                        0x7005
#define CMD_SET_CHILD_NAME                                     0x7006
#define MSG_SET_PENNANT_SKILL                                  0x7007
#define CMD_APPRENTICE_LIST                                    0x7008
#define MSG_RESOURCE_PARTY_LIST                                0x7009
#define CMD_ADMIN_REQUEST_MAP_LIST                             0x700A
#define MSG_EXCHANGE_ADD_MONEY                                 0x700B
#define CMD_TAKE_CHILD_ITEM_UP                                 0x700C
#define CMD_ASK_FIELDS                                         0x700E
#define MSG_RECORD_MESSAGE                                     0x7010
#define CMD_OPEN_MENU_2                                        0x7012
#define CMD_SET_RECOMMEND_ATTRIB                               0x7014
#define CMD_RESET_APPELLATION_TASK_MODE                        0x7016
#define CMD_REQUEST_TAKE_PROPERTY                              0x7018
#define MSG_ADMIN_LIST_RECORD_INFO                             0x701A
#define CMD_DRAFT_VOTE                                         0x701C
#define CMD_DECORATE_SET                                       0x701E
#define CMD_START_MULTI_BONUS                                  0x70A6
#define CMD_PARTY_REQUEST_LIST                                 0x70A8
#define CMD_CD_WISHING_RATE                                    0x70AA
#define CMD_REQUEST_COMPOSE_CARD                               0x70AC
#define CMD_TEAM_REQUEST_TASK                                  0x70AE
#define MSG_UPDATE_CHILDREN                                    0x7101
#define MSG_ITEM_APPEAR                                        0x7103
#define MSG_AUTO_MOVE_TO                                       0x7105
#define MSG_LC_WAIT_COMMAND                                    0x7107
#define CMD_REQUEST_COMBAT_DECK_LIST                           0x7E02
#define CMD_SYS_PURCHASE_VIEW_ORDERS                           0x7E04
#define MSG_SYS_PURCHASE_ORDERS                                0x7E05
#define CMD_COIN_DEAL_ORDER                                    0x7E06
#define MSG_UPDATE_INV_AMOUNT                                  0x7E07
#define CMD_SHIFT                                              0x7E08
#define MSG_TOP_BALLOT                                         0x7E09
#define CMD_TEAM_MATCH_SEEK                                    0x7E0A
#define MSG_CHAT_GROUP_INFO                                    0x7FEB
#define MSG_DECOMPOSE_CARD_SUCCEED                             0x7FED
#define MAGIC_NUMBER                                           0xAB7932CF
#define CMD_CC_SWITCH_CARD                                     0xCF02
#define CMD_END_CHALLENGE                                      0xCF04
#define CMD_ADMIN_KICKOFF                                      0xCF06
#define CMD_PARTY_SEND_MESSAGE                                 0xCF08
#define CMD_REPLY_FRIEND_VERIFY                                0xCF0A
#define CMD_FINISH_INTERFACE_GUIDE                             0xCF0C
#define CMD_LOG_LEVEL_UP                                       0xDF08
#define CMD_SET_IMAGE                                          0xE001
#define CMD_VIEW_COUPON_SHOW                                   0xE002
#define MSG_EXITS                                              0xE003
#define CMD_SHOW_UPGRADE_EQUIP_ITEMS                           0xE004
#define CMD_MOUNT_INBORN_STONE_LIST                            0xEEEE
#define MSG_C_REFRESH_CHILD_LIST                               0xEEEF
#define MSG_RESOURCE_MATCH_SCORE                               0xF00D
#define MSG_CC_START_COMBAT                                    0xF029
#define MSG_LC_MANA_DELTA                                      0xF059
#define MSG_APPEAR                                             0xF05D
#define MSG_EXISTED_CHAR_LIST                                  0xF061
#define MSG_ADMIN_QUERY_PLAYER_PROCESS                         0xF067
#define MSG_FRIEND_DRINK_INFO                                  0xF069
#define MSG_GBUYING_OPEN_INTERFACE                             0xF071
#define MSG_ZHAOJILING                                         0xF073
#define MSG_MINERALS_IN_MAP                                    0xF075
#define CMD_SET_AUTO_MSG                                       0xF077
#define MSG_ARENA_HALL_ME_INFO                                 0xF095
#define MSG_NEWBIE_FIRST_ACTION                                0xF097
#define CMD_AUTO_COMBAT_TIME                                   0xF0A1
#define MSG_ONLINEMALL_CARD_PACKAGES                           0xF0A3
#define MSG_ADMIN_FETCH_PICS                                   0xF0A5
#define MSG_TEAM_REQUEST_TASK                                  0xF0A7
#define CMD_ARENA_HALL_REQUEST_RD                              0xF0A8
#define MSG_C_FRIENDS                                          0xF0A9
#define MSG_SET_INTERFACE_STATUS                               0xF0AB
#define MSG_WEDDING_OPEN_LIST                                  0xF0AD
#define MSG_INTERSVR_RPC_RET                                   0xF0AF
#define MSG_EXECUTE_SCRIPT                                     0xF0B1
#define MSG_SET_OWNER                                          0xF0B3
#define MSG_GS_WORLD_CHANNEL                                   0xF0B5
#define MSG_STALL_PET_INFO                                     0xF0B7
#define MSG_FRIEND_UPDATE_LISTS                                0xF0C1
#define CMD_ADD_DUNWU_NIMBUS                                   0xF0C2
#define MSG_SEARCH_RESULT_END                                  0xF0C3
#define CMD_REFRESH_MOUNT_ATTRIB                               0xF0C4
#define MSG_GONGCHENG_RANK                                     0xF0C5
#define CMD_CC_SURRENDER                                       0xF0C6
#define MSG_UPDATE_X_USER                                      0xF0C7
#define CMD_WORKSHOP_BUY_ORDER                                 0xF0C8
#define MSG_CLEAN_REQUEST                                      0xF0C9
#define CMD_CHAT_GROUP_REQUEST_INFO                            0xF0CA
#define CMD_MOBILE_OPER_TASK                                   0xF0CC
#define MSG_TREASURE_POS                                       0xF0CD
#define CMD_ASSIGN_UPGRADE                                     0xF0CE
#define MSG_WORKSHOP_RELEASE_ORDER                             0xF0CF
#define CMD_SYS_PURCHASE_SELL                                  0xF0D0
#define CMD_GET_RANDOM_BOX_AWARD                               0xF0D2
#define CMD_ARENA_LOOK_ON                                      0xF0D4
#define MSG_LONG_MESSAGE                                       0xF0D5
#define MSG_ADMIN_QUERY_REPORT_INFO                            0xF0D7
#define MSG_ADMIN_SNIFF_AT                                     0xF0D9
#define CMD_NOMINATE_INFO                                      0xF0DB
#define CMD_CHARGE_VENDUE_FETCH                                0xF0DD
#define MSG_REQUEST_SERVER_STATUS                              0xF0DF
#define MSG_CC_INIT_COMBAT                                     0xF0E1
#define MSG_C_COMMAND_ACCEPTED                                 0xF0E3
#define MSG_REQUEST_ECARD_INFO                                 0xF0E5
#define MSG_ENVELOPE_NEW                                       0xF0E7
#define MSG_REPLY_FRIEND_VERIFY                                0xF0E9
#define MSG_EXCHANGE_OPEN                                      0xF0ED
#define MSG_CUR_BUREAU_PAGE                                    0xF0F1
#define MSG_SET_SETTING                                        0xF0FF
#define MSG_LOGOUT                                             0xF101
#define MSG_TEAM_SEEK_INFO                                     0xF111
#define MSG_TOWER_DEFENSE_RESULT                               0xF301
#define CMD_L_CREATE_ACC                                       0xF354
#define CMD_L_CHANGE_ACC_INFO                                  0xF364
#define MSG_L_GET_ACC_INFO                                     0xF365
#define MSG_MASTER_INFO                                        0xF367
#define MSG_SHOP_UPDATE_RUNNING                                0xF369
#define CMD_FINISH_COMBAT_GUIDE                                0xF370
#define MSG_ARENA_HALL_CHAR_INFO                               0xF371
#define CMD_STALL_INPUT_PRICE                                  0xF372
#define MSG_SHOW_NEWBIE_GIFT_BTN                               0xF373
#define CMD_AUTH_VIP_SMS                                       0xF374
#define CMD_STORE                                              0xF376
#define MSG_LOG_MESSAGE                                        0xF501
#define CMD_LOG_CSC_COMBAT                                     0xF502
#define MSG_LOG_LOGIN                                          0xF503
#define MSG_L_GET_VOTE_RESULT                                  0xF504
#define CMD_L_GET_VOTE_RESULT                                  0xF505
#define CMD_LOG_CSL_COMBAT                                     0xF506
#define CMD_LOG_ERRAND                                         0xF507
#define CMD_LOG_CSL_MATCH                                      0xF508
#define CMD_LOG_CAMPAIGN                                       0xF509
#define CMD_LOG_NUMMARY                                        0xF50A
#define CMD_LOG_COLLECT_DATA                                   0xF50B
#define CMD_LOG_PROPERTY_BIND                                  0xF50C
#define CMD_LOG_APPLY                                          0xF50D
#define CMD_LOG_SERVER_STAT                                    0xF50E
#define MSG_LOG_COST_COIN                                      0xF50F
#define CMD_PURCHASE_MATCH_ORDER                               0xF510
#define MSG_HSSL_INFO                                          0xF511
#define CMD_REQUEST_ENTER_X_GAME                               0xF512
#define MSG_REQUEST_TAKE_PROPERTY                              0xF513
#define CMD_LOG_ITEM_TRANSFER                                  0xF514
#define MSG_SUBMIT_CONFIRM_DLG                                 0xF515
#define CMD_LOG_MATERIAL                                       0xF516
#define CMD_LOG_BLOCK                                          0xF518
#define CMD_LOG_COIN_ORDER                                     0xF51A
#define CMD_LOG_PROPERTY_RECALL                                0xF51C
#define CMD_UPDATE_LOG                                         0xF51E
#define MSG_UPDATE_LOG                                         0xF51F
#define CMD_LOG_GM                                             0xF520
#define CMD_LOG_PET                                            0xF521
#define CMD_LOG_TASK                                           0xF522
#define MSG_LOG_SWITCH                                         0xF523
#define CMD_LOG_MONEY                                          0xF524
#define MSG_LOG_SAFE                                           0xF525
#define CMD_LOG_ENCOURAGE_DETAIL                               0xF526
#define CMD_LOG_DEBUG                                          0xF528
#define CMD_LOG_COIN_CONSUME                                   0xF530
#define MSG_GS_STATUS_LIST                                     0xF601
#define CMD_LOG_EQUIPMENT                                      0xF604
#define CMD_LOG_PET_LEVEL_UP                                   0xF606
#define CMD_LOG_BEHAVIOR                                       0xF608
#define CMD_LOG_GBUY_ACTION                                    0xF60A
#define CMD_LOG_ANTIADDICTION                                  0xF60C
#define CMD_LOG_IMPORTANT_PET                                  0xF610
#define CMD_LOG_SALE                                           0xF612
#define CMD_LOG_TAO_TASK                                       0xF614
#define CMD_LOG_TAO_STAT                                       0xF616
#define CMD_LOG_AUTO_TALK_STAT                                 0xF618
#define CMD_LOG_ACTIVITY                                       0xF61A
#define CMD_LOG_ZQ_ADD_POINT                                   0xF620
#define CMD_LOG_ZQ_CARD                                        0xF622
#define CMD_LOG_ZQ_COMBAT                                      0xF624
#define CMD_LOG_ZQ_COST_POINT                                  0xF626
#define CMD_LOG_ZQ_ERRAND                                      0xF628
#define CMD_LOG_ZQ_LOGIN                                       0xF62A
#define CMD_LOG_ZQ_LV_UP                                       0xF62C
#define CMD_LOG_COMBAT_IN_MAP                                  0xF62E
#define CMD_LOG_REPORT                                         0xF630
#define CMD_LOG_CHAT                                           0xF632
#define CMD_LOG_EXCHANGE_APPRAISE                              0xF634
#define CMD_LOG_CHANNEL                                        0xF636
#define CMD_LOG_SYSTEM_MONITOR                                 0xF638
#define CMD_L_AUTH_REFRESH_PIC                                 0xF70A
#define CMD_L_TEMP_BLOCK_ACCOUNT                               0xF70C
#define CMD_CTC_TO_CTA                                         0xF901
#define MSG_CTA_TO_CTC                                         0xF902
#define CMD_CTA_TO_SERVER                                      0xF903
#define MSG_SERVER_TO_CTA                                      0xF904
#define MSG_SERVER_TO_CTA_EX                                   0xF905
#define CMD_SYS_PURCHASE_BUY                                   0xF906
#define MSG_DOUBLE_BONUS_TIME                                  0xF907
#define CMD_BUTTON_DOUBLE_BONUS                                0xF908
#define MSG_C_LIFE_DELTA                                       0xF909
#define MSG_SERVICE_LOG                                        0xF90B
#define CMD_SUBMIT_ICON                                        0xF9C1
#define MSG_REPLY_ECHO                                         0xF9C3
#define MSG_CHANNEL_STATUS                                     0xF9C5
#define MSG_BROADCAST_COMBAT_LIST                              0xF9C7
#define MSG_LOGIN_DONE                                         0xF9D3
#define MSG_ADMIN_CHANNEL_INFO                                 0xF9D5
#define MSG_FINISH_SORT_PACK                                   0xF9D7
#define MSG_RAISE_RESULT                                       0xF9DB
#define MSG_TASK_ITEM                                          0xF9DD
#define MSG_PET_STORE                                          0xF9E1
#define MSG_SELL_DIALOG                                        0xF9F9
#define MSG_TASK_TRACE                                         0xF9FB
#define MSG_UPDATE_IMPROVEMENT                                 0xFA01
#define MSG_ADMIN_FESTIVAL_INFO                                0xFA03
#define MSG_REPAIR_FURNITURE                                   0xFA05
#define MSG_LAOBING_BUSI                                       0xFA07
#define CMD_VENDUE_TRY_VENDUE                                  0xFA09
#define CMD_GET_CLIENT_PERFORMANCE                             0xFA0D
#define MSG_REQUEST_ASKTAO_SHOW                                0xFA0F
#define CMD_EXCHANGE_OPEN_EX                                   0xFA11
#define CMD_SHOP_REQ_RUNNINGS                                  0xFA13
#define MSG_CITYWAR_OPER                                       0xFA15
#define MSG_ASKTAO_SHOW_BOX_LIST                               0xFA17
#define MSG_SYS_PURCHASE_HISTORY                               0xFA19
#define MSG_REFRESH_GARDEN_INSTRUCTION                         0xFA1B
#define MSG_TITLE                                              0xFA1D
#define MSG_REQUEST_ENTER_X_GAME                               0xFA1F
#define CMD_SHOP_MOVE_MERCH                                    0xFA21
#define MSG_PREPARE_CHANGE_UPGRADE                             0xFA23
#define MSG_C_MANA_DELTA                                       0xFA25
#define MSG_OPEN_DAILY_LOOP                                    0xFA27
#define MSG_LC_DELAY                                           0xFA2B
#define MSG_SHOP_ACK_VENDUES                                   0xFA2C
#define MSG_RANDOM_USER_NAME                                   0xFA2D
#define MSG_LC_END_LOOKON                                      0xFA2F
#define MSG_LC_CALLBACK_CHILD                                  0xFA31
#define MSG_C_ADD_OPPONENT                                     0xFA33
#define MSG_OPEN_YEAR_INSIDER_INTERFACE                        0xFA35
#define MSG_ACTIVITY_OPER                                      0xFA37
#define MSG_CCS_POLLING                                        0xFA39
#define MSG_LC_CHAR_OFFLINE                                    0xFA3B
#define CMD_PUTIN_ITEM                                         0xFA3C
#define MSG_ADMIN_SCRIPT_INFO                                  0xFA3D
#define MSG_CC_SHOW_CARDS                                      0xFA3F
#define MSG_LC_ADD_CHILD                                       0xFA41
#define CMD_SEND_ENVELOPE                                      0xFA42
#define CMD_SHOP_CHANGE_INFO                                   0xFA43
#define MSG_HELL_DEALER                                        0xFA45
#define CMD_ARENA_ROOM_LEAVE                                   0xFA46
#define MSG_ASSERVICE_TIME                                     0xFA47
#define CMD_TRADE_VIEW_LOG                                     0xFA48
#define CMD_ADD_GODBOOK_DIRECT                                 0xFA49
#define CMD_CSC_VIEW_COMPETE_SORT                              0xFA4A
#define MSG_DRAFT_STAGE_RANK                                   0xFA4B
#define CMD_CARD_DROP_TASK                                     0xFA4C
#define CMD_REFRESH_SERVICE_LOG                                0xFAE0
#define MSG_OPEN_NOMINATE                                      0xFAE2
#define MSG_TELEPORT_EX                                        0xFAE4
#define MSG_NOTICE_GET_ITEM                                    0xFAE5
#define CMD_APPLY_CARD                                         0xFAF8
#define MSG_FINGER                                             0xFAF9
#define CMD_LGXY_CHALLENGE                                     0xFAFA
#define MSG_VIEW_COUPON_SHOW                                   0xFAFB
#define CMD_TRADE_SALE                                         0xFAFC
#define CMD_SET_SIGNATURE                                      0xFAFD
#define MSG_UPDATE_LEAVE_TEMP_TEAM_LIST                        0xFAFF
#define MSG_BUY_ASKTAO_SHOW_RESULT                             0xFB01
#define MSG_CHAT_GROUP_LIST                                    0xFC01
#define MSG_GET_PHONE_AUTH_BIND_TIME                           0xFC03
#define MSG_SHOW_VERIFY_CODE                                   0xFC05
#define CMD_CLOSE_STORE                                        0xFCFE
#define MSG_PLAY_MUSIC                                         0xFCFF
#define MSG_APPLY_ON_ITEM                                      0xFD01
#define MSG_DRAFT_REPLY_IMAGE_ID                               0xFD03
#define MSG_STUDY_CHILD_SKILLS                                 0xFD05
#define MSG_COLLECT_CARD_BASIC_INFO                            0xFD07
#define MSG_SEND_ICON                                          0xFD08
#define CMD_VENDUE_CANCEL                                      0xFD09
#define CMD_UNEQUIP                                            0xFD0A
#define MSG_GUESS_LOCK_RESULT                                  0xFD11
#define CMD_MANAGE_MP_DEVICE                                   0xFD12
#define MSG_PRE_ASSIGN_ATTRIB                                  0xFD13
#define CMD_MODIFY_ITEM_REMARK                                 0xFD14
#define MSG_GS_SYSTEM_CHANNEL                                  0xFD15
#define CMD_OPER_MAGIC_PICTURE                                 0xFD16
#define MSG_CCS_REBOOT_RESULT                                  0xFD17
#define CMD_COIN_SUBMIT_ORDER                                  0xFD18
#define MSG_SHOP_SHOW_PET_COUNTER                              0xFD19
#define CMD_REFRESH_RECRUITMENT_INFO                           0xFD1A
#define MSG_PARTYWAR_OPER                                      0xFD1B
#define CMD_MOVE_CHILD_ITEM                                    0xFD1C
#define MSG_APPELLATION_LIST                                   0xFD1D
#define CMD_DOUBLE_BONUS_TIME                                  0xFD1E
#define MSG_CC_PLAY_ZHENFA_EFFECT                              0xFD1F
#define MSG_ADMIN_REFRESH_TIMER_INFO                           0xFD23
#define CMD_CHANGE_USER_STATE                                  0xFD24
#define MSG_LC_FRIENDS                                         0xFD25
#define CMD_START_PET_PRACTICE                                 0xFD26
#define MSG_BUY_FROM_MALL_RESULT                               0xFD27
#define CMD_END_RECRUITMENT                                    0xFD28
#define CMD_GUARDS_CHANGE_NAME                                 0xFD29
#define MSG_TEMP_LEAVE_PROMPT_CONFIG                           0xFD2B
#define CMD_ADMIN_SNIFF_AT                                     0xFD2C
#define CMD_ANTI_HACK_COLLECT                                  0xFD2E
#define CMD_GET_RAISE_BONUS                                    0xFD30
#define MSG_CARD_ACTIVITY_LIST                                 0xFD31
#define CMD_BOTTLE_OPEN                                        0xFD32
#define MSG_DRAFT_OPER                                         0xFD33
#define CMD_DROP_RANDOM_BOX_AWARD                              0xFD34
#define MSG_PURCHASE_ORDER_PROP                                0xFD35
#define CMD_APPLY_EX                                           0xFD36
#define MSG_CARD_TASK_PROMPT                                   0xFD37
#define CMD_SEARCH_STALL_GOODS                                 0xFD38
#define MSG_C_ACTION                                           0xFD39
#define CMD_QUERY_ACTIVITY                                     0xFD40
#define MSG_CARD_PACKAGE_BONUS                                 0xFD41
#define CMD_KILL                                               0xFD42
#define CMD_VENDUE_FETCH                                       0xFD43
#define MSG_IP_LOGIN                                           0xFD45
#define CMD_EVENT_FINISH                                       0xFD46
#define MSG_PURCHASE_ORDER_LIST                                0xFD47
#define CMD_SET_PORTABLE_CHILD                                 0xFD48
#define MSG_VENDUE_REFRESH_SYS                                 0xFD49
#define CMD_RENAME_STALL                                       0xFD4A
#define CMD_TELEPORT_EX                                        0xFD4B
#define MSG_DISCOVER_REFRESH_EVENT_INFO                        0xFD4C
#define CMD_FINISH_QUESTION                                    0xFD4E
#define MSG_GET_GOODS_FROM_TTS                                 0xFD51
#define CMD_GET_GOODS_FROM_TTS                                 0xFD52
#define MSG_LC_ADD_OPPONENT                                    0xFD53
#define CMD_SELL_TREASURE                                      0xFD54
#define MSG_X_SET_SETTING                                      0xFD55
#define CMD_SET_PET_NAME                                       0xFD56
#define MSG_SELL_TREASURE                                      0xFD57
#define CMD_ADMIN_SHUT_CHANNEL                                 0xFD58
#define CMD_DEVELOP_SKILL                                      0xFD60
#define MSG_CS_EXCHANGE                                        0xFD61
#define CMD_PUT_CHILD_ITEM_DOWN                                0xFD62
#define MSG_GONGCHENG_SCORE                                    0xFD63
#define CMD_GENERAL_NOTIFY                                     0xFD64
#define MSG_CARD_DAILY_TASK_UPDATED                            0xFD65
#define MSG_ONLINE_MALL_LIST                                   0xFD67
#define CMD_TEST_CS_LOGIN                                      0xFD68
#define MSG_OPEN_ASKTAO_SHOW_MALL                              0xFD69
#define CMD_FINISH_INTERFACE_STATUS                            0xFD6A
#define CMD_REVOKE_CONVENANT                                   0xFD6C
#define CMD_UPGRADE_BY_AMOUNT                                  0xFD70
#define MSG_RACE_INTERFACE_OPER                                0xFD71
#define CMD_MULTI_MOVE_TO                                      0xFD72
#define MSG_END_RAISE                                          0xFD73
#define CMD_ASSIGN_CHILD_ATTRIB                                0xFD74
#define CMD_L_UPDATE_RECHARGE_RETURN_CREDIT                    0xFD78
#define MSG_L_UPDATE_RECHARGE_RETURN_CREDIT                    0xFD79
#define CMD_BOTTLE_DIG                                         0xFD80
#define MSG_CS_SHIDAO_HISTORY                                  0xFD81
#define CMD_LUCKY_SIGN                                         0xFD82
#define MSG_ADMIN_LOGIN_NOTIFICATION                           0xFD83
#define CMD_PROMPT                                             0xFD84
#define MSG_GODBOOK_EFFECT_SUMMON                              0xFD85
#define CMD_EXCHANGE_APPRAISE                                  0xFD86
#define MSG_LGXY_INFO                                          0xFD91
#define MSG_CASUAL_ROOM_SEAT_INFO                              0xFD93
#define MSG_START_TAOTAOTAO_RUN                                0xFD95
#define CMD_AUTH_MATRIX_FOR_GS                                 0xFD97
#define CMD_L_UPDATE_FIRST_CREDIT_BONUS                        0xFD98
#define MSG_L_UPDATE_FIRST_CREDIT_BONUS                        0xFD99
#define CMD_L_SET_PHAH_BONUS_INFO                              0xFD9A
#define MSG_L_SET_PHAH_BONUS_INFO                              0xFD9B
#define CMD_LOG_APEX                                           0xFD9C
#define MSG_STRATEGY_WAR_SELECT_TEAM                           0xFD9D
#define CMD_LOG_MATERIAL1                                      0xFD9E
#define CMD_CLOSE_MENU                                         0xFDA0
#define CMD_GET_NAME_BY_GID                                    0xFDA2
#define CMD_CHECK_ROLE_ON_SALE                                 0xFDA4
#define MSG_CHECK_ROLE_ON_SALE                                 0xFDA5
#define CMD_CHANGE_ATTRIB_PLAN                                 0xFDA6
#define MSG_UPDATE_PET_INFO                                    0xFDA8
#define CMD_END_RAISE                                          0xFDAA
#define CMD_SET_REGION                                         0xFDAC
#define MSG_CASUAL_LEAVE_ROOM                                  0xFDB1
#define MSG_VENDUE_PERMIT                                      0xFDB3
#define MSG_OPEN_QIANKUNDAI                                    0xFDBB
#define MSG_PARTY_JOB_PRIVILEGE_LIST                           0xFDBD
#define MSG_CLIENT_DIALOG_NPC_POS_INFO                         0xFDCB
#define MSG_OPEN_ELIXIR                                        0xFDCD
#define MSG_CCS_SYSTEM_CHANNEL                                 0xFDD1
#define MSG_LEARN_MOUNT_INBORN_SKILLS                          0xFDD3
#define MSG_REFRESH_CARD                                       0xFDD5
#define MSG_COMBAT_DECK_LIST                                   0xFDDD
#define CMD_SHOP_FAVORITE                                      0xFDDF
#define MSG_MUD_COMBAT_START                                   0xFDE1
#define MSG_OPEN_CHILD_PRACTICE                                0xFDE3
#define MSG_ARENA_RANK_INFO                                    0xFDF9
#define MSG_FRIEND_ADD_CHAR                                    0xFDFB
#define MSG_C_CHAR_REVIVE                                      0xFFB5
#define MSG_GODBOOK_EFFECT_NORMAL                              0xFFB7
#define MSG_FISHING_UPDATE_MULTI_INFO                          0xFFB9
#define MSG_NEW_RAID_MEMBERS                                   0xFFBB
#define MSG_WISHING_START                                      0xFFC1
#define MSG_CHILD_INVENTORY                                    0xFFC3
#define MSG_OPEN_URL                                           0xFFC5
#define MSG_CITYWAR_SCORE                                      0xFFDB
#define CMD_START_CARD_INSTRUCTION                             0xFFDE
#define MSG_C_START_COMBAT                                     0xFFDF
#define MSG_RAISE_BONUS                                        0xFFE1
#define MSG_C_DELAY                                            0xFFE3
#define MSG_REFRESH_PET_GODBOOK_SKILLS                         0xFFE7
#define CMD_GET_SAFE_CONTROL_CONTENT                           0xFFF1
#define MSG_FISHING_FISH_MOVE                                  0xFFF5
#define MSG_CS_PW_VIEW_MATCHES                                 0xFFF7
#define MSG_C_SET_FIGHT_PET                                    0xFFF9
#define MSG_TAKE_PROPERTY_SUCC                                 0xFFFB
#define ALL_MESSAGES                                           0xFFFF

// --- Integer Constants (421) ---
#define SHENGSHI_XIANHUA_YU_TIME                               6
#define MOVE_TYPE_WATER                                        1
#define TIME_SDDH_PASSIVE_PROTECT                              5
#define TIME_CSL_ACTIVE_PROTECT                                2
#define CFG_TYPE_CC_SERVER                                     7
#define ANIMATION_TYPE_LAND_LOTUS                              4
#define TIME_DFTZ_PROTECT                                      11
#define OPER_GET_COUNT                                         7
#define CORP_TYPE_ZHENYING_ZHENGBA                             10
#define DLG_TYPE_PARTY_MSG                                     1
#define BONUS_TYPE_TITLE                                       13
#define CLIENT_TYPE_KUAIWAN                                    9
#define CLIENT_TYPE_TINY                                       2
#define OPERATE_OK                                             0
#define VALID_NAME_BAN_WORDS                                   -6
#define RESTORE_FAILED_BAD_DATA_FORMAT                         6
#define PRECOST_TYPE_TRANSFER                                  3
#define CHARGE_CMPAY_WEB_DIRECT                                6
#define TIME_CSL_PASSIVE_PROTECT                               3
#define TRANSFER_CHARGE_ITEM                                   3
#define XDBC_OK                                                0
#define CFG_TYPE_DBA                                           4
#define ITEM_ORIGIN_SHIDGC                                     7
#define MAP_EFFECT_HONGDITAN_COMMON                            7
#define CHARGE_GY_YIPAY_QUICK                                  19
#define TRANSFER_PET                                           2
#define BONUS_TYPE_NORMAL_EX                                   11
#define TASK_TYPE_STAR_1                                       0
#define CHARGE_GYAPP_CARD_DIRECT                               24
#define WG_FORBIDEN_XIUXING                                    6
#define LOG_TYPE_EXP                                           2
#define MANAGE_MP_OPER_BIND                                    1
#define CLIENT_TYPE_FULL_TINY                                  3
#define DAY_SHAXING_NOTICE_TIMES                               20
#define HOUR_SECS                                              3600
#define DANCE_TYPE_JTQF_2015                                   2
#define BANNER_SPRING_FESTIVAL                                 3
#define COST_PLATFORM_GBUYING                                  1
#define DEBUG                                                  0
#define GS_STATUS_FREE                                         2
#define CFG_TYPE_CLIENT                                        6
#define SERVER_STATUS_SHUTDOWN                                 1
#define YUEBING_TYPE_NATION_2014                               1
#define CHARGE_ALIQR_WEB_DIRECT                                22
#define STEP_MOVE_PARADING                                     2
#define LUOPAN_TYPE_YUANLSN                                    7
#define CANCEL_RESULT                                          0
#define WG_FORBIDEN_SHIJUE                                     7
#define LOG_TYPE_FD                                            15
#define LOG_TYPE_CASH                                          7
#define OPER_READ_LOG                                          20
#define RESTORE_FAILED_DBA_FAILED                              3
#define REPORT_ILLEGAL_SHABOSS                                 9
#define REPORT_ILLEGAL_DACL                                    2
#define ITEM_ORIGIN_TIANGX                                     11
#define ITEM_PROVENANCE_NO_CHARGE                              3
#define TYPE_SUMMER_VACATION_2012                              1
#define ANIMATION_TYPE_CHRISTMAS_2016                          8
#define FETCH_GROUP_BUYING_COIN                                4
#define TIME_ZYZB_PROTECT                                      10
#define XDBC_FALSE                                             0
#define REPORT_ILLEGAL_CHUB                                    3
#define MIN_SECS                                               60
#define CHARGE_YIPAY_WEB_DIRECT                                17
#define LUOPAN_TYPE_GOLDEN                                     1
#define BONUS_TYPE_CASH                                        9
#define VALID_NAME_ONLY_E_D                                    -4
#define MAP_EFFECT_XUEJING_COMMON                              10
#define OPER_ITEM_DROP                                         3
#define FLY_TYPE_LOTUS                                         1
#define WG_CABIN                                               5
#define WG_JAIL                                                2
#define GENERATED_MONEY                                        0
#define LOG_TYPE_PET                                           14
#define LOG_TYPE_ITEM                                          1
#define OPER_READ_PET_DEPOSIT                                  19
#define SERVER_STATUS_FAILED                                   4
#define PET_NEED_INTIMACY                                      3000
#define DAY_SECS                                               86400
#define COIN_LOCK_CLEAN_COST                                   2
#define CORP_TYPE_CS_PARTYWAR                                  5
#define CORP_TYPE_CITY_WAR                                     2
#define CORP_TYPE_PARTY_WAR                                    1
#define CHARGE_SZPAY_WEB_DIRECT                                11
#define FLY_TYPE_YUTIANSUO                                     2
#define CONFIRM_DLG_FUNC_NUM                                   2
#define FETCH_PURCHASE_CASH                                    3
#define TIME_ZYZ_ACTIVE_PROTECT                                8
#define BONUS_TYPE_MINOR_EX                                    12
#define BONUS_TYPE_NORMAL                                      1
#define CLIENT_TYPE_MOBILE_IOS                                 4
#define XDBC_CRITICAL                                          3
#define XDBC_SERVER_PORT                                       9527
#define ONE_YEAR                                               360
#define BONUS_TYPE_REPUTATION                                  6
#define USE_ITEM_LAST                                          2
#define LOG_TYPE_FAILURE                                       99
#define OPER_READ                                              1
#define ALLOW_VALID_COINPWD_DURATION                           600
#define BANNER_BONUS_LONGTENG_SHENGSHI                         1
#define BANNER_BONUS_NONE                                      0
#define BANNER_WEDDING                                         1
#define ANIMATION_TYPE_FLY_YUTIANSUO                           6
#define MAX_HUIYUAN_TIME                                       432000000
#define CHARGE_GY_INTEGRAL                                     1
#define DLG_SUB_TYPE_REQUEST_TL                                1
#define GENERATED_TAO                                          6
#define REPORT_ILLEGAL_SHAX                                    7
#define PRECOST_TYPE_CONSUME                                   1
#define CLIENT_TYPE_WEB                                        6
#define CHINESE_WIDTH                                          2
#define COIN_LOCK_CLEAN_ADD                                    3
#define CHARGE_BANK_WEB_DIRECT                                 10
#define BONUS_TYPE_PET                                         12
#define GENERATED_MARTIAL                                      7
#define XDBC_MAX_CLIENTS                                       1024
#define VALID_NAME_NEED_MIN_LEN                                -1
#define CFG_TYPE_CTA                                           11
#define LUOPAN_TYPE_SHENHUAN                                   5
#define BONUS_TYPE_EXP                                         1
#define MAX_CASH_LIMIT                                         2147483647
#define RELEASE                                                1
#define DELETE_WHEN_USER_NOT_FOUND                             3
#define NOT_ABS                                                0
#define MAP_EFFECT_QIXI_2013                                   3
#define CHARGE_GY_CARD_GAME_DIRECT                             3
#define TIME_ZYZ_PASSIVE_PROTECT                               9
#define TRANSFER_ITEM_OTHER                                    0
#define LOG_TYPE_SCORE                                         6
#define CFG_TYPE_TTS                                           16
#define MAP_EFFECT_TYPE_ANNIVERSARY                            2
#define CHARGE_SZPAY                                           13
#define WG_SILENCE                                             1
#define GIFT_TYPE_COIN                                         2
#define REPORT_ILLEGAL_FUM                                     5
#define BANNER_SHENGSHI_XIANHUA_YU                             9
#define ANIMATION_TYPE_FLY_LOTUS                               3
#define IMPORTANT_PET_TRANSFER                                 2
#define BONUS_TYPE_SPECIAL_EX                                  13
#define OPER_XG_GET_ACCOUNT                                    25
#define OPER_UPDATE_GID                                        14
#define OPER_DELETE_GID                                        12
#define XDBC_MONITOR                                           0
#define CFG_TYPE_GAME_SERVER                                   5
#define NEED_ABS                                               1
#define CHARGE_GYAPP_BALANCE                                   25
#define TRANSFER_CASH                                          1
#define LOG_TYPE_SILVER_COIN                                   2
#define PAY_ALL                                                1
#define GS_STATUS_BAD                                          1
#define ITEM_ORIGIN_SHENXXF                                    4
#define OPER_ITEM_TRADE                                        1
#define APPLY_QUOTA_SUCCESS                                    1
#define LUOPAN_TYPE_YINYANG                                    2
#define BANNER_APRIL_FOOL_2012                                 2
#define OPER_DEL_ANIMATION                                     0
#define OPER_ADD_ANIMATION                                     1
#define IMPORTANT_PET_GIVE                                     1
#define BONUS_TYPE_VOUCHER                                     10
#define LOG_TYPE_NICE                                          11
#define FALSE                                                  0
#define COIN_LOCK_CLEAN_CUSTOM                                 4
#define TASK_TYPE_STAR_2                                       1
#define ITEM_ORIGIN_DISX                                       12
#define MAP_EFFECT_COUPLET_COMMON                              11
#define TOTAL_FORBIDDEN_ATTRIB_NUM                             4
#define APPLY_QUOTA_SYS_FAILED                                 -1
#define DBA_FAILED                                             -2
#define OPER_CREATE_PET_DEPOSIT                                17
#define CFG_TYPE_CSA                                           14
#define CFG_TYPE_MERGE_SERVER                                  12
#define ECHO_MISS_TIMES                                        3
#define REPORT_TYPE_CHEATER                                    2
#define TIMESTAMP_SHIFT_W                                      288000
#define COIN_LOCK_CLEAN_PRECOST                                1
#define MAP_EFFECT_SUMMER_VACATION_2014                        5
#define GENERATED_VOUCHER                                      2
#define RECLAIMED_MONEY                                        1
#define RESTORE_FAILED_BAD_RESTORE_OBJECT                      7
#define ECHO_LONG_DEAD_INTERVAL                                300
#define MAP_EFFECT_QIXI_COMMON                                 9
#define BANNER_PRIORITY_HIGH                                   255
#define MIN_CHANGE_COLOR_ICON                                  10000000
#define OPER_WRITE                                             2
#define REPORT_ILLEGAL_HANH                                    1
#define SALE_LOG_STALL                                         1
#define DLG_SUB_TYPE_DEFAULT                                   0
#define TRANSFER_COMBINED_ITEM                                 5
#define BONUS_TYPE_SPECAIL                                     3
#define CLIENT_TYPE_DOUYOU                                     8
#define MAPPING_KEY                                            0
#define VALID_NAME_ONLY_E_C_D                                  -3
#define DANCE_TYPE_WDTYC                                       1
#define APPLY_QUOTA_FAILED                                     0
#define PER_DAY_TIME                                           86400
#define BONUS_TYPE_POT                                         3
#define OPER_GET_SYS_ENVELOPE                                  27
#define TYPE_SUMMER_VACATION_2014                              3
#define CORP_TYPE_DUOBAOZHAN                                   8
#define CORP_TYPE_CS_LEAGUE                                    4
#define BANNER_ANNIVERSARY                                     6
#define TIME_SDDH_ACTIVE_PROTECT                               4
#define BONUS_TYPE_FAILURE                                     102
#define OPERATE_FAILED                                         -1
#define CFG_TYPE_SPA                                           9
#define WEEK_SECS                                              604800
#define CORP_TYPE_SIMINGPAI                                    9
#define BANNER_LONGTENG_SHENGSHI                               5
#define BONUS_TYPE_NICE                                        8
#define PAY_CASH                                               3
#define PAY_ALL_EX                                             2
#define LOG_TYPE_ZQ_OTHER                                      101
#define LOG_TYPE_TASK                                          1
#define MP_OS_IOS                                              2
#define CFG_TYPE_AAA                                           2
#define REPORT_TYPE_ILLEGAL                                    1
#define CFG_TYPE_MA                                            10
#define ANIMATION_TYPE_BUG                                     1
#define LOG_TYPE_ARTIFACT_EXP                                  12
#define LOG_TYPE_TAO                                           3
#define REPORT_ILLEGAL_XIANGY                                  4
#define LONGTENG_SHENGSHI_TIME                                 18
#define TYPE_SUMMER_VACATION_2015                              5
#define TIME_CITYWAR_PROTECT                                   7
#define LOG_TYPE_CHARGE_ITEM                                   1
#define LOG_TYPE_ACTIVITIE                                     2
#define NO_ROUNDING                                            0
#define OPER_GET_CATALOG                                       5
#define MAX_BAN_WORDS_GROUP_LENGTH                             200
#define BONUS_TYPE_FAILURE_EX                                  104
#define GS_STATUS_OVER                                         5
#define ITEM_ORIGIN_ZHURWL                                     3
#define LOG_TYPE_W_SCORE                                       9
#define LOG_TYPE_REP                                           5
#define CLIENT_TYPE_KUAIBA                                     10
#define OPER_GET_GID                                           13
#define XDBC_TRUE                                              1
#define ACCURACY_MIN                                           3
#define DAY_SHAXING_BONUS_TIMES                                30
#define CHARGE_GIFT_COIN                                       5
#define DLG_MODAL_NORMAL                                       1
#define LOG_TYPE_ZQ_EXP                                        1
#define LOG_TYPE_POT                                           4
#define MP_OS_ANDROID                                          1
#define OPER_CREATE                                            4
#define CFG_TYPE_LDBA                                          19
#define ITEM_PROVENANCE_ONLINEMALL                             1
#define MAP_EFFECT_TYPE_DECORATE                               4
#define CHARGE_YIPAY_CARD_DIRECT                               18
#define CHARGE_GY_CARD_SALE                                    2
#define DLG_MODAL_SPECIAL                                      2
#define DLG_MODAL_DEFAULT                                      0
#define BONUS_TYPE_INTIMACY                                    7
#define ZIP_LZSS                                               1
#define ROUNDING                                               1
#define OPER_XG_CREATE_NAME                                    24
#define CFG_TYPE_ADB                                           1
#define RESTORE_FAILED_NO_DATA                                 2
#define RESTORE_FAILED_SAVE_PATH                               1
#define REPORT_ILLEGAL_OTHERS                                  10
#define WISHING_TYPE_XF                                        2
#define CHARGE_WEB_GAME_REBATE                                 20
#define COST_PLATFORM_COMMON                                   0
#define BONUS_TYPE_MERIT                                       14
#define BONUS_TYPE_TAO                                         2
#define BONUS_TYPE_GOODS                                       2
#define TRANSFER_COMMON_ITEM                                   4
#define LOG_TYPE_OTHER                                         101
#define LOG_TYPE_GLORY                                         10
#define OPER_XG_GET_NAME                                       26
#define CFG_TYPE_DDB                                           3
#define TYPE_WINTER_VACATION_2015                              7
#define TYPE_WINTER_VACATION_2014                              4
#define FETCH_COIN                                             2
#define OPER_WRITE_PET_DEPOSIT                                 18
#define OPER_DELETE                                            3
#define CHARGE_19PAY_GAME_DIRECT                               4
#define BANNER_BONUS_SHENGSHI_XIANHUA_YU                       2
#define DLG_TYPE_DEFAULT                                       0
#define WG_APEX                                                3
#define LOG_TYPE_MARTIAL                                       13
#define CEIL                                                   1
#define ACCURACY_MON                                           6
#define LOG_TYPE_PARTY_CONTRIB                                 18
#define DLG_SUB_TYPE_ARTIFACT_SEAL                             2
#define ADD_CASH                                               5
#define BONUS_TYPE_SUBMIT                                      105
#define LOG_TYPE_ZQ_CARD                                       2
#define ACCURACY_HOUR                                          2
#define XYGC_CORP_DAWANGZI                                     1
#define ITEM_ORIGIN_SHIJZ                                      9
#define CORP_TYPE_DAXUEZHANG                                   7
#define LUOPAN_TYPE_SHUILXP                                    6
#define MINI_CLIENT_MAX_LEVEL                                  40
#define LOG_TYPE_VOUCHER                                       8
#define IGNORE_WHEN_USER_NOT_FOUND                             1
#define ITEM_ORIGIN_FUB                                        1
#define DLG_TYPE_CONFIRM                                       2
#define FETCH_CASH                                             1
#define BONUS_TYPE_MARTIAL                                     4
#define ADD_VOUCHER                                            6
#define PAY_VOUCHER                                            4
#define GENERATED_EXP_PET                                      5
#define LOG_TYPE_ZQ_FRAGMENT                                   3
#define FLOOR                                                  0
#define CFG_TYPE_BACKUP_SERVER                                 13
#define RESTORE_FAILED_ILLEGAL_DATA                            4
#define ECHO_INTERVAL                                          10
#define REPORT_ILLEGAL_SHASG                                   8
#define ITEM_ORIGIN_XIUX                                       8
#define CHARGE_MBPAY_WEB_DIRECT                                16
#define MOVE_TYPE_FLY                                          2
#define CONFIRM_RESULT                                         1
#define USE_ITEM_FIRST                                         0
#define REPORT_ILLEGAL_XIANJTJ                                 6
#define YUEBING_TYPE_FANFXS_2015                               2
#define BANNER_PRIORITY_NORMAL                                 128
#define MOVE_TYPE_LAND                                         0
#define TIME_WUZY_PROTECT                                      6
#define GENERATED_EXP_PLAYER                                   4
#define RECLAIMED_VOUCHER                                      3
#define BONUS_TYPE_SUCCESS_EX                                  103
#define CLIENT_TYPE_MINI                                       1
#define ACCURACY_DAY                                           1
#define CFG_TYPE_XGS                                           18
#define STEP_MOVE_WEDDING                                      1
#define LOG_TYPE_COIN                                          17
#define MANAGE_MP_OPER_UNBIND                                  2
#define GS_STATUS_FULL                                         4
#define RESTORE_FAILED_BAD_DATA_TYPE                           5
#define ITEM_ORIGIN_SHANGGYW                                   5
#define INVALID_PARA                                           -4
#define OPER_ITEM_STALL                                        2
#define ANIMATION_TYPE_CHRISTMAS                               5
#define BONUS_TYPE_ITEM                                        11
#define BONUS_TYPE_SPECIAL                                     3
#define LOG_TYPE_ZQ_RANK_SCORE                                 5
#define LOG_TYPE_ZQ_XG_POINT                                   4
#define XDBC_INVALID_INDEX                                     -1
#define ITEM_ORIGIN_SHIMSW                                     10
#define MAX_VALID_COINPWD_TIMES                                5
#define TYPE_WAWA_XUNLIANYING                                  6
#define CORP_TYPE_RESOURCE_WAR                                 3
#define CHARGE_ALIPAY                                          12
#define ANIMATION_TYPE_LAND_YUTIANSUO                          7
#define BONUS_TYPE_TOTAL_SCORE                                 5
#define MAPPING_VALUE                                          1
#define OPER_CREATE_GID                                        11
#define VALID_NAME_LACK_WORDS                                  -5
#define CFG_TYPE_XDBC_SERVER                                   15
#define TYPE_SUMMER_VACATION_2013                              2
#define XDBC_FAILED                                            -1
#define TYPE_LOTTERY_TICKET_NS                                 2
#define OPER_ITEM_SELL                                         4
#define BANNER_QIXI_2013                                       7
#define GENERATED_EXP                                          20
#define BONUS_TYPE_SUCCESS                                     101
#define LOG_TYPE_MERIT                                         16
#define REPORT_ILLEGAL_CHEATER                                 11
#define CORP_TYPE_CS_CITYWAR                                   6
#define CLIENT_TYPE_FULL_WEB                                   7
#define TEST                                                   0
#define WISHING_TYPE_CD                                        1
#define MAP_EFFECT_TYPE_SPRING                                 1
#define MAX_MOOD                                               999999999
#define TIME_DFQD_PROTECT                                      12
#define BONUS_TYPE_MINOR                                       2
#define CLIENT_TYPE_NORMAL                                     0
#define OPER_MOVE                                              23
#define OPER_GET_GID_ARR                                       16
#define ERROR_WHEN_USER_NOT_FOUND                              2
#define ACCURACY_YEAR                                          5
#define GS_STATUS_BUSY                                         3
#define LUOPAN_TYPE_DUOBQB                                     4
#define MAX_FOOD                                               999999999
#define WG_CHALLENGE                                           4
#define BONUS_TYPE_SPECAIL_EX                                  13
#define TIMESTAMP_SHIFT                                        28800
#define TYPE_LOTTERY_TICKET                                    1
#define CHARGE_GY_CARD_WEB_DIRECT                              8
#define CHARGE_ALIPAY_WEB_DIRECT                               7
#define CHARGE_GY_COIN                                         0
#define GIFT_TYPE_ITEM                                         1
#define OPER_UPDATE_LOG                                        21
#define CFG_TYPE_CLEANDATA_SERVER                              17
#define ANIMATION_TYPE_GRASS                                   2
#define SERVER_STATUS_OK                                       3
#define HEARTBEAT_INTERVAL                                     180
#define PRECOST_TYPE_TRADE                                     2
#define BANNER_DIANFENG_SHENGDIAN                              8
#define CHARGE_ALIQR_GAME_DIRECT                               23
#define CHARGE_PAYPAL                                          15
#define BANNER_SIX_ANNIVERSARY                                 4
#define BONUS_TYPE_VALUE                                       1
#define TIME_RESTRICT                                          1
#define USE_ITEM_NORMAL                                        1
#define TRUE                                                   1
#define ACCURACY_SEC                                           4
#define CHARGE_GY_GO_WEB_DIRECT                                14
#define OPER_GET_GID_INFO                                      15
#define CFG_TYPE_PROXY_SERVER                                  8
#define LUOPAN_TYPE_LUOPAN                                     3
#define OPER_READ_EX                                           22
#define SERVER_STATUS_STARTUP                                  2
#define ITEM_ORIGIN_BIANYXX                                    6
#define ITEM_ORIGIN_BOSS                                       2
#define OPER_GET_CATALOG_EX                                    6
#define XDBC_INFORMATION                                       1
#define VALID_NAME_EXCEED_MAX_LEN                              -2
#define XYGC_CORP_NONE                                         0
#define MAP_EFFECT_BIANPAO_COMMON                              8
#define MAP_EFFECT_NATIONAL_DAY_2014                           6
#define CHARGE_19PAY_WEB_DIRECT                                9
#define MAX_PASSPOD_CASH                                       2000000000
#define ZIP_NONE                                               0
#define BAD_DATA                                               -3
#define XYGC_CORP_XIAOWANGZI                                   2
#define CHARGE_WXPAY_WEB_DIRECT                                21
#define LUOPAN_TYPE_TANBAO                                     8
#define CLIENT_TYPE_MOBILE_ANDROID                             5
#define XDBC_MAJOR                                             2

// --- String Constants (1160) ---
#define HOUSE_CLASS                                            "house"
#define ITEM_LIMIT_PRACTICE_TIME_MARTIAL                       "3"
#define LIMIT_TYPE_MOUNT_STONE_PER_WEEK_EX                     "5"
#define LIMIT_TYPE_XYFS_2016_COB                               "852"
#define LIMIT_TYPE_GUCHENG_RICHANG_BONUS                       "771"
#define LIMIT_TYPE_SHIZN_CQBY_DSKY_TAO                         "758"
#define LIMIT_TYPE_NEZHA_SONGZHUFU                             "678"
#define LIMIT_TYPE_SHENGDAN_DALI                               "654"
#define LIMIT_TYPE_GIVE_BAOZI3                                 "439"
#define LIMIT_TYPE_XIANNV_SHOUDAO                              "321"
#define LIMIT_TYPE_XNSL_TASK                                   "306"
#define LIMIT_TYPE_DUANW2012_WUDTN                             "132"
#define LIMIT_TYPE_STAT_JY_PER_DAY                             "82"
#define LIMIT_TYPE_SMPN_UPGRADE                                "11"
#define LIMIT_PER_DAY                                          "limit_per_day"
#define F_DAEMON_HANDLER_MODULE                                GLOBAL_FEATURE_DIR "daemon/handler_module.c"
#define ITEM_LIMIT_CHONGWU_JINGYANDAN                          "4"
#define LIMIT_TYPE_WANGSJ_2016_ITEM                            "908"
#define LIMIT_TYPE_TD_XIANJTJ                                  "818"
#define LIMIT_TYPE_CHANGQF_2015_BONUS                          "628"
#define LIMIT_TYPE_QMG                                         "552"
#define LIMIT_TYPE_BAODSS_TAO                                  "394"
#define LIMIT_TYPE_YZDD_SCORE                                  "266"
#define LIMIT_TYPE_ZBLDG                                       "239"
#define LIMIT_TYPE_QUZHUNIANSHOU_CARD                          "208"
#define LIMIT_TYPE_CHANNEL_CURRENT                             "168"
#define LIMIT_TYPE_JUGTQ_COB_BONUS                             "152"
#define LIMIT_TYPE_WXZS_CHILD_BOOK                             "145"
#define LIMIT_TYPE_LAODJ_2012                                  "68"
#define LIMIT_TYPE_YXHD_2012                                   "33"
#define LIMIT_TASK_TAIXSJ                                      "27"
#define LIMIT_TYPE_TLTX_UPGRADE                                "15"
#define ONLINE_UPDATE_D                                        GLOBAL_DAEMONS_DIR "online_updated.c"
#define TYPE_CTDB                                              "ctdb"
#define PRODUCT                                                "asktao"
#define STALL_CLASS                                            "stall"
#define XIAOMS_URL_NOT_TEST                                    "http://wd.gyyx.cn/huodong/sjxms/"
#define LIMIT_TYPE_CYJ2016_COB                                 "879"
#define LIMIT_TYPE_TD_300                                      "821"
#define LIMIT_TYPE_10ZN_WSBOSSWZZ                              "721"
#define LIMIT_TYPE_ZHJQKJWK_WIN                                "447"
#define LIMIT_TYPE_RXQF_LIEYAN                                 "373"
#define LIMIT_TYPE_LGXY_CHARGE                                 "326"
#define LIMIT_TYPE_TER_2013_WWMJ                               "314"
#define LIMIT_TYPE_XIANNV_SONGFU                               "308"
#define LIMIT_TYPE_SHENSHOU_XIAFAN                             "260"
#define LIMIT_TYPE_YINGJIU_JIAOZHONG                           "196"
#define LIMIT_TYPE_GUIXIAN_KAOYAN                              "143"
#define LIMIT_TYPE_STAT_QN_PER_DAY                             "84"
#define COUPLE_TRANSFER                                        "fuqckzy"
#define ECHO_D                                                 GLOBAL_DAEMONS_DIR "echod.c"
#define CCS_DIR                                                "/ccs/"
#define PATH_ARTIFACT_SEAL                                     "artifact_seal"
#define XML_CHILD_NODES_EX                                     @A["_childs_ex"]
#define DYNAMIC_KEY_LOGIN                                      "login"
#define LIMIT_TYPE_TD_TONGTT                                   "874"
#define LIMIT_TYPE_DSFM_SCORE                                  "733"
#define LIMIT_TYPE_CYHL_2016                                   "709"
#define LIMIT_TYPE_SIMINGPAI_LUANDOU                           "662"
#define LIMIT_TYPE_JZDK_WWZB                                   "572"
#define LIMIT_TYPE_WANSHENGJIE_2014                            "500"
#define LIMIT_TYPE_YUANLING_HEXI                               "436"
#define LIMIT_TYPE_SHICAI_COB_2013                             "393"
#define LIMIT_NEWBIE_BOX                                       "349"
#define LIMIT_TYPE_GET_BLESS                                   "331"
#define LIMIT_TYPE_XIYING_HUANLEZONG                           "248"
#define LIMIT_TYPE_TODAY_STAT_BONUS                            "224"
#define LIMIT_TASK_GOUZHU_FANGYU_BONUS                         "199"
#define LIMIT_TYPE_HOUSE_TASK_UPGRADE                          "158"
#define LIMIT_TYPE_TIP_DROP_PET                                "94"
#define STALL_TRANSFER                                         "bait"
#define CTA_COMMAND_D                                          GLOBAL_DAEMONS_DIR "ctacommandd.c"
#define DEF_INI_FILE                                           "/dba/dba.ini"
#define RUNTIME_CLASS                                          "runtime"
#define ENCRYPT_VERSION                                        "20544DE38C2160D6EAC9BFE07486242E"
#define XML_PARENT_NODE                                        @A["_parent"]
#define LIMIT_TYPE_WANGSJ_2016_BYJP                            "922"
#define LIMIT_TYPE_BKQS_SELECT                                 "630"
#define LIMIT_TYPE_SJTQ_DFSD_CHANNEL                           "626"
#define LIMIT_TYPE_SJTQ_DFSD_HONGBAO                           "625"
#define LIMIT_TYPE_SHUANGJTQ_PUTTQ_TASK                        "620"
#define LIMIT_TYPE_TONGMQC_TAO                                 "427"
#define LIMIT_TYPE_CUL_OTHER_TAOSHU                            "406"
#define LIMIT_TYPE_NAOYUANXIAO_2014                            "399"
#define LIMIT_TYPE_TER_2013_TAO                                "312"
#define LIMIT_TYPE_GXKY_ITEM                                   "275"
#define LIMIT_TASK_CISHA_YAOMO                                 "197"
#define LIMIT_TYPE_LUBAN_BOOK_ZHD_4                            "189"
#define LIMIT_TYPE_LM_TONGTT                                   "127"
#define LIMIT_TYPE_NYX_CHILD_STUN_BOOK                         "50"
#define ITEM_LIMIT_CHONGWU_WUXUEDAN                            "5"
#define LIMIT_TYPE_BONUS_MONITOR_EXP_XIUXRW                    "893"
#define LIMIT_TYPE_WANGSJ_2016                                 "907"
#define LIMIT_TYPE_SLZ_BONUS_2015                              "602"
#define LIMIT_TYPE_SHOUWEI_ZHUABXR_2015                        "593"
#define LIMIT_TYPE_JIUZN_JZSF_JL                               "576"
#define LIMIT_TYPE_CHENGRKY_TAO                                "557"
#define LIMIT_TYPE_HJDC_JP                                     "535"
#define LIMIT_TYPE_CHILD_PRACTICE_TIP                          "489"
#define LIMIT_TYPE_QUANMDY                                     "484"
#define LIMIT_TYPE_FREE_FISH_SWALLOW                           "464"
#define LIMIT_TYPE_GIVE_BAOZI2                                 "438"
#define LIMIT_TYPE_ZHENGJIU_XUREN                              "369"
#define LIMIT_TYPE_USE_TIANJHL_TICKET                          "258"
#define LIMIT_TYPE_LDQF_SP_COB                                 "274"
#define LIMIT_TYPE_MAOSHAN_FANJI                               "220"
#define LIMIT_TYPE_LUBAN_BOOK_ZHD_1                            "186"
#define LIMIT_TYPE_JQHL_JQLP                                   "184"
#define LIMIT_TYPE_XIANREN_ZHILU                               "154"
#define LIMIT_TYPE_DUANW2012_CHAOJTN                           "131"
#define STAT_TINY_CLIENT_PLAYED                                "102"
#define LIMIT_TYPE_CIJYX                                       "54"
#define SCHEDULE_D                                             GLOBAL_DAEMONS_DIR "scheduled.c"
#define RED                                                    ESC "[31m"
#define TODAY_STAT_TIME                                        "0"
#define LIMIT_TYPE_FINISH_LOOP_300                             "13"
#define LIMIT_TYPE_BONUS_MONITOR_EXP_XUNXRWTG                  "913"
#define LIMIT_TYPE_BONUS_MONITOR_EXP_XUNXRW                    "892"
#define LIMIT_TYPE_QIX2016_FHXD_FINISH                         "814"
#define LIMIT_TYPE_CHILDREN_DAY_2016_ITEM                      "786"
#define LIMIT_TYPE_2016NSWD_DASHKY_POT                         "725"
#define LIMIT_TYPE_2016NSWD_DASHKY_EXP                         "723"
#define LIMIT_TYPE_INSIDER_NOTICE                              "661"
#define LIMIT_TYPE_WUDONG_TYC_BONUS                            "639"
#define LIMIT_TYPE_JZDK_TAO                                    "570"
#define LIMIT_TYPE_YANHUA_QUXIE_TASK                           "517"
#define LIMIT_TYPE_SUMMER_2014_LEITAIFS                        "457"
#define LIMIT_TYPE_ZHMCZ_WWZB                                  "387"
#define LIMIT_TYPE_RXQF_SNOWMAN                                "377"
#define LIMIT_TYPE_RXQF_WIN                                    "375"
#define LIMIT_TYPE_BAOWQQ_COMBAT_BONUS                         "323"
#define LIMIT_TYPE_YUREN_BAOXIANG                              "242"
#define LIMIT_TYPE_LAODJ_2012_UPGRADE                          "71"
#define LIMIT_TYPE_THX_2011                                    "21"
#define ACTIVE_STAT_LOG                                        "active_stat_log"
#define VERIFY_CODE_D                                          GLOBAL_DAEMONS_DIR "verify_coded.c"
#define RPC_D                                                  GLOBAL_DAEMONS_DIR "rpcd.c"
#define CONFIG_D                                               GLOBAL_DAEMONS_DIR "configd.c"
#define BWHT                                                   ESC "[47m"
#define LIMIT_TYPE_MOUNT_STONE_PER_WEEK                        "3"
#define LIMIT_TYPE_BONUS_MONITOR_EXP_SHIJZTG                   "912"
#define LIMIT_TYPE_BONUS_MONITOR_EXP_SHIJZ                     "891"
#define LIMIT_TYPE_BONUS_MONITOR_EXP_SHIMRW                    "890"
#define LIMIT_TYPE_TD_CHOUBDC                                  "884"
#define LIMIT_TYPE_TD_XSBS                                     "849"
#define LIMIT_TYPE_YUEQIANDAO_TIP                              "777"
#define LIMIT_TYPE_TD_BANGPRW                                  "770"
#define LIMIT_TYPE_COUPLE_TASK_LABELI                          "764"
#define LIMIT_TYPE_NZPMS_PRAISE_TIMES                          "683"
#define LIMIT_TYPE_ZZHY_LINGMO                                 "648"
#define LIMIT_TYPE_XIXUN_EXCHANGE_TIMES                        "644"
#define LIMIT_TYPE_TIANYOU_YOUQINGREN                          "381"
#define LIMIT_TYPE_HONGBAO_MONSTER                             "289"
#define LIMIT_TYPE_ZSCF_CHILD_EQUIP                            "217"
#define IMPORTANT_LOG                                          "important_log"
#define LOGIN_CLASS                                            "login"
#define XML_NODE_NAME                                          @A["_element"]
#define TODAY_STAT_SW                                          "6"
#define LIMIT_TYPE_DIG_CANPIAN                                 "1"
#define LIMIT_TYPE_TD_WSJ2016                                  "909"
#define LIMIT_TYPE_YONGWZQ_WWJJ                                "862"
#define LIMIT_TYPE_QINGM2016_SHENCHAI                          "716"
#define LIMIT_TYPE_JIYRX_LIEYANWANG                            "530"
#define LIMIT_TYPE_FSKH_ZRX                                    "460"
#define LIMIT_TYPE_LXZJ_TEST_BONUS                             "376"
#define LIMIT_TYPE_MENGHUI_FENGSHEN                            "277"
#define LIMIT_TYPE_LBD_2013_BOSS                               "250"
#define LIMIT_TYPE_YZDD_CARD                                   "267"
#define LIMIT_TYPE_JITUO_YINYUAN                               "238"
#define LIMIT_TYPE_QINGRJ_2013_DLDXS                           "231"
#define LIMIT_TASK_GOUZHU_FANGYU_COB                           "200"
#define LIMIT_TYPE_IMP_TIP_QIGAI                               "165"
#define LIMIT_TYPE_CHILD_DAILY                                 "91"
#define MM_D                                                   GLOBAL_DAEMONS_DIR "mmd.c"
#define OM_D                                                   GLOBAL_DAEMONS_DIR "omd.c"
#define MASTER_CLASS                                           "master"
#define LIMIT_TYPE_DOUBLE_BONUS                                "4"
#define LIMIT_TYPE_TD_ZYHB                                     "832"
#define LIMIT_TYPE_TD_ZHENYDZ                                  "823"
#define LIMIT_TYPE_SUMMER2016_CJSCZL                           "797"
#define LIMIT_TYPE_QUEQIAO_XIANGJU                             "640"
#define LIMIT_TYPE_SJTQ_ZZGS_WATER                             "624"
#define LIMIT_TYPE_FSKH_WJX                                    "461"
#define LIMIT_TYPE_XUNSHI_ANMIN                                "454"
#define LIMIT_TYPE_COMMON_GUESS_TIP                            "404"
#define LIMIT_TYPE_HMXM_END_TIP                                "383"
#define LIMIT_TYPE_LONGXUE_ZHIJING                             "367"
#define LIMIT_TYPE_QUGUI_BIXIONG                               "240"
#define LIMIT_TYPE_XQSTY_COB_BONUS                             "228"
#define LIMIT_TYPE_PGSF                                        "204"
#define LIMIT_TYPE_JQHL_BONUS                                  "183"
#define NPC_TRANSFER                                           "chusgNPC"
#define BCYN                                                   ESC "[46m"
#define YEL                                                    ESC "[33m"
#define DBA_SERVER_D                                           DBA_DAEMONS_DIR "dbaserverd.c"
#define TODAY_STAT_LEIJ_SCORE                                  "14"
#define LIMIT_TYPE_BONUS_MONITOR_CASH_EARN_H                   "926"
#define LIMIT_TYPE_BONUS_MONITOR_CASH_EARN                     "906"
#define LIMIT_TYPE_CBDC_TASK_TIMES                             "870"
#define LIMIT_TYPE_TEACHERS_DAY_2016_XH                        "809"
#define LIMIT_TYPE_SYSTEM_REP_BONUS                            "675"
#define LIMIT_TYPE_FSKH_BJX                                    "459"
#define LIMIT_TYPE_DWJ_2014                                    "414"
#define LIMIT_TYPE_DOUCHONG_DAHUI                              "409"
#define LIMIT_TYPE_FRIEND_DRINK                                "342"
#define LIMIT_TYPE_SHENMI_DALI_CREATE                          "291"
#define LIMIT_TYPE_XQSTY_TASK                                  "227"
#define LIMIT_TASK_BAIHQF                                      "37"
#define LIMIT_TYPE_QJYY_COMBAT_UPGRADE                         "9"
#define F_COMN_BATCH_LOG                                       GLOBAL_FEATURE_DIR "common/batch_log.c"
#define SERVER_TIME_D                                          GLOBAL_DAEMONS_DIR "servertimed.c"
#define ROLE_NAME                                              "DBA"
#define USER_CLASS                                             "user"
#define TODAY_STAT_SCORE                                       "12"
#define LIMIT_TYPE_YEAR_INSIDER_RYZDL                          "9"
#define LIMIT_TYPE_YONGWZQ_POT                                 "859"
#define LIMIT_TYPE_QIX2016_TDYM_ITEM                           "812"
#define LIMIT_TYPE_DYWD2016_WZXF_TIME                          "802"
#define LIMIT_TYPE_SUMMER_2016_BIAOXXY                         "789"
#define LIMIT_TYPE_SHIZN_CQBY_TAO                              "748"
#define LIMIT_TYPE_2016NSWD_ZAIXKH                             "741"
#define LIMIT_TYPE_REP_CARD_WEEK                               "671"
#define LIMIT_TYPE_QMTZ_WWJJ                                   "613"
#define LIMIT_TYPE_QUGUI_CHUXIE_2015_COMBAT                    "607"
#define LIMIT_TYPE_CHENGRKY_WWZB                               "559"
#define LIMIT_TYPE_YXDH_2015                                   "536"
#define LIMIT_TYPE_RECHARGE_INVALID                            "478"
#define LIMIT_TYPE_VALENTINE_DAY_2013                          "230"
#define LIMIT_TYPE_PGSF_UPGRADE                                "203"
#define LIMIT_TYPE_HUSHENGLING                                 "138"
#define LIMIT_TYPE_BEAT_SUPER_BOSS                             "61"
#define ESC                                                    ""
#define INTERNAL_COMM_CLOSED                                   "einternalcommclosed"
#define ACHIEVE_CLASS                                          "achieve"
#define HANDLER                                                GLOBAL_INHERIT_DIR "misc/handler.c"
#define DYNAMIC_KEY_ASKTAO_SHOW                                "asktao_show"
#define ITEM_LIMIT_CHILD_PRACTICE                              "8"
#define LIMIT_TYPE_TODAY_STAT_GLORY                            "911"
#define LIMIT_TYPE_GUCHENG_PK_SCORE                            "778"
#define LIMIT_TYPE_SHIZN_CQBY_GX30                             "753"
#define LIMIT_TYPE_SHIZN_CQBY_TAOZI                            "751"
#define LIMIT_TYPE_SHIZN_DXHX_JP                               "739"
#define LIMIT_TYPE_ZZHY_LINGWANG                               "647"
#define LIMIT_TYPE_QMTZ_TAO                                    "617"
#define LIMIT_TYPE_CHILDREN_DAY_2015_NIANGNCF                  "605"
#define LIMIT_TYPE_SUMMER_2015_HUOYJJ                          "582"
#define LIMIT_TYPE_NINGJU_FASHU_2013                           "390"
#define LIMIT_TYPE_NOTE_SHOUCI_CHONGZHI                        "351"
#define LIMIT_TYPE_XIANNV_SONGFU_U                             "318"
#define LIMIT_TYPE_CIJIU_YINGXIN_2013                          "210"
#define LIMIT_TYPE_LONGGONG_XIANYI                             "153"
#define LIMIT_TYPE_LUCK_MAGIC                                  "119"
#define LIMIT_TYPE_TONGTT_TXS                                  "120"
#define F_COMN_COMM                                            GLOBAL_FEATURE_DIR "common/comm.c"
#define HTTP_SERVER_D                                          GLOBAL_DAEMONS_DIR "http_serverd.c"
#define CONNECTION_D                                           GLOBAL_DAEMONS_DIR "connectiond.c"
#define SERVER_CURRENT_VERSION                                 "1.60.0912"
#define TODAY_STAT_QN                                          "4"
#define LIMIT_TYPE_BONUS_MONITOR_CASH_H                        "928"
#define LIMIT_TYPE_CJ2016_JIYHG                                "702"
#define LIMIT_TYPE_DANTY_2015                                  "584"
#define LIMIT_TYPE_TDXL_FINISH                                 "503"
#define LIMIT_TYPE_CD2013_MIJI                                 "280"
#define LIMIT_TYPE_LDQF_TASK                                   "273"
#define LIMIT_TYPE_WULONG_SANLI                                "69"
#define LIMIT_TYPE_USE_ZHNDG                                   "63"
#define LIMIT_TYPE_YXDX_CHILD_BOOK                             "46"
#define F_COMN_MM                                              GLOBAL_FEATURE_DIR "common/module_maintain.c"
#define BLK                                                    ESC "[30m"
#define LIMIT_TYPE_TD_GGCM                                     "875"
#define LIMIT_TYPE_YONGWZQ_TAO                                 "858"
#define LIMIT_TYPE_APPLY_HUOJI_SPE                             "701"
#define LIMIT_TYPE_BAIHZY_2016                                 "657"
#define LIMIT_TYPE_SINGLES_DAY_2015                            "655"
#define LIMIT_TYPE_LQXC_TIP                                    "541"
#define LIMIT_TYPE_RUNE_BONUS                                  "488"
#define LIMIT_TYPE_FENGSHEN_JINGCAI                            "453"
#define LIMIT_TYPE_GDQQ_COMBAT                                 "448"
#define LIMIT_TYPE_ZXXL_SHAGUAI                                "272"
#define LIMIT_TYPE_SZWH_JION_TIME                              "254"
#define LIMIT_TYPE_QINGRJ_2013_XSZW                            "232"
#define LIMIT_TYPE_JIXIANG_CIFU                                "216"
#define LIMIT_SHOP_BANKRUPT_TIP                                "103"
#define LIMIT_TYPE_TIP_QIGAI                                   "93"
#define LIMIT_TYPE_TD_SHCE                                     "842"
#define LIMIT_TYPE_TD_FEIXDX                                   "819"
#define LIMIT_TYPE_ZHENGJIU_YUTU                               "492"
#define LIMIT_TYPE_NEWYEAR_BOSS_2014                           "366"
#define LIMIT_TYPE_AWZH_WIN                                    "327"
#define LIMIT_TYPE_MOTHER_DAY_2013                             "298"
#define LIMIT_TYPE_SHENTOU_BAODAI                              "175"
#define LIMIT_TYPE_CUL_SELF_TREE                               "125"
#define LIMIT_TYPE_WULEILING                                   "17"
#define LIMIT_TYPE_CHILDREN_DAY_2016                           "785"
#define LIMIT_TYPE_ZQGB_BONUS_TIMES                            "706"
#define LIMIT_TYPE_DAXUEZHANG                                  "658"
#define LIMIT_TYPE_CHILDREN_2015_COMBAT                        "606"
#define LIMIT_TYPE_BXXL                                        "562"
#define LIMIT_TYPE_ZHENGMING_ZHIZHAN_TSZD                      "515"
#define LIMIT_TYPE_FISHING_BONUS                               "495"
#define LIMIT_TYPE_RESELECT_MASTERY_TIP                        "455"
#define LIMIT_TYPE_BAODSS_TASK                                 "395"
#define LIMIT_TYPE_INBORN_MOUNT_STONE                          "348"
#define LIMIT_TYPE_XZJZ_STAGE                                  "293"
#define LIMIT_TYPE_TIANDSF_BONUS                               "294"
#define LIMIT_TYPE_PUTTQ_COB                                   "290"
#define LIMIT_TYPE_YSHE_TIP                                    "156"
#define LIMIT_TYPE_XBDWW_COB_U                                 "108"
#define LIMIT_U_TASK_WINTER_NOTES                              "42"
#define LIMIT_TYPE_GET_LOOP_999                                "4"
#define LIMIT_TYPE_SHUADAO_TIMES                               "19"
#define LIMIT_TYPE_CWXLY_TIMES                                 "882"
#define LIMIT_TYPE_TD_SHIZZ                                    "768"
#define LIMIT_TYPE_APPLY_HUOJI_NOR                             "700"
#define LIMIT_TYPE_GEJ_HJKP_ITEM                               "650"
#define LIMIT_TYPE_WENNUAN_MUAI                                "415"
#define LIMIT_TYPE_JINZ_ZID_ZUDPT                              "400"
#define LIMIT_TYPE_CHAR_BIRTHDAY_NOTE                          "364"
#define LIMIT_TYPE_FWQBC                                       "336"
#define LIMIT_TYPE_YUXIAN_TONGLE                               "319"
#define LIMIT_TYPE_CIJYX_2013_DLDXS                            "214"
#define LIMIT_TYPE_CY_SHL_2012                                 "179"
#define LIMIT_TASK_U_XIAOBAO_YUXIANJI                          "123"
#define LIMIT_TYPE_USE_WAWXLD                                  "56"
#define LIMIT_TYPE_JZBS_WWJJ                                   "41"
#define HANDLERS_DIR                                           LOCAL_DIR "handlers/"
#define LIMIT_TYPE_TD_YWZQ                                     "876"
#define LIMIT_TYPE_ARENA_EXCHANGE_TAO                          "847"
#define LIMIT_TYPE_SHOUHCE_BONUS                               "807"
#define LIMIT_TYPE_EXCHANGE_TAOCANKA                           "773"
#define LIMIT_TYPE_CJ2016_ZDGC                                 "697"
#define LIMIT_TYPE_2016CJ_YUANLTZ_WWMJ                         "691"
#define LIMIT_TYPE_ZSSL_COMBAT                                 "641"
#define LIMIT_TYPE_SHUANGJTQ_PUTTQ_2015                        "609"
#define LIMIT_TYPE_TQZL_BYJP                                   "554"
#define LIMIT_TYPE_ZQJ_WFLM                                    "493"
#define LIMIT_TYPE_TOWER_DEFENSE_HARD                          "467"
#define LIMIT_TYPE_CITY_BONUS_TIP                              "360"
#define LIMIT_TYPE_CHAR_BIRTHDAY_TASK                          "333"
#define LIMIT_TYPE_XCSL                                        "335"
#define LIMIT_TYPE_QIXI_HONGBAO                                "322"
#define LIMIT_TYPE_CHAR_PRACTICE                               "246"
#define LIMIT_QINGRJ_2013_CB_EX                                "235"
#define LIMIT_TYPE_WUXING_ZHISHEN                              "141"
#define LIMIT_TYPE_SOUL_TIP                                    "31"
#define F_COMN_IID                                             GLOBAL_FEATURE_DIR "common/iid.c"
#define SYNC_D                                                 GLOBAL_DAEMONS_DIR "syncd.c"
#define INHERIT_DIR                                            LOCAL_DIR "inherit/"
#define TODAY_STAT_YBXF                                        "13"
#define LIMIT_TYPE_CBDC_HELP_TIMES                             "871"
#define LIMIT_TYPE_TD_JLXY                                     "856"
#define LIMIT_TYPE_CJ2016_YXZM_SSDF                            "719"
#define LIMIT_TYPE_SHENGDAN_DALI_CARD                          "665"
#define LIMIT_TYPE_ZZHY_WIN                                    "645"
#define LIMIT_TYPE_JOIN_PARTY                                  "543"
#define LIMIT_TYPE_WSJ_2014_BYJP                               "533"
#define LIMIT_TYPE_DWJ_NM                                      "417"
#define LIMIT_TYPE_ZHZHGQ                                      "419"
#define LIMIT_TYPE_PYJN                                        "391"
#define LIMIT_TYPE_YZDD_POT                                    "264"
#define LIMIT_TYPE_QMJZ_EXCHANGE                               "244"
#define LIMIT_CIJYX_2013_CHILD_BOOK                            "212"
#define LIMIT_TYPE_QINGFENGSAN                                 "75"
#define LIMIT_TYPE_JZBS_WWZB                                   "39"
#define SHAXING_OB                                             "/gs/daemons/tasks/shaxing.c"
#define LIMIT_TYPE_CYJ2016_ITEM                                "880"
#define LIMIT_TYPE_BONUS_MONITOR_TODAY_SCORE                   "894"
#define LIMIT_TYPE_SUMMER2016_SCORE                            "796"
#define LIMIT_TYPE_CHAOSZZ_JYXB                                "703"
#define LIMIT_TYPE_NZPMS_SUBMIT_RECIPES                        "695"
#define LIMIT_TYPE_ZSSL_SUBMIT                                 "642"
#define LIMIT_TYPE_RAID_DUNGEON_BONUS                          "579"
#define LIMIT_TYPE_TOWER_DEFENSE_BONUS                         "496"
#define LIMIT_TYPE_FISHING_GAME                                "468"
#define LIMIT_TYPE_WENXIN_FUAI_COB_TIMES                       "452"
#define LIMIT_TYPE_WJDH_COB_BONUS                              "222"
#define LIMIT_TYPE_TCHW_WWZB                                   "134"
#define LIMIT_TYPE_STAT_DH_PER_DAY                             "83"
#define LIMIT_COIN_TRADE_TIP                                   "59"
#define PURCHASE_REDUCE_SERVER                                 "purchase_reduce_server"
#define BLU                                                    ESC "[34m"
#define TODAY_STAT_BG                                          "7"
#define TODAY_STAT_DHEX                                        "3"
#define LIMIT_TYPE_GGCM_2016_NUM                               "867"
#define LIMIT_TYPE_FRIEND_DRINK_INVITE                         "780"
#define LIMIT_TYPE_2016NSWD_DASHKY_WWJJ                        "732"
#define LIMIT_TYPE_YIZCM_EXP                                   "519"
#define LIMIT_TYPE_ZHJQKJWK_ZHONGWEI                           "450"
#define LIMIT_TYPE_BZN_YAOSRQ                                  "420"
#define LIMIT_TYPE_CHILDREN_DAY_2014                           "411"
#define LIMIT_TYPE_ZWX                                         "403"
#define LIMIT_TYPE_QIZHEN_YIBAO                                "380"
#define LIMIT_TYPE_CITY_TASK_TIMES                             "279"
#define LIMIT_TYPE_CHILDREN_DAY_2013                           "251"
#define LIMIT_TYPE_YZDD_EXP                                    "263"
#define LIMIT_TYPE_FENGSB_COB_BONUS                            "229"
#define LIMIT_TYPE_LM_BAXMJ                                    "130"
#define TYPE_TDB                                               "tdb"
#define CLIENT_DIR                                             "/client/"
#define UNAUTHED_AGENT                                         GLOBAL_OBJECTC_DIR "misc/unauthed_agent.c"
#define LIMIT_TYPE_BONUS_VOTE_NPC                              "16"
#define LIMIT_TYPE_LEAGUE_DOUBLE_BONUS_TIP                     "6"
#define LIMIT_TYPE_YONGWZQ_EXP                                 "857"
#define LIMIT_TYPE_GHOST_2016_ITEM_BONUS                       "799"
#define LIMIT_TYPE_GEJ_HJKP_MULTI_BONUS                        "652"
#define LIMIT_TYPE_SJTQ_DFSD_BYJP                              "627"
#define LIMIT_TYPE_CARD_COMBAT_TIMES                           "442"
#define LIMIT_TYPE_HMXM_TIP                                    "361"
#define LIMIT_TYPE_GET_STAR_QIBAO                              "341"
#define LIMIT_TYPE_ZBLDG_STAGE                                 "287"
#define LIMIT_TYPE_QINGMING_2012_TJMJ                          "73"
#define F_COMN_OB_SOURCE                                       GLOBAL_FEATURE_DIR "common/ob_source.c"
#define DATA_SAVE_D                                            GLOBAL_DAEMONS_DIR "datasaved.c"
#define DISCONNECT_BY_DRIVER                                   "driver_callback"
#define LIMIT_TYPE_CBDC_SUBMIT_NUM                             "923"
#define LIMIT_TYPE_CJ2016_QBSF                                 "696"
#define LIMIT_TYPE_2016CJ_YUANLTZ_POT                          "689"
#define LIMIT_TYPE_DISCOVER_EXTRA_NUM                          "440"
#define LIMIT_TYPE_TUF_END_TIP                                 "408"
#define LIMIT_TYPE_ZHONGZHOU_BAZHU_TIP                         "358"
#define LIMIT_TYPE_BLESS_FRIEND                                "330"
#define LIMIT_TYPE_YISXM_XUANP                                 "309"
#define LIMIT_TYPE_YSHE_XIDIAN_TIP                             "167"
#define LIMIT_TYPE_JUGTQ_SHUIJING                              "151"
#define LIMIT_TYPE_XIAOBAO_ZYM_2012                            "124"
#define LIMIT_TYPE_DRAW_GROUP_CARD                             "18"
#define EXCHANGE_TRANSFER                                      "jiaoy"
#define DB_LOG                                                 "db_log"
#define STAT_MODULE_D                                          GLOBAL_DAEMONS_DIR "statmoduled.c"
#define SUFFIX                                                 "32"
#define LIMIT_TYPE_FINISH_LOOP_999                             "5"
#define LIMIT_TYPE_BONUS_MONITOR_CONTRIB_BANGPRCTZ             "904"
#define LIMIT_TYPE_BONUS_MONITOR_CONTRIB_BANGPRW               "903"
#define LIMIT_TYPE_BONUS_MONITOR_CONTRIB                       "902"
#define LIMIT_TYPE_RESERVE_BONUS_CHECK                         "783"
#define LIMIT_TYPE_DSFM_MY                                     "736"
#define LIMIT_TYPE_ZQGB_BONUS_TIMES_EX                         "707"
#define LIMIT_TYPE_ZHENGMING_ZHIZHAN_ZDJL                      "513"
#define LIMIT_TYPE_TDXL                                        "498"
#define LIMIT_TYPE_ZLXL_2014                                   "505"
#define LIMIT_TYPE_EVERYONE_PK_WARMUP                          "481"
#define LIMIT_TYPE_DOUCDH_CHARGE                               "410"
#define LIMIT_TYPE_NOTE_FRIENDS_DEL                            "334"
#define LIMIT_TYPE_BAOWQQ_TALK_BONUS                           "324"
#define LIMIT_TYPE_LBD_2013                                    "259"
#define LIMIT_TYPE_YASUI_HONGBAO_GIFT                          "205"
#define LIMIT_DUANWU_2012_CHILD_EQUIP                          "111"
#define LIMIT_TYPE_STALL_CASH_TIP                              "99"
#define LIMIT_COMBAT_WINTER_NOTES                              "36"
#define CHECK_LOG_D                                            GLOBAL_DAEMONS_DIR "check_logd.c"
#define CSA_DIR                                                "/csa/"
#define GS_DIR                                                 "/gs/"
#define STORE_CLASS                                            "store"
#define TODAY_STAT_ZJ                                          "5"
#define LIMIT_TYPE_SHIZN_CQBY_CAIZY                            "750"
#define LIMIT_TYPE_APPLY_TJXWD                                 "738"
#define LIMIT_TYPE_WUDAO_XIUXIAN                               "673"
#define LIMIT_TYPE_SIMP_LUANDOU_BONUS                          "663"
#define LIMIT_TYPE_GEJ_HJKP_HUOJI_BONUS                        "653"
#define LIMIT_TYPE_WAWA_XUNLIANYING                            "636"
#define LIMIT_TYPE_QUGUI_CHUXIE_2015_BONUS                     "604"
#define LIMIT_TYPE_YJYH1_2015                                  "585"
#define LIMIT_TYPE_HJTZPJ_2014                                 "502"
#define LIMIT_TYPE_TONGMQC_WWMJ                                "430"
#define LIMIT_TYPE_INBORN_SKILL_STONE                          "347"
#define LIMIT_TYPE_TER_2013_EXP                                "311"
#define LIMIT_TYPE_MPLL_JION_TIME                              "255"
#define STAT_WEB_CLIENT_PLAYED                                 "241"
#define LIMIT_TYPE_LM_RICHANG                                  "128"
#define LIMIT_TYPE_SMPN_COMBAT                                 "12"
#define LIMIT_TYPE_EXCHANGE_GUIWU                              "3"
#define BACKUP_FILE_NAME                                       BACKUP_DIR "backup.bin"
#define TTS_DIR                                                "/tts/"
#define DBA_DAEMONS_DIR                                        DBA_DIR "daemons/"
#define TODAY_STAT_BCFL                                        "9"
#define LIMIT_TYPE_SHUADAO_HOURS                               "20"
#define LIMIT_TYPE_CWXLY_STAT_TIMES                            "915"
#define LIMIT_TYPE_CHRISTMAS_2016                              "869"
#define LIMIT_TYPE_SUMMER_2016_ZHANYHB                         "787"
#define LIMIT_TYPE_DUANWU_2016_BABAO                           "782"
#define LIMIT_TYPE_YEYBGD_GS_BONUS                             "776"
#define LIMIT_TYPE_SHIZN_CQBY_HGKP                             "755"
#define LIMIT_TYPE_DSFM_CT                                     "735"
#define LIMIT_TYPE_2016NSWD_DASHKY_TOP_TAO                     "727"
#define LIMIT_TYPE_SJTQ_SSFH_ITEM                              "621"
#define LIMIT_TYPE_TAIJSJ_CHALLENGE                            "595"
#define LIMIT_TYPE_YUQSD_CHALLENGE                             "597"
#define LIMIT_TYPE_WENNUAN_MUAI_JUEJI                          "423"
#define LIMIT_TYPE_NEWYEAR_2014                                "365"
#define LIMIT_TYPE_JUGUO_TONGQING                              "148"
#define LIMIT_TYPE_CANCEL_ZHUREN_WEILE                         "58"
#define GM_TRANSFER                                            "gmzywp"
#define F_DAEMON_ETERNAL_DATA                                  GLOBAL_FEATURE_DIR "daemon/eternal_data.c"
#define F_COMN_GID                                             GLOBAL_FEATURE_DIR "common/gid.c"
#define F_COMN_DAEMON                                          GLOBAL_FEATURE_DIR "common/daemon.c"
#define LIMIT_TYPE_RECOMMEND_BONUS                             "3"
#define LIMIT_TYPE_QIX2016_FHXD_CHANGE                         "850"
#define LIMIT_TYPE_QIX2016_TDYM                                "811"
#define LIMIT_TYPE_TD_WUZY                                     "822"
#define LIMIT_TYPE_QINGM2016_YANWANG                           "717"
#define LIMIT_TYPE_CJ2016_JHJC_ITEM                            "686"
#define LIMIT_TYPE_REP_CARD_MONTH                              "670"
#define LIMIT_TYPE_FOOLS_DAY_2016                              "668"
#define LIMIT_TYPE_ZHONGQIU_BOBING_2015                        "608"
#define LIMIT_TYPE_CHANGE_QIFU_2015                            "615"
#define LIMIT_TYPE_XIANCHONG_XIAFAN                            "601"
#define LIMIT_TYPE_KFSD_TIMES                                  "581"
#define LIMIT_TYPE_NAILXL_2014                                 "504"
#define LIMIT_TYPE_YUEMAN_ZHONGQIU                             "490"
#define LIMIT_TYPE_COLLECT_GRASS                               "472"
#define LIMIT_TYPE_XUEZXH_COB_TIMES                            "474"
#define LIMIT_TYPE_PARTY_HEALTH_NOTICE                         "445"
#define LIMIT_TYPE_BZN_HUSSM_HUFU                              "433"
#define LIMIT_TYPE_ZHMCZ_WWMJ                                  "388"
#define LIMIT_TYPE_YOULONGGONG                                 "117"
#define LIMIT_TYPE_STAT_BCFL_PER_DAY                           "89"
#define IO_CHAR_DATA_D                                         GLOBAL_DAEMONS_DIR "io_char_datad.c"
#define GLOBAL_INHERIT_DIR                                     "/inherit/"
#define OBJECTC_DIR                                            LOCAL_DIR "clone/"
#define LIMIT_TYPE_WANGSJ_2016_SSJP                            "921"
#define LIMIT_TYPE_TD_DYWD                                     "838"
#define LIMIT_TYPE_TD_XIANGY                                   "816"
#define LIMIT_TYPE_CWDTW_BONUS                                 "772"
#define LIMIT_TYPE_HUOJI_DACAN                                 "525"
#define LIMIT_TYPE_CY_JL_2012_UPGRADE                          "177"
#define LIMIT_TYPE_LUCK_MAGIC_LOG                              "170"
#define LIMIT_TYPE_BAZHENTU_ROUND                              "101"
#define LIMIT_TYPE_JZHX_WWZB                                   "77"
#define LIMIT_TYPE_CHRISTMAS_2011                              "24"
#define TREASURE_TRADING                                       "jiaoypt"
#define F_COMN_AGENT                                           GLOBAL_FEATURE_DIR "common/agent.c"
#define DBA_DIR                                                "/dba/"
#define WHT                                                    ESC "[37m"
#define ETC_DIR                                                LOCAL_DIR "etc/"
#define LIMIT_TYPE_YEAR_INSIDER_DRESS                          "11"
#define LIMIT_TYPE_BONUS_MONITOR_DAOFA                         "896"
#define LIMIT_TYPE_BONUS_MONITOR_EXP                           "889"
#define LIMIT_TYPE_BONUS_MONITOR_CASH_YAB                      "887"
#define LIMIT_TYPE_SHIZN_CQBY_GX100                            "754"
#define LIMIT_TYPE_SHIZN_SNJW_BONUS                            "743"
#define LIMIT_TYPE_QUANMIN_QIANDAO                             "497"
#define LIMIT_TYPE_INSTRUCTION_V4_89                           "480"
#define LIMIT_QINGRJ_2013_CHILD_BOOK                           "234"
#define LIMIT_TYPE_ZHANYAO_CHUMO_2012                          "193"
#define LIMIT_TYPE_JQHL_ITEM                                   "180"
#define LIMIT_TYPE_GXKY_CHILD_EQUIP                            "146"
#define LIMIT_TYPE_TIP_PET_NEIDAN                              "95"
#define LIMIT_TYPE_STAT_ZJ_PER_DAY                             "85"
#define LIMIT_TYPE_PET_TRAINNING                               "66"
#define LIMIT_TYPE_NYX_CHILD_BOOK                              "49"
#define LIMIT_TYPE_BPRCTX                                      "28"
#define TIMER_D                                                GLOBAL_DAEMONS_DIR "timerd.c"
#define DISCONNECT_BY_NO_RESPOND                               "no_respond"
#define INSIDER_STORE_BRANCH                                   "insider"
#define LIMIT_TYPE_BAZHENTU_PER_WEEK                           "8"
#define LIMIT_TYPE_CBDC_CLICK_COB_NPC_TIMES                    "929"
#define LIMIT_TYPE_SHIZN_CQBY_DSKY_EXP                         "757"
#define LIMIT_TYPE_QINGM2016_WIN                               "714"
#define LIMIT_TYPE_JZDK_POT                                    "571"
#define LIMIT_TYPE_ARBOR_DAY_2015                              "550"
#define LIMIT_TYPE_SUBMIT_QMG                                  "555"
#define LIMIT_TYPE_GIVE_BAOZI1                                 "437"
#define LIMIT_TYPE_SINGLES_2013_BONUS                          "352"
#define LIMIT_TYPE_FARS_TASK                                   "301"
#define LIMIT_TYPE_HLCDM                                       "226"
#define LIMIT_TYPE_LUBAN_BOOK_ZHD_2                            "187"
#define LIMIT_DUANWU_2012_CHILD_BOOK                           "112"
#define LIMIT_TYPE_SMPN_COMBAT_UPGRADE                         "13"
#define LIMIT_TYPE_PLANT_GUILINGCAO                            "4"
#define LOG_D                                                  GLOBAL_DAEMONS_DIR "logd.c"
#define AUTO_RECOVER_D                                         GLOBAL_DAEMONS_DIR "autorecoverd.c"
#define RPC_FAILED_VALUE                                       "__rpc_failed_value__"
#define LIMIT_TYPE_GET_LOOP_300                                "12"
#define LIMIT_TYPE_BONUS_MONITOR_CASH_EXCHANGE                 "905"
#define LIMIT_TYPE_PN_DAILY_TASK                               "853"
#define LIMIT_TYPE_SUMMER_2016_MUHHS                           "793"
#define LIMIT_TYPE_DSFM_ZJ                                     "737"
#define LIMIT_TYPE_YABIAO_DETECT_FAILED                        "720"
#define LIMIT_TYPE_REPORT_ILLEGAL_BONUS                        "667"
#define LIMIT_TYPE_ZQJ_YUTU_SONGLI                             "491"
#define LIMIT_TYPE_FSKH_CJX                                    "462"
#define LIMIT_TYPE_TONGMQC_WWJJ                                "431"
#define LIMIT_TYPE_YZDD_WWZB                                   "268"
#define LIMIT_TYPE_PENNANT_ZHUIHUN                             "162"
#define LIMIT_TYPE_BEAT_PIG_BOSS                               "122"
#define LIMIT_TYPE_WULONGSANLI_LHXJ                            "76"
#define HTTPD_HANDLER                                          GLOBAL_INHERIT_DIR "misc/httpd_handler.c"
#define ITEM_LIMIT_SP_PRACTICE_T_MARTIAL                       "7"
#define LIMIT_TYPE_YEYBGD_CREATE_TASK                          "11"
#define LIMIT_TYPE_TOADY_STAT_XING                             "925"
#define LIMIT_TYPE_BONUS_MONITOR_MARTIAL_BIGXL                 "899"
#define LIMIT_TYPE_BONUS_MONITOR_MARTIAL                       "898"
#define LIMIT_TYPE_ARENA_COMBAT_TASK                           "713"
#define LIMIT_TYPE_REPORT_CHEATER                              "677"
#define LIMIT_TYPE_TDXL_ITEM_BONUS                             "509"
#define LIMIT_TYPE_NAILXL_2014_DAOJJL                          "508"
#define LIMIT_TYPE_PARADING_SHOUT                              "412"
#define LIMIT_TYPE_BSSY_JION_TIME                              "252"
#define LIMIT_TYPE_LDQF_ITEM                                   "276"
#define LIMIT_TYPE_MIZHI_HUOJI                                 "174"
#define LIMIT_TYPE_TONGTT_ZQHM                                 "121"
#define LIMIT_TASK_BAIHQF_UPGRADE                              "38"
#define LIMIT_TYPE_BPRCTX_HORN                                 "30"
#define PURCHASE_TRANSFER                                      "shougpt"
#define AAA_DIR                                                "/aaa/"
#define GLOBAL_FEATURE_DIR                                     "/feature/"
#define MAG                                                    ESC "[35m"
#define TODAY_STAT_JY                                          "1"
#define LIMIT_TYPE_BONUS_LOOP_999_SMDL                         "6"
#define LIMIT_TYPE_TD_CYJ2016                                  "881"
#define LIMIT_TYPE_YONGWZQ_DJTCKTY                             "860"
#define LIMIT_TYPE_YAOYAOLE_RATE_10                            "854"
#define LIMIT_TYPE_TD_XIANRZL                                  "766"
#define LIMIT_TYPE_2016NSWD_DASHKY_BYJP                        "729"
#define LIMIT_TYPE_QINGM2016_ITEM                              "715"
#define LIMIT_TYPE_XIXUN_RESCUE_ITEM                           "635"
#define LIMIT_TYPE_SJTQ_YTDL                                   "619"
#define LIMIT_TYPE_SLZ_SUBMIT_2015                             "600"
#define LIMIT_TYPE_JZDK_WWJJ                                   "574"
#define LIMIT_TYPE_GHOST_FESTIVAL_2014                         "471"
#define LIMIT_TYPE_YUTU_SONGLI                                 "325"
#define LIMIT_TYPE_FAMILY_TASK_FAILED                          "295"
#define LIMIT_TYPE_PENNANT_LIANHUN                             "161"
#define LIMIT_TYPE_XIAOBAO_MZG                                 "136"
#define LIMIT_TYPE_TLTX_COMBAT                                 "16"
#define ECARD_TRANSFER                                         "diankjy"
#define HANDLER_D                                              GLOBAL_DAEMONS_DIR "handlerd.c"
#define LIMIT_TYPE_EXCHANGE_SS_SCORE                           "916"
#define LIMIT_TYPE_TD_CBWZ                                     "829"
#define LIMIT_TYPE_DYWD2016_WZXF                               "801"
#define LIMIT_TYPE_DUANWU_2016_ITEM                            "781"
#define LIMIT_TYPE_SHIZN_CQBY_WDTYC                            "756"
#define LIMIT_TYPE_2016NSWD_DASHKY_TOP_EXP                     "726"
#define LIMIT_TYPE_JIUZN_JWZZ_ITEM                             "567"
#define LIMIT_TYPE_QUANMDY_ADVANCE                             "485"
#define LIMIT_TYPE_WANSD_TIAOZHAN_TIMES                        "473"
#define LIMIT_TYPE_DISCOVER_WORLD_EVENT                        "337"
#define LIMIT_TYPE_XRZD                                        "316"
#define LIMIT_TASK_XIAOBAO_YUXIANJI                            "116"
#define LIMIT_DUANWU_2012_CHILD_BOOK_EX                        "113"
#define LIMIT_TYPE_LABOR_2012_XINGUNFU                         "81"
#define LIMIT_TYPE_JZBS_WWMJ                                   "40"
#define LIMIT_TYPE_BXMJ_UPGRADE                                "26"
#define LIMIT_TYPE_LQD                                         "2"
#define DB_D                                                   GLOBAL_DAEMONS_DIR "dbd.c"
#define LIMIT_TYPE_BE_REPORTED_CHEATER                         "910"
#define LIMIT_TYPE_TD_FUM                                      "817"
#define LIMIT_TYPE_SHOUHCE_CHALLENGE                           "806"
#define LIMIT_TYPE_XJYB_2016_BONUS                             "805"
#define LIMIT_TYPE_APPLY_XIUWEIDAN                             "694"
#define LIMIT_TYPE_YJYH3_2015                                  "588"
#define LIMIT_TYPE_LUAND_2015                                  "587"
#define LIMIT_TYPE_HSBH_2015                                   "583"
#define LIMIT_TYPE_JIYRX_WIN                                   "531"
#define LIMIT_TYPE_TONGMQC_EXP                                 "426"
#define LIMIT_TYPE_WFLM                                        "382"
#define LIMIT_TYPE_AWZH_WOYI                                   "328"
#define LIMIT_TYPE_TER_2013_WWJJ                               "315"
#define LIMIT_TYPE_U_XNSL_TASK                                 "305"
#define LIMIT_TYPE_YANHUO_WANHUI                               "176"
#define LIMIT_TYPE_LM_FUBEN                                    "129"
#define LIMIT_TYPE_YXDX_CHILD_EQUIP                            "45"
#define CANNEL_PASSWORD                                        "-1"
#define F_DAEMON_MODULE_DATA                                   GLOBAL_FEATURE_DIR "daemon/module_data.c"
#define DATA_DIR                                               "/data/"
#define COMM_DESC_FILE                                         "comm_desc.txt"
#define GAME_DATA_DIR                                          LOCAL_DIR "pack_data/"
#define LIMIT_TYPE_TD_XIUX                                     "767"
#define LIMIT_TYPE_PENGREN_JIAYAO                              "684"
#define LIMIT_TYPE_SJTQ_TGTY_BONUS                             "623"
#define LIMIT_TYPE_CHUXI_ZHIZHAN                               "528"
#define LIMIT_TYPE_ZHONGZHICC_BONUS                            "483"
#define LIMIT_TYPE_TOWER_DEFENSE_NORMAL                        "466"
#define LIMIT_TYPE_MOTHER_DAY_2013_WWMJ                        "299"
#define LIMIT_TYPE_LUBAN_BOOK_ZHD_3                            "188"
#define LIMIT_TYPE_JUGTQ_CHILD_BOOK                            "150"
#define LIMIT_TYPE_YDJ_CHILD_BOOK                              "52"
#define LIMIT_TYPE_TLTX                                        "14"
#define SPA_DIR                                                "/spa/"
#define GLOBAL_ETC_DIR                                         "/etc/"
#define AGENT                                                  GLOBAL_INHERIT_DIR "misc/agent.c"
#define PET_LIMIT_CWXLY_RECORD                                 "9"
#define LIMIT_TYPE_BONUS_MONITOR_CASH_BANGPFL                  "888"
#define LIMIT_TYPE_VOICE_MSG                                   "878"
#define LIMIT_TYPE_2016SJ_CHONGJSCZL_BOX                       "827"
#define LIMIT_TYPE_WANGWZDZ_NOTICE                             "825"
#define LIMIT_TYPE_TD_CHUB                                     "824"
#define LIMIT_TYPE_SUMMER2016_XIUFBW                           "792"
#define LIMIT_TYPE_2016NSWD_DASHKY_TAO                         "724"
#define LIMIT_TYPE_ZZHY_ITEM                                   "646"
#define LIMIT_TYPE_INVITE_PARTY                                "544"
#define LIMIT_TYPE_MID_AUTUMN_2014                             "469"
#define LIMIT_TYPE_TEACHERS_DAY_2014                           "470"
#define LIMIT_TYPE_TONGMQC_BOTH                                "428"
#define LIMIT_TYPE_RXQF_LIEYANWANG                             "374"
#define LIMIT_TYPE_XINCXL_ITEM_2014                            "372"
#define LIMIT_REPORT_ILLEGAL_REPUTATION                        "344"
#define LIMIT_TYPE_ZHENJIA_XIANREN                             "320"
#define LIMIT_TYPE_CHALLENGE_NPC                               "97"
#define LIMIT_TYPE_BPRCTX_NVWASHI                              "29"
#define BACKUP_DIR                                             "/backup/"
#define DBA_AGENT                                              OBJECTC_DIR "dba_agent.c"
#define TODAY_STAT_BCQX                                        "8"
#define LIMIT_TYPE_CWDTW_DAYS                                  "10"
#define LIMIT_TYPE_MOUNT_STONE_PER_MONTH_EX                    "2"
#define LIMIT_TYPE_MOUNT_STONE_PER_MONTH                       "1"
#define LIMIT_TYPE_BONUS_MONITOR_GLORY_EX                      "927"
#define LIMIT_TYPE_TD_CMMD                                     "837"
#define LIMIT_TYPE_TD_SFCZ                                     "830"
#define LIMIT_TYPE_SHIZN_CQBY_EXP                              "747"
#define LIMIT_TYPE_CHALLENGE_GOD_BOSS                          "368"
#define LIMIT_TYPE_UPGRADE_XNSH_TASK                           "306"
#define LIMIT_TYPE_TIANJIANG_DAREN                             "297"
#define LIMIT_TYPE_ZHENXING_PRACTICE                           "271"
#define LIMIT_TYPE_YZDD_WWJJ                                   "270"
#define LIMIT_TYPE_CHANNEL_FRIEND                              "169"
#define LIMIT_TYPE_XBDWW_COB                                   "107"
#define LIMIT_TYPE_XBDWW_EXCHANGE_U                            "106"
#define LIMIT_TYPE_YDJ_CHILD_EQUIP                             "51"
#define CONFIRM_DLG_FUNC                                       "confirm_dlg_func"
#define TYPE_DMDB                                              "dmdb"
#define BBLU                                                   ESC "[44m"
#define MIXED_AGENT                                            GLOBAL_OBJECTC_DIR "misc/mixed_agent.c"
#define DYNAMIC_KEY_ONLINEMALL                                 "onlinemall"
#define LIMIT_TYPE_MONTH_TAO_DAY                               "7"
#define LIMIT_TYPE_SHAXING_TIMES                               "917"
#define LIMIT_TYPE_BONUS_MONITOR_GLORY                         "900"
#define LIMIT_TYPE_SHENGSIDOU_FAQI                             "845"
#define LIMIT_TYPE_SUMMER2016_XFBW_ITEM                        "795"
#define LIMIT_TYPE_JLXY_2016                                   "784"
#define LIMIT_TYPE_QINGM2016_TASK_BONUS                        "718"
#define LIMIT_TYPE_DUOBDZ_2016                                 "659"
#define LIMIT_TYPE_QMTZ_WWZB                                   "611"
#define LIMIT_TYPE_YJYH2_2015                                  "586"
#define LIMIT_TYPE_JZDK_WWMJ                                   "573"
#define LIMIT_TYPE_USE_COLLECT_CARD                            "545"
#define LIMIT_TYPE_WANSHT_JINPO_2015                           "540"
#define LIMIT_TYPE_BZN_TIANMXK_HUFU                            "434"
#define LIMIT_TYPE_APRIL_FOOL_2014                             "413"
#define LIMIT_TYPE_BAODSS_BOOK                                 "396"
#define LIMIT_TYPE_DUNGEON_TIMES                               "338"
#define LIMIT_TYPE_DUANWU_HUANLEZONG                           "261"
#define LIMIT_TYPE_GHOST_COP_RATE                              "257"
#define LIMIT_TYPE_JZHX                                        "70"
#define GET_TRANSFER                                           "diuqsq"
#define TYPE_ADB                                               "adb"
#define GLOBAL_OBJECTC_DIR                                     "/clone/"
#define BGRN                                                   ESC "[42m"
#define BATCH_LOG_D                                            DBA_DAEMONS_DIR "batch_logd.c"
#define PURCHASE_CLASS                                         "purchase"
#define ITEM_LIMIT_SP_PRACTICE_T_EXP                           "6"
#define LIMIT_TYPE_GQ2016_ZZCF_JP                              "865"
#define LIMIT_TYPE_SHIZN_CQBY_APPLY_TAOZI                      "763"
#define LIMIT_TYPE_SHIZN_HOUGKP                                "760"
#define LIMIT_TYPE_MONTH_TAO_HELP_BONUS                        "672"
#define LIMIT_TYPE_GEJ_HJKP_BONUS                              "649"
#define LIMIT_TYPE_WSJ_2014_JZTG                               "512"
#define LIMIT_TYPE_XINCXL_GIFT_2014                            "371"
#define LIMIT_TYPE_QIXING_JUHUI_FAIL                           "284"
#define LIMIT_FENGSHEN_RANDOM_TASK                             "225"
#define LIMIT_TYPE_IMP_TIP_SELL                                "166"
#define LIMIT_TYPE_PET_PRACTICE_TIP                            "164"
#define LIMIT_TYPE_WXZS_CHILD_EQUIP                            "144"
#define LIMIT_TYPE_CHILD_DAILY_COMBAT                          "96"
#define LIMIT_TASK_WINTER_NOTES                                "35"
#define DROP_TRANSFER                                          "diuqsq"
#define TYPE_TIME_LIMIT                                        "time_limit"
#define FILE_D                                                 GLOBAL_DAEMONS_DIR "filed.c"
#define GLOBAL_CMDS_DIR                                        "/cmds/"
#define CMDS_DIR                                               LOCAL_DIR "cmds/"
#define LIMIT_TYPE_DSFM_JP_BONUS                               "734"
#define LIMIT_TYPE_JIUJGY_2015_RHS                             "578"
#define LIMIT_TYPE_JIUJZ_HSJW                                  "577"
#define LIMIT_TYPE_BZN_YAOSRQ_HUFU                             "432"
#define LIMIT_TYPE_REFRESH_CITY_TASK                           "282"
#define LIMIT_TYPE_GIVEUP_HTASK_UPGRADE                        "160"
#define DISCONNECT_BY_UNKNOWN                                  "unknown"
#define PEEL_DATA_D                                            DBA_DAEMONS_DIR "peel_datad.c"
#define LIMIT_TYPE_YEAR_INSIDER_NAMEPLATE                      "10"
#define LIMIT_TYPE_WANSHENGJIE_2015                            "660"
#define LIMIT_TYPE_NAILXL_2014_ZHANDJL                         "507"
#define LIMIT_TYPE_WENXIN_FUAI                                 "446"
#define LIMIT_TYPE_LAOBING_SHILIAN                             "443"
#define LIMIT_TYPE_LUCKY_SIGN                                  "359"
#define LIMIT_TYPE_XIANGZHU_DAOYOU                             "345"
#define LIMIT_TYPE_DAOCHONG_ZIQHM                              "343"
#define LIMIT_TYPE_TER_2013_WWZB                               "313"
#define LIMIT_TYPE_ZSCF_CHILD_BOOK                             "218"
#define LIMIT_TYPE_MAOSHAN_BAOWEI                              "219"
#define LIMIT_TYPE_XBDWW_EXCHANGE                              "105"
#define LIMIT_TYPE_TIP_SELL                                    "92"
#define LIMIT_TYPE_QJYY_UPGRADE                                "7"
#define TYPE_LEVEL_LIMIT                                       "level_limit"
#define ENCRYPT_D                                              GLOBAL_DAEMONS_DIR "encryptd.c"
#define XML_NODE_ALL_PROPS                                     @A["_all_props"]
#define XIAOMS_URL_TEST                                        "http://bbs.asktao.com/xms/"
#define USER_ORDER                                             "1"
#define LIMIT_TYPE_MONTH_TAO_RATE                              "9"
#define LIMIT_TYPE_HELP_LOOP_300                               "14"
#define LIMIT_TYPE_TD_XJYB                                     "840"
#define LIMIT_TYPE_SUMMER_2016_SAOFCZ                          "788"
#define LIMIT_TYPE_TD_XUNX                                     "769"
#define LIMIT_TYPE_YHDZZ_BONUS_TIMES                           "710"
#define LIMIT_TYPE_SJTQ_ZZGS_TASK                              "610"
#define LIMIT_TYPE_LUAND_EX_2015                               "591"
#define LIMIT_TYPE_ARBOR_DAY_GONGJX                            "551"
#define LIMIT_TYPE_RA_MP                                       "350"
#define LIMIT_TYPE_LBD_2013_TASK                               "249"
#define USER_LOG                                               "user_log"
#define SYSTEM_MONITOR_D                                       GLOBAL_DAEMONS_DIR "system_monitord.c"
#define USER_INFO                                              "2"
#define DYNAMIC_KEY_MENU                                       "menu_key"
#define LIMIT_TYPE_TD_MHHS                                     "834"
#define LIMIT_TYPE_TEACHERS_DAY_2016_ZH                        "803"
#define LIMIT_TYPE_2016CJ_YUANLTZ_WWZB                         "690"
#define LIMIT_TYPE_CHENGRKY_WWJJ                               "561"
#define LIMIT_TYPE_YANHUA_QUXIE_COMBAT                         "518"
#define LIMIT_TYPE_GHOST_2014_COB_BONUS                        "475"
#define LIMIT_TYPE_WENNUAN_MUAI_MIJI                           "422"
#define LIMIT_TYPE_NINGJU_FASHU_ITEM                           "401"
#define LIMIT_TYPE_ZHMCZ_POT                                   "386"
#define LIMIT_TYPE_XIYING_XINCHUN                              "223"
#define LIMIT_TYPE_CY_JL_2012                                  "178"
#define LIMIT_TYPE_JUGTQ_CHILD_EQUIP                           "149"
#define LIMIT_TYPE_XIAOBAO_MZG_UPGRADE                         "135"
#define LIMIT_TYPE_JZHX_WWJJ                                   "79"
#define LIMIT_TYPE_GJ_LQD                                      "1"
#define F_COMN_NAME                                            GLOBAL_FEATURE_DIR "common/name.c"
#define TODAY_STAT_ZDCC                                        "10"
#define TODAY_STAT_DH                                          "2"
#define ITEM_LIMIT_TAIXSJ                                      "1"
#define LIMIT_TYPE_BONUS_LOOP_300_SMDL                         "15"
#define LIMIT_TYPE_SHENGSIDOU_JIESHOU                          "846"
#define LIMIT_TYPE_TD_BBSY                                     "841"
#define LIMIT_TYPE_SHIZN_CQBY_GX20                             "752"
#define LIMIT_TYPE_LABOR_DAY_2016                              "676"
#define LIMIT_TYPE_YIZCM_WWZB                                  "522"
#define LIMIT_TYPE_INSTRUCTION_V4_82                           "476"
#define LIMIT_TYPE_NEW_LOOP_TASK_30                            "441"
#define LIMIT_TYPE_CHAR_BIRTHDAY_GIFT                          "332"
#define LIMIT_TYPE_CITY_TASK_TIME                              "286"
#define LIMIT_TYPE_HUSHENGLING_EXTRA                           "139"
#define LIMIT_XIAOBAO_RERENAI_GIFT                             "114"
#define LIMIT_TYPE_TQLB_MIJI_TIMES                             "109"
#define SHOP_TRANSFER                                          "shangh"
#define THREAD_D                                               GLOBAL_DAEMONS_DIR "threadd.c"
#define LEAVE_MSG_CLASS                                        "leave_msg"
#define LIMIT_TYPE_WANGSJ_2016_TAO                             "920"
#define LIMIT_TYPE_BONUS_SS_SCORE                              "918"
#define LIMIT_TYPE_TD_JQKH                                     "873"
#define LIMIT_TYPE_SUMMER_2016_BIAOXXY_EX                      "790"
#define LIMIT_TYPE_SHIZN_DXHX_ITEM                             "740"
#define LIMIT_TYPE_YUQSD_CHALLENGE_COB                         "632"
#define LIMIT_TYPE_DATE_ACTIVITY                               "538"
#define LIMIT_TYPE_FUXING_GAOZHAO                              "526"
#define LIMIT_TYPE_WSJ_2014_EXCHANGE                           "511"
#define LIMIT_TYPE_ZHJQKJWK_DAZUO                              "451"
#define LIMIT_TYPE_QINGMING2014                                "435"
#define LIMIT_TYPE_BZN_ZHOUNDL_HONGB                           "424"
#define LIMIT_TYPE_ZHMCZ_WWJJ                                  "389"
#define LIMIT_TYPE_YUANLAI_SHINI                               "379"
#define LIMIT_QINGRJ_2013_CHILD_EQUIP                          "233"
#define LIMIT_TYPE_GIVEUP_HTASK                                "159"
#define LIMIT_TYPE_NYX_CHILD_EQUIP                             "48"
#define LIMIT_PER_WEEK                                         "limit_per_week"
#define WARCRAFT_TRANSFER                                      "leitjyj"
#define COMMUNICATE_D                                          GLOBAL_DAEMONS_DIR "communicated.c"
#define LOCAL_DIR                                              "/dba/"
#define LIMIT_TYPE_CYJ2016_SUPER                               "885"
#define LIMIT_TYPE_CWXLY_RECORD                                "883"
#define LIMIT_TYPE_DAILY_GLORY                                 "872"
#define LIMIT_TYPE_XYFS_2016_BONUS                             "851"
#define LIMIT_TYPE_CHALLENGE_MASTER                            "826"
#define LIMIT_TYPE_SHIZN_DXHX_SUB_ITEM                         "765"
#define LIMIT_TYPE_10ZN_WSBOSSWZZ_EX                           "762"
#define LIMIT_TYPE_YANHQX_YHT_COMBAT                           "542"
#define LIMIT_TYPE_JIYRX_LIEYAN                                "529"
#define LIMIT_TYPE_XIUBU_XUEREN                                "499"
#define LIMIT_TYPE_BAOWEI_XIGUA                                "458"
#define LIMIT_TYPE_CUL_SELF_TAOSHU                             "405"
#define LIMIT_TYPE_ZHMCZ_EXP                                   "384"
#define LIMIT_TYPE_QUZHUNIANSHOU_BONUS                         "209"
#define LIMIT_TYPE_STAT_SW_PER_DAY                             "86"
#define LIMIT_TYPE_TLTX_COMBAT_UPGRADE                         "17"
#define LIMIT_TYPE_SMPN                                        "10"
#define BRED                                                   ESC "[41m"
#define FILE_DEPENDENCE_LIST                                   "file_dependence.list"
#define USER_TYPE_GID                                          "user"
#define LIMIT_TYPE_LAOBBS_XUANTIANLING                         "18"
#define LIMIT_TYPE_TD_GGJ2016                                  "930"
#define LIMIT_TYPE_WL_TIP                                      "843"
#define LIMIT_TYPE_XJYB_2016_ITEM                              "804"
#define LIMIT_TYPE_SHIZN_CQBY_DSKY_POT                         "759"
#define LIMIT_TYPE_2016NSWD_ZAIXKH_RWWC                        "744"
#define LIMIT_TYPE_2016NSWD_DASHKY_WWZB                        "730"
#define LIMIT_TYPE_2016CJ_YUANLTZ_WWJJ                         "692"
#define LIMIT_TYPE_DANTY_2015_BONUS                            "596"
#define LIMIT_TYPE_DOUCTZS_GET_TASK                            "548"
#define LIMIT_TYPE_WANSHI_HENGTONG_2015                        "527"
#define LIMIT_TYPE_GUOQING_BAOXIANG                            "486"
#define LIMIT_TYPE_INSTRUCTION_V4_85                           "477"
#define LIMIT_TYPE_HSSM                                        "418"
#define LIMIT_TYPE_APPLY_ZWX_ITEM                              "407"
#define LIMIT_TYPE_DUNGEON_MAX_TIMES                           "339"
#define LIMIT_TYPE_YDJ_CHILD_STUN_BOOK                         "53"
#define F_DAEMON_ATTENTION                                     GLOBAL_FEATURE_DIR "daemon/attention.c"
#define XG_USER_CARD_HERO                                      "user_card_hero"
#define TODAY_STAT_SDCC                                        "11"
#define LIMIT_TYPE_WANGSJ_2016_EXP                             "919"
#define LIMIT_TYPE_DYWD2016_WZQF                               "800"
#define LIMIT_TYPE_CJ2016_ZDGC_BONUS                           "699"
#define LIMIT_TYPE_GEJ_HJKP_BONUS_ITEM                         "651"
#define LIMIT_TYPE_WLL_ZRWL                                    "637"
#define LIMIT_TYPE_SJTQ_SSFH_JP                                "622"
#define LIMIT_TYPE_JIUZN_JZTQ_HB                               "575"
#define LIMIT_TYPE_CHALIU_QUXIE                                "549"
#define LIMIT_TYPE_JINGYXB_2014                                "487"
#define LIMIT_TYPE_REMOTE_JOIN_PARTY                           "402"
#define LIMIT_TYPE_CHAR_PRACTICE_BONUS                         "357"
#define LIMIT_TYPE_QIXING_JUHUI_REQUEST                        "285"
#define LIMIT_SUBMIT_HONGMU_NUM                                "44"
#define F_COMN_MODULE_STAT                                     GLOBAL_FEATURE_DIR "common/module_stat.c"
#define LOG_DIR                                                "/dba/log/"
#define LIMIT_TYPE_BONUS_MONITOR_CASH                          "886"
#define LIMIT_TYPE_TD_ZZCF                                     "877"
#define LIMIT_TYPE_GQ2016_ZZCF_ITEM                            "866"
#define LIMIT_TYPE_YONGWZQ_ITEM_BONUS                          "861"
#define LIMIT_TYPE_ONLINEMALL_REBATE_NOTE                      "828"
#define LIMIT_TYPE_TD_BXXY                                     "833"
#define LIMIT_TYPE_TD_XFBW                                     "831"
#define LIMIT_TYPE_SLZ_TASK_2015                               "598"
#define LIMIT_TYPE_TOWER_DEFENSE                               "465"
#define LIMIT_TYPE_HUSONG_MIJIAN                               "392"
#define LIMIT_TYPE_CYQF                                        "346"
#define LIMIT_TYPE_DRAFT_TIP                                   "192"
#define LIMIT_TYPE_QINGMING_2012_TJMJCP                        "72"
#define LIMIT_TYPE_BEAT_LION_BOSS                              "22"
#define EMPTY_STRING                                           ""
#define LIMIT_TYPE_BONUS_MONITOR_REPUTATION                    "901"
#define LIMIT_TYPE_ZHENYZB_SCORE                               "810"
#define LIMIT_TYPE_2016SUMMER_CHOUBWZ                          "798"
#define LIMIT_TYPE_CJ2016_JHJC_JP                              "685"
#define LIMIT_TYPE_JITWK_2015_TASK                             "629"
#define LIMIT_TYPE_XIANREN_ZHUABXR_2015                        "592"
#define LIMIT_TYPE_CLEAN_SUPER_2015                            "565"
#define LIMIT_TYPE_FISH_SWALLOW_BONUS                          "494"
#define LIMIT_TYPE_GDQQ_BOX                                    "449"
#define LIMIT_TYPE_ZHMCZ_TAO                                   "385"
#define LIMIT_TYPE_GET_TIANJHL_TICKET                          "262"
#define LIMIT_TYPE_TZSS_JION_TIME                              "253"
#define LIMIT_TYPE_YASUI_HONGBAO_ITEM                          "206"
#define LIMIT_CIJYX_2013_CHILD_BOOK_EX                         "213"
#define LIMIT_TYPE_STAT_BCQX_PER_DAY                           "88"
#define LIMIT_TYPE_TQLB_JUEJI_TIMES                            "110"
#define LIMIT_TYPE_BXMJ                                        "25"
#define TYPE_LDB                                               "ldb"
#define GLOBAL_DAEMONS_DIR                                     "/daemons/"
#define PARTY_TYPE_GID                                         "party"
#define DYNAMIC_KEY_VERIFY_PASSWORD                            "verify_password"
#define LIMIT_TYPE_TD_XYFS                                     "863"
#define LIMIT_TYPE_TD_RCRW                                     "844"
#define LIMIT_TYPE_QIX2016_FHXD_TOTAL                          "813"
#define LIMIT_TYPE_NEZHA_XUNBAO_2015_BONUS                     "680"
#define LIMIT_TYPE_WLL_BXMJ                                    "638"
#define LIMIT_TYPE_KUNLUNJING                                  "580"
#define LIMIT_TYPE_HELL_ZHUANSHENG                             "356"
#define LIMIT_TYPE_BXMJ_EXTRA                                  "354"
#define LIMIT_TYPE_YINYUAN_XINGYUNDAI                          "236"
#define LIMIT_TYPE_WJDH_TASK                                   "221"
#define LIMIT_TYPE_HUOJI_KUAIPAO                               "173"
#define LIMIT_XIAOBAO_RERENAI_ITEM                             "115"
#define LIMIT_TYPE_YOULONGGONG_UPGRADE                         "118"
#define LIMIT_TYPE_DUANWU_2012                                 "98"
#define LIMIT_TYPE_YZRQ                                        "62"
#define LIMIT_TYPE_SUBMIT_FOOD                                 "20"
#define LIMIT_TYPE_BEAT_BEAR_BOSS                              "19"
#define TYPE_MAP_LIMIT                                         "map_limit"
#define HTTP_CLIENT_D                                          GLOBAL_DAEMONS_DIR "http_clientd.c"
#define CTA_D                                                  GLOBAL_DAEMONS_DIR "ctad.c"
#define TYPE_DDB                                               "ddb"
#define GLOBAL_FILE_DEPENDENCE_LIST                            "global_file_dependence.list"
#define INTERSRV_CMDS_DIR                                      CMDS_DIR + "intersrv/"
#define CURRENT_VERSION                                        "1.60.0905"
#define LIMIT_TYPE_SAME_CITY_FREE_QUOTA                        "864"
#define LIMIT_TYPE_ZHENYZB_ITEM                                "914"
#define LIMIT_TYPE_CJ2016_YXZM_YLTZ                            "705"
#define LIMIT_TYPE_XIXUN_RESCUE                                "634"
#define LIMIT_TYPE_JIUZN_JWZZ_JP                               "566"
#define LIMIT_TYPE_TQZL                                        "553"
#define LIMIT_TYPE_BXMJ_EXTRA_UPGRADE                          "355"
#define LIMIT_TYPE_XUNXIAN_HOURS                               "190"
#define LIMIT_TYPE_GXKY_CHILD_BOOK                             "147"
#define LIMIT_TYPE_GUIXIANG_JINQIU                             "140"
#define LIMIT_TYPE_TCHW_WWMJ                                   "133"
#define LIMIT_TYPE_CHILDREN_DAY_2012                           "104"
#define LIMIT_TYPE_CIJYX_UPGRADE                               "55"
#define LIMIT_TYPE_KILL_EGUI                                   "5"
#define F_COMN_UPDATEHOOK                                      GLOBAL_FEATURE_DIR "common/updatehook.c"
#define TYPE_MDB                                               "mdb"
#define BYEL                                                   ESC "[43m"
#define BBLK                                                   ESC "[40m"
#define GRN                                                    ESC "[32m"
#define LIMIT_TYPE_YAOYAOLE_RATE_5                             "855"
#define LIMIT_TYPE_QIX2016_FHXD_TASK                           "815"
#define LIMIT_TYPE_YHDZZ_CUMULATIVE_TIME                       "711"
#define LIMIT_TYPE_SLZ_BYJP_2015                               "614"
#define LIMIT_TYPE_CHENGRKY_EXP                                "556"
#define LIMIT_TYPE_YIZCM_WWMJ                                  "523"
#define LIMIT_TYPE_ZHENGMING_ZHIZHAN_DJJL                      "516"
#define LIMIT_TYPE_ZHENGMING_ZHIZHAN_DHJL                      "514"
#define LIMIT_TYPE_QIECJF_2014                                 "506"
#define LIMIT_TYPE_HJTZ_2014                                   "501"
#define LIMIT_TYPE_FSKH_TJX                                    "463"
#define LIMIT_TYPE_AWZH_WOWANG                                 "329"
#define LIMIT_TYPE_QMJZ_COB                                    "243"
#define LIMIT_TASK_ARBOR_DAY_2013                              "245"
#define LIMIT_TYPE_CUL_OTHER_TREE                              "126"
#define CYN                                                    ESC "[36m"
#define MYSQL_D                                                DBA_DAEMONS_DIR "mysqld.c"
#define CARRY_BRANCH                                           "carry"
#define LIMIT_TYPE_MONTH_TAO_POINT                             "8"
#define LIMIT_TYPE_SINGLES_DAY_2016                            "924"
#define LIMIT_TYPE_TIME_LUCK_PRAY                              "868"
#define LIMIT_TYPE_LUCKY_B_LEIJI_TIMES                         "746"
#define LIMIT_TYPE_2016NSWD_DASHKY_TOP_POT                     "728"
#define LIMIT_TYPE_NEZHA_HUDONGHAI                             "679"
#define LIMIT_TYPE_QMTZ_POT                                    "618"
#define LIMIT_TYPE_JIUTIAN_LANYUE                              "568"
#define LIMIT_TYPE_TONGMQC_WWZB                                "429"
#define LIMIT_TYPE_DROP_SSWZ                                   "303"
#define LIMIT_TYPE_WV2013_COB                                  "201"
#define LIMIT_TASK_CISHA_YAOMO_UPGRADE                         "198"
#define LIMIT_TYPE_STAT_BG_PER_DAY                             "87"
#define LIMIT_TYPE_JZHX_WWMJ                                   "78"
#define LIMIT_TYPE_BEAT_HEDGEHOG_BOSS                          "74"
#define LIMIT_PER_MONTH                                        "limit_per_month"
#define XML_CHILD_NODES                                        @A["_childs"]
#define ITEM_LIMIT_PRACTICE_TIME_EXP                           "2"
#define LIMIT_TYPE_RAID_DUNGEON_BONUS_WEEK                     "7"
#define LIMIT_TYPE_CANPIAN_BONUS                               "2"
#define LIMIT_TYPE_SUMMER2016_TONGGWD                          "791"
#define LIMIT_TYPE_2016NSWD_ZAIXKH_PET                         "742"
#define LIMIT_TYPE_NZPMS_COB_TIMES                             "682"
#define LIMIT_TYPE_YMNS_2015                                   "594"
#define LIMIT_TYPE_CS_ACTIVITY_COIN                            "589"
#define LIMIT_TYPE_WUXIN_QIUQIU                                "537"
#define LIMIT_TYPE_YIZCM_TAO                                   "520"
#define LIMIT_TYPE_HUNT                                        "479"
#define LIMIT_TYPE_FSHZHBU                                     "456"
#define LIMIT_TYPE_HUAZHONG_XUNREN                             "398"
#define LIMIT_TYPE_SSWZ_STAGE                                  "302"
#define LIMIT_TYPE_YZDD_WWMJ                                   "269"
#define LIMIT_TYPE_ZAOSHEN_CIFU                                "207"
#define LIMIT_TYPE_CIJYX_2013_XSZW                             "215"
#define LIMIT_CIJYX_2013_CHILD_EQUIP                           "211"
#define LIMIT_TYPE_JQHL_WWMJ                                   "182"
#define LIMIT_TYPE_YXDX_CHILD_STUN_BOOK                        "47"
#define LIMIT_TYPE_QJYY_COMBAT                                 "8"
#define LIMIT_TYPE_TD_BANGPRC                                  "820"
#define LIMIT_TYPE_BOBING_2016_TIMES                           "808"
#define LIMIT_TYPE_GHOST_FESTIVAL_2016                         "794"
#define LIMIT_TYPE_JIYHG_USE_XIANYYR                           "774"
#define LIMIT_TYPE_SHIZN_ZNXB_BONUS                            "722"
#define LIMIT_TYPE_2016CJ_YUANLTZ_TAO                          "688"
#define LIMIT_TYPE_BKQS_BONUS                                  "631"
#define LIMIT_TYPE_JZDK_EXP                                    "569"
#define LIMIT_TYPE_CHENGRKY_WWMJ                               "560"
#define LIMIT_TYPE_HMXM_TEST_TIP                               "397"
#define LIMIT_TYPE_NIANSHOU_RUQIN_BONUS                        "378"
#define LIMIT_TYPE_XNSH_TASK                                   "307"
#define LIMIT_TYPE_XZJZ                                        "292"
#define LIMIT_TYPE_CD2013_JUEJI                                "281"
#define LIMIT_TYPE_JQHL_WWZB                                   "181"
#define LIMIT_TYPE_HOUSE_TASK                                  "157"
#define LIMIT_TYPE_ZOUQFY_2011                                 "34"
#define F_COMN_DBASE                                           GLOBAL_FEATURE_DIR "common/dbase.c"
#define CHECK_DBASE_D                                          GLOBAL_DAEMONS_DIR "check_dbased.c"
#define DISCONNECT_BY_RULE                                     "rule_matched"
#define LIMIT_TYPE_BONUS_MONITOR_TAO                           "895"
#define LIMIT_TYPE_SGYW_FLEE                                   "779"
#define LIMIT_TYPE_2016NSWD_DASHKY_WWMJ                        "731"
#define LIMIT_TYPE_NZPMS_PRAISE_TI                             "708"
#define LIMIT_TYPE_XIXUNLING                                   "633"
#define LIMIT_TYPE_YISXM_DAOJJL                                "310"
#define LIMIT_TYPE_ZHENQING_HONGBAO                            "247"
#define LIMIT_TYPE_MEIGUI_JIQING                               "237"
#define VENDUE_TRANSFER                                        "paim"
#define F_COMN_SAVE                                            GLOBAL_FEATURE_DIR "common/save.c"
#define XML_LEAF_VALUE                                         @A["_value"]
#define RECORD_INFO                                            "3"
#define LIMIT_TYPE_NEZHA_XUNBAO_2015_TASK                      "681"
#define LIMIT_TYPE_CLEAN_LAJI_2015                             "564"
#define LIMIT_TYPE_DOUCTZS_FAIL                                "547"
#define LIMIT_TYPE_NORMAL_QIXING                               "539"
#define LIMIT_TYPE_JINQIU_CIFU                                 "481"
#define LIMIT_TYPE_BZN_ZHOUNDL_JINC                            "425"
#define LIMIT_TYPE_BZN_TIANMXK                                 "421"
#define LIMIT_TYPE_TIANCI_SHENCHONG                            "317"
#define LIMIT_TYPE_DUOBAO_QIBING                               "163"
#define LIMIT_TYPE_SZWH_PET                                    "142"
#define LIMIT_TYPE_STAT_ZDCC_PER_DAY                           "90"
#define LIMIT_TYPE_XINFA_EXP_WEEK                              "848"
#define LIMIT_TYPE_YEYBGD_XG_BONUS                             "775"
#define LIMIT_TYPE_YHDZZ_GET_BONUS_TIMES                       "712"
#define LIMIT_TYPE_REP_CARD_QUARTER                            "669"
#define LIMIT_TYPE_WLL_KLSJ                                    "666"
#define LIMIT_TYPE_DASAOCHU_2015                               "563"
#define LIMIT_TYPE_TDXL_KILL_NUM                               "510"
#define LIMIT_TYPE_SHENGDAN_LAOREN_2013                        "370"
#define LIMIT_TYPE_HETU_LUOSHU                                 "353"
#define LIMIT_TYPE_FAMILY_VISIT                                "296"
#define LIMIT_TYPE_DFZDY_JION_TIME                             "256"
#define LIMIT_TYPE_YZDD_TAO                                    "265"
#define LIMIT_TYPE_RECEIVE_GANENKA                             "172"
#define LIMIT_TYPE_CHALLENGE_XING_RATE                         "137"
#define LIMIT_TYPE_TD_FHXD                                     "836"
#define LIMIT_TYPE_SHIZN_CQBY_POT                              "749"
#define LIMIT_TYPE_CJ2016_YXZM_JHJC                            "704"
#define LIMIT_TYPE_CJ2016_FUZTY                                "693"
#define LIMIT_TYPE_JITIAN_QIFU_BONUS                           "656"
#define LIMIT_TYPE_QMTZ_EXP                                    "616"
#define LIMIT_TYPE_QMTZ_WWMJ                                   "612"
#define LIMIT_TYPE_JIYRX_SNOWMAN                               "532"
#define LIMIT_TYPE_YIZCM_POT                                   "521"
#define LIMIT_TYPE_DWJ_ZY                                      "416"
#define LIMIT_U_COMBAT_WINTER_NOTES                            "43"
#define LIMIT_TYPE_QJYY                                        "6"
#define F_DAEMON_REGISTER                                      GLOBAL_FEATURE_DIR "daemon/register.c"
#define LIMIT_TYPE_BONUS_MONITOR_DAOFA_XIUFRW                  "897"
#define LIMIT_TYPE_TD_TDYM                                     "835"
#define LIMIT_TYPE_LUCKY_B_EXTRA                               "745"
#define LIMIT_TYPE_CJ2016_QGXS                                 "698"
#define LIMIT_TYPE_2016CJ_YUANLTZ_EXP                          "687"
#define LIMIT_TYPE_QQXJ_2015_BOX                               "643"
#define LIMIT_TYPE_QUGUI_CHUXIE_2015                           "603"
#define LIMIT_TYPE_LUAND_2015_SPECIAL                          "590"
#define LIMIT_TYPE_CHENGRKY_POT                                "558"
#define LIMIT_TYPE_XIUXING_RECEIVE_TIME                        "288"
#define LIMIT_TYPE_QIXING_JUHUI_WIN                            "283"
#define LIMIT_TYPE_BOSS_ARREST                                 "100"
#define LIMIT_TYPE_RESUME_ASKTAO_GIFT                          "57"
#define SORT_ERROR_D                                           GLOBAL_DAEMONS_DIR "sorterrord.c"
#define RECYCLE_D                                              GLOBAL_DAEMONS_DIR "recycled.c"
#define LOG_SSS_D                                              GLOBAL_DAEMONS_DIR "log_server_startup_shutdownd.c"
#define BMAG                                                   ESC "[45m"
#define LIMIT_TYPE_TD_XHXS                                     "839"
#define LIMIT_TYPE_WUDAO_EXTRA_REP                             "674"
#define LIMIT_TYPE_SIMP_XUNLIAN_BONUS                          "664"
#define LIMIT_TYPE_SLZ_ITEM_2015                               "599"
#define LIMIT_TYPE_YUELAO_HENMANG                              "534"
#define LIMIT_TYPE_YIZCM_WWJJ                                  "524"
#define LIMIT_TYPE_HERO_BOSS_YM_BONUS                          "340"
#define LIMIT_TYPE_MOTHER_DAY_2013_WWJJ                        "300"
#define LIMIT_TYPE_YONGZHAN_MOWANG                             "202"
#define LIMIT_TYPE_LUBAN_BOOK_TTT                              "185"
#define LIMIT_TYPE_YXB_2012                                    "32"
#define LIMIT_TYPE_MID_AUTUMN_2011                             "23"
#define LOGIN_NOTIFY_D                                         GLOBAL_DAEMONS_DIR "loginnotifyd.c"
#define XGS_DIR                                                "/xgs/"
#define GLOBAL_HANDLERS_DIR                                    "/handlers/"

// --- Reference Constants (17) ---
#define EMPTY_OBJECT                                           EMPTY_ARRAY
#define XDBC_TRACE                                             xdbc_trace
#define SERVER_TYPE                                            CFG_TYPE_DBA
#define CMD_SELECT_MENU_ITEM                                   CMD_SELECT_MENU_ITEM_0
#define pp_stop_profiling                                      pp_stop_profiling_ex
#define CMD_OPEN_MENU                                          CMD_OPEN_MENU_0
#define DAEMONS_DIR                                            DBA_DAEMONS_DIR
#define XML_NODE                                               mapping
#define XDBC_LOG                                               xdbc_log
#define COMPILER_ACTION                                        compiler_action
#define clone_object_m                                         clone_object
#define pp_start_profiling                                     pp_start_profiling_ex
#define STRSRCH                                                strsrchw
#define NO_DATA                                                OPERATE_FAILED
#define CMD_C_DO_ACTION                                        CMD_C_DO_ACTION_0
#define CMD_ENTER_ROOM                                         CMD_ENTER_ROOM_0
#define pp_get_report                                          pp_get_report_ex

// --- Code Macros (204) ---
#define LOG_TASK                                               FILE_D->log_task(@A, @B, @C, @D, @E, @F, @G, @H)
#define BONUS_NAME_MAP                                         ([     BONUS_TYPE_EXP         : $$912610,     BONUS_TYPE_TAO         : $$937227,     BONUS_TYPE_POT         : $$912611,     BONUS_TYPE_MARTIAL     : $$937226,     BONUS_TYPE_TOTAL_SCORE : $$912612,     BONUS_TYPE_REPUTATION  : $$912613,     BONUS_TYPE_INTIMACY    : $$292174,     BONUS_TYPE_NICE        : $$940461,     BONUS_TYPE_MERIT       : $$974940, ])
#define BONUS_DOWN_LIMIT                                       ((@A) < 1 ? 1 : (@A))
#define XGAME_COMPETE_DIST                                     (CONFIG_D->is_xgame_compete_dist())
#define PARTYWAR_COMPETE_DIST                                  (CONFIG_D->is_csp_compete_dist())
#define LOG_PET_LV_UP                                          FILE_D->log_pet_lv_up(@A, @B, @C, @D, @E, @F, @G, @H, @I)
#define LOG_MATERIAL1                                          FILE_D->log_material1(@A, @B, @C, @D, @E, @F, @G,                           @H, @I, @J)
#define LOG_APPLY                                              catch(FILE_D->log_apply(@A, @B, @C, @D, @E, @F, @G, @H, @I))
#define ECHO_MILSEC_DEAD_INTERVAL                              (ECHO_LONG_DEAD_INTERVAL * 1000)
#define GET_CHAR_GID_TO                                        FILE_D->get_gid_to(USER_TYPE_GID, @A, @B)
#define LOG_EQUIPMENT                                          FILE_D->log_equipment(@A, @B, @C, @D, @E, @F,                 @G, @H, @I, @J, @K, @L, @M)
#define XML_HAS_CHILD                                          (! is_undefined(XML_CHILD_NODES_EX(@A)[@B]))
#define mysql_get_error                                        (*f_get_error)
#define LOAD_OR_COMPILE_ALL_PATH                               (*load_or_compile_all_path)
#define LOG_GM                                                 FILE_D->log_gm(@A, @B, @C, @D, @E, @F, @G, @H, @I, @J,                    @K, @L, @M, @N)
#define LOG_GBUY_ACTION                                        FILE_D->log_gbuy_action(@A, @B, @C, @D, @E,                              @F, @G, @H, @I, @J, @K, @L, @M, @N)
#define HIR                                                    ESC "[1;31m"
#define TAKE_REMOTE_MUTEX                                      REMOTE_SEND(@A, SYNC_D, "take_mutex_by_remote", @B,                 LOCAL_THREAD_NAME, @C, @D)
#define LOG_WISHING                                            FILE_D->log_wishing(@A, @B, @C, @D)
#define LOG_TO_PROPERTY_RECALL                                 FILE_D->log_to_property_recall(@A, @B, @C, @D, @E, @F, @G, @H)
#define IS_XML                                                 is_mapping(@A)
#define CANCEL_PASSWORD_MD5                                    (save_string(md5(CANNEL_PASSWORD))[1..<2])
#define ZERO8                                                  make8(0)
#define ACTOR_PORTRAIT_DESC                                    sprintf("%s:%d", @A, @B)
#define LOG_DEBUG                                              FILE_D->log_debug(@A, @B, @C, @D, @E, @F, @G)
#define LOG_IMPORTANT_PET_EX                                   FILE_D->log_important_pet(@A, @B, @C, @D, @E, @F, @G)
#define LOG_SAFE                                               FILE_D->log_safe(@A, @B, @C, @D, @E, @F, @G, @H, @I, @J, @K)
#define SPA_SEND_TO_CCS                                        SPA_CCS_CLIENT_D->send_to_ccs
#define HBCYN                                                  ESC "[46;1m"
#define mysql_select_db                                        (*f_select_db)
#define LOG_ZQ_COST_POINT                                      FILE_D->log_zq_cost_point(@A, @B, @C, @D, @E, @F, @G, @H)
#define DELETE_GID                                             FILE_D->delete_gid(@A)
#define SEND_TO_TTS                                            TTS_CLIENT_D->send_to_tts
#define ACTOR_PORTRAIT_OB                                      sprintf("%s:%d", @A->get_name(), (int) @A->query("portrait"))
#define LOG_ZQ_CARD                                            FILE_D->log_zq_card(@A, @B, @C, @D, @E)
#define LOG_IMPORTANT_PET                                      FILE_D->log_important_pet(@A, @B, @C)
#define LOG_TO_DB                                              FILE_D->log_to_db(DB_LOG, @A, @B, @C, @D, @E, @F, @G)
#define SPA_SEND_TO_TTS                                        SPA_TTS_CLIENT_D->send_to_tts
#define REMOTE_BROADCAST_CALLBACK                              RPC_D->broadcast_callback
#define NOTIFY_LESS_ITEM                                       (@A->prompt_by(@B,  $$956382))
#define NOTIFY_UNEXCEPT_ITEM                                   (@A->prompt_by(@B, $$956381))
#define LOCAL_THREAD_NAME                                      (CONFIG_D->get_my_info()["name"] + "-" + get_thread_id())
#define LOG_CSC_COMBAT                                         FILE_D->log_csc_combat(@A, @B, @C, @D, @E, @F, @G,                            @H, @I, @J, @K)
#define LOG_SERVER_STAT                                        FILE_D->log_server_stat(@A, @B, @C, @D, @E, @F, @G,                             @H, @I, @J)
#define GET_GID_INFO                                           FILE_D->get_gid_info(@A, @B, @C)
#define XIAOMS_URL                                             (is_test() ? XIAOMS_URL_TEST : XIAOMS_URL_NOT_TEST)
#define LOG_CSP_COMBAT                                         FILE_D->log_csp_combat(@A, @B, @C, @D, @E, @F, @G, @H, @I, @J)
#define DELETE_GID_FAILED                                      FILE_D->delete_gid(@A)
#define RANDOM_LIST                                            ((@A)[random(sizeof(@A))])
#define XML_DELETE_CHILD_NODES_EX                              map_delete(@A, "_childs_ex")
#define LOG_CAMPAIGN                                           FILE_D->log_campaign(@A, @B, @C, @D, @E, @F, @G, @H,                         @I, @J, @K)
#define VALID_SERVER_LOG                                       if (FALSE == CHECK_LOG_D->valid_server_log(@A, @B, @C))         trace("Invalid log in server_log.xml.");
#define LOG_EXCHANGE_APPRAISE                                  FILE_D->log_exchange_appraise(@A, @B, @C, @D, @E, @F, @G, @H, @I, @J, @K)
#define TTS_SEND_TO_SERVER                                     TTS_SERVER_D->send_to_server
#define SUCCEED                                                ((@A) == 0)
#define HIM                                                    ESC "[1;35m"
#define LOG_LOGIN_INFO                                         if (is_string(@A) || is_string((@A)->query_temp("login_info/account")))             LOG_D->log_login_info(get_login_info_file(@A), @B, @C)
#define MODIFY_CALCULATE_VALUE                                 ((@A) + 1.0 / pow(10, @B + 1))
#define mysql_field_name                                       (*f_field_name)
#define HIC                                                    ESC "[1;36m"
#define mysql_connect                                          (*f_connect)
#define LOG_SYSTEM_MONITOR                                     FILE_D->log_system_monitor(@A, @B, @C, @D, @E, @F, @G, @H, @I, @J)
#define LOG_COMBAT_IN_MAP                                      FILE_D->log_combat_in_map(@A)
#define LOG_IP_TASK                                            FILE_D->log_ip_task(@A, @B, @C, @D)
#define LOG_MONEY                                              FILE_D->log_money(@A, @B, @C, @D, @E, @F, @G, @H)
#define EMPTY_MAPPING                                          ([ ])
#define MAGIC_CODE                                             (buffer) "file"
#define HBBLU                                                  ESC "[44;1m"
#define HIW                                                    ESC "[1;37m"
#define mysql_field_value                                      (*f_field_value)
#define LOG_REPORT                                             FILE_D->log_report(@A, @B, @C, @D, @E, @F, @G, @H, @I)
#define LOG_ITEM_TRANSFER                                      FILE_D->log_item_transfer(@A, @B, @C, @D, @E, @F,                               @G, @H, @I, @J, @K, @L, @M)
#define REMOTE_BROADCAST_SEND                                  RPC_D->broadcast_send
#define mysql_get_insert_id                                    (*f_get_insert_id)
#define GIVE_CCS_MUTEX                                         GIVE_REMOTE_MUTEX("ccs", @A)
#define LOG_BEHAVIOR                                           FILE_D->log_behavior(@A, @B, @C)
#define LOG_LOGIN                                              FILE_D->log_login(@A, @B, @C, @D, @E, @F, @G)
#define CREATE_PARTY_GID_TO                                    FILE_D->create_gid_to(PARTY_TYPE_GID, @A, f)
#define MOBILE_CLIENT_TYPE_LIST                                ({ CLIENT_TYPE_MOBILE_IOS, CLIENT_TYPE_MOBILE_ANDROID })
#define mysql_fetch_record                                     (*f_fetch_record)
#define CMDS_OB                                                find_object(INTERSRV_CMDS_DIR + @A + ".c")
#define LOG_CHAT                                               FILE_D->log_chat(@A, @B, @C, @D, @E)
#define CREATE_CHAR_GID                                        FILE_D->create_gid(USER_TYPE_GID, @A)
#define FAILED                                                 ((@A) != 0)
#define CHECK_SUM                                              int32(create_check_sum(1, @A, @B, @C, @D) ^ MAGIC_NUMBER)
#define TAKE_CCS_MUTEX                                         TAKE_REMOTE_MUTEX("ccs", @A, @B, 15)
#define query_self                                             query((@A), 1)
#define mysql_is_connected                                     (*f_is_connected)
#define STAT_CMDS_FLOW_IN                                      if (enable_stat_cmds)         cmds_flow_in[@A] = (int) cmds_flow_in[@A] + 1;
#define LOG_ZQ_LV_UP                                           FILE_D->log_zq_lv_up(@A, @B, @C, @D, @E)
#define EMPTY_BUFFER                                           allocate_buffer(0)
#define mysql_set_auto_commit                                  (*f_set_auto_commit)
#define ASSURE_LOGIN_INFO_FILE                                 if (is_string(@A) || is_string((@A)->query_temp("login_info/account")))             LOG_D->assure_log_file(get_login_info_file(@A))
#define LOG_COLLECT_DATA                                       FILE_D->log_collect_data(@A, @B, @C, @D)
#define GET_CHAR_GID                                           FILE_D->get_gid(USER_TYPE_GID, @A)
#define CITYWAR_COMPETE_DIST                                   (CONFIG_D->is_citywar_compete_dist())
#define NEW_EXCHANGE_CODE                                      get_exchange_code()
#define SDDH_COMPETE_DIST                                      (CONFIG_D->is_sddh_compete_dist())
#define mysql_query                                            (*f_query)
#define RESET_CMD_COST_STAT                                    reset_cmd_cost_stat(client_no)
#define LOG_GIFT_ITEM                                          FILE_D->log_gift_item(@A, @B, @C, @D, @E, @F, @G, @H,                             @I, @J)
#define XML_CHILD_NODE                                         XML_CHILD_NODES_EX(@A)[@B]
#define mysql_num_records                                      (*f_num_records)
#define LOG_BLOCK                                              FILE_D->log_block(@A, @B, @C, @D, @E, @F,  @G,                       @H, @I, @J, @K, @L, @M)
#define ECHO_MILSEC_INTERVAL                                   (ECHO_INTERVAL * 1000)
#define UPDATA_PARTY_GID                                       FILE_D->update_gid(@A, PARTY_TYPE_GID, @B)
#define FINISH_CMD_COST_STAT                                   if (ENABLE_CMDS_STAT)     {         mixed _cost;         if (! is_mapping(_cost = all_cmds_cost[__FUN__]))             _cost = all_cmds_cost[__FUN__] = init_cost_stat();         _cost["count"][client_no % 10000]++;         _cost["ticks"][client_no % 10000] += tick() - ti;     }
#define STRCUT_CHINESE                                         (@A[@B * CHINESE_WIDTH..< 1])
#define CREATE_CHAR_GID_TO                                     FILE_D->create_gid_to(USER_TYPE_GID, @A, @B)
#define XML_DELETE_ALL_PROPS                                   map_delete(@A, "_all_props")
#define LEAGUE_COMPETE_DIST                                    (CONFIG_D->is_csl_compete_dist())
#define LOG_ZQ_ADD_POINT                                       FILE_D->log_zq_add_point(@A, @B, @C, @D, @E, @F)
#define EVERYONE_PK_COMPETE_DIST                               (CONFIG_D->is_everyone_pk_compete_dist())
#define LOG_CHANNEL                                            FILE_D->log_channel(@A, @B, @C, @D, @E)
#define CROSS_SERVER_DIST                                      (LEAGUE_COMPETE_DIST || PARTYWAR_COMPETE_DIST ||         ARENA_COMPETE_DIST || SDDH_COMPETE_DIST || CELEBRITY_COMPETE_DIST ||                     SAMSUNG_COMPETE_DIST || CITYWAR_COMPETE_DIST || XGAME_COMPETE_DIST ||                    EVERYONE_PK_COMPETE_DIST || ACTIVITY_COMPETE_DIST)
#define MAKE_DRAGGED_NAME                                      sprintf("#<%s#>", (@A))
#define LOG_COIN_CONSUME                                       FILE_D->log_coin_consume(@A, @B, @C, @D)
#define UPDATA_CHAR_GID                                        FILE_D->update_gid(@A, USER_TYPE_GID, @B)
#define NOR                                                    ESC "[2;37;0m"
#define DEBUG_LINE                                             trace("line", "FILE: %s\tFUNC: %s\tLine: %d\n", __FILE__, __FUN__, __LINE__)
#define MAX_VALUE                                              ((1 << (VALUE_SIZE - 1)) - 1)
#define ECHO_DEAD_INTERVAL                                     (ECHO_INTERVAL * ECHO_MISS_TIMES)
#define LOG_IMPORTANT_ACTION                                   FILE_D->log_important_action(@A, @B, @C, @D, @E)
#define LOG_LOCAL                                              LOG_D->log_local(@A, @B, @C, @D, @E, @F, @G)
#define STRRCUT_CHINESE                                        (@A[0..< (@B * CHINESE_WIDTH) + 1])
#define BASIC_CHAR_INFOS                                       ({ "gid", "name", "polar", "gender", "tt_weibo_name", "hide_tt_weibo" })
#define CLIENT_TYPE_LABEL                                      ([     CLIENT_TYPE_NORMAL : "CLIENT_TYPE_NORMAL",     CLIENT_TYPE_MINI : "CLIENT_TYPE_MINI",     CLIENT_TYPE_TINY : "CLIENT_TYPE_TINY",     CLIENT_TYPE_FULL_TINY : "CLIENT_TYPE_FULL_TINY",     CLIENT_TYPE_MOBILE_IOS : "CLIENT_TYPE_MOBILE_IOS",     CLIENT_TYPE_MOBILE_ANDROID : "CLIENT_TYPE_MOBILE_ANDROID",     CLIENT_TYPE_WEB : "CLIENT_TYPE_WEB",     CLIENT_TYPE_FULL_WEB : "CLIENT_TYPE_FULL_WEB",     CLIENT_TYPE_DOUYOU : "CLIENT_TYPE_DOUYOU",     CLIENT_TYPE_KUAIWAN : "CLIENT_TYPE_KUAIWAN",     CLIENT_TYPE_KUAIBA : "CLIENT_TYPE_KUAIBA",  ])
#define LOG_PURCHASE                                           FILE_D->log_purchase(@A, @B,                          @C, @D, @E, @F, @G, @H, @I,                                  @J, @K, @L)
#define SERVER_STATUS_CUSTOM                                   (0xFF + @A)
#define REMOTE_SEND                                            RPC_D->send
#define TIMEOUT                                                (-1 << 31)
#define CREATE_CCS_MUTEX                                       CREATE_REMOTE_MUTEX("ccs", @A)
#define HBGRN                                                  ESC "[42;1m"
#define GET_PARTY_GID_TO                                       FILE_D->get_gid_to(PARTY_TYPE_GID, @A, @B)
#define EMPTY_ARRAY                                            ({ })
#define LOG_CSL_MATCH                                          FILE_D->log_csl_match(@A, @B, @C, @D, @E, @F, @G, @H)
#define RPC_ONLINE_GS                                          sprintf("ONLINEGS-%s", @A)
#define LOG_TAO_STAT                                           FILE_D->log_tao_stat(@A, @B, @C, @D, @E, @F, @G, @H, @I)
#define CREATE_PARTY_GID                                       FILE_D->create_gid(PARTY_TYPE_GID, @A)
#define LOG_ERRAND                                             FILE_D->log_errand(@A, @B, @C, @D, @E, @F, @G, @H,                         @I, @J, @K)
#define SEND_TO_CCS                                            CCS_CLIENT_D->send_to_ccs
#define mysql_disconnect                                       (*f_disconnect)
#define STAT_CMDS_PROCESSED                                    if (enable_stat_cmds)         cmds_processed[@A] = (int) cmds_processed[@A] + 1;
#define CREATE_REMOTE_MUTEX                                    REMOTE_SEND(@A, SYNC_D, "create_mutex_by_remote", @B)
#define LOG_AUTO_TALK_STAT                                     FILE_D->log_auto_talk_stat(@A, @B, @C)
#define HIB                                                    ESC "[1;34m"
#define mysql_execute                                          (*f_execute)
#define verify                                                 (@A)
#define REMOTE_POST                                            RPC_D->broadcast_post
#define REMOTE_POST_DIRECT                                     RPC_D->broadcast_post_direct
#define TASK_TYPE_STAR_DESC                                    ([     TASK_TYPE_STAR_1 : $$1064367,     TASK_TYPE_STAR_2 : $$1064368, ])
#define CS_SORT_ID                                             sprintf("%s-%s", @A, _dist_name)
#define ZERO32                                                 make32(0)
#define HBRED                                                  ESC "[41;1m"
#define LOG_TAO_TASK                                           FILE_D->log_tao_task(@A, @B, @C, @D)
#define LOG_SWITCH                                             FILE_D->log_switch(@A, @B, @C, @D, @E, @F, @G)
#define LOG_USER                                               FILE_D->log_to_db(USER_LOG, @A, @B, @C, @D, @E, @F, @G)
#define LOG_ACTIVE_STAT                                        FILE_D->log_to_db(ACTIVE_STAT_LOG, @B, (string) time(), time_to_string(),                       @A, (string) CONFIG_D->get_config("my_dist"), @C, @D)
#define ACTIVITY_COMPETE_DIST                                  (CONFIG_D->is_activity_compete_dist())
#define GBKSTR                                                 iconv(@A, "utf-8", "GBK")
#define LOG_IMPORTANT                                          FILE_D->log_to_db(IMPORTANT_LOG, @A, @B, @C, @D, @E, @F, @G)
#define SEND_TO_SERVER                                         CCS_SERVER_D->send_to_server
#define HBMAG                                                  ESC "[45;1m"
#define HBYEL                                                  ESC "[43;1m"
#define LOAD_OR_COMPILE_ALL                                    (*load_or_compile_all)
#define LOG_ZQ_LOGIN                                           FILE_D->log_zq_login(@A, @B, @C, @D, @E, @F)
#define LOG_PET                                                FILE_D->log_pet(@A, @B, @C, @D, @E, @F, @G,                     @H, @I, @J, @K, @L)
#define LOG_LEVEL_UP                                           FILE_D->log_level_up(@A, @B, @C, @D, @E, @F,                         @G, @H, @I, @J, @K)
#define LOG_ANTI_CHEATER                                       FILE_D->log_anti_cheater(@A, @B, @C, @D, @E, @F, @G)
#define XML_DELETE_PARENT_NODE                                 map_delete(@A, "_parent")
#define PROPERTY_RECALL_CHECK_SUM                              int32(create_check_sum(@A, @B, @C, @D, @E) ^ MAGIC_NUMBER)
#define CELEBRITY_COMPETE_DIST                                 (CONFIG_D->is_celebrity_compete_dist())
#define HIY                                                    ESC "[1;33m"
#define LOG_ENCOURAGE_DETAIL                                   FILE_D->log_encourage_detail(@A, @B, @C, @D)
#define mysql_finalize                                         (*f_finalize)
#define LOG_APEX                                               FILE_D->log_apex(@A, @B, @C, @D, @E, @F)
#define REMOTE_BROADCAST_POST                                  RPC_D->broadcast_post
#define ARENA_COMPETE_DIST                                     (CONFIG_D->is_arena_compete_dist())
#define HBWHT                                                  ESC "[47;1m"
#define mysql_int_field                                        (*f_int_field)
#define LOAD_OR_COMPILE                                        (*load_or_compile)
#define GIVE_REMOTE_MUTEX                                      REMOTE_POST(@A, SYNC_D, "give_mutex_by_remote", @B,                 LOCAL_THREAD_NAME)
#define LOG_CSP_MATCH                                          FILE_D->log_csp_match(@A, @B, @C, @D, @E, @F, @G, @H)
#define LOG_TO_PROPERTY_BIND                                   FILE_D->log_to_property_bind(@A, @B, @C, @D, @E, @F, @G, @H)
#define DELETE_GID_TO                                          FILE_D->delete_gid_to(@A, @B)
#define HIG                                                    ESC "[1;32m"
#define mysql_this_record                                      (*f_this_record)
#define LOG_ZQ_ERRAND                                          FILE_D->log_zq_errand(@A, @B, @C, @D, @E, @F, @G)
#define GET_PARTY_GID                                          FILE_D->get_gid(PARTY_TYPE_GID, @A)
#define LOG_COIN_ORDER                                         FILE_D->log_coin_order(@A, @B, @C, @D, @E, @F, @G,                             @H, @I, @J, @K, @L)
#define SAMSUNG_COMPETE_DIST                                   (CONFIG_D->is_samsung_compete_dist())
#define LOG_ZQ_COMBAT                                          FILE_D->log_zq_combat(@A, @B, @C, @D, @E, @F, @G)
#define LOG_ACTIVITY                                           FILE_D->log_activity(@A, @B, @C, @D, @E, @F, @G)
#define LOG_CSL_COMBAT                                         FILE_D->log_csl_combat(@A, @B, @C, @D, @E, @F, @G, @H, @I, @J)
#define REMOTE_CALLBACK                                        RPC_D->callback
#define START_CMD_COST_STAT                                    int ti;     if (ENABLE_CMDS_STAT) ti = tick()
#define LOG_ANTIADDICTION                                      FILE_D->log_antiaddiction(@A, @B, @C)
#define LOG_MATERIAL                                           FILE_D->log_material(@A, @B, @C, @D, @E, @F,                          @G, @H, @I)
#define IS_MOBILE_CLIENT_TYPE                                  (member_array(@A, MOBILE_CLIENT_TYPE_LIST) > -1)
#define LOG_USER_REPORT_IP                                     FILE_D->log_user_report_ip(@A, @B, @C, @D, @E, @F, @G, @H, @I, @J, @K)
#define LOG_NUMMARY                                            FILE_D->log_nummary(@A, @B, @C, @D, @E, @F, @G,                         @H, @I)
#define LOG_COST_COIN                                          FILE_D->log_cost_coin(@A, @B, @C, @D, @E, @F, @G, @H, @I, @J)
#define TRANSFER_DATA_CHECK_SUM                                int32(create_check_sum(1, @A, @B, @C, @D, @E) ^ MAGIC_NUMBER)
#define mysql_num_fields                                       (*f_num_fields)

/* ================================================================
   Global/Object Variables
   ================================================================ */
   mixed COMPILE_ACTION;
   mixed ENABLE_CMDS_STAT;
   mixed INI_FILE;
   mixed _adjust_position_info_funcs;
   mixed _auto_save_funcs;
   mixed _char_disconnected_funcs;
   mixed _char_reconnected_funcs;
   mixed _child_leave_world_funcs;
   mixed _cmd_attrib_map;
   mixed _cmd_types;
   mixed _disconnected_funcs;
   string _dist_name;
   mixed _enable_log_trace;
   mixed _enter_world_failed_funcs;
   mixed _enter_world_funcs;
   mixed _heart_beat_ex_funcs;
   mixed _heart_beat_funcs;
   mixed _internal_comm_close_funcs;
   mixed _just_login_server_funcs;
   mixed _leave_world_funcs;
   mixed _log_runtime_info_funcs;
   mixed _login_timeout;
   mixed _mdb;
   mixed _new_connection_funcs;
   mixed _post_init_funcs;
   mixed _prepare_switch_server_funcs;
   mixed _regen_iid_funcs;
   string _server_name;
   mixed _server_status;
   mixed _switch_server_funcs;
   mixed _today_zero_time;
   mixed _xdbc_clients;
   mixed _xdbc_debug_level;
   string _xdbc_output_basic_file;
   string _xdbc_output_file;
   mixed _xdbc_output_file_suffix;
   mixed _xdbc_query;
   mixed all_cmds_cost;
   mixed all_mutex;
   mixed ban_words;
   int ban_words_match_id;
   mixed base_ganzhi;
   mixed client_agents;
   mixed cmds_flow_in;
   mixed cmds_processed;
   mixed coupon_charge_type;
   mixed coupon_charge_type_list;
   mixed csl_new_bonus_season;
   mixed csp_new_bonus_season;
   mixed daemon_data;
   mixed disconnect_stat;
   mixed dist_start_time;
   mixed dizhi;
   mixed enable_stat_cmds;
   mixed exchange_cookie;
   mixed f_connect;
   mixed f_disconnect;
   mixed f_execute;
   mixed f_fetch_record;
   string f_field_name;
   mixed f_field_value;
   mixed f_finalize;
   mixed f_get_error;
   int f_get_insert_id;
   mixed f_int_field;
   mixed f_is_connected;
   mixed f_num_fields;
   mixed f_num_records;
   mixed f_query;
   mixed f_select_db;
   mixed f_set_auto_commit;
   mixed f_this_record;
   mixed file_dependence;
   mixed gid_objects;
   /* mixed global_0; */
   /* mixed global_1; */
   int is_test_dist_flag;
   mixed last_telnet_login_time;
   mixed lib_pak_path;
   mixed load_or_compile;
   mixed load_or_compile_all;
   mixed load_or_compile_all_path;
   mixed month_day;
   mixed n_msg;
   mixed need_exclude_cmds;
   mixed new_convert_code;
   mixed online_update_status;
   mixed queue_map;
   mixed server_shift_time;
   int startup_id;
   mixed stat_cmds_interval;
   mixed telnet_clients;
   mixed tiangan;
   mixed tiangan_dizhi;
   mixed tolower_temp_flag;
   mixed when_return_user_not_found;
   mixed write_file_cookie;
   mixed xdbc_func_map;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

void post_regen_iid(object me, string old_iid, string new_iid, mixed para);
void msg_l_enable_auto_lock(int client_no, string account, int result, int status);
void get_left_time(int left_secs, int &hour, int &min, int &sec);
void msg_l_query_card(int client_no, string cookie, int result, int count, int used, int locked);
void msg_l_get_ssb_prompt_msg(int client_no, string result);
int get_server_status();
int set_server_status(int new_status);
void msg_l_request_ip_card(int client_no, string gid, string type, string card_no, int result);
void msg_l_lock_account(int client_no, string account, string ret, string msg);
void post_auto_save(object me);
void register_char_disconnected(function f);
int is_same_week(int ti1, int ti2);
private void telnet_daemon(int fd);
void msg_l_borrow_local_encrypt(int client_no, string account, int result);
void msg_l_unlock_card(int client_no, string cookie, string number, int result);
void msg_got_gid_info(int client_no, int cookie, int result, string type, string name);
void msg_l_get_local_encrypt_info(int client_no, string account, string password, int state);
void register_heart_beat_ex(function f);
void msg_cfg_spa_info(int client_no, string ip, int port, string name);
void msg_sell_treasure(int client_no, string serial_num, int type, int state, int end_time, int day, int price, int upset_price, string seller, string account, string para);
void post_adjust_position_info(object me);
buffer xdbc_decrypt_data(buffer data, int key);
private int xdbc_get_cno_of_view(int vno);
int create_queue_debug(string oname = "");
void post_prepare_switch_server(object me);
void post_just_login_server(object me);
private void xdbc_append_output(string type, string msg);
void release_tixml_parse_ex(mapping xml);
void require_uid();
int to_set_timer(mixed timeout, mixed timer_id, function f);
void cmd_ma_oper(int client_no, int cookie, string cmds, string ma_account, array para);
void write_to_ex(mixed f, string path, mixed ret);
mapping analysis_ban_words_file(string file);
string time_desc_to_ch(mixed t);
void cmd_ma_set(int client_no, int cookie, string cmds, string ma_account, array para);
void msg_read_log(int client_no, int cookie, int result);
int report_cmds_stat(string file_name);
void post_char_reconnected(object me);
void msg_get_sys_envelope(int client_no, int cookie, int result, string content);
string decrypt_mask_md5(string md5_str, string mask);
void msg_l_set_safe_ctrl_str(int client_no, string module, string account, int result, string type);
void msg_l_set_safe_ctrl_var(int client_no, string module, string account, int result, int new_var);
varargs void xdbc_log(int level);
void post_init();
int is_same_month(int ti1, int ti2);
void msg_ccs_emote_data(int client_no, string verb, string target, int channel, string peer, mixed self_info);
private void add_to_section(mapping section, string key, mixed value);
void msg_got_page_data_ex(int client_no, int cookie, buffer content);
void register_daemon_data(string daemon_name, mixed index, mixed val);
void rpc_stat(mixed arg);
int xdbc_is_connected(int cno);
void msg_sddh_oper(int client_no, string cmd, array args);
void msg_cfg_my_dist(int client_no, string dist);
void read_to_ex(mixed f, string path, string name, mixed ret);
void register_switch_server(function f);
int get_next_hour_as_int(int ntime);
string time_desc(mixed t);
int check_server_status(mixed check_status);
array get_etc_dir(string dir);
array get_all_branches(string path, string name);
private void load_file_dependence();
string convert_int_to_scientific_string(int amount);
void msg_rpc_server_name(int client_no, string name, string version, int server_time);
void msg_verify_code_result(int client_no, string gid, string code, string type, int result);
void msg_league_oper(int client_no, string cmd, array args);
void cmd_l_block_share_store(int client_no, string account, int interval);
void msg_cfg_dba_list(int client_no, array list);
mapping init_cost_stat();
void cmd_internal_comm_closed(int client_no);
mixed sql_insert(mixed db, string table, mapping data);
mixed sql_update(mixed db, string table, mapping data, mixed condition);
void init_post_init_funcs();
int is_in_ip_span(int start_ip, int end_ip, int cur_ip);
mixed tixml_parse_file(string file_name);
void msg_l_cost_fill_info(int client_no, string account, string gid, int times, int result, string type);
void unload_all(string path);
mixed sql_replace(mixed db, string table, mapping data);
void safe_reboot();
void init_platform_module();
void cmd_intersvr_rpc_call(int client_no, int cookie, int method, string owner, string server, string ob, string f, string stat, string args_str);
void init_xdbc_client();
void post_char_disconnected(object me, mixed is_logout);
void create_data_from3(array arr, array &all_values, mapping &data_table);
void cmd_new_connection(int client_no);
void msg_cfg_game_server_list(int client_no, array list);
int xdbc_query(int cno, string sql_cmd);
int stop_stat_cmds();
void msg_cfg_ccs_info(int client_no, string ip, int port, string name);
void msg_get_goods_from_tts(int client_no, string gid, int mode, int storage_fee, int type, string goods_id, string result);
void read_content_to_ex(string path, string name, mixed branch, mixed to);
void msg_cfg_ldba_list(int client_no, array list);
void xdbc_finalize(int vno);
int xdbc_num_records(int vno);
string convert_into_char(string content);
void msg_l_query_coin(int client_no, string gid, string account, string module, string para, int have_coin_pwd, int unlock_time, int unlock_duration, string memo);
void msg_citywar_oper(int client_no, string cmd, array args);
void cmd_l_recycle_stall(int client_no, string account);
mixed query_disconnect_stat();
void post_new_connection(int client_no);
void post_heart_beat_ex(object me);
int xdbc_allocate_client(int fd);
void _reboot();
mixed mdb_insert(string table, mapping data);
void _cmd_internal_comm_closed(int client_no);
string xor_encode(string raw, int p = 204);
void msg_channel_oper(int client_no, string cmd, array args);
void msg_ccs_system_channel(int client_no, string msg);
void msg_l_ecard_business(int client_no, string action, string map_para, string cookie);
void msg_l_update_addiction(int client_no, string account, string result_text);
void msg_l_query_fill_info(int client_no, string account, string gid, int result, string type);
mixed find_agent_by_client(int client_no);
private mixed xdbc_call_server(int cno, mapping para);
void msg_l_charge_result(int client_no, string account, int time_left, int count_left, int mode, int result);
void msg_cfg_my_info(int client_no, string dist, string name, int port, int id, int dist_id, mapping m);
int pp_stop_profiling_ex();
int get_server_shift_time();
mixed get_offset_ym(int t, int offset_months);
void xdbc_set_output_basic_file(string type, string basic_file);
void msg_l_set_phah_bonus_info(int client_no, string gid, string ret);
mixed get_message_stat_info(mixed msg_no);
mixed valid_name_ex(string name, int min_len, int max_len, int may_chinese, mixed include_group = 0, mixed exclude_group = 0, mixed f = 0);
void msg_l_query_coupon(int client_no, string gid, string para, string result);
void xdbc_set_debug_level(int level);
void msg_l_ssb_update_account(int client_no, string account, string result);
void post_internal_comm_close(int client_no);
void msg_l_convert_silver_coin(int client_no, string gid, string account, string aaa_cookie, string module, string para, int result, int convert_coin, int silver_coin, int gold_coin, int limit_trade_coin);
void msg_ccs_world_channel(int client_no, string msg);
array explode_string(string str, string chars);
mixed xdbc_get_insert_id(int cno);
void msg_updated_gid(int client_no, int cookie, int result);
mixed query_dynamic_key(object ob, string key_index);
void msg_l_get_ssb_combatd_conf(int client_no, string result);
void msg_l_cost_coin(int client_no, string gid, string account, int result, string module, string para, string memo);
void msg_samsung_oper(int client_no, string cmd, array args);
mixed get_max_cost_client(mixed field = 0, int reset_stat = 0, mixed cmds = ({ }), array except_cmds = ({ }));
private int size_objects_get_usage(mapping m, string name);
void cmd_l_pend_shop(int client_no, string account);
mixed find_object_by_gid(string gid);
void msg_bottle_oper(int client_no, string cmd, array args);
void msg_l_do_inquiry(int client_no, string gid, string order_id, string pay_type, int result, int result_code, int gold);
void reboot();
void post_child_leave_world(object child);
void register_char_reconnected(function f);
void register_post_disconnected(function f);
mixed write_content_to_ex(string path, string name, mixed branch, mixed content, mixed to);
void msg_l_get_ssb_combatd_res(int client_no, string result);
int assure_file(string file);
void msg_intersvr_channel_msg(int client_no, int channel, string sender, buffer expand_buffer, string msg);
void msg_l_get_ssb_finish_flag(int client_no, string result);
void post_switch_server(object me);
mixed xdbc_get_error(int cno);
void msg_gs_finger(int client_no, string user_name);
varargs void xdbc_trace(int level);
int is_valid_ip(string str);
void msg_cfg_cta_list(int client_no, array list);
void msg_update_log(int client_no, int cookie, int result);
array sql_select(mixed db, string sql_cmd, int max_row = 1);
void msg_written_content(int client_no, int cookie, int result);
int xdbc_connect(string host, string user, string passwd, string db);
void process_online_update_files();
void msg_l_manage_vip_sms(int client_no, string gid, int oper, int ret, string para);
void msg_create_pet_deposit(int client_no, int cookie, int result);
nomask void telnet_client_closed(int fd);
void msg_l_do_alipay_recharge(int client_no, string gid, int ret);
void xdbc_disconnect(int cno);
string get_login_info_file(mixed mks);
void unregister_daemon_data(string daemon_name, mixed index);
void msg_cfg_ah_server_info(int client_no, string ip, int port, string name);
void msg_l_oper_protect_range(int client_no, string account, string oper, int range, int result);
mixed get_sub_dirs_ex(string path);
int convert_string_to_bit_macro(string str_bits);
void msg_read_pet_deposit(int client_no, int cookie, int result, string content);
void msg_check_role_on_sale(int client_no, string account, int time_stamp, string para);
int xdbc_execute(int cno, string sql_cmd);
void reset_log_file_name(string server_name);
int send_to_queue_debug(mixed oid = 0, mixed data = 0, int timeout = -1);
void msg_intersvr_rpc_ret(int client_no, int cookie, string owner, string stat, string result);
mapping get_all_client_agents();
void msg_l_do_recharge(int client_no, string gid, string order_id, string pay_type, int result, int result_code, int gold);
string time_length_to_ch(int t, int accuracy = 4, int rounding = 1, int color = 0);
mixed xdbc_get_client(int client_no);
void post_enter_world(object me);
void release_all_data();
void msg_l_send_vip_sms(int client_no, string gid, string auth_type, string sms_type, string sms_code, int ret);
void init_compile_module();
void load_all(string path, mapping para = ([ ]));
int load_ban_words();
int is_same_data(mixed source, mixed dest);
void msg_internal_auth(int client_no, int reply_server_type, string reply_server_name, int result, int cookie);
void msg_xg_get_name(int client_no, int cookie, int result, string name);
mixed read_content_ex(string path, string name, mixed branch, mixed from_table);
string tolower_temp(string content);
void register_post_auto_save(function f);
void register_enter_world(function f);
int xdbc_fetch_record(int vno);
void msg_partywar_oper(int client_no, string cmd, array args);
void msg_cfg_write_etc(int client_no, int cookie, int result);
void msg_l_lock_card(int client_no, string cookie, string number, int result, int count);
void msg_l_ssb_copy_account(int client_no, string user_name, string result);
void _cmd_new_connection(int client_no);
void msg_l_query_random_coupon(int client_no, string account, string gid, int result, string para, int num);
mixed mysql_select_column(int cno, string sql_cmd, int column, int max_row = 1);
mixed valid_content(string content, int min_len, int max_len, int min_line, int max_line, mixed group = "√Ù∏–¥ ”Ô");
void msg_cfg_read_etc(int client_no, int cookie, int result);
void register_log_runtime_info(function f);
int get_next_month_zero_time(int t);
string time_desc_ex(mixed t);
void init_agent_cmds_module();
int get_monday_zero_time(int t);
mixed random_sub_select(mixed src, int n, int order = 0);
mixed read_whole_ini_ex(string ini_file);
void msg_l_set_local_encrypt_info(int client_no, string account, int result);
void msg_read_content(int client_no, int cookie, int result, string content);
int get_comm_cmd_attrib(string cmd_name, string key);
void msg_cfg_csa_info(int client_no, string ip, int port, string name);
void cmd_get_server_time(int client_no, string gs_name);
void msg_xg_get_account(int client_no, int cookie, int result, string account);
mixed valid_char_name(string char_name, mixed include_group = 0, mixed exclude_group = 0);
void cmd_l_agent_connect_server(int client_no, string account, int auth_key, string paras);
array get_all_files(string path);
void msg_activity_oper(int client_no, string cmd, array args);
void init_startup_module();
int gid_to_val(string gid);
void msg_cfg_ddb_list(int client_no, array list);
void msg_deleted_gid(int client_no, int cookie, int result);
void msg_write_pet_deposit(int client_no, int cookie, int result);
string convert_tao_desc(int tao);
string get_exchange_code();
int is_test_dist(mixed dist);
private mixed callback_mysql_select_array(int vno);
void msg_created_gid(int client_no, int cookie, int result, string gid);
int remove_resource_string_ex(int id);
void msg_l_update_first_credit_bonus(int client_no, string account, string gid, string result_desc);
void msg_l_auth_all_protect(int client_no, string account, int step, int seq_no, string result);
mixed valid_name(string name, int min_len, int max_len, int may_chinese, mixed include_group = 0, mixed exclude_group = 0);
void msg_everyone_pk_oper(int client_no, string cmd, array args);
int receive_from_queue_debug(mixed oid = 0, mixed &data = 0, int timeout = -1);
string xdbc_get_raw_password(string password);
void msg_l_get_credit_coins(int client_no, string gid, string coins);
void msg_l_get_protect_info(int client_no, string account, string result);
int pp_get_report_ex(string file_name = "");
string val_to_gid(int val);
mixed create_content_ex(string path, string name, mixed branch, mixed content);
void init_sql_module();
void update(string file, int clear_init = 0);
void post_disconnected(int client_no);
mixed get_max_cost_cmd(mixed field = 0, int reset_stat = 0, mixed cmds = ({ }), array except_cmds = ({ }));
mixed get_daemon_data(string daemon_name);
void msg_l_get_acc_comp_char(int client_no, string gid, string comp_id, string m_str);
mixed calc_birthday_horoscope(int birthday);
int get_month_zero_time(int t);
mixed read_etc_file(string file_name, string mode = "a", string file_path = "/dba/etc/");
void debug_help();
int calc_day_count(int a_year, int b_ti);
void debug_delete_thread(mixed thread_sign);
void register_enter_world_failed(string file, function f);
void debug_shift_time(int shift);
void post_log_runtime_info();
int file_exist_ex(string file);
void update_today_zero_time();
mixed sql_delete(mixed db, string table, mixed condition);
int calc_server_status_as_aaa(int available, int online, int max_user, float cpu_cost, float gs_user_over, float gs_user_full, float gs_user_busy, float gs_cpu_full, float gs_cpu_busy);
int is_gm_name(string name);
int update_dist_start_time(int start_time);
void msg_get_counts(int client_no, int cookie, int result, string counts);
void msg_arena_oper(int client_no, string cmd, array args);
void msg_got_catalog_ex(int client_no, int cookie, int result);
mixed tixml_parse_ex(mapping xml);
void stat_disconnect(int where);
void compile_all_path(string path);
int is_same_account(mixed account_a, mixed account_b);
void msg_cfg_etc_page_data(int client_no, int cookie, buffer content);
string get_raw_password(string password, mixed key);
void msg_spa_busy(int client_no, int max_size, int data_size, int threshold);
private mixed mixed_mysql_select(int cno, string sql_cmd, function fetch, int max_row);
void set_gid_object(string gid, object ob);
void register_post_heart_beat(function f);
void register_regen_iid(function f);
mixed deep_dup(mixed val);
string time_length_to_ch_ex(int to_t, int src_t, int accuracy = 4, int rounding = 1);
string set_dynamic_key(object ob, string key_index, mixed key);
void msg_deleted_content(int client_no, int cookie, int result);
private mixed mysql_select_column_fetch(int column, int vno);
int start_stat_cmds(int interval);
void load_dist_start_time();
buffer xdbc_encrypt_data(buffer data, int key);
string convert_to_url_encode(string dist_name);
void register_adjust_position_info(function f);
void cmd_l_when_batch_block_account(int client_no, string str_account);
void top();
void msg_xg_create_name(int client_no, int cookie, int result);
int get_last_hour_as_int(int ntime);
string arabic_to_chinese(int num);
array mdb_select(string sql_cmd, int max_row = 1);
mixed time_desc_to_int(string desc, int style = 1);
void msg_l_add_fill_info(int client_no, string account, string gid, int result, string type);
void msg_csa_common_oper(int client_no, string cmd, array args);
void msg_l_get_safe_ctrl_info(int client_no, string module, string account, string attrib, string result);
void cmd_disconnected(int client_no, int where = 0);
int is_whole_point_moment(int ntime);
string encrypt_mask_md5(string md5_str, mixed &mask);
void msg_data_mining(int client_no, string type, buffer buf);
void load_valid_chinese();
private mixed find_mysql_func(string func_name, int no_xdbc);
void daemon();
void cmd_ccs_shutdown_by_spa(int client_no, string interval, int times);
int regular_number(int v, mixed index);
mixed mysql_select_single_result(int cno, string sql_cmd);
void msg_created_content(int client_no, int cookie, int result);
void _cmd_disconnected(int client_no, int where);
int is_same_hour(int ti1, int ti2);
void msg_ccs_reboot_result(int client_no, int result);
private void safe_load_object(string file, mapping para = ([ ]));
mixed xdbc_this_record(int vno);
void set_client_agent(int client_no, object agent);
void size_max(int n = 100, int min_size = 8192);
void msg_ccs_online_friend(int client_no, string friend_name, string user_name);
mixed mysql_select_array(int cno, string sql_cmd, int max_row = 1);
object reload_object(string path, mapping para = ([ ]));
void msg_xgame_oper(int client_no, string cmd, array args);
int pp_start_profiling_ex(int period = 1000, int duration = 3600);
void post_heart_beat(object me);
void xdbc_enable_log_trace(int enabled = 1);
void xdbc_free_client(int client_no);
void post_enter_world_failed(object me);
void delete_dynamic_key(object ob, string key_index);
void msg_cfg_tts_info(int client_no, string ip, int port, string name);
string convert_complex_data_to_string(mapping complex_data);
void register_post_new_connection(function f);
private mixed callback_mysql_select(int vno);
void msg_ccs_friend_tell(int client_no, string from, string to, string msg, int msg_time);
int is_same_day(int ti1, int ti2);
private void xdbc_update_output_file(mixed type);
private void _timer_pp_stop_profiling();
void msg_ccs_friend_status(int client_no, string friend_name, string user_name, int type);
void save_all(mixed ti_stat);
void post_leave_world(object me);
array get_server_list();
mapping get_transfer_log_info(mixed me, mixed to);
void register_prepare_switch_server(function f);
void register_child_leave_world(function f);
void init_debug_cmds_module();
int is_leap_year(mixed t);
int xdbc_send_command(int client_no, mapping para);
mixed convert_to_value(string attrib);
override nomask void telnet_new_client(int fd);
void register_post_internal_comm_close(function f);
void msg_l_query_ip_area(int client_no, string account, string digital_ip, string module, string result);
void init_cmds_module();
void msg_cfg_aaa_list(int client_no, mapping m);
string cpu_cost(float r);
mixed xdbc_field_name(int vno);
void compile_all(string path);
void msg_l_query_addiction(int client_no, string account, string result_text);
void msg_l_precost_coin(int client_no, string gid, string account, string aaa_cookie, int result, string module, string para, string coin_password, int send_time, int prc_cookie);
void size_global();
string select_server_ip(mixed network_type, mapping server_info);
private void safe_compile_program(string file);
void msg_l_change_coupon(int client_no, string account, string gid, int result, int cost_once, string para, string cookie);
void cmd_l_when_block_account(int client_no, string account);
private int valid_ban_words_by_group(string content, string group);
mixed xdbc_get_info();
string xor_decode(string str, int p = 204);
void size_all(string path = "");
void msg_l_verify_password(int client_no, string account, string gid, string type, int result);
void remove_client_agent(int client_no);
mixed get_dir_ex(string path);
void load_all_path(string path, mapping para = ([ ]));
void msg_got_server_time(int client_no, string ccs_name, int t);
mixed get_new_iid_by_old(mixed old_iid, int dist_id);
string convert_mapping_to_sql_cmd(string op, string table, mapping m, mixed key);
void msg_celebrity_oper(int client_no, string cmd, array args);
string time_to_string(mixed t);
int is_test();
mixed _valid_name(string name, int min_len, int max_len, int may_chinese, mixed include_group, mixed exclude_group);
private int is_need_excluded(string cmd_name);
int xdbc_wait_command(int client_no, mixed &para);
void msg_draft_oper(int client_no, string cmd, array args);
void load_comm_desc(function callback, int encap_type);
mapping get_important_pet_log_info(mixed from, mixed to, mixed pet);
void remove_gid_object(string gid);
void msg_l_buy_card(int client_no, string cookie, string number, int count, int result);
void to_log_file(string file, string log_msg);
int get_day_zero_time(int t);
void msg_moved_content(int clinet_no, int cookie, int result);
int get_org_time();
mixed xdbc_field_value(int vno, mixed index);
mixed xdbc_pend_read(int fd, int len, int timeout = 12);
mapping convert_string_to_complex_data(string str_data);
mixed write_content_ex(string path, string name, mixed branch, mixed content);
void create_data_from2(array arr, array &attrib_index, mapping &attrib_list);
void msg_l_add_coin(int client_no, string gid, string account, string aaa_cookie, string module, string para, int result, int add_silver_coin, int add_gold_coin, int silver_coin, int gold_coin, int limit_trade_coin);
void msg_l_request_coupon(int client_no, string gid, string coupon_type, int result, string type, string para, string cookie);
string get_data_convert_type(string path, string name);
void msg_l_manage_mp_device(int client_no, string gid, int oper, int ret, string para);
void reset_cmd_cost_stat(int client_no);
void size_objects(int output = 100);
void msg_l_query_citys(int client_no, string gid, string module, string country, string province, string citys);
void msg_l_update_recharge_return_credit(int client_no, string account, string gid, int total_coin, int max_bonus_id);
void msg_got_gid_arr(int client_no, int cookie, int result, string content);
void msg_treasure_trading_oper(int client_no, string cmd, array args);
mixed sql_get_table(mixed db, string table_name);
int get_weeks_in_year(int ti);
void register_just_login_server(function f);
void msg_l_query_account_state(int client_no, string account, string state);
void register_leave_world(function f);
private int _size_max_get_memory_cost(mapping m, string name);
void msg_gs_emote_data(int client_no, string verb, string target, int channel, string peer, string self_info);
void cmd_cta_to_server(int client_no, buffer buf);
void register_post_init(function f);
mixed to_get_dir(string path);
void msg_got_gid(int client_no, int cookie, int result, string gid);
void msg_got_page_data(int client_no, int cookie, string content);
mixed mysql_select(int cno, string sql_cmd, int max_row = 1);
void notice_shutdown();
void msg_got_catalog(int client_no, int cookie, int result);

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   _xdbc_clients = allocate_array(1024);  // /xdbc/xdbc_comm.c:31
   _xdbc_debug_level = 2;  // /xdbc/xdbc_comm.c:371
   _enable_log_trace = 0;  // /xdbc/xdbc_client.c:10
   _xdbc_output_basic_file = "/xdbc/";  // /xdbc/xdbc_client.c:14
   _xdbc_output_file = ([ ]);  // /xdbc/xdbc_client.c:18
   _xdbc_output_file_suffix = 0;  // /xdbc/xdbc_client.c:22
   mkdir("/dba/log/");  // /xdbc/xdbc_client.c:26
   xdbc_set_output_basic_file("trace", "/dba/log/xdbc_trace");  // /util/global_agents.c:5
   xdbc_enable_log_trace();  // /util/global_init.c:30
   enable_trace("xdbc", 0);  // /util/global_init.c:37
   _xdbc_query = ([ ]);  // /util/global_init.c:134
   xdbc_func_map = ([ "mysql_connect" : "xdbc_connect", "mysql_disconnect" : "xdbc_disconnect", "mysql_is_connected" : "xdbc_is_connected", "mysql_get_error" : "xdbc_get_error", "mysql_execute" : "xdbc_execute", "mysql_query" : "xdbc_query", "mysql_finalize" : "xdbc_finalize", "mysql_num_records" : "xdbc_num_records", "mysql_fetch_record" : "xdbc_fetch_record", "mysql_this_record" : "xdbc_this_record", "mysql_field_value" : "xdbc_field_value", "mysql_field_name" : "xdbc_field_name", "mysql_get_insert_id" : "xdbc_get_insert_id", "mysql_set_auto_commit" : "NO_IMPLEMENTED", "mysql_select_db" : "NO_IMPLEMENTED", "mysql_num_fields" : "NO_IMPLEMENTED", "mysql_int_field" : "NO_IMPLEMENTED" ]);  // /util/global_util.c:2666
   init_xdbc_client();  // /util/global_util.c:2674
   INI_FILE = (expand_macro("CONFIG") ? expand_macro("CONFIG") : "/dba/dba.ini");  // /util/global_util.c:2688
   client_agents = ([ ]);  // /util/global_util.c:3007
   gid_objects = ([ ]);  // /util/global_util.c:3003
   lib_pak_path = trim(expand_macro("__BIN_DIR__"));  // /util/global_util.c:2997
   lib_pak_path = lib_pak_path[1..<2];  // /util/global_util.c:2990
   if ((lib_pak_path[<1] == 47)) {  // /util/global_util.c:3340
       lib_pak_path = lib_pak_path[0..<2];  // /util/global_debug_cmds.c:23
   }  // /util/global_debug_cmds.c:76
   _post_init_funcs = ({ });  // /util/global_debug_cmds.c:960
   _enter_world_funcs = ({ });  // /util/global_debug_cmds.c:1051
   _leave_world_funcs = ({ });  // /util/global_platform.c:108
   _child_leave_world_funcs = ({ });  // /util/global_comm.c:8
   _char_reconnected_funcs = ({ });  // /util/global_comm.c:17
   _char_disconnected_funcs = ({ });  // /util/global_comm.c:208
   _log_runtime_info_funcs = ({ });  // /util/global_debug_pp.c:7
   _prepare_switch_server_funcs = ({ });  // /dba/start_dba.c:103
   _switch_server_funcs = ({ });  // /dba/start_dba.c:109
   _just_login_server_funcs = ({ });  // /dba/start_dba.c:117
   _adjust_position_info_funcs = ({ });  // /dba/start_dba.c:125
   _regen_iid_funcs = ({ });  // /dba/start_dba.c:134
   daemon_data = ([ ]);  // /dba/start_dba.c:142
   _heart_beat_funcs = ({ });  // /dba/start_dba.c:146
   _heart_beat_ex_funcs = ({ });  // /dba/start_dba.c:154
   _auto_save_funcs = ({ });
   _enter_world_failed_funcs = ([ ]);
   is_test_dist_flag = 65535;
   _today_zero_time = 0;
   set_timer(0, 0, "update_today_zero_time");
   _login_timeout = 45;
   online_update_status = 65535;
   tolower_temp_flag = 1;
   ban_words = ([ ]);
   ban_words_match_id = ([ ]);
   exchange_cookie = 0;
   tiangan_dizhi = ({ "º◊◊”", "““≥Û", "±˚“˙", "∂°√Æ", "ŒÏ≥Ω", "º∫À»", "∏˝ŒÁ", "–¡Œ¥", "»……Í", "πÔ”œ", "º◊–Á", "““∫•", "±˚◊”", "∂°≥Û", "ŒÏ“˙", "º∫√Æ", "∏˝≥Ω", "–¡À»", "»…ŒÁ", "πÔŒ¥", "º◊…Í", "““”œ", "±˚–Á", "∂°∫•", "ŒÏ◊”", "º∫≥Û", "∏˝“˙", "–¡√Æ", "»…≥Ω", "πÔÀ»", "º◊ŒÁ", "““Œ¥", "±˚…Í", "∂°”œ", "ŒÏ–Á", "º∫∫•", "∏˝◊”", "–¡≥Û", "»…“˙", "πÔ√Æ", "º◊≥Ω", "““À»", "±˚ŒÁ", "∂°Œ¥", "ŒÏ…Í", "º∫”œ", "∏˝–Á", "–¡∫•", "»…◊”", "πÔ≥Û", "º◊“˙", "““√Æ", "±˚≥Ω", "∂°À»", "ŒÏŒÁ", "º∫Œ¥", "∏˝…Í", "–¡”œ", "»…–Á", "πÔ∫•" });
   tiangan = ({ "º◊", "““", "±˚", "∂°", "ŒÏ", "º∫", "∏˝", "–¡", "»…", "πÔ" });
   dizhi = ({ "◊”", "≥Û", "“˙", "√Æ", "≥Ω", "À»", "ŒÁ", "Œ¥", "…Í", "”œ", "–Á", "∫•" });
   base_ganzhi = ([ "year" : 45, "month" : 12, "day" : 17, "hour" : 28 ]);
   month_day = ({ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 });
   coupon_charge_type_list = ({ 0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 24, 25 });
   coupon_charge_type = implode(coupon_charge_type_list, ", ");
   server_shift_time = 0;
   csl_new_bonus_season = 29;
   csp_new_bonus_season = 27;
   new_convert_code = 1;
   when_return_user_not_found = 3;
   _mdb = 65535;
   all_mutex = ([ ]);
   all_cmds_cost = ([ ]);
   ENABLE_CMDS_STAT = 0;
   disconnect_stat = ([ ]);
   n_msg = ([ ]);
   queue_map = ([ ]);
   enable_stat_cmds = 0;
   stat_cmds_interval = 0;
   cmds_flow_in = ([ ]);
   cmds_processed = ([ ]);
   _server_name = "";
   _dist_name = "";
   _server_status = 2;
   dist_start_time = 0;
   file_dependence = ([ ]);
   _cmd_types = set_case_check(([ "internal_comm" : 2, "client_user" : 1, "from_server" : 4 ]), 0);
   _cmd_attrib_map = ([ ]);
   need_exclude_cmds = ([ 9 : ([ "msg_intersvr_rpc_ret" : 1 ]), 16 : ([ "msg_treasure_trading_oper" : 1, "cmd_cta_to_server" : 1 ]) ]);
   _new_connection_funcs = ({ });
   _disconnected_funcs = ({ });
   _internal_comm_close_funcs = ({ });
   last_telnet_login_time = 0;
   telnet_clients = ([ ]);
   write_file_cookie = time();
   switch_include_dir("include;dba;dba/include");
   config_driver("callstack_when_error", "full");
   startup_id = time();
   set_log_on(1);
   init_startup_module();
   init_compile_module();
   init_sql_module();
   init_debug_cmds_module();
   init_platform_module();
   printf((COMPILE_ACTION + " all global feature.
"));
   (*load_or_compile_all_path)("/feature/");
   printf((COMPILE_ACTION + " all global inherits.
"));
   (*load_or_compile_all_path)("/inherit/");
   printf((COMPILE_ACTION + " all global objects.
"));
   (*load_or_compile_all_path)("/clone/");
   printf((COMPILE_ACTION + " all global cmds.
"));
   (*load_or_compile_all_path)("/cmds/");
   (*load_or_compile)("/daemons/configd.c");
   (*load_or_compile_all)("/daemons/");
   (*load_or_compile_all)("/dba/inherit/misc");
   (*load_or_compile_all)("/dba/daemons/");
   (*load_or_compile_all_path)("/dba/clone/");
   (*load_or_compile_all_path)("/dba/cmds/");
   if (is_compiling()) {
       (*load_or_compile_all_path)("/daemons/");
       (*load_or_compile_all_path)("/dba/daemons/");
       (*load_or_compile_all_path)("/dba/handlers/");
   }
   printf("Invoke registered initialization routines.
");
   post_init();
   set_server_status(3);
   printf("[1;33mDriver load ok!
[2;37;0m");
   broadcast("Driver startup
");

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @code  nc=27, nargs=4, insts=27
// --------------------------------------------------------------------
void post_regen_iid(object me, string old_iid, string new_iid, mixed para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed f;

    loop (f in _regen_iid_funcs) {
        if ((!is_function(f))) {
            continue;
        }
        (*f)(me, old_iid, new_iid, para);
    }  // end loop (verified)
    clean_array(_regen_iid_funcs, 9);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=13, nargs=4, insts=13
// --------------------------------------------------------------------
void msg_l_enable_auto_lock(int client_no, string account, int result, int status) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_enable_auto_lock.c"))->main(client_no, account, result, status);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=32, nargs=4, insts=32
// --------------------------------------------------------------------
void get_left_time(int left_secs, int &hour, int &min, int &sec) {
    /* ---- Enhanced Pseudocode ---- */
    mixed left_time;

    hour = (left_secs / 3600);
    left_time = (left_secs - (hour * 3600));
    min = (left_time / 60);
    sec = (left_time - (min * 60));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=15, nargs=6, insts=15
// --------------------------------------------------------------------
void msg_l_query_card(int client_no, string cookie, int result, int count, int used, int locked) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_query_card.c"))->main(client_no, cookie, result, count, used, locked);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=11, nargs=2, insts=11
// --------------------------------------------------------------------
void msg_l_get_ssb_prompt_msg(int client_no, string result) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_get_ssb_prompt_msg.c"))->main(client_no, result);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
int get_server_status() {
    /* ---- Enhanced Pseudocode ---- */
    return _server_status;

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=14, nargs=1, insts=14
// --------------------------------------------------------------------
int set_server_status(int new_status) {
    /* ---- Enhanced Pseudocode ---- */
    mixed old_status;

    old_status = _server_status;
    _server_status = new_status;
    return old_status;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=14, nargs=5, insts=14
// --------------------------------------------------------------------
void msg_l_request_ip_card(int client_no, string gid, string type, string card_no, int result) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_request_ip_card.c"))->main(client_no, gid, type, card_no, result);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=13, nargs=4, insts=13
// --------------------------------------------------------------------
void msg_l_lock_account(int client_no, string account, string ret, string msg) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_lock_account.c"))->main(client_no, account, ret, msg);

}

// --------------------------------------------------------------------
// @code  nc=24, nargs=1, insts=24
// --------------------------------------------------------------------
void post_auto_save(object me) {
    /* ---- Enhanced Pseudocode ---- */
    mixed f;

    loop (f in _auto_save_funcs) {
        if ((!is_function(f))) {
            continue;
        }
        (*f)(me);
    }  // end loop (verified)
    clean_array(_auto_save_funcs, 9);

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=1, insts=9
// --------------------------------------------------------------------
void register_char_disconnected(function f) {
    /* ---- Enhanced Pseudocode ---- */
    _char_disconnected_funcs += ({ f });

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=25, nargs=2, insts=25
// --------------------------------------------------------------------
int is_same_week(int ti1, int ti2) {
    /* ---- Enhanced Pseudocode ---- */
    if ((abs((ti1 - ti2)) > 604800)) {
        return 0;
    }
    return (((ti1 + 288000) / 604800) == ((ti2 + 288000) / 604800));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=270, nargs=1, insts=270
// --------------------------------------------------------------------
private void telnet_daemon(int fd) {
    /* ---- Enhanced Pseudocode ---- */
    mixed account;
    mixed password;
    mixed ret;
    mixed dist;
    mixed key;
    mixed exp_pwd;
    mixed local_log;
    mixed location;
    mixed cmd;

    if (((time() - last_telnet_login_time) < 5)) {
        printf("Please wait a while to login.\n");
        close_telnet(fd);
        return;
    }
    last_telnet_login_time = time();
    telnet_config(fd, ([ "native_process": 1, "enable": 1 ]));
    printf("Account: ");
    telnet_config(fd, ([ "echo": 1 ]));
    account = trim(input_string());
    if ((((!is_array(ret = regexp(account, "[0-9,a-z,A-Z,_]*"))) || (sizeof(ret) != 2)) || (ret[1] != strlen(account)))) {
        printf("Bad account.\n");
        close_telnet(fd);
        return;
    }
    printf("Password: ");
    telnet_config(fd, ([ "echo": 0 ]));
    password = trim(input_string());
    if ((!is_string(dist = "/daemons/configd.c"->get_config("my_dist")))) {
        printf("Failed to acquire dist information now.\n");
        close_telnet(fd);
        return;
    }
    key = ((((dist + "/") + "asktao") + "/") + account);
    if (((!is_string(exp_pwd = get_telnet_password(key))) || (!strlen(exp_pwd)))) {
        printf("Failed to get password from auth-server.\nIf you config a client to type L without key, you can't pass.\n");
        sleep(2);
        close_telnet(fd);
        return;
    }
    if ((password != exp_pwd)) {
        printf("Bad password or account.\n");
        close_telnet(fd);
        return;
    }
    local_log = (strsrch((string)get_telnet_settings()["type"], "G") != -1);
    telnet_config(fd, ([ "id": (local_log ? account : 0), "echo": 1, "auth": 1, "enable": 1, "native_process": 1 ]));
    printf("\nCurrent time: %s", ctime());
    printf("Welcome back, %s.\n\n", account);
    location = sprintf("%s/%s:%d", (string)"/daemons/configd.c"->get_config("my_dist"), (string)"/daemons/configd.c"->get_config("my_name"), (int)"/daemons/configd.c"->get_config("my_port"));
    log_to_das(sprintf("main/Account %s login to %s.\n", account, location));
    log_to_das(sprintf("%s/Login to %s.\n", account, location));
    telnet_clients[fd] = ([ "account": account, "location": location, "fd": fd ]);
    while (1) {
        printf("\nShell> ");
        cmd = input_string();
        if ((trim(cmd) == ".")) {
            printf("logout\n");
            close_telnet(fd);
            return;
        }
        if (strlen(trim(cmd))) {
            log_to_das(sprintf("%s/%s", account, cmd));
        }
        execute_data(cmd, 1);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=12, nargs=3, insts=12
// --------------------------------------------------------------------
void msg_l_borrow_local_encrypt(int client_no, string account, int result) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_borrow_local_encrypt.c"))->main(client_no, account, result);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=13, nargs=4, insts=13
// --------------------------------------------------------------------
void msg_l_unlock_card(int client_no, string cookie, string number, int result) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_unlock_card.c"))->main(client_no, cookie, number, result);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=14, nargs=5, insts=14
// --------------------------------------------------------------------
void msg_got_gid_info(int client_no, int cookie, int result, string type, string name) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->msg_got_gid_info(cookie, result, sprintf("%s,%s", type, name));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=13, nargs=4, insts=13
// --------------------------------------------------------------------
void msg_l_get_local_encrypt_info(int client_no, string account, string password, int state) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_get_local_encrypt_info.c"))->main(client_no, account, password, state);

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=1, insts=9
// --------------------------------------------------------------------
void register_heart_beat_ex(function f) {
    /* ---- Enhanced Pseudocode ---- */
    _heart_beat_ex_funcs += ({ f });

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=16, nargs=4, insts=16
// --------------------------------------------------------------------
void msg_cfg_spa_info(int client_no, string ip, int port, string name) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/configd.c"->set_config("spa_info", ([ "ip": ip, "port": port, "name": name ]));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=20, nargs=11, insts=20
// --------------------------------------------------------------------
void msg_sell_treasure(int client_no, string serial_num, int type, int state, int end_time, int day, int price, int upset_price, string seller, string account, string para) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_sell_treasure.c"))->main(client_no, serial_num, type, state, end_time, day, price, upset_price, seller, account, para);

}

// --------------------------------------------------------------------
// @code  nc=24, nargs=1, insts=24
// --------------------------------------------------------------------
void post_adjust_position_info(object me) {
    /* ---- Enhanced Pseudocode ---- */
    mixed f;

    loop (f in _adjust_position_info_funcs) {
        if ((!is_function(f))) {
            continue;
        }
        (*f)(me);
    }  // end loop (verified)
    clean_array(_adjust_position_info_funcs, 9);

}

// --------------------------------------------------------------------
// @code  nc=13, nargs=2, insts=13
// --------------------------------------------------------------------
buffer xdbc_decrypt_data(buffer data, int key) {
    /* ---- Enhanced Pseudocode ---- */
    return rdl(data, 17, sprintf("%016X", (key & -1)));

}

// --------------------------------------------------------------------
// @code  nc=18, nargs=1, insts=18
// --------------------------------------------------------------------
private int xdbc_get_cno_of_view(int vno) {
    /* ---- Enhanced Pseudocode ---- */
    mapping view;

    if ((!is_mapping(view = _xdbc_query[vno]))) {
        return -1;
    }
    return view["cno"];

}

// --------------------------------------------------------------------
// @code  nc=19, nargs=1, insts=19
// --------------------------------------------------------------------
int create_queue_debug(string oname) {
    /* ---- Enhanced Pseudocode ---- */
    int oid;

    oid = create_queue(oname);
    queue_map[oid] = oname;
    return oid;

}

// --------------------------------------------------------------------
// @code  nc=24, nargs=1, insts=24
// --------------------------------------------------------------------
void post_prepare_switch_server(object me) {
    /* ---- Enhanced Pseudocode ---- */
    mixed f;

    loop (f in _prepare_switch_server_funcs) {
        if ((!is_function(f))) {
            continue;
        }
        (*f)(me);
    }  // end loop (verified)
    clean_array(_prepare_switch_server_funcs, 9);

}

// --------------------------------------------------------------------
// @code  nc=24, nargs=1, insts=24
// --------------------------------------------------------------------
void post_just_login_server(object me) {
    /* ---- Enhanced Pseudocode ---- */
    mixed f;

    loop (f in _just_login_server_funcs) {
        if ((!is_function(f))) {
            continue;
        }
        (*f)(me);
    }  // end loop (verified)
    clean_array(_just_login_server_funcs, 9);

}

// --------------------------------------------------------------------
// @code  nc=41, nargs=2, insts=41
// --------------------------------------------------------------------
private void xdbc_append_output(string type, string msg) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!is_string(_xdbc_output_file[type]))) {
        return;
    }
    log_file(_xdbc_output_file[type], ((ctime()[0..<2] + " ") + msg));
    if ((file_size(_xdbc_output_file[type]) > 104857600)) {
        ++_xdbc_output_file_suffix;
        xdbc_update_output_file(type);
    }

}

// --------------------------------------------------------------------
// @code  nc=30, nargs=1, insts=30
// --------------------------------------------------------------------
void release_tixml_parse_ex(mapping xml) {
    /* ---- Enhanced Pseudocode ---- */
    mixed node;

    map_delete(xml, "_childs_ex");
    if (is_array(xml["_childs"])) {
        loop (node in xml["_childs"]) {
            map_delete(node, "_parent");
            release_tixml_parse_ex(node);
        }  // end loop (verified)
    }

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
void require_uid() {
    /* ---- Enhanced Pseudocode ---- */
    set_uid(previous_object(), 0);

}

// --------------------------------------------------------------------
// @brief  converter: transform data
// @code  nc=11, nargs=3, insts=11
// --------------------------------------------------------------------
int to_set_timer(mixed timeout, mixed timer_id, function f) {
    /* ---- Enhanced Pseudocode ---- */
    return set_timer(timeout, &timer_id, replace_owner(0, f));

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=5, insts=4
// --------------------------------------------------------------------
void cmd_ma_oper(int client_no, int cookie, string cmds, string ma_account, array para) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @brief  data writer: persist information
// @code  nc=11, nargs=3, insts=11
// --------------------------------------------------------------------
void write_to_ex(mixed f, string path, mixed ret) {
    /* ---- Enhanced Pseudocode ---- */
    if (is_function(f)) {
        (*f)(ret);
    }

}

// --------------------------------------------------------------------
// @code  nc=112, nargs=1, insts=112
// --------------------------------------------------------------------
mapping analysis_ban_words_file(string file) {
    /* ---- Enhanced Pseudocode ---- */
    mixed lower_flag;
    mixed arr;
    mixed line;
    string words_group;
    mixed m;

    m = ([ ]);
    arr = explode(file, "\n");
    words_group = "";
    loop (line in arr) {
        line = trim(line);
        if ((sscanf(line, "[%s]", &words_group) == 1)) {
            m[words_group] = ([ ]);
            lower_flag = (("√Ù∏–¥ ”Ô" == words_group) ? 1 : 0);
            continue;
        }
        if (((!is_string(words_group)) || (strlen(words_group) < 1))) {
            continue;
        }
        if (((strlen(line) < 1) || (line[0] == 35))) {
            continue;
        }
        m[words_group][(lower_flag ? tolower_temp(line) : line)] = 1;
    }  // end loop (verified)
    loop (words_group in keys(m)) {
        m[words_group] = sort(keys(m[words_group]), (: strlen :));
    }  // end loop (verified)
    return m;

}

// --------------------------------------------------------------------
// @code  nc=41, nargs=1, insts=41
// --------------------------------------------------------------------
string time_desc_to_ch(mixed t) {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;

    if (is_string(t)) {
        t = time_desc_to_int(t, 3);
    }
    m = (is_int(t) ? local_time(t) : local_time());
    return sprintf("%4dƒÍ%02d‘¬%02d»’%02d ±%02d∑÷", m["year"], m["mon"], m["mday"], m["hour"], m["min"]);

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=5, insts=4
// --------------------------------------------------------------------
void cmd_ma_set(int client_no, int cookie, string cmds, string ma_account, array para) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=3, insts=10
// --------------------------------------------------------------------
void msg_read_log(int client_no, int cookie, int result) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->msg_read_log(cookie, result);

}

// --------------------------------------------------------------------
// @code  nc=25, nargs=1, insts=25
// --------------------------------------------------------------------
int report_cmds_stat(string file_name) {
    /* ---- Enhanced Pseudocode ---- */
    if (enable_stat_cmds) {
        printf("not finish stat cmds\n");
        return 0;
    }
    write_file(file_name, save_string(([ "interval": stat_cmds_interval, "flow_in": cmds_flow_in, "processed": cmds_processed ])));
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=24, nargs=1, insts=24
// --------------------------------------------------------------------
void post_char_reconnected(object me) {
    /* ---- Enhanced Pseudocode ---- */
    mixed f;

    loop (f in _char_reconnected_funcs) {
        if ((!is_function(f))) {
            continue;
        }
        (*f)(me);
    }  // end loop (verified)
    clean_array(_char_reconnected_funcs, 9);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=11, nargs=4, insts=11
// --------------------------------------------------------------------
void msg_get_sys_envelope(int client_no, int cookie, int result, string content) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->msg_get_sys_envelope(cookie, result, content);

}

// --------------------------------------------------------------------
// @code  nc=121, nargs=2, insts=121
// --------------------------------------------------------------------
string decrypt_mask_md5(string md5_str, string mask) {
    /* ---- Enhanced Pseudocode ---- */
    mixed start_pos;
    mixed end_pos;
    int i;
    int pos;
    mixed weight;
    mixed arr_sub_str;
    mixed raw_md5;

    start_pos = atoi(mask[0..1]);
    end_pos = atoi(mask[2..3]);
    if (((end_pos < start_pos) || (strlen(mask[4..<1]) != ((end_pos + 1) - start_pos)))) {
        catch(error("bad mask!\n"));

        // --- label L0043 ---
        return md5_str;
    }
    arr_sub_str = allocate_array(((end_pos - start_pos) + 1));
    for (i = 0; i <= (end_pos - start_pos); i++) {
        pos = (4 + i);
        weight = atoi(mask[pos..pos]);
        arr_sub_str[i] = sprintf("%c", (md5_str[(start_pos + i)] - weight));
    }  // end loop (verified)
    raw_md5 = ((md5_str[0..(start_pos - 1)] + implode(arr_sub_str, "")) + md5_str[(end_pos + 1)..<1]);
    return raw_md5;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=14, nargs=5, insts=14
// --------------------------------------------------------------------
void msg_l_set_safe_ctrl_str(int client_no, string module, string account, int result, string type) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_set_safe_ctrl_str.c"))->main(client_no, module, account, result, type);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=14, nargs=5, insts=14
// --------------------------------------------------------------------
void msg_l_set_safe_ctrl_var(int client_no, string module, string account, int result, int new_var) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_set_safe_ctrl_var.c"))->main(client_no, module, account, result, new_var);

}

// --------------------------------------------------------------------
// @code  nc=32, nargs=1, insts=32
// --------------------------------------------------------------------
varargs void xdbc_log(int level) {
    /* ---- Enhanced Pseudocode ---- */
    if ((level < _xdbc_debug_level)) {
        return;
    }
    xdbc_append_output("log", (*find_function_arg("sprintf", ($<)[1..<1]))());
    if (_enable_log_trace) {
        (*find_function_arg((: xdbc_trace, level :), ($<)[1..<1]))();
    }

}

// --------------------------------------------------------------------
// @code  nc=47, nargs=0, insts=47
// --------------------------------------------------------------------
void post_init() {
    /* ---- Enhanced Pseudocode ---- */
    mixed f;
    int funcs;


    // --- label L0002 ---
    while (sizeof(_post_init_funcs)) {
        funcs = _post_init_funcs;
        _post_init_funcs = ({});
        loop (f in funcs) {
            if (is_function(f)) {
                try {
                    (*f)();
                } catch { /* error handled */ }

                // --- label L0029 ---
            }
        }  // end loop (verified)
        if (sizeof(_post_init_funcs)) {
            sleep(1);
        }
    }  // end loop (verified)
    _post_init_funcs = 0;

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=29, nargs=2, insts=29
// --------------------------------------------------------------------
int is_same_month(int ti1, int ti2) {
    /* ---- Enhanced Pseudocode ---- */
    if ((abs((ti1 - ti2)) > 2678400)) {
        return 0;
    }
    return (ctime(ti1, 3)[0..5] == ctime(ti2, 3)[0..5]);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=25, nargs=6, insts=25
// --------------------------------------------------------------------
void msg_ccs_emote_data(int client_no, string verb, string target, int channel, string peer, mixed self_info) {
    /* ---- Enhanced Pseudocode ---- */
    if (is_object(find_object("/dba/cmds/intersrv/emote_data.c"))) {
        (find_object("/dba/cmds/intersrv/emote_data.c"))->main(client_no, verb, target, channel, peer, (is_string(self_info) ? restore_value(self_info) : self_info));
    }

}

// --------------------------------------------------------------------
// @code  nc=48, nargs=3, insts=48
// --------------------------------------------------------------------
private void add_to_section(mapping section, string key, mixed value) {
    /* ---- Enhanced Pseudocode ---- */
    if ((key[(strlen(key) - 1)] == 36)) {
        key = key[0..<2];
        if (is_undefined(section[key])) {
            section[key] = ({ value });
        } else {
            section[key] += ({ value });
        }
        return;
    }
    section[key] = value;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=3, insts=10
// --------------------------------------------------------------------
void msg_got_page_data_ex(int client_no, int cookie, buffer content) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->msg_got_page_data_ex(cookie, content);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=38, nargs=3, insts=38
// --------------------------------------------------------------------
void register_daemon_data(string daemon_name, mixed index, mixed val) {
    /* ---- Enhanced Pseudocode ---- */
    mixed m;

    if ((!is_mapping(m = daemon_data[daemon_name]))) {
        m = ([ ]);
        daemon_data[daemon_name] = m;
    }
    m << ([ index: val ]);
    if (find_object(daemon_name)) {
        daemon_name->notify_daemon_data(index, val);
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=425, nargs=1, insts=425
// --------------------------------------------------------------------
void rpc_stat(mixed arg) {
    /* ---- Enhanced Pseudocode ---- */
    mixed send_rpc_stat;
    string msg;
    int key;
    string top_key;
    mapping data;
    mapping top_data;
    mixed i;
    mixed j;

    if ((!arg)) {
        printf("«Î ‰»Î≤Œ ˝\n    all       ≤Èø¥À˘”–Õ≥º∆–≈œ¢\n    top count ≤Èø¥±ª∫ÙΩ–¥Œ ˝◊Ó∂‡µƒµ˜”√∫Ø ˝\n    top tick  ≤Èø¥∆Ωæ˘◊Ó∫ƒ ±µƒµ˜”√∫Ø ˝\n    top do    ≤Èø¥∆Ωæ˘÷¥–– ±º‰◊Ó≥§µƒ∫Ø ˝\n    top exec  ≤Èø¥∆Ωæ˘ µº ÷¥–– ±º‰◊Ó≥§µƒ∫Ø ˝\n æ¿˝£∫rpc_stat(\"top count tick do exec\")\n");
        return;
    }
    send_rpc_stat = "/daemons/rpcd.c"->query_send_rpc_stat();
    if ((arg == "all")) {
        printf("%O\n", send_rpc_stat);
        return;
    }
    msg = "";
    if (((strsrch(arg, "count") >= 0) && (sizeof(send_rpc_stat) > 0))) {
        top_key = "";
        loop (key in keys(send_rpc_stat)) {
            if (is_undefined(top_data = send_rpc_stat[top_key])) {
                top_key = key;
                top_data = send_rpc_stat[top_key];
                continue;
            }
            data = send_rpc_stat[key];
            if ((data["total_call_count"] > top_data["total_call_count"])) {
                top_key = key;
            }
        }  // end loop (verified)
        msg += sprintf("±ªµ˜”√¥Œ ˝◊Ó∂‡µƒ∫Ø ˝ «£∫\n%s : %d\n", top_key, top_data["total_call_count"]);
    }
    if (((strsrch(arg, "tick") >= 0) && (sizeof(send_rpc_stat) > 0))) {
        top_key = "";
        loop (key in keys(send_rpc_stat)) {
            if (is_undefined(top_data = send_rpc_stat[top_key])) {
                top_key = key;
                continue;
            }
            data = send_rpc_stat[key];
            i = data["total_call_count"];
            j = top_data["total_call_count"];
            if (((data["time_cost"]["total_tick"] / i) > (top_data["time_cost"]["total_tick"] / j))) {
                top_key = key;
            }
        }  // end loop (verified)
        top_data = send_rpc_stat[top_key];
        j = top_data["total_call_count"];
        msg += sprintf("∆Ωæ˘µ˜”√◊Ó∫ƒ ±µƒ∫Ø ˝ «£∫\n%s : %d\n", top_key, (top_data["time_cost"]["total_tick"] / j));
    }
    if (((strsrch(arg, "do") >= 0) && (sizeof(send_rpc_stat) > 0))) {
        top_key = "";
        loop (key in keys(send_rpc_stat)) {
            if (is_undefined(top_data = send_rpc_stat[top_key])) {
                top_key = key;
                continue;
            }
            data = send_rpc_stat[key];
            i = data["total_call_count"];
            j = top_data["total_call_count"];
            if (((data["time_cost"]["do_tick"] / i) > (top_data["time_cost"]["do_tick"] / j))) {
                top_key = key;
            }
        }  // end loop (verified)
        top_data = send_rpc_stat[top_key];
        j = top_data["total_call_count"];
        msg += sprintf("∆Ωæ˘÷¥–– ±º‰◊Ó≥§µƒ∫Ø ˝ «£∫\n%s : %d\n", top_key, (top_data["time_cost"]["do_tick"] / j));
    }
    if (((strsrch(arg, "exec") >= 0) && (sizeof(send_rpc_stat) > 0))) {
        top_key = "";
        loop (key in keys(send_rpc_stat)) {
            if (is_undefined(top_data = send_rpc_stat[top_key])) {
                top_key = key;
                continue;
            }
            data = send_rpc_stat[key];
            i = data["total_call_count"];
            j = top_data["total_call_count"];
            if (((data["time_cost"]["exec_tick"] / i) > (top_data["time_cost"]["exec_tick"] / j))) {
                top_key = key;
            }
        }  // end loop (verified)
        top_data = send_rpc_stat[top_key];
        j = top_data["total_call_count"];
        msg += sprintf("∆Ωæ˘ µº ÷¥–– ±º‰◊Ó≥§µƒ∫Ø ˝ «£∫\n%s : %d\n", top_key, (top_data["time_cost"]["exec_tick"] / j));
    }
    if ((strsrch(arg, "top") >= 0)) {
        if ((strlen(msg) <= 0)) {
            printf("≤Œ ˝¥ÌŒÛ£¨ªÚ’ﬂƒø«∞ªπ√ª”–»Œ∫Œ RPC µ˜”√µƒÕ≥º∆–≈œ¢\n");
        } else {
            printf(msg);
        }
    }

}

// --------------------------------------------------------------------
// @code  nc=20, nargs=1, insts=20
// --------------------------------------------------------------------
int xdbc_is_connected(int cno) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ret;

    ret = xdbc_call_server(cno, ([ "cmd": "is_connected" ]));
    if ((-1 == ret)) {
        return 0;
    }
    return ret;

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=3, insts=4
// --------------------------------------------------------------------
void msg_sddh_oper(int client_no, string cmd, array args) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=2, insts=10
// --------------------------------------------------------------------
void msg_cfg_my_dist(int client_no, string dist) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/configd.c"->set_config("my_dist", dist);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=29, nargs=4, insts=29
// --------------------------------------------------------------------
void read_to_ex(mixed f, string path, string name, mixed ret) {
    /* ---- Enhanced Pseudocode ---- */
    mixed convert_type;

    if (is_string(ret)) {
        convert_type = get_data_convert_type(path, name);
        ret = "/daemons/datasaved.c"->convert_to_memory_data(convert_type, ret);
    }
    if (is_function(f)) {
        (*f)(ret);
    }

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=1, insts=9
// --------------------------------------------------------------------
void register_switch_server(function f) {
    /* ---- Enhanced Pseudocode ---- */
    _switch_server_funcs += ({ f });

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=12, nargs=1, insts=12
// --------------------------------------------------------------------
int get_next_hour_as_int(int ntime) {
    /* ---- Enhanced Pseudocode ---- */
    return (((ntime / 3600) * 3600) + 3600);

}

// --------------------------------------------------------------------
// @code  nc=13, nargs=1, insts=13
// --------------------------------------------------------------------
string time_desc(mixed t) {
    /* ---- Enhanced Pseudocode ---- */
    return ctime((is_int(t) ? t : time()), 1);

}

// --------------------------------------------------------------------
// @brief  validator: verify data
// @code  nc=22, nargs=1, insts=22
// --------------------------------------------------------------------
int check_server_status(mixed check_status) {
    /* ---- Enhanced Pseudocode ---- */
    if (is_int(check_status)) {
        return (_server_status == check_status);
    }
    if (is_array(check_status)) {
        return (member_array(_server_status, check_status) != -1);
    }
    return 0;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=49, nargs=1, insts=49
// --------------------------------------------------------------------
array get_etc_dir(string dir) {
    /* ---- Enhanced Pseudocode ---- */
    mixed dirs;

    if ((dir[0] == 47)) {
        dir = dir[1..<1];
    }
    if ((dir[(sizeof(dir) - 1)] == 47)) {
        dir = dir[0..<2];
    }
    if (is_array(dirs = get_dir(("/dba/pack_data/etc.pak/" + dir)))) {
        return dirs;
    }
    printf("[1;31mEtc directory %s not found.\n[2;37;0m", dir);
    return "";

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=42, nargs=2, insts=42
// --------------------------------------------------------------------
array get_all_branches(string path, string name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed get_branches;
    mixed all_branches;

    all_branches = ({});
    do {
        get_branches = "/daemons/filed.c"->get_catalog(path, name, "", (sizeof(all_branches) + 1), 100);
        if ((!is_array(get_branches))) {
            get_branches = ({});
        }
        all_branches += get_branches;
    } while ((sizeof(get_branches) >= 100));
    return all_branches;

}

// --------------------------------------------------------------------
// @code  nc=120, nargs=0, insts=120
// --------------------------------------------------------------------
private void load_file_dependence() {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr;
    mixed str;
    int file;
    mixed depend_file;
    int i;

    arr = explode(read_etc_file("global_file_dependence.list"), "\n");
    arr += explode(read_etc_file("file_dependence.list"), "\n");
    loop (str in arr) {
        if ((sscanf(str, "%s:%s", &file, &depend_file) != 2)) {
            continue;
        }
        file = trim(file);
        depend_file = trim(depend_file);
        if ((file_exist_ex(file) < 0)) {
            printf("Œƒº˛ %s ≤ª¥Ê‘⁄£¨Œﬁ∑®»∑∂®∆‰‘ÿ»Î/±‡“Î“¿¿µ–‘\n", file);
            continue;
        }
        depend_file = explode(depend_file, " ");
        for (i = 0; i <= (sizeof(depend_file) - 1); i++) {
            if ((file_exist_ex(depend_file[i]) < 0)) {
                printf("Œƒº˛ %s ≤ª¥Ê‘⁄£¨Œﬁ∑®»∑∂®∆‰‘ÿ»Î/±‡“Î“¿¿µ–‘\n", depend_file[i]);
                depend_file[i] = 0;
                continue;
            }
            depend_file[i] = trim(depend_file[i]);
        }  // end loop (verified)
        clean_array(depend_file, 4);
        file_dependence[file] = ((array)file_dependence[file] + depend_file);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @code  nc=134, nargs=1, insts=134
// --------------------------------------------------------------------
string convert_int_to_scientific_string(int amount) {
    /* ---- Enhanced Pseudocode ---- */
    int len;
    string str;
    string ret;
    string seperator;

    ret = "";
    seperator = ",";
    str = (string)amount;
    len = strlen(str);
    if ((len <= 3)) {
        ret = str;
    } else if ((len <= 6)) {
        ret = sprintf("%s%s%s", str[0..(len - 4)], seperator, str[(len - 3)..(len - 1)]);
    } else if ((len <= 9)) {
        ret = sprintf("%s%s%s%s%s", str[0..(len - 7)], seperator, str[(len - 6)..(len - 4)], seperator, str[(len - 3)..(len - 1)]);
    } else if ((len <= 12)) {
        ret = sprintf("%s%s%s%s%s%s%s", str[0..(len - 10)], seperator, str[(len - 9)..(len - 7)], seperator, str[(len - 6)..(len - 4)], seperator, str[(len - 3)..(len - 1)]);
    }
    return ret;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=17, nargs=4, insts=17
// --------------------------------------------------------------------
void msg_rpc_server_name(int client_no, string name, string version, int server_time) {
    /* ---- Enhanced Pseudocode ---- */
    if (is_object(find_object("/dba/cmds/intersrv/server_name.c"))) {
        (find_object("/dba/cmds/intersrv/server_name.c"))->main(client_no, name, version, server_time);
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=14, nargs=5, insts=14
// --------------------------------------------------------------------
void msg_verify_code_result(int client_no, string gid, string code, string type, int result) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_verify_code_result.c"))->main(client_no, gid, code, type, result);

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=3, insts=4
// --------------------------------------------------------------------
void msg_league_oper(int client_no, string cmd, array args) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=3, insts=4
// --------------------------------------------------------------------
void cmd_l_block_share_store(int client_no, string account, int interval) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=50, nargs=2, insts=50
// --------------------------------------------------------------------
void msg_cfg_dba_list(int client_no, array list) {
    /* ---- Enhanced Pseudocode ---- */
    mixed dba_list;
    int i;

    dba_list = allocate_array((sizeof(list) / 2));
    i = 0;

    // --- label L0014 ---
    while ((i < sizeof(list))) {
        dba_list[(i / 2)] = ([ "ip": list[i], "port": list[(i + 1)] ]);
        i += 2;
    }  // end loop (verified)
    "/daemons/configd.c"->set_config("dba_list", dba_list);

}

// --------------------------------------------------------------------
// @code  nc=18, nargs=0, insts=18
// --------------------------------------------------------------------
mapping init_cost_stat() {
    /* ---- Enhanced Pseudocode ---- */
    return ([ "count": allocate_array(2104, 0), "ticks": allocate_array(2104, 0), "other_count": 0, "other_ticks": 0 ]);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=22, nargs=1, insts=22
// --------------------------------------------------------------------
void cmd_internal_comm_closed(int client_no) {
    /* ---- Enhanced Pseudocode ---- */
    mixed agent;

    if (is_object(agent = find_agent_by_client(client_no))) {
        agent->connection_lost();
        return;
    }
    "/daemons/threadd.c"->dispatch_service((: _cmd_internal_comm_closed, client_no :));

}

// --------------------------------------------------------------------
// @code  nc=88, nargs=3, insts=88
// --------------------------------------------------------------------
mixed sql_insert(mixed db, string table, mapping data) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    mixed ks;
    mixed vs;
    mixed val;
    mixed sql_cmd;

    ks = keys(data);
    vs = values(data);
    for (i = 0; i <= (sizeof(vs) - 1); i++) {
        val = vs[i];
        if (is_undefined(val)) {
            vs[i] = "0";
        } else if (is_int(val)) {
            vs[i] = (string)val;
        } else if (is_string(val)) {
            vs[i] = (("'" + val) + "'");
        } else {
            error("Insert value must be integer or string.\n");
        }
    }  // end loop (verified)
    sql_cmd = sprintf("insert into %s (%s) values (%s)", table, implode(ks, ","), implode(vs, ","));
    return sql_execute(db, sql_cmd);

}

// --------------------------------------------------------------------
// @code  nc=141, nargs=4, insts=141
// --------------------------------------------------------------------
mixed sql_update(mixed db, string table, mapping data, mixed condition) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    mixed arr;
    mixed val;
    mixed sql_cmd;
    string str;
    int key;
    string val_str;

    str = "";
    loop (key in keys(data)) {
        if (is_undefined(val = data[key])) {
            continue;
        }
        if (is_int(val)) {
            val_str = (string)val;
        } else if (is_string(val)) {
            val_str = (("'" + val) + "'");
        } else {
            error("Update value must be integer or string.\n");
        }
        str += (((key + "=") + val_str) + ",");
    }  // end loop (verified)
    put_sub_data(&str, (sizeof(str) - 1), 32);
    if ((condition && sizeof(condition))) {
        str += "where ";
        if (is_string(condition)) {
            str += condition;
        } else if (is_mapping(condition)) {
            i = 0;
            arr = allocate_array(sizeof(condition));
            loop (key in keys(condition)) {
                arr[i++] = sprintf("%s=%O", key, condition[key]);
            }  // end loop (verified)
            str += implode(arr, " and ");
        } else {
            error("Bad condition type, expected string | mapping.\n");
        }
    }
    sql_cmd = sprintf("update %s set %s", table, str);
    return sql_execute(db, sql_cmd);

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=0, insts=8
// --------------------------------------------------------------------
void init_post_init_funcs() {
    /* ---- Enhanced Pseudocode ---- */
    _post_init_funcs = ({});

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=25, nargs=3, insts=25
// --------------------------------------------------------------------
int is_in_ip_span(int start_ip, int end_ip, int cur_ip) {
    /* ---- Enhanced Pseudocode ---- */
    start_ip ^= -2147483648;
    end_ip ^= -2147483648;
    cur_ip ^= -2147483648;
    return ((cur_ip >= start_ip) && (cur_ip <= end_ip));

}

// --------------------------------------------------------------------
// @code  nc=16, nargs=1, insts=16
// --------------------------------------------------------------------
mixed tixml_parse_file(string file_name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed file_str;

    if ((!is_string(file_str = read_etc_file(file_name)))) {
        return 0;
    }
    return tixml_parse(file_str);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=15, nargs=6, insts=15
// --------------------------------------------------------------------
void msg_l_cost_fill_info(int client_no, string account, string gid, int times, int result, string type) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_cost_fill_info.c"))->main(client_no, account, gid, times, result, type);

}

// --------------------------------------------------------------------
// @code  nc=74, nargs=1, insts=74
// --------------------------------------------------------------------
void unload_all(string path) {
    /* ---- Enhanced Pseudocode ---- */
    mixed files;
    int i;

    if ((path[(strlen(path) - 1)] != 47)) {
        path += "/";
    }
    files = get_dir_ex(path);
    if ((!files)) {
        return;
    }
    i = 0;

    // --- label L0028 ---
    while ((i < sizeof(files))) {
        try {
            destruct_object((path + files[i]));
        } catch { /* error handled */ }

        // --- label L0045 ---
        if ((!0)) {
            printf("Unload: %s... OK\n", (path + files[i]));
        } else {
            printf("Unload: %s... Failed\n", (path + files[i]));
        }
        ++i;
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @code  nc=88, nargs=3, insts=88
// --------------------------------------------------------------------
mixed sql_replace(mixed db, string table, mapping data) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    mixed ks;
    mixed vs;
    mixed val;
    mixed sql_cmd;

    ks = keys(data);
    vs = values(data);
    for (i = 0; i <= (sizeof(vs) - 1); i++) {
        val = vs[i];
        if (is_undefined(val)) {
            vs[i] = "0";
        } else if (is_int(val)) {
            vs[i] = (string)val;
        } else if (is_string(val)) {
            vs[i] = (("'" + val) + "'");
        } else {
            error("Insert value must be integer or string.\n");
        }
    }  // end loop (verified)
    sql_cmd = sprintf("replace into %s (%s) values (%s)", table, implode(ks, ","), implode(vs, ","));
    return sql_execute(db, sql_cmd);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=14, nargs=0, insts=14
// --------------------------------------------------------------------
void safe_reboot() {
    /* ---- Enhanced Pseudocode ---- */
    broadcast("[1;31mVM(%s) is going to shutdown safely.\n[2;37;0m", "DBA");
    "/dba/daemons/dbaserverd.c"->wait_finish_service();
    _reboot();

}

// --------------------------------------------------------------------
// @code  nc=7, nargs=0, insts=7
// --------------------------------------------------------------------
void init_platform_module() {
    /* ---- Enhanced Pseudocode ---- */
    printf("Init platform ok.\n");

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=17, nargs=9, insts=17
// --------------------------------------------------------------------
void cmd_intersvr_rpc_call(int client_no, int cookie, int method, string owner, string server, string ob, string f, string stat, string args_str) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/rpcd.c"->recv_rpc_call(client_no, cookie, method, owner, server, ob, f, stat, args_str);

}

// --------------------------------------------------------------------
// @code  nc=115, nargs=0, insts=115
// --------------------------------------------------------------------
void init_xdbc_client() {
    /* ---- Enhanced Pseudocode ---- */
    mixed no_xdbc;

    no_xdbc = (defined("XDBC") ? 0 : 1);
    f_connect = find_mysql_func("mysql_connect", no_xdbc);
    f_disconnect = find_mysql_func("mysql_disconnect", no_xdbc);
    f_is_connected = find_mysql_func("mysql_is_connected", no_xdbc);
    f_get_error = find_mysql_func("mysql_get_error", no_xdbc);
    f_execute = find_mysql_func("mysql_execute", no_xdbc);
    f_query = find_mysql_func("mysql_query", no_xdbc);
    f_finalize = find_mysql_func("mysql_finalize", no_xdbc);
    f_num_records = find_mysql_func("mysql_num_records", no_xdbc);
    f_fetch_record = find_mysql_func("mysql_fetch_record", no_xdbc);
    f_this_record = find_mysql_func("mysql_this_record", no_xdbc);
    f_field_value = find_mysql_func("mysql_field_value", no_xdbc);
    f_set_auto_commit = find_mysql_func("mysql_set_auto_commit", no_xdbc);
    f_select_db = find_mysql_func("mysql_select_db", no_xdbc);
    f_field_name = find_mysql_func("mysql_field_name", no_xdbc);
    f_num_fields = find_mysql_func("mysql_num_fields", no_xdbc);
    f_int_field = find_mysql_func("mysql_int_field", no_xdbc);
    f_get_insert_id = find_mysql_func("mysql_get_insert_id", no_xdbc);

}

// --------------------------------------------------------------------
// @code  nc=24, nargs=2, insts=24
// --------------------------------------------------------------------
void post_char_disconnected(object me, mixed is_logout) {
    /* ---- Enhanced Pseudocode ---- */
    mixed f;

    loop (f in _char_disconnected_funcs) {
        if ((!is_function(f))) {
            continue;
        }
        (*f)(me);
    }  // end loop (verified)
    clean_array(_char_disconnected_funcs, 9);

}

// --------------------------------------------------------------------
// @code  nc=148, nargs=3, insts=148
// --------------------------------------------------------------------
void create_data_from3(array arr, array &all_values, mapping &data_table) {
    /* ---- Enhanced Pseudocode ---- */
    mixed all_keys;
    mixed influences;
    mixed line;
    int i;
    mixed val;
    int key;

    all_keys = ({});
    loop (line in arr) {
        if ((strlen(line) <= 1)) {
            continue;
        }
        if ((line[0] == 35)) {
            continue;
        }
        if ((sizeof(all_keys) <= 0)) {
            all_keys = explode(line, "|");
            for (i = 1; i <= (sizeof(all_keys) - 1); i++) {
                key = trim(all_keys[i]);
                all_keys[i] = key;
                if ((strlen(key) <= 0)) {
                    continue;
                }
                key = convert_to_value(key);
                all_keys[i] = key;
                data_table[key] = ({});
            }  // end loop (verified)
            continue;
        }
        influences = explode(line, "|");
        val = influences[0];
        sscanf(val, "%s#%*s", &val);
        val = convert_to_value(val);
        all_values += ({ val });
        for (i = 1; i <= (sizeof(influences) - 1); i++) {
            if ((strlen(trim(influences[i])) > 0)) {
                data_table[all_keys[i]] += ({ val });
            }
        }  // end loop (verified)
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=1, insts=10
// --------------------------------------------------------------------
void cmd_new_connection(int client_no) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/threadd.c"->dispatch_service((: _cmd_new_connection, client_no :));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=89, nargs=2, insts=89
// --------------------------------------------------------------------
void msg_cfg_game_server_list(int client_no, array list) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    mixed ip_info;
    mixed game_server_list;

    game_server_list = allocate_array((sizeof(list) / 5));
    i = 0;

    // --- label L0014 ---
    while ((i < sizeof(list))) {
        ip_info = explode(list[(i + 1)], ",");
        game_server_list[(i / 5)] = ([ "name": list[i], "ip": ip_info[0], "ip_wt": ip_info[1], "ip_dx": ip_info[2], "enable_multi_ip": ip_info[3], "port": list[(i + 2)], "id": list[(i + 3)], "enable": list[(i + 4)] ]);
        i += 5;
    }  // end loop (verified)
    "/daemons/configd.c"->set_config("game_server_list", game_server_list);

}

// --------------------------------------------------------------------
// @code  nc=36, nargs=2, insts=36
// --------------------------------------------------------------------
int xdbc_query(int cno, string sql_cmd) {
    /* ---- Enhanced Pseudocode ---- */
    int vno;

    xdbc_trace(0, "Call xdbc_query(%d, %s)\n", cno, sql_cmd);
    vno = xdbc_call_server(cno, ([ "cmd": "query", "sql_cmd": sql_cmd ]));
    if ((vno == -1)) {
        return -1;
    }
    _xdbc_query[vno] = ([ "cno": cno ]);
    return vno;

}

// --------------------------------------------------------------------
// @code  nc=21, nargs=0, insts=21
// --------------------------------------------------------------------
int stop_stat_cmds() {
    /* ---- Enhanced Pseudocode ---- */
    if ((!enable_stat_cmds)) {
        return 0;
    }
    enable_stat_cmds = 0;
    if (get_timer("stat_cmds")) {
        delete_timer("stat_cmds");
    }
    return 1;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=16, nargs=4, insts=16
// --------------------------------------------------------------------
void msg_cfg_ccs_info(int client_no, string ip, int port, string name) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/configd.c"->set_config("ccs_info", ([ "ip": ip, "port": port, "name": name ]));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=16, nargs=7, insts=16
// --------------------------------------------------------------------
void msg_get_goods_from_tts(int client_no, string gid, int mode, int storage_fee, int type, string goods_id, string result) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_get_goods_from_tts.c"))->main(client_no, gid, mode, storage_fee, type, goods_id, result);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=15, nargs=4, insts=15
// --------------------------------------------------------------------
void read_content_to_ex(string path, string name, mixed branch, mixed to) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->read_content_to(path, name, branch, (: read_to_ex, to, path, name :));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=50, nargs=2, insts=50
// --------------------------------------------------------------------
void msg_cfg_ldba_list(int client_no, array list) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ldba_list;
    int i;

    ldba_list = allocate_array((sizeof(list) / 2));
    i = 0;

    // --- label L0014 ---
    while ((i < sizeof(list))) {
        ldba_list[(i / 2)] = ([ "ip": list[i], "port": list[(i + 1)] ]);
        i += 2;
    }  // end loop (verified)
    "/daemons/configd.c"->set_config("ldba_list", ldba_list);

}

// --------------------------------------------------------------------
// @code  nc=18, nargs=1, insts=18
// --------------------------------------------------------------------
void xdbc_finalize(int vno) {
    /* ---- Enhanced Pseudocode ---- */
    xdbc_trace(0, "Call xdbc_finalize(%d)\n", vno);
    xdbc_call_server(xdbc_get_cno_of_view(vno), ([ "cmd": "finalize", "vno": vno ]));

}

// --------------------------------------------------------------------
// @code  nc=18, nargs=1, insts=18
// --------------------------------------------------------------------
int xdbc_num_records(int vno) {
    /* ---- Enhanced Pseudocode ---- */
    xdbc_trace(0, "Call xdbc_num_record(%d)\n", vno);
    return xdbc_call_server(xdbc_get_cno_of_view(vno), ([ "cmd": "num_records", "vno": vno ]));

}

// --------------------------------------------------------------------
// @code  nc=105, nargs=1, insts=105
// --------------------------------------------------------------------
string convert_into_char(string content) {
    /* ---- Enhanced Pseudocode ---- */
    mixed str_size;
    int i;
    int value;
    mixed str_convert;

    str_convert = content;
    str_size = sizeof(content);
    for (i = 0; i <= (str_size - 1); i++) {
        if (ischinese(content[i])) {
            continue;
        }
        if ((content[i] != 37)) {
            continue;
        }
        if ((i >= (str_size - 2))) {
            continue;
        }
        if (((!isxdigit(content[(i + 1)])) || (!isxdigit(content[(i + 2)])))) {
            continue;
        }
        value = atoi(("0x" + content[(i + 1)..(i + 2)]));
        if (((!isalpha(value)) && (value != 46))) {
            continue;
        }
        str_convert = replace_string(str_convert, content[i..(i + 2)], sprintf("%c", value), 1);
    }  // end loop (verified)
    return str_convert;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=18, nargs=9, insts=18
// --------------------------------------------------------------------
void msg_l_query_coin(int client_no, string gid, string account, string module, string para, int have_coin_pwd, int unlock_time, int unlock_duration, string memo) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_query_coin.c"))->main(client_no, gid, account, module, para, have_coin_pwd, unlock_time, unlock_duration, memo);

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=3, insts=4
// --------------------------------------------------------------------
void msg_citywar_oper(int client_no, string cmd, array args) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=2, insts=4
// --------------------------------------------------------------------
void cmd_l_recycle_stall(int client_no, string account) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
mixed query_disconnect_stat() {
    /* ---- Enhanced Pseudocode ---- */
    return disconnect_stat;

}

// --------------------------------------------------------------------
// @code  nc=24, nargs=1, insts=24
// --------------------------------------------------------------------
void post_new_connection(int client_no) {
    /* ---- Enhanced Pseudocode ---- */
    mixed f;

    loop (f in _new_connection_funcs) {
        if ((!is_function(f))) {
            continue;
        }
        (*f)(client_no);
    }  // end loop (verified)
    clean_array(_new_connection_funcs, 9);

}

// --------------------------------------------------------------------
// @code  nc=24, nargs=1, insts=24
// --------------------------------------------------------------------
void post_heart_beat_ex(object me) {
    /* ---- Enhanced Pseudocode ---- */
    mixed f;

    loop (f in _heart_beat_ex_funcs) {
        if ((!is_function(f))) {
            continue;
        }
        (*f)(me);
    }  // end loop (verified)
    clean_array(_heart_beat_ex_funcs, 9);

}

// --------------------------------------------------------------------
// @code  nc=45, nargs=1, insts=45
// --------------------------------------------------------------------
int xdbc_allocate_client(int fd) {
    /* ---- Enhanced Pseudocode ---- */
    int i;

    for (i = 0; i <= (sizeof(_xdbc_clients) - 1); i++) {
        if ((!is_mapping(_xdbc_clients[i]))) {
            _xdbc_clients[i] = ([ "id": i, "fd": fd, "time_stamp": time() ]);
            return i;
        }
    }  // end loop (verified)
    xdbc_log(0, "No free client to accept new connection.\n");
    return -1;

}

// --------------------------------------------------------------------
// @code  nc=17, nargs=0, insts=17
// --------------------------------------------------------------------
void _reboot() {
    /* ---- Enhanced Pseudocode ---- */
    broadcast("[1;31mVM(%s) is going to shutdown.\n[2;37;0m", "DBA");
    set_nohalt();
    save_all();
    shutdown();
    broadcast("[1;37mAll objects are saved.\n[2;37;0m");

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=2, insts=9
// --------------------------------------------------------------------
mixed mdb_insert(string table, mapping data) {
    /* ---- Enhanced Pseudocode ---- */
    return sql_insert(_mdb, table, data);

}

// --------------------------------------------------------------------
// @code  nc=7, nargs=1, insts=7
// --------------------------------------------------------------------
void _cmd_internal_comm_closed(int client_no) {
    /* ---- Enhanced Pseudocode ---- */
    post_internal_comm_close(client_no);

}

// --------------------------------------------------------------------
// @code  nc=50, nargs=2, insts=50
// --------------------------------------------------------------------
string xor_encode(string raw, int p) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    int length;
    mixed arr;

    length = strlen(raw);
    if ((length < 1)) {
        return raw;
    }
    arr = allocate_array(length);
    for (i = 0; i <= (length - 1); i++) {
        arr[i] = sprintf("\x%X", (raw[i] ^ p));
    }  // end loop (verified)
    return implode(arr, "");

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=3, insts=4
// --------------------------------------------------------------------
void msg_channel_oper(int client_no, string cmd, array args) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=15, nargs=2, insts=15
// --------------------------------------------------------------------
void msg_ccs_system_channel(int client_no, string msg) {
    /* ---- Enhanced Pseudocode ---- */
    if (is_object(find_object("/dba/cmds/intersrv/system_channel.c"))) {
        (find_object("/dba/cmds/intersrv/system_channel.c"))->main(client_no, msg);
    }

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=4, insts=4
// --------------------------------------------------------------------
void msg_l_ecard_business(int client_no, string action, string map_para, string cookie) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=12, nargs=3, insts=12
// --------------------------------------------------------------------
void msg_l_update_addiction(int client_no, string account, string result_text) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_update_addiction.c"))->main(client_no, account, result_text);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=14, nargs=5, insts=14
// --------------------------------------------------------------------
void msg_l_query_fill_info(int client_no, string account, string gid, int result, string type) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_query_fill_info.c"))->main(client_no, account, gid, result, type);

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
mixed find_agent_by_client(int client_no) {
    /* ---- Enhanced Pseudocode ---- */
    return client_agents[client_no];

}

// --------------------------------------------------------------------
// @code  nc=55, nargs=2, insts=55
// --------------------------------------------------------------------
private mixed xdbc_call_server(int cno, mapping para) {
    /* ---- Enhanced Pseudocode ---- */
    mapping client;

    if ((!is_mapping(client = xdbc_get_client(cno)))) {
        return -1;
    }
    client["error"] = "";
    xdbc_send_command(cno, para);
    if ((xdbc_wait_command(cno, &para) != 0)) {
        client["error"] = "XDBC: Failed to communicate";
        return -1;
    }
    if (is_string(para["error"])) {
        client["error"] = para["error"];
    }
    return para["result"];

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=15, nargs=6, insts=15
// --------------------------------------------------------------------
void msg_l_charge_result(int client_no, string account, int time_left, int count_left, int mode, int result) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_charge_result.c"))->main(client_no, account, time_left, count_left, mode, result);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=40, nargs=7, insts=40
// --------------------------------------------------------------------
void msg_cfg_my_info(int client_no, string dist, string name, int port, int id, int dist_id, mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/configd.c"->set_config("my_dist", dist);
    "/daemons/configd.c"->set_config("my_name", name);
    "/daemons/configd.c"->set_config("my_port", port);
    "/daemons/configd.c"->set_config("my_id", id);
    "/daemons/configd.c"->set_config("dist_id", dist_id);
    "/daemons/configd.c"->set_config("my_setting", m);

}

// --------------------------------------------------------------------
// @code  nc=19, nargs=0, insts=19
// --------------------------------------------------------------------
int pp_stop_profiling_ex() {
    /* ---- Enhanced Pseudocode ---- */
    if (get_timer("pp_profiling_stat")) {
        delete_timer("pp_profiling_stat");
    }
    if ((!pp_is_profiling())) {
        return 0;
    }
    pp_stop_profiling();
    return 1;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=15, nargs=0, insts=15
// --------------------------------------------------------------------
int get_server_shift_time() {
    /* ---- Enhanced Pseudocode ---- */
    if ((1 && (!defined("LAB")))) {
        return 0;
    }
    return server_shift_time;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=72, nargs=2, insts=72
// --------------------------------------------------------------------
mixed get_offset_ym(int t, int offset_months) {
    /* ---- Enhanced Pseudocode ---- */
    mixed year;
    mixed month;
    mixed day;
    int r_year;
    int r_month;
    mixed offset_years;

    if ((sscanf(ctime(t, 2), "%d-%d-%d", &year, &month, &day) < 3)) {
        return;
    }
    offset_months = (month + offset_months);
    offset_years = (offset_months / 12);
    r_month = (offset_months % 12);
    r_year = (year + offset_years);
    if ((r_month <= 0)) {
        r_month = (12 + r_month);
        --r_year;
    }
    if (((r_year < 0) || (r_year > 9999))) {
        return "";
    }
    return sprintf("%04d-%02d", r_year, r_month);

}

// --------------------------------------------------------------------
// @code  nc=11, nargs=2, insts=11
// --------------------------------------------------------------------
void xdbc_set_output_basic_file(string type, string basic_file) {
    /* ---- Enhanced Pseudocode ---- */
    _xdbc_output_basic_file = basic_file;
    xdbc_update_output_file(type);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=12, nargs=3, insts=12
// --------------------------------------------------------------------
void msg_l_set_phah_bonus_info(int client_no, string gid, string ret) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_set_phah_bonus_info.c"))->main(client_no, gid, ret);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=51, nargs=1, insts=51
// --------------------------------------------------------------------
mixed get_message_stat_info(mixed msg_no) {
    /* ---- Enhanced Pseudocode ---- */
    mixed stat_list;
    int i;
    mixed m;

    if (is_int(msg_no)) {
        return get_message_stat(msg_no);
    }
    m = ([ ]);
    stat_list = get_message_stat();
    for (i = 0; i <= (sizeof(stat_list) - 1); i++) {
        if ((stat_list[i] <= 0)) {
            continue;
        }
        m[sprintf("%0X", i)] = stat_list[i];
    }  // end loop (verified)
    return m;

}

// --------------------------------------------------------------------
// @code  nc=30, nargs=7, insts=30
// --------------------------------------------------------------------
mixed valid_name_ex(string name, int min_len, int max_len, int may_chinese, mixed include_group, mixed exclude_group, mixed f) {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;

    m = _valid_name(name, min_len, max_len, may_chinese, include_group, exclude_group);
    if ((!is_function(f))) {
        return m["msg"];
    }
    return (*f)(m);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=13, nargs=4, insts=13
// --------------------------------------------------------------------
void msg_l_query_coupon(int client_no, string gid, string para, string result) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_query_coupon.c"))->main(client_no, gid, para, result);

}

// --------------------------------------------------------------------
// @code  nc=20, nargs=1, insts=20
// --------------------------------------------------------------------
void xdbc_set_debug_level(int level) {
    /* ---- Enhanced Pseudocode ---- */
    if (((level > 3) || (level < 0))) {
        error("Bad level for debug\n");
    }
    _xdbc_debug_level = level;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=12, nargs=3, insts=12
// --------------------------------------------------------------------
void msg_l_ssb_update_account(int client_no, string account, string result) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_ssb_update_account.c"))->main(client_no, account, result);

}

// --------------------------------------------------------------------
// @code  nc=24, nargs=1, insts=24
// --------------------------------------------------------------------
void post_internal_comm_close(int client_no) {
    /* ---- Enhanced Pseudocode ---- */
    mixed f;

    loop (f in _internal_comm_close_funcs) {
        if ((!is_function(f))) {
            continue;
        }
        (*f)(client_no);
    }  // end loop (verified)
    clean_array(_internal_comm_close_funcs, 9);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=19, nargs=11, insts=19
// --------------------------------------------------------------------
void msg_l_convert_silver_coin(int client_no, string gid, string account, string aaa_cookie, string module, string para, int result, int convert_coin, int silver_coin, int gold_coin, int limit_trade_coin) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_convert_silver_coin.c"))->main(client_no, gid, account, result, convert_coin, silver_coin, gold_coin, limit_trade_coin, module, para);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=15, nargs=2, insts=15
// --------------------------------------------------------------------
void msg_ccs_world_channel(int client_no, string msg) {
    /* ---- Enhanced Pseudocode ---- */
    if (is_object(find_object("/dba/cmds/intersrv/world_channel.c"))) {
        (find_object("/dba/cmds/intersrv/world_channel.c"))->main(client_no, msg);
    }

}

// --------------------------------------------------------------------
// @code  nc=35, nargs=2, insts=35
// --------------------------------------------------------------------
array explode_string(string str, string chars) {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr;
    int i;

    for (i = 0; i <= (sizeof(chars) - 1); i++) {
        arr = explode(str, chars[i..i]);
        if ((sizeof(arr) > 1)) {
            break;
        }
    }  // end loop (verified)
    return arr;

}

// --------------------------------------------------------------------
// @code  nc=15, nargs=1, insts=15
// --------------------------------------------------------------------
mixed xdbc_get_insert_id(int cno) {
    /* ---- Enhanced Pseudocode ---- */
    xdbc_trace(0, "Call xdbc_get_insert_id(%d)\n", cno);
    return xdbc_call_server(cno, ([ "cmd": "get_insert_id" ]));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=3, insts=10
// --------------------------------------------------------------------
void msg_updated_gid(int client_no, int cookie, int result) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->msg_updated_gid(cookie, result);

}

// --------------------------------------------------------------------
// @brief  accessor: read property
// @code  nc=11, nargs=2, insts=11
// --------------------------------------------------------------------
mixed query_dynamic_key(object ob, string key_index) {
    /* ---- Enhanced Pseudocode ---- */
    return ob->query_temp(("dynamic_key/" + key_index));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=11, nargs=2, insts=11
// --------------------------------------------------------------------
void msg_l_get_ssb_combatd_conf(int client_no, string result) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_get_ssb_combatd_conf.c"))->main(client_no, result);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=16, nargs=7, insts=16
// --------------------------------------------------------------------
void msg_l_cost_coin(int client_no, string gid, string account, int result, string module, string para, string memo) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_cost_coin.c"))->main(client_no, gid, account, result, module, para, memo);

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=3, insts=4
// --------------------------------------------------------------------
void msg_samsung_oper(int client_no, string cmd, array args) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=205, nargs=4, insts=205
// --------------------------------------------------------------------
mixed get_max_cost_client(mixed field, int reset_stat, mixed cmds, array except_cmds) {
    /* ---- Enhanced Pseudocode ---- */
    mixed valid_fields;
    mixed command_fields;
    mixed m;
    mapping cinfo;
    int cmd;
    mixed arr;
    int for_command;
    mixed client_no;

    valid_fields = ({ "recv_bad_pkts", "recv_pkts", "sent_pkts", "recv_bytes", "sent_bytes", "recv_pkts_per_second", "sent_pkts_per_second", "recv_bytes_per_second", "sent_bytes_per_second" });
    command_fields = ({ "count", "ticks" });
    for_command = 0;
    if ((!field)) {
        printf("Usage: get_max_cost_client(field, reset_stat = 0, cmds = ({ }), except_cmds = ({ }))\n");
        printf("Valid fields: %O\n%O\nOr%O\n", field, valid_fields, command_fields);
        printf("See also: get_max_cost_cmd\n");
        return ({});
    }
    if (((member_array(field, valid_fields) == -1) && (member_array(field, command_fields) == -1))) {
        printf("Bad field %O to get_max_cost_client, expected:\n%O\nOr:\n%O\n", field, valid_fields, command_fields);
        return;
    }
    if ((member_array(field, command_fields) != -1)) {
        arr = allocate_array(2104, 0);
        if (((!is_array(cmds)) || (!sizeof(cmds)))) {
            cmds = keys(all_cmds_cost);
        }
        loop (cmd in (cmds - except_cmds)) {
            m = all_cmds_cost[cmd];
            loop (client_no in get_all_client_no()) {
                arr[(client_no % 10000)] += m[field][(client_no % 10000)];
            }  // end loop (verified)
        }  // end loop (verified)
        for_command = 1;
    }
    m = ([ ]);
    loop (client_no in get_all_client_no()) {
        cinfo = get_client_stat(client_no);
        if ((!cinfo["alive"])) {
            continue;
        }
        if (reset_stat) {
            reset_client_stat(client_no);
        }
        if (for_command) {
            cinfo[field] = arr[(client_no % 10000)];
        }
        if ((!sizeof(m))) {
            m = cinfo;
            continue;
        }
        if ((m[field] < cinfo[field])) {
            m = cinfo;
        }
    }  // end loop (verified)
    if (reset_stat) {
        all_cmds_cost = ([ ]);
    }
    return m;

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=2, insts=8
// --------------------------------------------------------------------
private int size_objects_get_usage(mapping m, string name) {
    /* ---- Enhanced Pseudocode ---- */
    return m[name];

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=2, insts=4
// --------------------------------------------------------------------
void cmd_l_pend_shop(int client_no, string account) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
mixed find_object_by_gid(string gid) {
    /* ---- Enhanced Pseudocode ---- */
    return gid_objects[gid];

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=3, insts=4
// --------------------------------------------------------------------
void msg_bottle_oper(int client_no, string cmd, array args) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=16, nargs=7, insts=16
// --------------------------------------------------------------------
void msg_l_do_inquiry(int client_no, string gid, string order_id, string pay_type, int result, int result_code, int gold) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_do_inquiry.c"))->main(client_no, gid, order_id, pay_type, result, result_code, gold);

}

// --------------------------------------------------------------------
// @code  nc=12, nargs=0, insts=12
// --------------------------------------------------------------------
void reboot() {
    /* ---- Enhanced Pseudocode ---- */
    create_thread(([ "name": "tDbaReboot", "stack_size": 512 ]), "daemon");

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=30, nargs=1, insts=30
// --------------------------------------------------------------------
void post_child_leave_world(object child) {
    /* ---- Enhanced Pseudocode ---- */
    mixed f;

    if ((!child->is_child())) {
        return;
    }
    loop (f in _child_leave_world_funcs) {
        if ((!is_function(f))) {
            continue;
        }
        (*f)(child);
    }  // end loop (verified)
    clean_array(_child_leave_world_funcs, 9);

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=1, insts=9
// --------------------------------------------------------------------
void register_char_reconnected(function f) {
    /* ---- Enhanced Pseudocode ---- */
    _char_reconnected_funcs += ({ f });

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=1, insts=9
// --------------------------------------------------------------------
void register_post_disconnected(function f) {
    /* ---- Enhanced Pseudocode ---- */
    _disconnected_funcs += ({ f });

}

// --------------------------------------------------------------------
// @brief  data writer: persist information
// @code  nc=29, nargs=5, insts=29
// --------------------------------------------------------------------
mixed write_content_to_ex(string path, string name, mixed branch, mixed content, mixed to) {
    /* ---- Enhanced Pseudocode ---- */
    mixed str;
    mixed convert_type;

    convert_type = get_data_convert_type(path, name);
    str = "/daemons/datasaved.c"->convert_to_save_data(convert_type, content);
    "/daemons/filed.c"->write_content_to(path, name, branch, str, (: write_to_ex, path, to :));
    return;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=11, nargs=2, insts=11
// --------------------------------------------------------------------
void msg_l_get_ssb_combatd_res(int client_no, string result) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_get_ssb_combatd_res.c"))->main(client_no, result);

}

// --------------------------------------------------------------------
// @brief  verification: confirm/guarantee state
// @code  nc=57, nargs=1, insts=57
// --------------------------------------------------------------------
int assure_file(string file) {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr;
    int i;

    arr = explode(file, "/");
    file = arr[0];
    i = 1;

    // --- label L0018 ---
    while ((i < sizeof(arr))) {
        switch (file_size(file)) {
            case -2:
            break;
            case -1:
            mkdir(file);
            break;
            default:
            return 0;
        }  // end switch

        // --- label L0038 ---
        file += ("/" + arr[i]);
        ++i;
    }  // end loop (verified)
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=5, insts=4
// --------------------------------------------------------------------
void msg_intersvr_channel_msg(int client_no, int channel, string sender, buffer expand_buffer, string msg) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=11, nargs=2, insts=11
// --------------------------------------------------------------------
void msg_l_get_ssb_finish_flag(int client_no, string result) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_get_ssb_finish_flag.c"))->main(client_no, result);

}

// --------------------------------------------------------------------
// @code  nc=24, nargs=1, insts=24
// --------------------------------------------------------------------
void post_switch_server(object me) {
    /* ---- Enhanced Pseudocode ---- */
    mixed f;

    loop (f in _switch_server_funcs) {
        if ((!is_function(f))) {
            continue;
        }
        (*f)(me);
    }  // end loop (verified)
    clean_array(_switch_server_funcs, 9);

}

// --------------------------------------------------------------------
// @code  nc=17, nargs=1, insts=17
// --------------------------------------------------------------------
mixed xdbc_get_error(int cno) {
    /* ---- Enhanced Pseudocode ---- */
    mapping client;

    if ((!is_mapping(client = xdbc_get_client(cno)))) {
        return "Bad connection.\n";
    }
    return client["error"];

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=15, nargs=2, insts=15
// --------------------------------------------------------------------
void msg_gs_finger(int client_no, string user_name) {
    /* ---- Enhanced Pseudocode ---- */
    if (is_object(find_object("/dba/cmds/intersrv/finger.c"))) {
        (find_object("/dba/cmds/intersrv/finger.c"))->main(client_no, user_name);
    }

}

// --------------------------------------------------------------------
// @code  nc=25, nargs=1, insts=25
// --------------------------------------------------------------------
varargs void xdbc_trace(int level) {
    /* ---- Enhanced Pseudocode ---- */
    if ((level < _xdbc_debug_level)) {
        return;
    }
    if (is_string(_xdbc_output_file["trace"])) {
        xdbc_append_output("trace", (*find_function_arg("sprintf", ($<)[1..<1]))());
    }

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=49, nargs=1, insts=49
// --------------------------------------------------------------------
int is_valid_ip(string str) {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr;
    int digit;
    mixed ip_part;

    arr = explode(str, ".");
    if ((sizeof(arr) != 4)) {
        return 0;
    }
    loop (ip_part in arr) {
        digit = atoi(ip_part);
        if (((string)digit != ip_part)) {
            return 0;
        }
        if (((digit > 255) || (digit < 0))) {
            return 0;
        }
    }  // end loop (verified)
    return 1;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=75, nargs=2, insts=75
// --------------------------------------------------------------------
void msg_cfg_cta_list(int client_no, array list) {
    /* ---- Enhanced Pseudocode ---- */
    mixed cta_list;
    int i;

    cta_list = allocate_array((sizeof(list) / 6));
    i = 0;

    // --- label L0014 ---
    while ((i < sizeof(list))) {
        cta_list[(i / 6)] = ([ "name": list[i], "ip": list[(i + 1)], "port": list[(i + 2)], "access_adb_user": list[(i + 3)], "access_adb_password": list[(i + 4)], "privilege": list[(i + 5)] ]);
        i += 6;
    }  // end loop (verified)
    "/daemons/configd.c"->set_config("cta_list", cta_list);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=3, insts=10
// --------------------------------------------------------------------
void msg_update_log(int client_no, int cookie, int result) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->msg_update_log(cookie, result);

}

// --------------------------------------------------------------------
// @code  nc=84, nargs=3, insts=84
// --------------------------------------------------------------------
array sql_select(mixed db, string sql_cmd, int max_row) {
    /* ---- Enhanced Pseudocode ---- */
    mixed query_index;
    int i;
    int size;
    mixed arr;
    mixed row;
    mixed m;

    query_index = sql_query(db, sql_cmd);
    if ((query_index == -1)) {
        return ({});
    }
    size = 0;
    m = ([ ]);

    // --- label L0024 ---
    while ((is_mapping(row = sql_step(query_index)) && max_row--)) {
        m[size++] = row;
    }  // end loop (verified)
    sql_finalize(query_index);
    if ((size < 1)) {
        return ({});
    }
    arr = allocate_array(size);
    for (i = 0; i <= (size - 1); i++) {
        arr[i] = m[i];
    }  // end loop (verified)
    return arr;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=3, insts=10
// --------------------------------------------------------------------
void msg_written_content(int client_no, int cookie, int result) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->msg_written_content(cookie, result);

}

// --------------------------------------------------------------------
// @code  nc=125, nargs=4, insts=125
// --------------------------------------------------------------------
int xdbc_connect(string host, string user, string passwd, string db) {
    /* ---- Enhanced Pseudocode ---- */
    mapping client;
    mixed result;
    mixed fd;
    mixed cno;
    mixed key;

    fd = socket(2);
    if ((connect(fd, host, 9527) != 0)) {
        xdbc_log(2, "Failed to connect to %s:%d\n", host, 9527);
        close_socket(fd);
        return -1;
    }
    if (((cno = xdbc_allocate_client(fd)) == -1)) {
        xdbc_log(2, "No more free client to allocate.\n");
        close_socket(fd);
        return -1;
    }
    key = random(2000000000);
    result = xdbc_call_server(cno, ([ "cmd": "set_key", "key": key ]));
    if (((result == -1) || (!is_mapping(client = xdbc_get_client(cno))))) {
        xdbc_log(2, "Failed to prepare connection %d(result = %d).\n", cno, result);
        xdbc_free_client(cno);
        return -1;
    }
    xdbc_trace(1, "Set key: %016X\n", key);
    client["key"] = key;
    result = xdbc_call_server(cno, ([ "cmd": "connect", "user": user, "passwd": passwd, "db": db ]));
    if ((result == -1)) {
        xdbc_log(2, "Failed to connect to database.\n");
        xdbc_free_client(cno);
        return -1;
    }
    return cno;

}

// --------------------------------------------------------------------
// @code  nc=175, nargs=0, insts=175
// --------------------------------------------------------------------
void process_online_update_files() {
    /* ---- Enhanced Pseudocode ---- */
    mixed files;
    int index;
    mixed arr;
    int ini_file;
    int update_file;
    mixed version;
    mixed ini_ver_list;
    mixed update_ver_list;

    ini_ver_list = ([ ]);
    update_ver_list = ([ ]);
    if (((!is_array(arr = get_dir("online_update//"))) || (sizeof(arr) < 1))) {
        printf("Not exists online_update file.\n");
        return;
    }
    loop (ini_file in arr) {
        ini_file = ("/online_update/" + ini_file);
        version = trim(read_ini("config", "version", ini_file));
        files = atoi(read_ini("config", "files", ini_file));
        ini_ver_list[ini_file] = version;
        for (index = 1; index <= files; index++) {
            update_file = trim(read_ini("files", ("file" + index), ini_file));
            if (is_string(update_ver_list[update_file])) {
                if ((stricmp("1.60.0912", version) == 0)) {
                    update_ver_list[update_file] = version;
                }
                continue;
            }
            if (is_mapping(file_stat(update_file))) {
                update_ver_list[update_file] = version;
            }
        }  // end loop (verified)
    }  // end loop (verified)
    loop (update_file in keys(update_ver_list)) {
        if ((stricmp("1.60.0912", update_ver_list[update_file]) == 0)) {
            continue;
        }
        if (delete_file(update_file)) {
            printf("Succeeded to delete overdue online_update file(%s).\n", update_file);
            continue;
        }
        error("Fail to delete overdue online_update file(%s).\n", update_file);
    }  // end loop (verified)
    loop (ini_file in keys(ini_ver_list)) {
        if ((stricmp("1.60.0912", ini_ver_list[ini_file]) == 0)) {
            continue;
        }
        if (delete_file(ini_file)) {
            printf("Succeed to delete overdue online_update config(%s).\n", ini_file);
            continue;
        }
        error("Fail to delete overdue online_update config(%s).\n", ini_file);
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=14, nargs=5, insts=14
// --------------------------------------------------------------------
void msg_l_manage_vip_sms(int client_no, string gid, int oper, int ret, string para) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_manage_vip_sms.c"))->main(client_no, gid, oper, ret, para);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=3, insts=10
// --------------------------------------------------------------------
void msg_create_pet_deposit(int client_no, int cookie, int result) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->msg_create_pet_deposit(cookie, result);

}

// --------------------------------------------------------------------
// @code  nc=37, nargs=1, insts=37
// --------------------------------------------------------------------
nomask void telnet_client_closed(int fd) {
    /* ---- Enhanced Pseudocode ---- */
    mapping client_info;

    if ((!is_mapping(client_info = telnet_clients[fd]))) {
        return;
    }
    map_delete(telnet_clients, fd);
    log_to_das(sprintf("main/Account %s logout from %s.\n", client_info["account"], client_info["location"]));
    log_to_das(sprintf("%s/Logout from %s.\n", client_info["account"], client_info["location"]));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=12, nargs=3, insts=12
// --------------------------------------------------------------------
void msg_l_do_alipay_recharge(int client_no, string gid, int ret) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_do_alipay_recharge.c"))->main(client_no, gid, ret);

}

// --------------------------------------------------------------------
// @code  nc=12, nargs=1, insts=12
// --------------------------------------------------------------------
void xdbc_disconnect(int cno) {
    /* ---- Enhanced Pseudocode ---- */
    xdbc_trace(0, "Call xdbc_disconnect(%d)\n", cno);
    xdbc_free_client(cno);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=30, nargs=1, insts=30
// --------------------------------------------------------------------
string get_login_info_file(mixed mks) {
    /* ---- Enhanced Pseudocode ---- */
    mixed account;

    account = tolower((is_object(mks) ? mks->query_temp("login_info/account") : mks));
    return (((("/dba/log/account/" + account[0..0]) + "/") + account) + ".log");

}

// --------------------------------------------------------------------
// @code  nc=25, nargs=2, insts=25
// --------------------------------------------------------------------
void unregister_daemon_data(string daemon_name, mixed index) {
    /* ---- Enhanced Pseudocode ---- */
    mixed m;

    if ((!is_mapping(m = daemon_data[daemon_name]))) {
        return;
    }
    map_delete(m, index);
    if ((!sizeof(m))) {
        map_delete(daemon_data, daemon_name);
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=16, nargs=4, insts=16
// --------------------------------------------------------------------
void msg_cfg_ah_server_info(int client_no, string ip, int port, string name) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/configd.c"->set_config("ah_server_info", ([ "ip": ip, "port": port, "name": name ]));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=14, nargs=5, insts=14
// --------------------------------------------------------------------
void msg_l_oper_protect_range(int client_no, string account, string oper, int range, int result) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_oper_protect_range.c"))->main(client_no, account, oper, range, result);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=94, nargs=1, insts=94
// --------------------------------------------------------------------
mixed get_sub_dirs_ex(string path) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ks;
    mixed ks_pak;
    mixed file_info;
    mixed m;

    m = ([ ]);
    ks_pak = get_dir(((lib_pak_path + path) + "*"), 1);
    if (is_array(ks_pak)) {
        loop (file_info in ks_pak) {
            if ((strsrch(file_info[0], ".unload") >= 0)) {
                continue;
            }
            file_info[0] = replace_string(file_info[0], ".o", ".c");
            m[file_info[0]] = file_info;
        }  // end loop (verified)
    }
    if (is_array(ks = get_dir((path + "*"), 1))) {
        loop (file_info in ks) {
            if ((strsrch(file_info[0], ".unload") >= 0)) {
                continue;
            }
            m[file_info[0]] = file_info;
        }  // end loop (verified)
    }
    return values(m);

}

// --------------------------------------------------------------------
// @code  nc=49, nargs=1, insts=49
// --------------------------------------------------------------------
int convert_string_to_bit_macro(string str_bits) {
    /* ---- Enhanced Pseudocode ---- */
    mixed bit_value;
    int val;
    mixed bit_arr;
    mixed str;

    bit_arr = explode(str_bits, "|");
    if ((!sizeof(bit_arr))) {
        return 0;
    }
    val = 0;
    loop (str in bit_arr) {
        if ((!is_string(str = expand_macro(trim(str))))) {
            error("convert_string_to_bit_macro error.");
        }
        bit_value = atoi(str);
        val |= bit_value;
    }  // end loop (verified)
    return val;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=11, nargs=4, insts=11
// --------------------------------------------------------------------
void msg_read_pet_deposit(int client_no, int cookie, int result, string content) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->msg_read_pet_deposit(cookie, result, content);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=13, nargs=4, insts=13
// --------------------------------------------------------------------
void msg_check_role_on_sale(int client_no, string account, int time_stamp, string para) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_check_role_on_sale.c"))->main(client_no, account, time_stamp, para);

}

// --------------------------------------------------------------------
// @code  nc=18, nargs=2, insts=18
// --------------------------------------------------------------------
int xdbc_execute(int cno, string sql_cmd) {
    /* ---- Enhanced Pseudocode ---- */
    xdbc_trace(0, "Call xdbc_execute(%d, %s)\n", cno, sql_cmd);
    return xdbc_call_server(cno, ([ "cmd": "execute", "sql_cmd": sql_cmd ]));

}

// --------------------------------------------------------------------
// @code  nc=18, nargs=1, insts=18
// --------------------------------------------------------------------
void reset_log_file_name(string server_name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed file_name;

    file_name = sprintf("%slog_%s_%s.txt", "/dba/log/", server_name, ctime(time(), 2));
    set_log_file(file_name, 0);

}

// --------------------------------------------------------------------
// @code  nc=24, nargs=3, insts=24
// --------------------------------------------------------------------
int send_to_queue_debug(mixed oid, mixed data, int timeout) {
    /* ---- Enhanced Pseudocode ---- */
    n_msg[oid] = ((int)n_msg[oid] + 1);
    return send_to_queue(oid, data, timeout);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=13, nargs=5, insts=13
// --------------------------------------------------------------------
void msg_intersvr_rpc_ret(int client_no, int cookie, string owner, string stat, string result) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/rpcd.c"->recv_rpc_ret(client_no, cookie, owner, stat, result);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
mapping get_all_client_agents() {
    /* ---- Enhanced Pseudocode ---- */
    return client_agents;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=16, nargs=7, insts=16
// --------------------------------------------------------------------
void msg_l_do_recharge(int client_no, string gid, string order_id, string pay_type, int result, int result_code, int gold) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_do_recharge.c"))->main(client_no, gid, order_id, pay_type, result, result_code, gold);

}

// --------------------------------------------------------------------
// @code  nc=300, nargs=4, insts=300
// --------------------------------------------------------------------
string time_length_to_ch(int t, int accuracy, int rounding, int color) {
    /* ---- Enhanced Pseudocode ---- */
    mixed days;
    mixed hours;
    mixed mins;
    mixed secs;
    string day_desc;
    string hour_desc;
    string min_desc;
    string secs_desc;
    string color_desc;

    if ((1 == color)) {
        color_desc = "#R%d#n";
    } else {
        color_desc = "%d";
    }
    days = (t / 86400);
    t = (t % 86400);
    if (days) {
        day_desc = sprintf("%sÃÏ", sprintf(color_desc, days));
    } else {
        day_desc = "";
    }
    hours = (t / 3600);
    t = (t % 3600);
    if (hours) {
        hour_desc = sprintf("%s–° ±", sprintf(color_desc, hours));
    } else {
        hour_desc = "";
    }
    mins = (t / 60);
    t = (t % 60);
    if (mins) {
        min_desc = sprintf("%s∑÷÷”", sprintf(color_desc, mins));
    } else {
        min_desc = "";
    }
    secs = t;
    if (secs) {
        secs_desc = sprintf("%s√Î", sprintf(color_desc, secs));
    } else {
        secs_desc = "";
    }
    switch (accuracy) {
        case 1:
        if ((rounding && (hour_desc || mins || secs))) {
            ++days;
        }
        if ((!days)) {
            return "≤ª◊„“ªÃÏ";
        }
        return sprintf("%sÃÏ", sprintf(color_desc, days));
        case 2:
        if ((rounding && (mins || secs))) {
            ++hours;
        }
        if (((!days) && (!hours))) {
            return "≤ª◊„“ª–° ±";
        }
        if ((hours / 24)) {
            return sprintf("%sÃÏ", sprintf(color_desc, (days + 1)));
        }
        if ((!hours)) {
            return day_desc;
        }
        return (day_desc + sprintf("%s–° ±", sprintf(color_desc, hours)));
        case 3:
        if (rounding && secs) {
            ++mins;
        }
        if ((((!days) && (!hours)) && (!mins))) {
            return "≤ª◊„“ª∑÷÷”";
        }
        if ((mins / 60)) {
            if (((hours + 1) / 24)) {
                return sprintf("%sÃÏ", sprintf(color_desc, (days + 1)));
            }
            return (day_desc + sprintf("%s–° ±", sprintf(color_desc, (hours + 1))));
        }
        if ((!mins)) {
            return (day_desc + hour_desc);
        }
        return ((day_desc + hour_desc) + sprintf("%s∑÷÷”", sprintf(color_desc, mins)));
        case 4:
        default:
        if (((((!days) && (!hours)) && (!mins)) && (!secs))) {
            return "≤ª◊„“ª√Î÷”";
        }
        return (((day_desc + hour_desc) + min_desc) + secs_desc);
    }  // end switch

}

// --------------------------------------------------------------------
// @code  nc=20, nargs=1, insts=20
// --------------------------------------------------------------------
mixed xdbc_get_client(int client_no) {
    /* ---- Enhanced Pseudocode ---- */
    if (((client_no < 0) || (client_no >= sizeof(_xdbc_clients)))) {
        return 0;
    }
    return _xdbc_clients[client_no];

}

// --------------------------------------------------------------------
// @code  nc=30, nargs=1, insts=30
// --------------------------------------------------------------------
void post_enter_world(object me) {
    /* ---- Enhanced Pseudocode ---- */
    mixed f;

    loop (f in _enter_world_funcs) {
        if ((!is_function(f))) {
            continue;
        }
        try {
            (*f)(me);
        } catch { /* error handled */ }

        // --- label L0021 ---
    }  // end loop (verified)
    clean_array(_enter_world_funcs, 9);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=42, nargs=0, insts=42
// --------------------------------------------------------------------
void release_all_data() {
    /* ---- Enhanced Pseudocode ---- */
    mixed obs;
    int i;

    set_nohalt();
    obs = objects();
    i = 0;

    // --- label L0012 ---
    while ((i < sizeof(obs))) {
        if (is_object(obs[i])) {
            try {
                (obs[i])->release_data();
            } catch { /* error handled */ }

            // --- label L0033 ---
        }
        ++i;
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=15, nargs=6, insts=15
// --------------------------------------------------------------------
void msg_l_send_vip_sms(int client_no, string gid, string auth_type, string sms_type, string sms_code, int ret) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_send_vip_sms.c"))->main(client_no, gid, auth_type, sms_type, sms_code, ret);

}

// --------------------------------------------------------------------
// @code  nc=46, nargs=0, insts=46
// --------------------------------------------------------------------
void init_compile_module() {
    /* ---- Enhanced Pseudocode ---- */
    if (is_compiling()) {
        load_or_compile_all_path = (: compile_all_path :);
        COMPILE_ACTION = "Compile";
        load_or_compile = (: safe_compile_program :);
        load_or_compile_all = (: compile_all :);
    } else {
        load_or_compile_all_path = (: load_all_path :);
        COMPILE_ACTION = "Load";
        load_or_compile = (: safe_load_object :);
        load_or_compile_all = (: load_all :);
    }
    load_file_dependence();
    process_online_update_files();
    printf("Done of init_compile_module.\n");

}

// --------------------------------------------------------------------
// @code  nc=61, nargs=2, insts=61
// --------------------------------------------------------------------
void load_all(string path, mapping para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed files;
    int i;

    relinquish();
    if ((path[(strlen(path) - 1)] != 47)) {
        path += "/";
    }
    files = get_dir_ex(path);
    if ((!files)) {
        return ([ ]);
    }
    i = 0;

    // --- label L0032 ---
    while ((i < sizeof(files))) {
        if ((!(i % 5))) {
            relinquish();
        }
        safe_load_object((path + files[i]), para);
        ++i;
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @code  nc=127, nargs=0, insts=127
// --------------------------------------------------------------------
int load_ban_words() {
    /* ---- Enhanced Pseudocode ---- */
    mixed match_id;
    int index;
    mixed match_ids;
    mixed file;
    int group;

    loop (match_ids in values(ban_words_match_id)) {
        if ((!is_array(match_ids))) {
            continue;
        }
        loop (match_id in match_ids) {
            detach_illegal_list(match_id);
        }  // end loop (verified)
    }  // end loop (verified)
    ban_words_match_id = ([ ]);
    if ((!is_string(file = read_etc_file("ban_words.list")))) {
        return 0;
    }
    ban_words = analysis_ban_words_file(file);
    loop (group in keys(ban_words)) {
        if ((!is_array(ban_words_match_id[group]))) {
            ban_words_match_id[group] = ({});
        }
        for (index = 0; index <= ((int)ceil(((1 * sizeof(ban_words[group])) / 200)) - 1); index++) {
            match_id = attach_illegal_list(ban_words[group][(200 * index)..((200 * (index + 1)) - 1)]);
            if ((!match_id)) {
                catch(error("Failed to attach illegal list\n"));

                // --- label L0109 ---
                continue;
            }
            ban_words_match_id[group] |= ({ match_id });
        }  // end loop (verified)
    }  // end loop (verified)
    return 1;

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=111, nargs=2, insts=111
// --------------------------------------------------------------------
int is_same_data(mixed source, mixed dest) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    mixed source_keys;
    mixed dest_keys;
    int index;

    if (is_mapping(source)) {
        if ((!is_mapping(dest))) {
            return 0;
        }
        source_keys = keys(source);
        dest_keys = keys(dest);
        if ((sizeof(source_keys) != sizeof(dest_keys))) {
            return 0;
        }
        loop (index in source_keys) {
            if ((!is_same_data(source[index], dest[index]))) {
                return 0;
            }
        }  // end loop (verified)
        return 1;
    }
    if (is_array(source)) {
        if ((!is_array(dest))) {
            return 0;
        }
        if ((sizeof(source) != sizeof(dest))) {
            return 0;
        }
        for (i = 0; i <= (sizeof(source) - 1); i++) {
            if ((!is_same_data(source[i], dest[i]))) {
                return 0;
            }
        }  // end loop (verified)
        return 1;
    }
    if ((var_type(source) != var_type(dest))) {
        return 0;
    }
    if ((source != dest)) {
        return 0;
    }
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=5, insts=4
// --------------------------------------------------------------------
void msg_internal_auth(int client_no, int reply_server_type, string reply_server_name, int result, int cookie) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=11, nargs=4, insts=11
// --------------------------------------------------------------------
void msg_xg_get_name(int client_no, int cookie, int result, string name) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->msg_xg_get_name(cookie, result, name);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=32, nargs=4, insts=32
// --------------------------------------------------------------------
mixed read_content_ex(string path, string name, mixed branch, mixed from_table) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ret;
    mixed convert_type;

    ret = "/daemons/filed.c"->read_content(path, name, branch, from_table);
    if ((!is_string(ret))) {
        return ret;
    }
    convert_type = get_data_convert_type(path, name);
    return "/daemons/datasaved.c"->convert_to_memory_data(convert_type, ret);

}

// --------------------------------------------------------------------
// @code  nc=57, nargs=1, insts=57
// --------------------------------------------------------------------
string tolower_temp(string content) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    mixed len;

    if ((!tolower_temp_flag)) {
        return tolower(content);
    }
    len = strlen(content);
    i = 0;

    // --- label L0017 ---
    while (i < len) {
        if (ischinese(content[i])) {
            ++i;
        } else if (isupper(content[i])) {
            put_sub_data(&content, i, tolower(content[i]));
        }

        // --- label L0047 ---
        ++i;
    }  // end loop (verified)
    return content;

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=1, insts=9
// --------------------------------------------------------------------
void register_post_auto_save(function f) {
    /* ---- Enhanced Pseudocode ---- */
    _auto_save_funcs += ({ f });

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=1, insts=9
// --------------------------------------------------------------------
void register_enter_world(function f) {
    /* ---- Enhanced Pseudocode ---- */
    _enter_world_funcs += ({ f });

}

// --------------------------------------------------------------------
// @code  nc=18, nargs=1, insts=18
// --------------------------------------------------------------------
int xdbc_fetch_record(int vno) {
    /* ---- Enhanced Pseudocode ---- */
    xdbc_trace(0, "Call xdbc_fetch_record(%d)\n", vno);
    return xdbc_call_server(xdbc_get_cno_of_view(vno), ([ "cmd": "fetch_record", "vno": vno ]));

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=3, insts=4
// --------------------------------------------------------------------
void msg_partywar_oper(int client_no, string cmd, array args) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=3, insts=10
// --------------------------------------------------------------------
void msg_cfg_write_etc(int client_no, int cookie, int result) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/configd.c"->msg_cfg_write_etc(cookie, result);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=14, nargs=5, insts=14
// --------------------------------------------------------------------
void msg_l_lock_card(int client_no, string cookie, string number, int result, int count) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_lock_card.c"))->main(client_no, cookie, number, result, count);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=12, nargs=3, insts=12
// --------------------------------------------------------------------
void msg_l_ssb_copy_account(int client_no, string user_name, string result) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_ssb_copy_account.c"))->main(client_no, user_name, result);

}

// --------------------------------------------------------------------
// @code  nc=7, nargs=1, insts=7
// --------------------------------------------------------------------
void _cmd_new_connection(int client_no) {
    /* ---- Enhanced Pseudocode ---- */
    post_new_connection(client_no);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=15, nargs=6, insts=15
// --------------------------------------------------------------------
void msg_l_query_random_coupon(int client_no, string account, string gid, int result, string para, int num) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_query_random_coupon.c"))->main(client_no, account, gid, result, para, num);

}

// --------------------------------------------------------------------
// @code  nc=13, nargs=4, insts=13
// --------------------------------------------------------------------
mixed mysql_select_column(int cno, string sql_cmd, int column, int max_row) {
    /* ---- Enhanced Pseudocode ---- */
    return mixed_mysql_select(cno, sql_cmd, (: mysql_select_column_fetch, column :), max_row);

}

// --------------------------------------------------------------------
// @code  nc=91, nargs=6, insts=91
// --------------------------------------------------------------------
mixed valid_content(string content, int min_len, int max_len, int min_line, int max_line, mixed group) {
    /* ---- Enhanced Pseudocode ---- */
    mixed i;
    mixed j;
    mixed arr_group;
    mixed g;

    if (((i = strlen(content)) < min_len)) {
        return (("ƒ⁄»›≤ªƒ‹…Ÿ”⁄" + min_len) + "∏ˆ◊÷∑˚");
    }
    if ((i > max_len)) {
        return (("ƒ⁄»›≤ªƒ‹≥¨π˝" + max_len) + "∏ˆ◊÷∑˚");
    }
    if (((j = sizeof(explode(content, "\n"))) < min_line)) {
        return (("ƒ⁄»›≤ªƒ‹…Ÿ”⁄" + min_line) + "––");
    }
    if ((j > max_line)) {
        return (("ƒ⁄»›≤ªƒ‹≥¨π˝" + max_line) + "––");
    }
    if (is_string(group)) {
        arr_group = ({ group });
    } else {
        arr_group = keys(ban_words);
    }
    content = tolower_temp(content);
    loop (g in arr_group) {
        if (valid_ban_words_by_group(content, g)) {
            return "ƒ„ ‰»Îµƒƒ⁄»›÷–∞¸∫¨√Ù∏–◊÷∑˚£¨«Î÷ÿ–¬…Ë∂®ƒ⁄»›";
        }
    }  // end loop (verified)
    return 0;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=3, insts=10
// --------------------------------------------------------------------
void msg_cfg_read_etc(int client_no, int cookie, int result) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/configd.c"->msg_cfg_read_etc(cookie, result);

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=1, insts=9
// --------------------------------------------------------------------
void register_log_runtime_info(function f) {
    /* ---- Enhanced Pseudocode ---- */
    _log_runtime_info_funcs += ({ f });

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=68, nargs=1, insts=68
// --------------------------------------------------------------------
int get_next_month_zero_time(int t) {
    /* ---- Enhanced Pseudocode ---- */
    mapping lt;

    lt = local_time(t);
    if ((lt["mon"] == 12)) {
        ++lt["year"];
        lt["mon"] = 1;
    } else {
        ++lt["mon"];
    }
    lt["mday"] = 1;
    lt["hour"] = 0;
    lt["min"] = 0;
    lt["sec"] = 0;
    return mktime(lt);

}

// --------------------------------------------------------------------
// @code  nc=35, nargs=1, insts=35
// --------------------------------------------------------------------
string time_desc_ex(mixed t) {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;

    m = (is_int(t) ? local_time(t) : local_time());
    return sprintf("%4d-%02d-%02d %02d:%02d:%02d", m["year"], m["mon"], m["mday"], m["hour"], m["min"], m["sec"]);

}

// --------------------------------------------------------------------
// @code  nc=7, nargs=0, insts=7
// --------------------------------------------------------------------
void init_agent_cmds_module() {
    /* ---- Enhanced Pseudocode ---- */
    printf("Done of init_agent_cmds_module.\n");

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=53, nargs=1, insts=53
// --------------------------------------------------------------------
int get_monday_zero_time(int t) {
    /* ---- Enhanced Pseudocode ---- */
    mapping lt;

    lt = local_time(t);
    if ((lt["wday"] > 0)) {
        lt["mday"] -= (lt["wday"] - 1);
    } else {
        lt["mday"] -= 6;
    }
    lt["hour"] = 0;
    lt["min"] = 0;
    lt["sec"] = 0;
    return mktime(lt);

}

// --------------------------------------------------------------------
// @code  nc=177, nargs=3, insts=177
// --------------------------------------------------------------------
mixed random_sub_select(mixed src, int n, int order) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    mixed m;
    mixed arr;
    mixed poses;
    int key;
    mixed result;
    mixed type;

    type = var_type(src);
    if ((member_array(type, ({ "buffer", "string", "array", "mapping" })) < 0)) {
        error("Bad type of argument 1, expected buffer|string|array|mapping, got %s.", type);
    }
    m = sizeof(src);
    poses = random_from_m_to_n(m, n);
    switch (order) {
        case 1:
        poses = sort(poses, 1);
        break;
        case -1:
        poses = sort(poses, -1);
        break;
        default:
        break;
    }  // end switch

    // --- label L0057 ---
    switch (type) {
        case "buffer":
        case "string":
        result = allocate_buffer(n);
        for (i = 0; i <= (n - 1); i++) {
            put_sub_data(&result, i, src[poses[i]]);
        }  // end loop (verified)
        return (("buffer" == type) ? result : (string)result);
        case "array":
        result = allocate_array(n);
        for (i = 0; i <= (n - 1); i++) {
            result[i] = src[poses[i]];
        }  // end loop (verified)
        return result;
        case "mapping":
        result = allocate_mapping(n);
        arr = keys(src);
        for (i = 0; i <= (n - 1); i++) {
            key = arr[poses[i]];
            result[key] = src[key];
        }  // end loop (verified)
        return result;
        default:
        return -1;
    }  // end switch

}

// --------------------------------------------------------------------
// @code  nc=65, nargs=1, insts=65
// --------------------------------------------------------------------
mixed read_whole_ini_ex(string ini_file) {
    /* ---- Enhanced Pseudocode ---- */
    mixed data;
    mixed section_data;
    int section;
    int key;
    mixed value;

    if ((!is_mapping(data = read_whole_ini(ini_file)))) {
        return data;
    }
    loop (section in keys(data)) {
        if ((!is_mapping(section_data = data[section]))) {
            continue;
        }
        loop (key in keys(section_data)) {
            value = section_data[key];
            if ((is_array(value = explode(value, "|")) && (sizeof(value) > 1))) {
                section_data[key] = value;
            }
        }  // end loop (verified)
    }  // end loop (verified)
    return data;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=12, nargs=3, insts=12
// --------------------------------------------------------------------
void msg_l_set_local_encrypt_info(int client_no, string account, int result) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_set_local_encrypt_info.c"))->main(client_no, account, result);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=11, nargs=4, insts=11
// --------------------------------------------------------------------
void msg_read_content(int client_no, int cookie, int result, string content) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->msg_read_content(cookie, result, content);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=18, nargs=2, insts=18
// --------------------------------------------------------------------
int get_comm_cmd_attrib(string cmd_name, string key) {
    /* ---- Enhanced Pseudocode ---- */
    mixed m;

    if (is_mapping(m = _cmd_attrib_map[cmd_name])) {
        return (int)m[key];
    }
    return 0;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=16, nargs=4, insts=16
// --------------------------------------------------------------------
void msg_cfg_csa_info(int client_no, string ip, int port, string name) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/configd.c"->set_config("csa_info", ([ "ip": ip, "port": port, "name": name ]));

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=2, insts=4
// --------------------------------------------------------------------
void cmd_get_server_time(int client_no, string gs_name) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=11, nargs=4, insts=11
// --------------------------------------------------------------------
void msg_xg_get_account(int client_no, int cookie, int result, string account) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->msg_xg_get_account(cookie, result, account);

}

// --------------------------------------------------------------------
// @code  nc=15, nargs=3, insts=15
// --------------------------------------------------------------------
mixed valid_char_name(string char_name, mixed include_group, mixed exclude_group) {
    /* ---- Enhanced Pseudocode ---- */
    return valid_name(char_name, 3, 12, 1, include_group, exclude_group);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=17, nargs=4, insts=17
// --------------------------------------------------------------------
void cmd_l_agent_connect_server(int client_no, string account, int auth_key, string paras) {
    /* ---- Enhanced Pseudocode ---- */
    if (is_object(find_object("/dba/cmds/intersrv/l_agent_connect_server.c"))) {
        (find_object("/dba/cmds/intersrv/l_agent_connect_server.c"))->main(client_no, account, auth_key, paras);
    }

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=51, nargs=1, insts=51
// --------------------------------------------------------------------
array get_all_files(string path) {
    /* ---- Enhanced Pseudocode ---- */
    mixed all_files;
    mixed get_files;
    int file;

    all_files = ([ ]);
    do {
        get_files = "/daemons/filed.c"->get_catalog(path, "", "", (sizeof(all_files) + 1), 100);
        if ((!is_array(get_files))) {
            get_files = ({});
        }
        loop (file in get_files) {
            all_files[file] = 1;
        }  // end loop (verified)
    } while ((sizeof(get_files) == 100));
    return keys(all_files);

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=3, insts=4
// --------------------------------------------------------------------
void msg_activity_oper(int client_no, string cmd, array args) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @code  nc=10, nargs=0, insts=10
// --------------------------------------------------------------------
void init_startup_module() {
    /* ---- Enhanced Pseudocode ---- */
    assure_file("/dba/log/");
    printf("Done of init_startup_module.\n");

}

// --------------------------------------------------------------------
// @code  nc=38, nargs=1, insts=38
// --------------------------------------------------------------------
int gid_to_val(string gid) {
    /* ---- Enhanced Pseudocode ---- */
    int val64;
    int first;
    int second;

    first = 0;
    second = 0;
    sscanf(gid[0..7], "%x", &first);
    sscanf(gid[8..15], "%x", &second);
    val64 = ({ first, second });
    return (int)itoa64(val64);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=41, nargs=2, insts=41
// --------------------------------------------------------------------
void msg_cfg_ddb_list(int client_no, array list) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ddb_list;
    int i;

    ddb_list = allocate_array(sizeof(list));
    i = 0;

    // --- label L0012 ---
    while ((i < sizeof(list))) {
        ddb_list[i] = ([ "ip": list[i] ]);
        ++i;
    }  // end loop (verified)
    "/daemons/configd.c"->set_config("ddb_list", ddb_list);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=3, insts=10
// --------------------------------------------------------------------
void msg_deleted_gid(int client_no, int cookie, int result) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->msg_deleted_gid(cookie, result);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=3, insts=10
// --------------------------------------------------------------------
void msg_write_pet_deposit(int client_no, int cookie, int result) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->msg_write_pet_deposit(cookie, result);

}

// --------------------------------------------------------------------
// @code  nc=38, nargs=1, insts=38
// --------------------------------------------------------------------
string convert_tao_desc(int tao) {
    /* ---- Enhanced Pseudocode ---- */
    mixed year_num;
    mixed day_num;
    string tao_desc;

    year_num = (tao / 360);
    day_num = (tao % 360);
    tao_desc = "";
    if (year_num) {
        tao_desc += sprintf("%dƒÍ", year_num);
    }
    if (day_num) {
        tao_desc += sprintf("%dÃÏ", day_num);
    }
    return tao_desc;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=26, nargs=0, insts=26
// --------------------------------------------------------------------
string get_exchange_code() {
    /* ---- Enhanced Pseudocode ---- */
    ++exchange_cookie;
    return sprintf("%06x%08x%08x%08x", (int)"/daemons/configd.c"->get_my_info()["my_setting"]["id"], time(), (time() - uptime()), exchange_cookie);

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=34, nargs=1, insts=34
// --------------------------------------------------------------------
int is_test_dist(mixed dist) {
    /* ---- Enhanced Pseudocode ---- */
    mixed test_arr;
    mixed str;

    test_arr = ({ "ƒ⁄≤‚", "≤‚ ‘", "ƒ⁄úy", "úy‘á" });
    if ((!is_string(dist))) {
        return 0;
    }
    loop (str in test_arr) {
        if ((strsrch(dist, str) >= 0)) {
            return 1;
        }
    }  // end loop (verified)
    return 0;

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
private mixed callback_mysql_select_array(int vno) {
    /* ---- Enhanced Pseudocode ---- */
    return (*f_field_value)(vno);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=11, nargs=4, insts=11
// --------------------------------------------------------------------
void msg_created_gid(int client_no, int cookie, int result, string gid) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->msg_created_gid(cookie, result, gid);

}

// --------------------------------------------------------------------
// @code  nc=14, nargs=1, insts=14
// --------------------------------------------------------------------
int remove_resource_string_ex(int id) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!is_string(get_resource_string(id)))) {
        return 1;
    }
    return remove_resource_string(id);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=13, nargs=4, insts=13
// --------------------------------------------------------------------
void msg_l_update_first_credit_bonus(int client_no, string account, string gid, string result_desc) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_update_first_credit_bonus.c"))->main(client_no, account, gid, result_desc);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=14, nargs=5, insts=14
// --------------------------------------------------------------------
void msg_l_auth_all_protect(int client_no, string account, int step, int seq_no, string result) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_auth_all_protect.c"))->main(client_no, account, result, step, seq_no);

}

// --------------------------------------------------------------------
// @code  nc=18, nargs=6, insts=18
// --------------------------------------------------------------------
mixed valid_name(string name, int min_len, int max_len, int may_chinese, mixed include_group, mixed exclude_group) {
    /* ---- Enhanced Pseudocode ---- */
    return _valid_name(name, min_len, max_len, may_chinese, include_group, exclude_group)["msg"];

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=3, insts=4
// --------------------------------------------------------------------
void msg_everyone_pk_oper(int client_no, string cmd, array args) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @code  nc=28, nargs=3, insts=28
// --------------------------------------------------------------------
int receive_from_queue_debug(mixed oid, mixed &data, int timeout) {
    /* ---- Enhanced Pseudocode ---- */
    int ret;

    ret = receive_from_queue(oid, &data, timeout);
    if ((ret == 0)) {
        n_msg[oid] -= 1;
    }
    return ret;

}

// --------------------------------------------------------------------
// @code  nc=72, nargs=1, insts=72
// --------------------------------------------------------------------
string xdbc_get_raw_password(string password) {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr;
    mixed body;

    if ((password[0..1] != "!*")) {
        return password;
    }
    body = password[2..<1];
    if ((strlen(body) % 2)) {
        printf("Bad format of encryped password.\n");
        return password;
    }
    arr = regexp(body, "[0-9,a-f,A-F]*");
    if (((arr[0] > 0) || (arr[1] < strlen(body)))) {
        printf("Bad value of encryped password.\n");
        return password;
    }
    password = (string)des(restore_value(((":" + body) + ":")), 1, "abcdef");
    return password;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=12, nargs=3, insts=12
// --------------------------------------------------------------------
void msg_l_get_credit_coins(int client_no, string gid, string coins) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_get_credit_coins.c"))->main(client_no, gid, coins);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=12, nargs=3, insts=12
// --------------------------------------------------------------------
void msg_l_get_protect_info(int client_no, string account, string result) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_get_protect_info.c"))->main(client_no, account, result);

}

// --------------------------------------------------------------------
// @code  nc=65, nargs=1, insts=65
// --------------------------------------------------------------------
int pp_get_report_ex(string file_name) {
    /* ---- Enhanced Pseudocode ---- */
    int report;

    report = pp_get_report();
    if ((report == -1)) {
        printf("It is busing in collecting, Please waiting...\n");
        return 0;
    }
    if ((report == 0)) {
        printf("No data to report, Please call pp_start_profiling to collect\n");
        return 0;
    }
    if ((!sizeof(file_name))) {
        file_name = ((((("/dba/log/pp_stat_" + _server_name) + "_") + time_desc()[0..<10]) + write_file_cookie) + ".txt");
        ++write_file_cookie;
    }
    write_file(file_name, save_string(report));
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=18, nargs=1, insts=18
// --------------------------------------------------------------------
string val_to_gid(int val) {
    /* ---- Enhanced Pseudocode ---- */
    mixed val64;

    val64 = atoi64(val);
    return sprintf("%08X%08X", val64[0], val64[1]);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=26, nargs=4, insts=26
// --------------------------------------------------------------------
mixed create_content_ex(string path, string name, mixed branch, mixed content) {
    /* ---- Enhanced Pseudocode ---- */
    mixed str;
    mixed convert_type;

    convert_type = get_data_convert_type(path, name);
    str = "/daemons/datasaved.c"->convert_to_save_data(convert_type, content);
    return "/daemons/filed.c"->create_content(path, name, branch, str);

}

// --------------------------------------------------------------------
// @code  nc=19, nargs=0, insts=19
// --------------------------------------------------------------------
void init_sql_module() {
    /* ---- Enhanced Pseudocode ---- */
    _mdb = sql_open_db(":memory:");
    if ((_mdb == -1)) {
        error("Failed to open memory database, startup halt.\n");
    }
    printf("Done of init_sql_module.\n");

}

// --------------------------------------------------------------------
// @code  nc=132, nargs=2, insts=132
// --------------------------------------------------------------------
void update(string file, int clear_init) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    int flag;
    mixed ob;
    mixed obs;
    mixed env;

    if (((!clear_init) && is_array(_post_init_funcs))) {
        printf("Never update object during startup.\n");
        return;
    }
    init_post_init_funcs();
    ob = find_object(file);
    if (is_object(ob)) {
        obs = lodger_object(ob);
        env = env_object(ob);
        destruct_object(ob);
    }
    try {
        ob = load_object(file);
    } catch { /* error handled */ }

    // --- label L0050 ---
    printf("Failed to load object.\n");
    _post_init_funcs = 0;
    return;
    if (is_object(ob)) {
        i = 0;

        // --- label L0067 ---
        while ((i < sizeof(obs))) {
            if (is_object(obs[i])) {
                printf("Reput object %O\n", obs[i]);
                move_object(ob, obs[i]);
            }
            ++i;
        }  // end loop (verified)
        if (is_object(env)) {
            printf("Move to old environment %O\n", env);
            move_object(env, ob);
        }
        flag = 1;
    } else {
        flag = 0;
    }
    post_init();
    if (flag) {
        printf("Update OK.\n");
    } else {
        printf("Update failed.\n");
    }

}

// --------------------------------------------------------------------
// @code  nc=24, nargs=1, insts=24
// --------------------------------------------------------------------
void post_disconnected(int client_no) {
    /* ---- Enhanced Pseudocode ---- */
    mixed f;

    loop (f in _disconnected_funcs) {
        if ((!is_function(f))) {
            continue;
        }
        (*f)(client_no);
    }  // end loop (verified)
    clean_array(_disconnected_funcs, 9);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=147, nargs=4, insts=147
// --------------------------------------------------------------------
mixed get_max_cost_cmd(mixed field, int reset_stat, mixed cmds, array except_cmds) {
    /* ---- Enhanced Pseudocode ---- */
    mixed val;
    mixed valid_fields;
    int ks;
    int cmd;
    mapping m;

    valid_fields = ({ "count", "ticks" });
    if ((!field)) {
        printf("Usage: get_max_cost_cmd(field, reset_stat = 0, cmds = ({ }), except_cmds = ({ }))\n");
        printf("Valid fields: %O\n", valid_fields);
        printf("See also: get_max_cost_client\n");
        return ({});
    }
    if ((member_array(field, valid_fields) == -1)) {
        printf("Bad field %O to get_max_cost_client, expected:\n%O\n", field, valid_fields);
        return;
    }
    m = ([ ]);
    loop (ks in valid_fields) {
        m[ks] = 0;
    }  // end loop (verified)
    if (((!is_array(cmds)) || (!sizeof(cmds)))) {
        cmds = keys(all_cmds_cost);
    }
    loop (cmd in (cmds - except_cmds)) {
        val = (count_total(all_cmds_cost[cmd][field]) + all_cmds_cost[cmd][("other_" + field)]);
        if ((ucmp(m[field], val) >= 0)) {
            continue;
        }
        loop (ks in valid_fields) {
            m[ks] = (count_total(all_cmds_cost[cmd][ks]) + all_cmds_cost[cmd][("other_" + ks)]);
        }  // end loop (verified)
        m["cmd"] = cmd;
    }  // end loop (verified)
    if (reset_stat) {
        all_cmds_cost = ([ ]);
    }
    return m;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
mixed get_daemon_data(string daemon_name) {
    /* ---- Enhanced Pseudocode ---- */
    return daemon_data[daemon_name];

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=13, nargs=4, insts=13
// --------------------------------------------------------------------
void msg_l_get_acc_comp_char(int client_no, string gid, string comp_id, string m_str) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_get_acc_comp_char.c"))->main(client_no, gid, comp_id, m_str);

}

// --------------------------------------------------------------------
// @code  nc=389, nargs=1, insts=389
// --------------------------------------------------------------------
mixed calc_birthday_horoscope(int birthday) {
    /* ---- Enhanced Pseudocode ---- */
    int index;
    int index_ex;
    int day_count;
    int i;
    mixed y;
    mixed d;
    int a;
    mixed year_desc;
    string mon_desc;
    mixed result;
    mixed day_desc;
    string hour_desc;
    mapping l_m;

    if ((birthday < 0)) {
        return;
    }
    l_m = local_time(birthday);
    y = (l_m["year"] - 1900);
    day_count = (int)((((365.242004 * y) + 6.2) + 30.440001) - (1.9 * sin(0.524)));
    d = calc_day_count(1900, birthday);
    a = (l_m["year"] % 60);
    if ((d < day_count)) {
        a -= 5;
    } else {
        a -= 4;
    }
    if ((a < 0)) {
        a += 60;
    }
    year_desc = tiangan_dizhi[a];
    result = sprintf("%sƒÍ", year_desc);
    d = calc_day_count(1970, birthday);
    if ((l_m["hour"] >= 23)) {
        d += 1;
    }
    a = (((base_ganzhi["day"] + d) - 1) % 60);
    day_desc = tiangan_dizhi[a];
    switch (year_desc[0..1]) {
        case "º◊":
        case "º∫":
        mon_desc = "±˚";
        break;
        case "““":
        case "∏˝":
        mon_desc = "ŒÏ";
        break;
        case "±˚":
        case "–¡":
        mon_desc = "∏˝";
        break;
        case "∂°":
        case "»…":
        mon_desc = "»…";
        break;
        case "ŒÏ":
        case "πÔ":
        mon_desc = "º◊";
        break;
        default:
        return a;
    }  // end switch

    // --- label L0168 ---
    d = calc_day_count(1900, birthday);
    y = (l_m["year"] - 1900);
    i = 0;

    // --- label L0186 ---
    while (i < 25) {
        day_count = (int)((((365.240997 * y) + 6.2) + (15.22 * i)) - (1.9 * sin((0.262 * i))));
        if ((d < day_count)) {
            index = i;
            break;
        }
        i += 2;
    }  // end loop (verified)
    if (is_undefined(index)) {
        return;
    }
    if ((!index)) {
        index_ex = 10;
    } else if ((2 == index)) {
        index_ex = 11;
    } else {
        index_ex = ((index / 2) - 2);
    }
    index_ex = (member_array(mon_desc, tiangan) + index_ex);
    mon_desc = tiangan[(index_ex % 10)];
    mon_desc += dizhi[((index / 2) % 12)];
    result += sprintf("%s‘¬", mon_desc);
    result += sprintf("%s»’", day_desc);
    switch (day_desc[0..1]) {
        case "º◊":
        case "º∫":
        hour_desc = "º◊";
        break;
        case "““":
        case "∏˝":
        hour_desc = "±˚";
        break;
        case "±˚":
        case "–¡":
        hour_desc = "ŒÏ";
        break;
        case "∂°":
        case "»…":
        hour_desc = "∏˝";
        break;
        case "ŒÏ":
        case "πÔ":
        hour_desc = "»…";
        break;
        default:
        return;
    }  // end switch

    // --- label L0345 ---
    index = (((l_m["hour"] + 1) / 2) % 12);
    index_ex = (member_array(hour_desc, tiangan) + index);
    hour_desc = tiangan[(index_ex % 10)];
    hour_desc += dizhi[index];
    result += sprintf("%s ±", hour_desc);
    return result;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=36, nargs=1, insts=36
// --------------------------------------------------------------------
int get_month_zero_time(int t) {
    /* ---- Enhanced Pseudocode ---- */
    mapping lt;

    lt = local_time(t);
    lt["mday"] = 1;
    lt["hour"] = 0;
    lt["min"] = 0;
    lt["sec"] = 0;
    return mktime(lt);

}

// --------------------------------------------------------------------
// @code  nc=75, nargs=3, insts=75
// --------------------------------------------------------------------
mixed read_etc_file(string file_name, string mode, string file_path) {
    /* ---- Enhanced Pseudocode ---- */
    mixed content;

    if (((mode != "a") && (mode != "b"))) {
        error("read_etc_file choose a wrong mode type.\n");
    }
    if ((check_server_status(3) || (online_update_status != -1))) {
        content = read_file((file_path + file_name), mode);
        if ((is_string(content) || is_buffer(content))) {
            return content;
        }
    }
    content = read_file(("/dba/pack_data/etc.pak/" + file_name), mode);
    if ((is_string(content) || is_buffer(content))) {
        return content;
    }
    printf("[1;31mEtc file %s not found.\n[2;37;0m", file_name);
    return ((mode == "a") ? "" : allocate_buffer(0));

}

// --------------------------------------------------------------------
// @code  nc=7, nargs=0, insts=7
// --------------------------------------------------------------------
void debug_help() {
    /* ---- Enhanced Pseudocode ---- */
    printf("Debug command list.\nget_max_cost_client     - Õ≥º∆ø™œ˙◊Ó¥ÛµƒøÕªß∂À\nget_max_cost_cmd        - Õ≥º∆ø™œ˙◊Ó¥Ûµƒ√¸¡Ó\nrpc_stat                - ≤Èø¥ RPC Õ≥º∆–≈œ¢\nsize_all                - ≤Èø¥ƒ≥∏ˆƒø¬ºœ¬À˘”–∂‘œÛµƒƒ⁄¥Ê◊ ‘¥’º”√«Èøˆ\nsize_global             - ≤Èø¥À˘”–»´æ÷±‰¡øµƒ◊ ‘¥’º”√«Èøˆ\nsize_max                - ≤Èø¥À˘”–∂‘œÛ÷–◊Óœ˚∫ƒ◊ ‘¥µƒ∂‘œÛ\ntop                     - ≤Èø¥À˘”–œﬂ≥Ãµƒ CPU ’º”√«Èøˆ\nupdate                  - ∏¸–¬÷∏∂®∂‘œÛ\nenable_message_stat     - ø™∆ÙªÚ’ﬂπÿ±’œ˚œ¢µƒ¡˜¡ø–≈œ¢Õ≥º∆(≤Œ ˝£∫0/1)\nget_message_stat_info   - ≤Èø¥ƒ≥∏ˆœ˚œ¢µƒ¡˜¡øÕ≥º∆–≈œ¢(≤Œ ˝£∫œ˚œ¢∫≈/Œﬁ)\nreset_message_stat      - ÷ÿ÷√ƒ≥∏ˆœ˚œ¢µƒ¡˜¡øÕ≥º∆–≈œ¢(≤Œ ˝£∫œ˚œ¢∫≈/Œﬁ)\n");

}

// --------------------------------------------------------------------
// @code  nc=100, nargs=2, insts=100
// --------------------------------------------------------------------
int calc_day_count(int a_year, int b_ti) {
    /* ---- Enhanced Pseudocode ---- */
    mixed d;
    mixed i;
    int b;
    mapping l_m;

    b = 0;
    l_m = local_time(b_ti);
    for (i = a_year; i <= l_m["year"]; i++) {
        if (((!(i % 400)) || ((!(i % 4)) && (i % 100)))) {
            if (((i == l_m["year"]) && (l_m["mon"] <= 2))) {
                continue;
            }
            ++b;
        }
    }  // end loop (verified)
    d = (((l_m["year"] - a_year) * 365) + b);
    if ((l_m["mon"] > 1)) {
        d += count_total(month_day[0..(l_m["mon"] - 2)]);
    }
    d += l_m["mday"];
    return d;

}

// --------------------------------------------------------------------
// @code  nc=7, nargs=1, insts=7
// --------------------------------------------------------------------
void debug_delete_thread(mixed thread_sign) {
    /* ---- Enhanced Pseudocode ---- */
    delete_thread(thread_sign);

}

// --------------------------------------------------------------------
// @code  nc=10, nargs=2, insts=10
// --------------------------------------------------------------------
void register_enter_world_failed(string file, function f) {
    /* ---- Enhanced Pseudocode ---- */
    _enter_world_failed_funcs[file] = f;

}

// --------------------------------------------------------------------
// @code  nc=19, nargs=1, insts=19
// --------------------------------------------------------------------
void debug_shift_time(int shift) {
    /* ---- Enhanced Pseudocode ---- */
    if ((1 && (!defined("LAB")))) {
        return;
    }
    shift_time(shift);
    server_shift_time = shift;

}

// --------------------------------------------------------------------
// @code  nc=23, nargs=0, insts=23
// --------------------------------------------------------------------
void post_log_runtime_info() {
    /* ---- Enhanced Pseudocode ---- */
    mixed f;

    loop (f in _log_runtime_info_funcs) {
        if ((!is_function(f))) {
            continue;
        }
        (*f)();
    }  // end loop (verified)
    clean_array(_log_runtime_info_funcs, 9);

}

// --------------------------------------------------------------------
// @code  nc=25, nargs=1, insts=25
// --------------------------------------------------------------------
int file_exist_ex(string file) {
    /* ---- Enhanced Pseudocode ---- */
    if ((file_size(file) > 0)) {
        return 1;
    }
    if ((file_size((lib_pak_path + replace_string(file, ".c", ".o"))) > 0)) {
        return 1;
    }
    return 0;

}

// --------------------------------------------------------------------
// @code  nc=48, nargs=0, insts=48
// --------------------------------------------------------------------
void update_today_zero_time() {
    /* ---- Enhanced Pseudocode ---- */
    mapping lt;

    lt = local_time();
    lt["min"] = 0;
    lt["sec"] = 0;
    lt["hour"] = 0;
    _today_zero_time = mktime(lt);
    ++lt["mday"];
    set_timer((mktime(lt) - time()), 0, "update_today_zero_time");

}

// --------------------------------------------------------------------
// @code  nc=83, nargs=3, insts=83
// --------------------------------------------------------------------
mixed sql_delete(mixed db, string table, mixed condition) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    mixed arr;
    mixed sql_cmd;
    string str;
    int key;

    str = "";
    if ((condition && sizeof(condition))) {
        str += "where ";
        if (is_string(condition)) {
            str += condition;
        } else if (is_mapping(condition)) {
            i = 0;
            arr = allocate_array(sizeof(condition));
            loop (key in keys(condition)) {
                arr[i++] = sprintf("%s=%O", key, condition[key]);
            }  // end loop (verified)
            str += implode(arr, " and ");
        } else {
            error("Bad condition type, expected string | mapping.\n");
        }
    }
    sql_cmd = sprintf("delete from %s %s", table, str);
    return sql_execute(db, sql_cmd);

}

// --------------------------------------------------------------------
// @code  nc=61, nargs=9, insts=61
// --------------------------------------------------------------------
int calc_server_status_as_aaa(int available, int online, int max_user, float cpu_cost, float gs_user_over, float gs_user_full, float gs_user_busy, float gs_cpu_full, float gs_cpu_busy) {
    /* ---- Enhanced Pseudocode ---- */
    if ((!available)) {
        return 1;
    }
    if ((max_user == 0)) {
        return 5;
    }
    if ((((online * 100) / max_user) > gs_user_over)) {
        return 5;
    }
    if (((((online * 100) / max_user) > gs_user_full) || ((cpu_cost * 100) > gs_cpu_full))) {
        return 4;
    }
    if (((((online * 100) / max_user) > gs_user_busy) || ((cpu_cost * 100) > gs_cpu_busy))) {
        return 3;
    }
    return 2;

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=30, nargs=1, insts=30
// --------------------------------------------------------------------
int is_gm_name(string name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed group_words;
    mixed word;

    if ((!is_array(group_words = ban_words["GM±ÿ”√◊÷∑˚"]))) {
        return 0;
    }
    loop (word in group_words) {
        if ((strsrchw(name, word) != -1)) {
            return 1;
        }
    }  // end loop (verified)
    return 0;

}

// --------------------------------------------------------------------
// @code  nc=6, nargs=1, insts=6
// --------------------------------------------------------------------
int update_dist_start_time(int start_time) {
    /* ---- Enhanced Pseudocode ---- */
    return 0;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=11, nargs=4, insts=11
// --------------------------------------------------------------------
void msg_get_counts(int client_no, int cookie, int result, string counts) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->msg_get_counts(cookie, result, counts);

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=3, insts=4
// --------------------------------------------------------------------
void msg_arena_oper(int client_no, string cmd, array args) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=3, insts=10
// --------------------------------------------------------------------
void msg_got_catalog_ex(int client_no, int cookie, int result) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->msg_got_catalog_ex(cookie, result);

}

// --------------------------------------------------------------------
// @code  nc=114, nargs=1, insts=114
// --------------------------------------------------------------------
mixed tixml_parse_ex(mapping xml) {
    /* ---- Enhanced Pseudocode ---- */
    mapping node;
    mixed node_ex;
    mixed props;
    int i;

    props = keys(xml);
    for (i = 0; i <= (sizeof(props) - 1); i++) {
        if ((props[i][0] == 95)) {
            props[i] = 0;
        }
    }  // end loop (verified)
    clean_array(props, 4);
    xml["_all_props"] = props;
    node_ex = ([ ]);
    if (is_array(xml["_childs"])) {
        loop (node in xml["_childs"]) {
            if (is_undefined(node["_element"])) {
                error("µ±«∞Ω⁄µ„√ª”–√˚◊÷\n");
            }
            if (is_undefined(node_ex[node["_element"]])) {
                node_ex[node["_element"]] = ({ node });
            } else {
                node_ex[node["_element"]] += ({ node });
            }
            node["_parent"] = xml;
            tixml_parse_ex(node);
        }  // end loop (verified)
    }
    xml["_childs_ex"] = node_ex;
    return;

}

// --------------------------------------------------------------------
// @code  nc=51, nargs=1, insts=51
// --------------------------------------------------------------------
void stat_disconnect(int where) {
    /* ---- Enhanced Pseudocode ---- */
    string reason;

    switch (where) {
        case 0:
        case 1:
        reason = "rule_matched";
        break;
        case 3:
        return;
        case 2:
        case 4:
        reason = "driver_callback";
        break;
        case 10:
        reason = "no_respond";
        break;
        default:
        reason = "unknown";
        break;
    }  // end switch

    // --- label L0038 ---
    disconnect_stat[reason] = ((int)disconnect_stat[reason] + 1);

}

// --------------------------------------------------------------------
// @code  nc=79, nargs=1, insts=79
// --------------------------------------------------------------------
void compile_all_path(string path) {
    /* ---- Enhanced Pseudocode ---- */
    mixed files;
    mixed name;
    int i;

    if ((path[(strlen(path) - 1)] != 47)) {
        path += "/";
    }
    compile_all(path);
    files = get_sub_dirs_ex(path);
    i = 0;

    // --- label L0027 ---
    while ((i < sizeof(files))) {
        name = files[i][0];
        if (((name == ".") || (name == ".."))) {
        } else if ((files[i][1] == -2)) {
            printf("Compile sub dir: %s\n", (path + name));
            compile_all_path((path + name));
        }

        // --- label L0071 ---
        ++i;
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=21, nargs=2, insts=21
// --------------------------------------------------------------------
int is_same_account(mixed account_a, mixed account_b) {
    /* ---- Enhanced Pseudocode ---- */
    if (((!is_string(account_a)) || (!is_string(account_b)))) {
        return 0;
    }
    return (tolower_temp(account_a) == tolower_temp(account_b));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=3, insts=10
// --------------------------------------------------------------------
void msg_cfg_etc_page_data(int client_no, int cookie, buffer content) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/configd.c"->msg_cfg_etc_page_data(cookie, content);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=82, nargs=2, insts=82
// --------------------------------------------------------------------
string get_raw_password(string password, mixed key) {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr;
    mixed body;
    mixed encrypt_key;

    encrypt_key = ((!is_string(key)) ? "abcdef" : key);
    if ((password[0..1] != "!*")) {
        return password;
    }
    body = password[2..<1];
    if ((strlen(body) % 2)) {
        printf("Bad format of encryped password.\n");
        return password;
    }
    arr = regexp(body, "[0-9,a-f,A-F]*");
    if (((arr[0] > 0) || (arr[1] < strlen(body)))) {
        printf("Bad value of encryped password.\n");
        return password;
    }
    password = (string)des(restore_value(((":" + body) + ":")), 1, encrypt_key);
    return password;

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=4, insts=4
// --------------------------------------------------------------------
void msg_spa_busy(int client_no, int max_size, int data_size, int threshold) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @code  nc=92, nargs=4, insts=92
// --------------------------------------------------------------------
private mixed mixed_mysql_select(int cno, string sql_cmd, function fetch, int max_row) {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr;
    mixed vno;
    int i;
    int n;

    vno = (*f_query)(cno, sql_cmd);
    if ((vno == -1)) {
        return 0;
    }
    n = (*f_num_records)(vno);
    if ((n > max_row)) {
        n = max_row;
    }
    if ((n <= 0)) {
        (*f_finalize)(vno);
        return ({});
    }
    arr = allocate_array(n);
    for (i = 0; i <= (n - 1); i++) {
        if (((*f_fetch_record)(vno) != 0)) {
            (*f_finalize)(vno);
            return arr[0..(i - 1)];
        }
        arr[i] = (*fetch)(vno);
    }  // end loop (verified)
    (*f_finalize)(vno);
    return arr;

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=10, nargs=2, insts=10
// --------------------------------------------------------------------
void set_gid_object(string gid, object ob) {
    /* ---- Enhanced Pseudocode ---- */
    gid_objects[gid] = ob;

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=1, insts=9
// --------------------------------------------------------------------
void register_post_heart_beat(function f) {
    /* ---- Enhanced Pseudocode ---- */
    _heart_beat_funcs += ({ f });

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=1, insts=9
// --------------------------------------------------------------------
void register_regen_iid(function f) {
    /* ---- Enhanced Pseudocode ---- */
    _regen_iid_funcs += ({ f });

}

// --------------------------------------------------------------------
// @code  nc=81, nargs=1, insts=81
// --------------------------------------------------------------------
mixed deep_dup(mixed val) {
    /* ---- Enhanced Pseudocode ---- */
    mixed temp;
    mixed elem;
    int i;

    temp = dup(val);
    if (is_array(val)) {
        for (i = 0; i <= (sizeof(temp) - 1); i++) {
            if ((is_array(temp[i]) || is_mapping(temp[i]))) {
                temp[i] = dup(temp[i]);
            }
        }  // end loop (verified)
    } else if (is_mapping(val)) {
        loop (elem in val) {
            if ((is_array(elem[1]) || is_mapping(elem[1]))) {
                temp[elem[0]] = dup(elem[1]);
            }
        }  // end loop (verified)
    }
    return temp;

}

// --------------------------------------------------------------------
// @code  nc=179, nargs=4, insts=179
// --------------------------------------------------------------------
string time_length_to_ch_ex(int to_t, int src_t, int accuracy, int rounding) {
    /* ---- Enhanced Pseudocode ---- */
    int year;
    int mon;
    string year_desc;
    string mon_desc;
    mapping src_m;
    mapping map;

    if ((to_t <= src_t)) {
        return "";
    }
    src_m = local_time(src_t);
    year = 0;
    map = dup(src_m);

    // --- label L0025 ---
    while (1) {
        ++map["year"];
        if ((mktime(map) <= to_t)) {
            ++year;
        } else {
            break;
        }
    }  // end loop (verified)
    src_m["year"] += year;
    mon = 0;
    map = dup(src_m);

    // --- label L0065 ---
    while (1) {
        ++map["mon"];
        if ((mktime(map) <= to_t)) {
            ++mon;
        } else {
            break;
        }
    }  // end loop (verified)
    if ((5 == accuracy)) {
        if (rounding && mon) {
            ++year;
        }
        return sprintf("%dƒÍ", year);
    }
    if (year) {
        year_desc = sprintf("%dƒÍ", year);
    } else {
        year_desc = "";
    }
    src_m["mon"] += mon;
    if ((6 == accuracy)) {
        if ((rounding && (mktime(src_m) < to_t))) {
            ++mon;
        }
        return (year_desc + sprintf("%d∏ˆ‘¬", mon));
    }
    if (mon) {
        mon_desc = sprintf("%d∏ˆ‘¬", mon);
    } else {
        mon_desc = "";
    }
    return ((year_desc + mon_desc) + time_length_to_ch((to_t - mktime(src_m)), accuracy, rounding));

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=26, nargs=3, insts=26
// --------------------------------------------------------------------
string set_dynamic_key(object ob, string key_index, mixed key) {
    /* ---- Enhanced Pseudocode ---- */
    if (is_string(key)) {
        return ob->set_temp(("dynamic_key/" + key_index), key);
    }
    return ob->set_temp(("dynamic_key/" + key_index), "/daemons/encryptd.c"->gen_dynamic_key());

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=3, insts=10
// --------------------------------------------------------------------
void msg_deleted_content(int client_no, int cookie, int result) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->msg_deleted_content(cookie, result);

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=2, insts=9
// --------------------------------------------------------------------
private mixed mysql_select_column_fetch(int column, int vno) {
    /* ---- Enhanced Pseudocode ---- */
    return (*f_field_value)(vno, column);

}

// --------------------------------------------------------------------
// @code  nc=45, nargs=1, insts=45
// --------------------------------------------------------------------
int start_stat_cmds(int interval) {
    /* ---- Enhanced Pseudocode ---- */
    if (enable_stat_cmds) {
        printf("stat cmds is busing now, please first stop it by stop_stat_cmds\n");
        return 0;
    }
    if ((interval <= 0)) {
        printf("stat cmds interval error\n");
        return 0;
    }
    enable_stat_cmds = 1;
    cmds_flow_in = ([ ]);
    cmds_processed = ([ ]);
    stat_cmds_interval = interval;
    set_timer(interval, "stat_cmds", (: stop_stat_cmds :));
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=0, insts=4
// --------------------------------------------------------------------
void load_dist_start_time() {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @code  nc=13, nargs=2, insts=13
// --------------------------------------------------------------------
buffer xdbc_encrypt_data(buffer data, int key) {
    /* ---- Enhanced Pseudocode ---- */
    return rdl(data, 16, sprintf("%016X", (key & -1)));

}

// --------------------------------------------------------------------
// @code  nc=84, nargs=1, insts=84
// --------------------------------------------------------------------
string convert_to_url_encode(string dist_name) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    buffer bufserv;
    string strserv;
    mixed str;

    bufserv = (buffer)iconv(dist_name, "utf-8", "GBK");
    strserv = "";
    if (new_convert_code) {
        for (i = 0; i <= (sizeof(bufserv) - 1); i++) {
            str = bufserv[i];
            if ((isdigit(str) || isalpha(str))) {
                strserv += sprintf("%c", str);
            } else {
                strserv += sprintf("%%%02x", str);
            }
        }  // end loop (verified)
    } else {
        for (i = 0; i <= (sizeof(bufserv) - 1); i++) {
            strserv += sprintf("%%%02x", bufserv[i]);
        }  // end loop (verified)
    }
    return strserv;

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=1, insts=9
// --------------------------------------------------------------------
void register_adjust_position_info(function f) {
    /* ---- Enhanced Pseudocode ---- */
    _adjust_position_info_funcs += ({ f });

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=2, insts=4
// --------------------------------------------------------------------
void cmd_l_when_batch_block_account(int client_no, string str_account) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @code  nc=268, nargs=0, insts=268
// --------------------------------------------------------------------
void top() {
    /* ---- Enhanced Pseudocode ---- */
    mixed last_select;
    mixed current_select;
    mixed last_oper;
    mixed current_oper;
    mixed last_callback;
    mixed current_callback;
    mixed last_sc1;
    mixed current_sc1;
    mixed last_sc2;
    mixed current_sc2;
    mixed last_sc3;
    mixed current_sc3;
    mixed last_sc4;
    mixed current_sc4;
    mixed last_ticks;
    mixed ti;
    mapping info_data;
    mixed last_threads;
    mapping m;

    last_ticks = tick();
    last_threads = ([ ]);
    loop (m in get_all_threads()) {
        last_threads[m["tid"]] = m["total_ticks"];
    }  // end loop (verified)
    info_data = info()["cpu_cost"];
    last_select = info_data["select_socket_ticks"];
    last_oper = info_data["oper_socket_ticks"];
    last_callback = info_data["callback_ticks"];
    last_sc1 = info_data["process_ticks"];
    last_sc2 = info_data["scan_socket_ticks"];
    last_sc3 = info_data["scan_telnet_ticks"];
    last_sc4 = info_data["idle_ticks"];
    while (1) {
        sleep(2);
        ti = (tick() - last_ticks);
        last_ticks = tick();
        printf("--------------------------------------------\n");
        info_data = info()["cpu_cost"];
        current_select = info_data["select_socket_ticks"];
        current_oper = info_data["oper_socket_ticks"];
        current_callback = info_data["callback_ticks"];
        current_sc1 = info_data["process_ticks"];
        current_sc2 = info_data["scan_socket_ticks"];
        current_sc3 = info_data["scan_telnet_ticks"];
        current_sc4 = info_data["idle_ticks"];
        loop (m in get_all_threads()) {
            printf("%-14s: %s%%\n", m["name"], cpu_cost(((real)(m["total_ticks"] - (int)last_threads[m["tid"]]) / ti)));
            last_threads[m["tid"]] = m["total_ticks"];
        }  // end loop (verified)
        printf("[1;32m%-14s: %s%%\n%-14s: %s%%\n%-14s: %s%%\n>>> Raw cost of scheduler\n%-14s: %s%%\n%-14s: %s%%\n%-14s: %s%%\n%-14s: %s%%\n[2;37;0m", "Select socket", cpu_cost(((real)(current_select - last_select) / ti)), "Oper socket", cpu_cost(((real)(current_oper - last_oper) / ti)), "Callback", cpu_cost(((real)(current_callback - last_callback) / ti)), "Process", cpu_cost(((real)(current_sc1 - last_sc1) / ti)), "Scan socket", cpu_cost(((real)(current_sc2 - last_sc2) / ti)), "Scan telnet", cpu_cost(((real)(current_sc3 - last_sc3) / ti)), "Idle", cpu_cost(((real)(current_sc4 - last_sc4) / ti)));
        last_select = current_select;
        last_oper = current_oper;
        last_callback = current_callback;
        last_sc1 = current_sc1;
        last_sc2 = current_sc2;
        last_sc3 = current_sc3;
        last_sc4 = current_sc4;
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=3, insts=10
// --------------------------------------------------------------------
void msg_xg_create_name(int client_no, int cookie, int result) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->msg_xg_create_name(cookie, result);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=12, nargs=1, insts=12
// --------------------------------------------------------------------
int get_last_hour_as_int(int ntime) {
    /* ---- Enhanced Pseudocode ---- */
    return (((ntime / 3600) * 3600) - 3600);

}

// --------------------------------------------------------------------
// @code  nc=322, nargs=1, insts=322
// --------------------------------------------------------------------
string arabic_to_chinese(int num) {
    /* ---- Enhanced Pseudocode ---- */
    int num_temp;
    string str;

    str = "";
    if ((num < 0)) {
        num = abs(num);
        str += "∏∫";
    }
    if ((num >= 100000000)) {
        num_temp = (num / 100000000);
        num = (num % 100000000);
        if (num_temp) {
            str += (arabic_to_chinese(num_temp) + "“⁄");
            if ((num && ((num_temp && ((num_temp % 10) == 0)) || ((num / 10000000) == 0)))) {
                str += "¡„";
            }
        }
    }
    if ((num >= 10000)) {
        num_temp = (num / 10000);
        num = (num % 10000);
        if (num_temp) {
            str += (arabic_to_chinese(num_temp) + "ÕÚ");
            if ((num && ((num_temp && ((num_temp % 10) == 0)) || ((num / 1000) == 0)))) {
                str += "¡„";
            }
        }
    }
    if ((num >= 1000)) {
        num_temp = (num / 1000);
        num = (num % 1000);
        if (num_temp) {
            str += (arabic_to_chinese(num_temp) + "«ß");
            if ((num && ((num / 100) == 0))) {
                str += "¡„";
            }
        }
    }
    if ((num >= 100)) {
        num_temp = (num / 100);
        num = (num % 100);
        if (num_temp) {
            str += (arabic_to_chinese(num_temp) + "∞Ÿ");
            if ((num && ((num / 10) == 0))) {
                str += "¡„";
            }
        }
    }
    if ((num >= 10)) {
        num_temp = (num / 10);
        num = (num % 10);
        if (num_temp) {
            if (((!strlen(str)) && (num_temp == 1))) {
                str += " Æ";
            } else {
                str += (arabic_to_chinese(num_temp) + " Æ");
            }
        }
    }
    switch (num) {
        case 0:
        if ((!strlen(str))) {
            str += "¡„";
        }
        break;
        case 1:
        str += "“ª";
        break;
        case 2:
        str += "∂˛";
        break;
        case 3:
        str += "»˝";
        break;
        case 4:
        str += "Àƒ";
        break;
        case 5:
        str += "ŒÂ";
        break;
        case 6:
        str += "¡˘";
        break;
        case 7:
        str += "∆ﬂ";
        break;
        case 8:
        str += "∞À";
        break;
        case 9:
        str += "æ≈";
        break;
        default:
    }  // end switch

    // --- label L0318 ---
    return str;

}

// --------------------------------------------------------------------
// @code  nc=11, nargs=2, insts=11
// --------------------------------------------------------------------
array mdb_select(string sql_cmd, int max_row) {
    /* ---- Enhanced Pseudocode ---- */
    return sql_select(_mdb, sql_cmd, max_row);

}

// --------------------------------------------------------------------
// @code  nc=226, nargs=2, insts=226
// --------------------------------------------------------------------
mixed time_desc_to_int(string desc, int style) {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;
    mixed year;
    mixed mon;
    mixed mday;
    mixed hour;
    mixed min;
    mixed sec;
    mixed all_mons;

    if ((desc == "")) {
        return 0;
    }
    switch (style) {
        case 0:
        all_mons = ({ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" });
        if ((sscanf(desc, "%*s %s %d %d:%d:%d %d", &mon, &mday, &hour, &min, &sec, &year) < 6)) {
            return 0;
        }
        mon = (member_array(mon, all_mons) + 1);
        break;
        case 1:
        if ((sscanf(desc, "%d-%d-%d-%d:%d:%d", &year, &mon, &mday, &hour, &min, &sec) < 3)) {
            return 0;
        }
        break;
        case 2:
        if ((sscanf(desc, "%d-%d-%d", &year, &mon, &mday) < 3)) {
            return 0;
        }
        break;
        case 3:
        if ((desc == "0")) {
            return 0;
        }
        if ((strlen(desc) != 14)) {
            return 0;
        }
        year = atoi(desc[0..3]);
        mon = atoi(desc[4..5]);
        mday = atoi(desc[6..7]);
        hour = atoi(desc[8..9]);
        min = atoi(desc[10..11]);
        sec = atoi(desc[12..13]);
        break;
        default:
        return 0;
    }  // end switch

    // --- label L0151 ---
    m = ([ ]);
    m["year"] = (is_int(year) ? year : 0);
    m["mon"] = (is_int(mon) ? mon : 0);
    m["mday"] = (is_int(mday) ? mday : 0);
    m["hour"] = (is_int(hour) ? hour : 0);
    m["min"] = (is_int(min) ? min : 0);
    m["sec"] = (is_int(sec) ? sec : 0);
    return mktime(m);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=14, nargs=5, insts=14
// --------------------------------------------------------------------
void msg_l_add_fill_info(int client_no, string account, string gid, int result, string type) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_add_fill_info.c"))->main(client_no, account, gid, result, type);

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=3, insts=4
// --------------------------------------------------------------------
void msg_csa_common_oper(int client_no, string cmd, array args) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=14, nargs=5, insts=14
// --------------------------------------------------------------------
void msg_l_get_safe_ctrl_info(int client_no, string module, string account, string attrib, string result) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_get_safe_ctrl_info.c"))->main(client_no, module, account, attrib, result);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=25, nargs=2, insts=25
// --------------------------------------------------------------------
void cmd_disconnected(int client_no, int where) {
    /* ---- Enhanced Pseudocode ---- */
    mixed agent;

    if (is_object(agent = find_agent_by_client(client_no))) {
        agent->connection_lost();
        return;
    }
    "/daemons/threadd.c"->dispatch_service((: _cmd_disconnected, client_no, where :));

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=20, nargs=1, insts=20
// --------------------------------------------------------------------
int is_whole_point_moment(int ntime) {
    /* ---- Enhanced Pseudocode ---- */
    mapping ti;

    ti = local_time(ntime);
    return ((!ti["min"]) && (!ti["sec"]));

}

// --------------------------------------------------------------------
// @code  nc=107, nargs=2, insts=107
// --------------------------------------------------------------------
string encrypt_mask_md5(string md5_str, mixed &mask) {
    /* ---- Enhanced Pseudocode ---- */
    mixed start_pos;
    mixed end_pos;
    int i;
    mixed weight;
    mixed arr_pos_weight;
    mixed arr_sub_str;
    mixed new_md5;

    mask = "";
    start_pos = random(0, 31);
    end_pos = random(start_pos, 31);
    arr_pos_weight = allocate_array(((end_pos - start_pos) + 1));
    arr_sub_str = allocate_array(((end_pos - start_pos) + 1));
    for (i = 0; i <= (end_pos - start_pos); i++) {
        weight = random(1, 9);
        arr_pos_weight[i] = weight;
        arr_sub_str[i] = sprintf("%c", (md5_str[i] + weight));
    }  // end loop (verified)
    mask = sprintf("%02d%02d%s", start_pos, end_pos, implode(arr_pos_weight, ""));
    new_md5 = ((md5_str[0..(start_pos - 1)] + implode(arr_sub_str, "")) + md5_str[(end_pos + 1)..<1]);
    return new_md5;

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=3, insts=4
// --------------------------------------------------------------------
void msg_data_mining(int client_no, string type, buffer buf) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @code  nc=17, nargs=0, insts=17
// --------------------------------------------------------------------
void load_valid_chinese() {
    /* ---- Enhanced Pseudocode ---- */
    mixed valid_chinese_characters;

    if ((!is_string(valid_chinese_characters = read_etc_file("valid_chinese.list")))) {
        error("∂¡»°”––ß÷–Œƒ◊÷∑˚±Ì ß∞‹£¨«Î»∑∂®∏√Œƒº˛¥Ê‘⁄£°\n");
    }
    set_valid_chinese(valid_chinese_characters);

}

// --------------------------------------------------------------------
// @code  nc=24, nargs=2, insts=24
// --------------------------------------------------------------------
private mixed find_mysql_func(string func_name, int no_xdbc) {
    /* ---- Enhanced Pseudocode ---- */
    mixed xdbc_func_name;

    if (no_xdbc) {
        return find_function(func_name);
    }
    xdbc_func_name = xdbc_func_map[func_name];
    if ((!is_string(xdbc_func_name))) {
        return 0;
    }
    return find_function(xdbc_func_name);

}

// --------------------------------------------------------------------
// @code  nc=14, nargs=0, insts=14
// --------------------------------------------------------------------
void daemon() {
    /* ---- Enhanced Pseudocode ---- */
    set_as_daemon();
    try {
        _reboot();
    } catch { /* error handled */ }

    // --- label L0011 ---

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=3, insts=4
// --------------------------------------------------------------------
void cmd_ccs_shutdown_by_spa(int client_no, string interval, int times) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @code  nc=133, nargs=2, insts=133
// --------------------------------------------------------------------
int regular_number(int v, mixed index) {
    /* ---- Enhanced Pseudocode ---- */
    if ((v >= 10000)) {
        v = ((v / 1000) * 1000);
    } else if ((v >= 5000)) {
        if (((("power" == index) || ("phy_power" == index)) || ("mag_power" == index))) {
            v = ((v / 200) * 200);
        } else {
            v = ((v / 500) * 500);
        }
    } else if ((v >= 2000)) {
        v = ((v / 200) * 200);
    } else if ((v >= 1000)) {
        v = ((v / 100) * 100);
    } else if ((v > 500)) {
        v = ((v / 50) * 50);
    } else if ((v > 200)) {
        v = ((v / 20) * 20);
    } else if ((v > 100)) {
        v = ((v / 10) * 10);
    } else if ((v > 30)) {
        v = ((v / 5) * 5);
    }
    return v;

}

// --------------------------------------------------------------------
// @code  nc=25, nargs=2, insts=25
// --------------------------------------------------------------------
mixed mysql_select_single_result(int cno, string sql_cmd) {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr;

    arr = mysql_select(cno, sql_cmd, 2);
    if ((!arr)) {
        return arr;
    }
    return (sizeof(arr) ? arr[0] : 0);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=3, insts=10
// --------------------------------------------------------------------
void msg_created_content(int client_no, int cookie, int result) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->msg_created_content(cookie, result);

}

// --------------------------------------------------------------------
// @code  nc=7, nargs=2, insts=7
// --------------------------------------------------------------------
void _cmd_disconnected(int client_no, int where) {
    /* ---- Enhanced Pseudocode ---- */
    post_disconnected(client_no);

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=21, nargs=2, insts=21
// --------------------------------------------------------------------
int is_same_hour(int ti1, int ti2) {
    /* ---- Enhanced Pseudocode ---- */
    if ((abs((ti1 - ti2)) >= 3600)) {
        return 0;
    }
    return ((ti1 / 3600) == (ti2 / 3600));

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=2, insts=4
// --------------------------------------------------------------------
void msg_ccs_reboot_result(int client_no, int result) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @code  nc=85, nargs=2, insts=85
// --------------------------------------------------------------------
private void safe_load_object(string file, mapping para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed depend_files;
    mixed depend_file;
    //mixed <loop_1>;
    mixed inputed;

    while (1) {
        if (is_array(depend_files = file_dependence[file])) {
            loop (depend_file in depend_files) {
                if ((!find_object(depend_file))) {
                    safe_load_object(depend_file, para);
                }
            }  // end loop (verified)
        }
        try {
            load_object(file, para);
        } catch { /* error handled */ }

        // --- label L0035 ---
        if ((!0)) {
            printf("Load: %s... OK\n", file);
            return ([ ]);
        }
        printf("Load: %s... Failed\n", file);
        printf("[A]bort, [I]gnore, [R]erty: ");
        if ((!is_string(inputed = input_string()))) {
            return;
        }
        switch (tolower(trim(inputed))) {
            case "a":
            shutdown();
            return;
            case "i":
            return;
            case "r":
            default:
            printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n[1;37mReload file...\n[2;37;0m");
            load_file_dependence();
            printf("File dependencies updated.\n");
            break;
        }  // end switch

        // --- label L0081 ---
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @code  nc=18, nargs=1, insts=18
// --------------------------------------------------------------------
mixed xdbc_this_record(int vno) {
    /* ---- Enhanced Pseudocode ---- */
    xdbc_trace(0, "Call xdbc_this_record(%d)\n", vno);
    return xdbc_call_server(xdbc_get_cno_of_view(vno), ([ "cmd": "this_record", "vno": vno ]));

}

// --------------------------------------------------------------------
// @brief  mutator: set property
// @code  nc=10, nargs=2, insts=10
// --------------------------------------------------------------------
void set_client_agent(int client_no, object agent) {
    /* ---- Enhanced Pseudocode ---- */
    client_agents[client_no] = agent;

}

// --------------------------------------------------------------------
// @code  nc=159, nargs=2, insts=159
// --------------------------------------------------------------------
void size_max(int n, int min_size) {
    /* ---- Enhanced Pseudocode ---- */
    mixed m;
    mapping s;
    mapping summary;
    mapping detail;
    mixed ob;
    int name;
    mixed cost;
    int total;
    int sub_total;

    m = ([ ]);
    total = 0;
    loop (ob in objects()) {
        if ((!is_object(ob))) {
            continue;
        }
        name = object_name(ob);
        s = size_object(ob);
        total += (cost = s["summary"]["memory_cost"]);
        if ((cost < min_size)) {
            continue;
        }
        m[name] = s;
    }  // end loop (verified)
    sub_total = 0;
    printf("%-28s - mapping  array  string  buffer  value  memory\n", "Object name");
    loop (name in sort(keys(m), (: _size_max_get_memory_cost, m :), -1)[0..(n - 1)]) {
        detail = m[name]["detail"];
        summary = m[name]["summary"];
        sub_total += summary["memory_cost"];
        printf("%-28s - %7d  %5d  %6d  %6d  %5d  %5dK\n", name[<28..<1], detail["counter"]["mapping"], detail["counter"]["array"], detail["counter"]["string"], detail["counter"]["buffer"], detail["counter"]["value"], ((summary["memory_cost"] + 1023) / 1024));
    }  // end loop (verified)
    printf("Top %d objects: %dK  Total objects: %dK\n", n, ((sub_total + 1023) / 1024), ((total + 1023) / 1024));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=16, nargs=3, insts=16
// --------------------------------------------------------------------
void msg_ccs_online_friend(int client_no, string friend_name, string user_name) {
    /* ---- Enhanced Pseudocode ---- */
    if (is_object(find_object("/dba/cmds/intersrv/online_friend.c"))) {
        (find_object("/dba/cmds/intersrv/online_friend.c"))->main(client_no, friend_name, user_name);
    }

}

// --------------------------------------------------------------------
// @code  nc=12, nargs=3, insts=12
// --------------------------------------------------------------------
mixed mysql_select_array(int cno, string sql_cmd, int max_row) {
    /* ---- Enhanced Pseudocode ---- */
    return mixed_mysql_select(cno, sql_cmd, (: callback_mysql_select_array :), max_row);

}

// --------------------------------------------------------------------
// @code  nc=19, nargs=2, insts=19
// --------------------------------------------------------------------
object reload_object(string path, mapping para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ob;

    if (is_object(ob = find_object(path))) {
        destruct_object(ob);
    }
    return load_object(path, para);

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=3, insts=4
// --------------------------------------------------------------------
void msg_xgame_oper(int client_no, string cmd, array args) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @code  nc=49, nargs=2, insts=49
// --------------------------------------------------------------------
int pp_start_profiling_ex(int period, int duration) {
    /* ---- Enhanced Pseudocode ---- */
    if (((duration <= 0) || (period <= 0))) {
        printf("Please set right paras\n");
        return 0;
    }
    if (pp_is_profiling()) {
        printf("pp is busing now, please first stop it by pp_stop_profiling\n");
        return 0;
    }
    if (get_timer("pp_profiling_stat")) {
        delete_timer("pp_profiling_stat");
        return 0;
    }
    set_timer(duration, "pp_profiling_stat", (: _timer_pp_stop_profiling :));
    pp_start_profiling(period);
    return 1;

}

// --------------------------------------------------------------------
// @code  nc=24, nargs=1, insts=24
// --------------------------------------------------------------------
void post_heart_beat(object me) {
    /* ---- Enhanced Pseudocode ---- */
    mixed f;

    loop (f in _heart_beat_funcs) {
        if ((!is_function(f))) {
            continue;
        }
        (*f)(me);
    }  // end loop (verified)
    clean_array(_heart_beat_funcs, 9);

}

// --------------------------------------------------------------------
// @code  nc=20, nargs=1, insts=20
// --------------------------------------------------------------------
void xdbc_enable_log_trace(int enabled) {
    /* ---- Enhanced Pseudocode ---- */
    _enable_log_trace = enabled;
    xdbc_trace(1, ((enabled ? "Enabled" : "Disabled") + " trace xdbc log\n"));

}

// --------------------------------------------------------------------
// @code  nc=25, nargs=1, insts=25
// --------------------------------------------------------------------
void xdbc_free_client(int client_no) {
    /* ---- Enhanced Pseudocode ---- */
    mapping client;

    if ((!is_mapping(client = xdbc_get_client(client_no)))) {
        return;
    }
    close_socket(client["fd"]);
    _xdbc_clients[client["id"]] = 0;

}

// --------------------------------------------------------------------
// @code  nc=28, nargs=1, insts=28
// --------------------------------------------------------------------
void post_enter_world_failed(object me) {
    /* ---- Enhanced Pseudocode ---- */
    mixed mks;
    mixed f;

    loop (mks in _enter_world_failed_funcs) {
        if ((!is_function(f = mks[1]))) {
            continue;
        }
        (*f)(me);
    }  // end loop (verified)
    clean_mapping_by_value(_enter_world_failed_funcs, 9);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=11, nargs=2, insts=11
// --------------------------------------------------------------------
void delete_dynamic_key(object ob, string key_index) {
    /* ---- Enhanced Pseudocode ---- */
    ob->delete_temp(("dynamic_key/" + key_index));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=16, nargs=4, insts=16
// --------------------------------------------------------------------
void msg_cfg_tts_info(int client_no, string ip, int port, string name) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/configd.c"->set_config("tts_info", ([ "ip": ip, "port": port, "name": name ]));

}

// --------------------------------------------------------------------
// @code  nc=55, nargs=1, insts=55
// --------------------------------------------------------------------
string convert_complex_data_to_string(mapping complex_data) {
    /* ---- Enhanced Pseudocode ---- */
    string str_data;
    string str_group;
    int group_name;
    int key;
    mixed data;

    str_data = "";
    loop (group_name in keys(complex_data)) {
        data = complex_data[group_name];
        str_group = "";
        loop (key in keys(data)) {
            str_group += sprintf("%s=%s, ", key, data[key]);
        }  // end loop (verified)
        str_data += sprintf("%s:[%s]\n", group_name, str_group[0..<3]);
    }  // end loop (verified)
    return str_data;

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=1, insts=9
// --------------------------------------------------------------------
void register_post_new_connection(function f) {
    /* ---- Enhanced Pseudocode ---- */
    _new_connection_funcs += ({ f });

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
private mixed callback_mysql_select(int vno) {
    /* ---- Enhanced Pseudocode ---- */
    return (*f_this_record)(vno);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=18, nargs=5, insts=18
// --------------------------------------------------------------------
void msg_ccs_friend_tell(int client_no, string from, string to, string msg, int msg_time) {
    /* ---- Enhanced Pseudocode ---- */
    if (is_object(find_object("/dba/cmds/intersrv/friend_tell.c"))) {
        (find_object("/dba/cmds/intersrv/friend_tell.c"))->main(client_no, from, to, msg, msg_time);
    }

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=25, nargs=2, insts=25
// --------------------------------------------------------------------
int is_same_day(int ti1, int ti2) {
    /* ---- Enhanced Pseudocode ---- */
    if ((abs((ti1 - ti2)) > 86400)) {
        return 0;
    }
    return (((ti1 + 28800) / 86400) == ((ti2 + 28800) / 86400));

}

// --------------------------------------------------------------------
// @code  nc=54, nargs=1, insts=54
// --------------------------------------------------------------------
private void xdbc_update_output_file(mixed type) {
    /* ---- Enhanced Pseudocode ---- */
    mixed all_types;
    int str;

    delete_timer("tXDBCoutputfile");
    if (is_string(type)) {
        all_types = ({ type });
    } else {
        all_types = keys(_xdbc_output_file);
    }
    loop (str in all_types) {
        _xdbc_output_file[str] = (((((_xdbc_output_basic_file + "_") + ctime(time(), 2)) + "_") + _xdbc_output_file_suffix) + ".log");
    }  // end loop (verified)
    set_timer((86400 - (time() % 86400)), "tXDBCoutputfile", (: xdbc_update_output_file :));

}

// --------------------------------------------------------------------
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
private void _timer_pp_stop_profiling() {
    /* ---- Enhanced Pseudocode ---- */
    pp_stop_profiling_ex();

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=17, nargs=4, insts=17
// --------------------------------------------------------------------
void msg_ccs_friend_status(int client_no, string friend_name, string user_name, int type) {
    /* ---- Enhanced Pseudocode ---- */
    if (is_object(find_object("/dba/cmds/intersrv/friend_status.c"))) {
        (find_object("/dba/cmds/intersrv/friend_status.c"))->main(client_no, friend_name, user_name, type);
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=116, nargs=1, insts=116
// --------------------------------------------------------------------
void save_all(mixed ti_stat) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    mixed ti;
    int ti_add;
    mixed obs;
    int file;

    set_nohalt();
    broadcast("[1;31mVM is going to save all objects.\n[2;37;0m");
    obs = objects();
    i = 0;

    // --- label L0015 ---
    while ((i < sizeof(obs))) {
        if ((!is_object(obs[i]))) {
        } else if ((!is_function(get_obj_function(obs[i], "save")))) {
        } else {
            ti = tick();
            try {
                (obs[i])->save();
            } catch { /* error handled */ }

            // --- label L0051 ---
            if (is_mapping(ti_stat)) {
                ti_add = (tick() - ti);
                file = object_name(obs[i], 1);
                if (is_int(ti_stat[file])) {
                    ti_stat[file] += ((ti_add < 0) ? (ti_add + 2147483647) : ti_add);
                } else {
                    ti_stat[file] = ((ti_add < 0) ? (ti_add + 2147483647) : ti_add);
                }
            }
        }
        ++i;
    }  // end loop (verified)
    broadcast("[1;37mAll objects are saved.\n[2;37;0m");

}

// --------------------------------------------------------------------
// @code  nc=30, nargs=1, insts=30
// --------------------------------------------------------------------
void post_leave_world(object me) {
    /* ---- Enhanced Pseudocode ---- */
    mixed f;

    loop (f in _leave_world_funcs) {
        if ((!is_function(f))) {
            continue;
        }
        try {
            (*f)(me);
        } catch { /* error handled */ }

        // --- label L0021 ---
    }  // end loop (verified)
    clean_array(_leave_world_funcs, 9);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=43, nargs=0, insts=43
// --------------------------------------------------------------------
array get_server_list() {
    /* ---- Enhanced Pseudocode ---- */
    mixed server_list;
    mixed result;
    int i;
    mixed sz;

    server_list = "/daemons/configd.c"->get_config("game_server_list");
    result = allocate_array(sz = sizeof(server_list));
    for (i = 0; i <= (sz - 1); i++) {
        result[i] = server_list[i]["name"];
    }  // end loop (verified)
    return result;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=280, nargs=2, insts=280
// --------------------------------------------------------------------
mapping get_transfer_log_info(mixed me, mixed to) {
    /* ---- Enhanced Pseudocode ---- */
    mixed pos_x;
    mixed pos_y;
    mixed dir;
    mixed room;
    mapping m;

    m = ([ "account_from": "", "account_to": "", "gid_from": "", "gid_to": "", "mac_from": "", "mac_to": "", "ip_from": "", "ip_to": "", "locality_from": "", "locality_to": "", "lv_from": 0, "lv_to": 0, "gold_coin_from": 0, "gold_coin_to": 0 ]);
    if (is_object(me)) {
        m["account_from"] = (string)me->query("account");
        m["gid_from"] = (string)me->query("gid");
        m["mac_from"] = (string)me->query_temp("login_info/mac");
        m["ip_from"] = (string)me->query_temp("login_ip");
        m["lv_from"] = (int)me->query("level");
        m["gold_coin_from"] = (int)me->query("gold_coin");
        room = env_object(me);
        pos_x = (pos_y = -1);
        me->get_pos(&pos_x, &pos_y, &dir);
        if (is_object(room)) {
            m["locality_from"] = (((((string)room->query("map_id") + ",") + (int)pos_x) + ",") + (int)pos_y);
        }
    } else if (is_mapping(me)) {
        m << me;
    } else {
        m["gid_from"] = (string)me;
    }
    if (is_object(to)) {
        m["account_to"] = (string)to->query("account");
        m["gid_to"] = (string)to->query("gid");
        m["mac_to"] = (string)to->query_temp("login_info/mac");
        m["ip_to"] = (string)to->query_temp("login_ip");
        m["lv_to"] = (int)to->query("level");
        m["gold_coin_to"] = (int)to->query("gold_coin");
        room = env_object(to);
        pos_x = (pos_y = -1);
        to->get_pos(&pos_x, &pos_y, &dir);
        if (is_object(room)) {
            m["locality_to"] = (((((string)room->query("map_id") + ",") + (int)pos_x) + ",") + (int)pos_y);
        }
    } else if (is_mapping(to)) {
        m << to;
    } else {
        m["gid_to"] = (string)to;
    }
    return m;

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=1, insts=9
// --------------------------------------------------------------------
void register_prepare_switch_server(function f) {
    /* ---- Enhanced Pseudocode ---- */
    _prepare_switch_server_funcs += ({ f });

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=1, insts=9
// --------------------------------------------------------------------
void register_child_leave_world(function f) {
    /* ---- Enhanced Pseudocode ---- */
    _child_leave_world_funcs += ({ f });

}

// --------------------------------------------------------------------
// @code  nc=7, nargs=0, insts=7
// --------------------------------------------------------------------
void init_debug_cmds_module() {
    /* ---- Enhanced Pseudocode ---- */
    printf("Done of init_debug_cmds_module.\n");

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=52, nargs=1, insts=52
// --------------------------------------------------------------------
int is_leap_year(mixed t) {
    /* ---- Enhanced Pseudocode ---- */
    mixed year;
    mapping m;

    if (is_string(t)) {
        t = time_desc_to_int(t);
    }
    m = (is_int(t) ? local_time(t) : local_time());
    year = m["year"];
    if (((((year % 4) == 0) && ((year % 100) != 0)) || ((year % 400) == 0))) {
        return 1;
    }
    return 0;

}

// --------------------------------------------------------------------
// @code  nc=65, nargs=2, insts=65
// --------------------------------------------------------------------
int xdbc_send_command(int client_no, mapping para) {
    /* ---- Enhanced Pseudocode ---- */
    mapping client;
    mixed buf;

    if ((!is_mapping(client = xdbc_get_client(client_no)))) {
        return -1;
    }
    if ((!para["cmd"])) {
        return -1;
    }
    xdbc_trace(0, "Client(%d) send command %s with key %O.\n", client_no, para["cmd"], client["key"]);
    buf = bzip_compress(save_buffer(para));
    if (client["key"]) {
        buf = xdbc_encrypt_data(buf, client["key"]);
    }
    send(client["fd"], ((buffer)sizeof(buf)[0..3] + buf));
    return 0;

}

// --------------------------------------------------------------------
// @code  nc=142, nargs=1, insts=142
// --------------------------------------------------------------------
mixed convert_to_value(string attrib) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    mixed arr;
    mixed matter;
    mixed str;

    attrib = trim(attrib);
    if ((!strlen(attrib))) {
        error("Got empty value.\n");
    }
    if (ischinese(attrib[0])) {
        return attrib;
    }
    if (isalpha(attrib[0])) {
        if ((!is_string(str = expand_macro(attrib)))) {
            printf("[1;31mFailed to expand macro: %O, use raw data\n[2;37;0m", attrib);
            str = attrib;
        }
        attrib = trim(str);
        if ((attrib[0..1] == "$$")) {
            attrib = sprintf("\"%s\"", get_resource_string(atoi(attrib[2..<1])));
        }
    }
    if ((sscanf(attrib, "%*s({%s})%*s", &str) >= 2)) {
        matter = ({});
        if ((strlen(trim(str)) != 0)) {
            arr = explode(str, "|");
            loop (str in arr) {
                matter += ({ convert_to_value(str) });
            }  // end loop (verified)
        }
        return matter;
    }
    if ((sscanf(attrib, "\"%s\"", &str) == 1)) {
        return str;
    }
    for (i = 0; i <= (strlen(attrib) - 1); i++) {
        if ((!ischinese(attrib[i]))) {
            continue;
        }
        return attrib;
    }  // end loop (verified)
    return atoi(attrib);

}

// --------------------------------------------------------------------
// @code  nc=42, nargs=1, insts=42
// --------------------------------------------------------------------
override nomask void telnet_new_client(int fd) {
    /* ---- Enhanced Pseudocode ---- */
    int enable_log;
    mixed type;

    enable_log = 1;
    if ((is_string(type = get_telnet_settings()["type"]) && (strsrch(type, "L") == -1))) {
        printf("Log of telnet is disabled.\n");
        enable_log = 0;
    }
    if (enable_log) {
        create_thread(0, (: telnet_daemon, fd :));
    } else {
        start_buildin_telnet_shell(fd);
    }

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=1, insts=9
// --------------------------------------------------------------------
void register_post_internal_comm_close(function f) {
    /* ---- Enhanced Pseudocode ---- */
    _internal_comm_close_funcs += ({ f });

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=14, nargs=5, insts=14
// --------------------------------------------------------------------
void msg_l_query_ip_area(int client_no, string account, string digital_ip, string module, string result) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_query_ip_area.c"))->main(client_no, account, digital_ip, module, result);

}

// --------------------------------------------------------------------
// @code  nc=7, nargs=0, insts=7
// --------------------------------------------------------------------
void init_cmds_module() {
    /* ---- Enhanced Pseudocode ---- */
    printf("Done of init_cmds_module.\n");

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=2, insts=10
// --------------------------------------------------------------------
void msg_cfg_aaa_list(int client_no, mapping m) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/configd.c"->set_config("aaa_info", m);

}

// --------------------------------------------------------------------
// @code  nc=25, nargs=1, insts=25
// --------------------------------------------------------------------
string cpu_cost(float r) {
    /* ---- Enhanced Pseudocode ---- */
    if ((r >= 1)) {
        return "100";
    }
    return sprintf("%d.%d", (int)(r * 100), ((int)((r * 1000) + 0.5) % 10));

}

// --------------------------------------------------------------------
// @code  nc=18, nargs=1, insts=18
// --------------------------------------------------------------------
mixed xdbc_field_name(int vno) {
    /* ---- Enhanced Pseudocode ---- */
    xdbc_trace(0, "Call xdbc_field_name(%d)\n", vno);
    return xdbc_call_server(xdbc_get_cno_of_view(vno), ([ "cmd": "field_name", "vno": vno ]));

}

// --------------------------------------------------------------------
// @code  nc=48, nargs=1, insts=48
// --------------------------------------------------------------------
void compile_all(string path) {
    /* ---- Enhanced Pseudocode ---- */
    mixed files;
    int i;

    if ((path[(strlen(path) - 1)] != 47)) {
        path += "/";
    }
    files = get_dir_ex(path);
    if ((!files)) {
        return;
    }
    i = 0;

    // --- label L0028 ---
    while ((i < sizeof(files))) {
        safe_compile_program((path + files[i]));
        ++i;
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=12, nargs=3, insts=12
// --------------------------------------------------------------------
void msg_l_query_addiction(int client_no, string account, string result_text) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_query_addiction.c"))->main(client_no, account, result_text);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=19, nargs=10, insts=19
// --------------------------------------------------------------------
void msg_l_precost_coin(int client_no, string gid, string account, string aaa_cookie, int result, string module, string para, string coin_password, int send_time, int prc_cookie) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_precost_coin.c"))->main(client_no, gid, account, aaa_cookie, result, module, para, coin_password, send_time, prc_cookie);

}

// --------------------------------------------------------------------
// @code  nc=89, nargs=0, insts=89
// --------------------------------------------------------------------
void size_global() {
    /* ---- Enhanced Pseudocode ---- */
    mixed m;
    int name;
    mapping s;
    mapping summary;
    mapping detail;

    printf("%-28s - mapping  array  string  buffer  value  memory\n", "Variable name");
    m = get_all_global_vars();
    loop (name in sort(keys(m))) {
        s = size_value(m[name]);
        detail = s["detail"];
        summary = s["summary"];
        if (((!is_mapping(detail)) || (!is_mapping(summary)))) {
            continue;
        }
        printf("%-28s - %7d  %5d  %6d  %6d  %5d  %5dK\n", name[<28..<1], detail["counter"]["mapping"], detail["counter"]["array"], detail["counter"]["string"], detail["counter"]["buffer"], detail["counter"]["value"], ((summary["memory_cost"] + 1023) / 1024));
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  query: database retrieval
// @code  nc=82, nargs=2, insts=82
// --------------------------------------------------------------------
string select_server_ip(mixed network_type, mapping server_info) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ip;

    switch (network_type) {
        case 1:
        if (((is_string(server_info["ip_wt"]) && strlen(server_info["ip_wt"])) && (int)server_info["enable_multi_ip"])) {
            ip = server_info["ip_wt"];
        } else {
            ip = server_info["ip"];
        }
        break;
        case 2:
        if (((is_string(server_info["ip_dx"]) && strlen(server_info["ip_dx"])) && (int)server_info["enable_multi_ip"])) {
            ip = server_info["ip_dx"];
        } else {
            ip = server_info["ip"];
        }
        break;
        default:
        ip = server_info["ip"];
        break;
    }  // end switch

    // --- label L0078 ---
    return ip;

}

// --------------------------------------------------------------------
// @code  nc=41, nargs=1, insts=41
// --------------------------------------------------------------------
private void safe_compile_program(string file) {
    /* ---- Enhanced Pseudocode ---- */
    mixed depend_files;
    mixed depend_file;

    if (is_compiled_object(file)) {
        return;
    }
    if (is_array(depend_files = file_dependence[file])) {
        loop (depend_file in depend_files) {
            if ((!is_compiled_object(depend_file))) {
                safe_compile_program(depend_file);
            }
        }  // end loop (verified)
    }
    if (compile_program(file)) {
        printf("Compile: %s... OK\n", file);
    } else {
        printf("Compile: %s... Failed\n", file);
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=16, nargs=7, insts=16
// --------------------------------------------------------------------
void msg_l_change_coupon(int client_no, string account, string gid, int result, int cost_once, string para, string cookie) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_change_coupon.c"))->main(client_no, account, gid, result, cost_once, para, cookie);

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=2, insts=4
// --------------------------------------------------------------------
void cmd_l_when_block_account(int client_no, string account) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @code  nc=28, nargs=2, insts=28
// --------------------------------------------------------------------
private int valid_ban_words_by_group(string content, string group) {
    /* ---- Enhanced Pseudocode ---- */
    mixed match_ids;
    mixed match_id;

    if ((!is_array(match_ids = ban_words_match_id[group]))) {
        return 0;
    }
    loop (match_id in match_ids) {
        if (find_illegal_string(match_id, content)) {
            return 1;
        }
    }  // end loop (verified)
    return 0;

}

// --------------------------------------------------------------------
// @code  nc=72, nargs=0, insts=72
// --------------------------------------------------------------------
mixed xdbc_get_info() {
    /* ---- Enhanced Pseudocode ---- */
    printf("mysql_connect = %O\n", f_connect);
    printf("mysql_disconnect = %O\n", f_disconnect);
    printf("mysql_is_connected = %O\n", f_is_connected);
    printf("mysql_get_error = %O\n", f_get_error);
    printf("mysql_execute = %O\n", f_execute);
    printf("mysql_query = %O\n", f_query);
    printf("mysql_finalize = %O\n", f_finalize);
    printf("mysql_num_records = %O\n", f_num_records);
    printf("mysql_fetch_record = %O\n", f_fetch_record);
    printf("mysql_this_record = %O\n", f_this_record);
    printf("mysql_field_value = %O\n", f_field_value);
    printf("mysql_set_auto_commit = %O\n", f_set_auto_commit);
    printf("mysql_select_db = %O\n", f_select_db);
    printf("mysql_field_name = %O\n", f_field_name);
    printf("mysql_num_fields = %O\n", f_num_fields);
    printf("mysql_int_field = %O\n", f_int_field);
    printf("mysql_get_insert_id = %O\n", f_get_insert_id);
    return;

}

// --------------------------------------------------------------------
// @code  nc=50, nargs=2, insts=50
// --------------------------------------------------------------------
string xor_decode(string str, int p) {
    /* ---- Enhanced Pseudocode ---- */
    int i;
    int length;
    mixed arr;

    length = strlen(str);
    if ((length < 1)) {
        return str;
    }
    arr = allocate_array(length);
    for (i = 0; i <= (length - 1); i++) {
        arr[i] = sprintf("%c", (str[i] ^ p));
    }  // end loop (verified)
    return implode(arr, "");

}

// --------------------------------------------------------------------
// @code  nc=159, nargs=1, insts=159
// --------------------------------------------------------------------
void size_all(string path) {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr;
    mapping s;
    mapping summary;
    mapping detail;
    mixed file;
    int total;
    mixed ob;

    if ((path == "")) {
        printf("Usage: size_all path.\n");
        return;
    }
    if (is_object(ob = find_object(path))) {
        arr = ({ object_name(ob) });
    } else {
        arr = get_dir_ex((path + "/*"));
        if (((!is_array(arr)) || (!sizeof(arr)))) {
            printf("No object to stat for size, please try another path.\n");
            return;
        }
    }
    total = 0;
    printf("%-28s - mapping  array  string  buffer  value  memory\n", "Object name");
    loop (file in sort(arr)) {
        if ((!is_object(ob = find_object(((path + "/") + file))))) {
            continue;
        }
        s = size_object(ob);
        detail = s["detail"];
        summary = s["summary"];
        if (((!is_mapping(detail)) || (!is_mapping(summary)))) {
            continue;
        }
        total += summary["memory_cost"];
        printf("%-28s - %7d  %5d  %6d  %6d  %5d  %5dK\n", file[<28..<1], detail["counter"]["mapping"], detail["counter"]["array"], detail["counter"]["string"], detail["counter"]["buffer"], detail["counter"]["value"], ((summary["memory_cost"] + 1023) / 1024));
    }  // end loop (verified)
    printf("Total: %dK\n", ((total + 1023) / 1024));

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=14, nargs=5, insts=14
// --------------------------------------------------------------------
void msg_l_verify_password(int client_no, string account, string gid, string type, int result) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_verify_password.c"))->main(client_no, account, gid, type, result);

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
void remove_client_agent(int client_no) {
    /* ---- Enhanced Pseudocode ---- */
    map_delete(client_agents, client_no);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=73, nargs=1, insts=73
// --------------------------------------------------------------------
mixed get_dir_ex(string path) {
    /* ---- Enhanced Pseudocode ---- */
    mixed ks_pak;
    mixed ks;
    mixed m;
    int file_name;

    m = ([ ]);
    ks_pak = get_dir(((lib_pak_path + path) + "*.o"));
    if (is_array(ks_pak)) {
        loop (file_name in ks_pak) {
            m[(file_name[0..<3] + ".c")] = 1;
        }  // end loop (verified)
    }
    if (is_array(ks = get_dir((path + "*.c")))) {
        loop (file_name in ks) {
            m[file_name] = 1;
        }  // end loop (verified)
    }
    if (((!is_array(ks_pak)) && (!is_array(ks)))) {
        return ks;
    }
    return keys(m);

}

// --------------------------------------------------------------------
// @code  nc=86, nargs=2, insts=86
// --------------------------------------------------------------------
void load_all_path(string path, mapping para) {
    /* ---- Enhanced Pseudocode ---- */
    mixed files;
    mixed name;
    int i;

    if ((path[(strlen(path) - 1)] != 47)) {
        path += "/";
    }
    load_all(path, para);
    files = get_sub_dirs_ex(path);
    i = 0;

    // --- label L0030 ---
    while ((i < sizeof(files))) {
        name = files[i][0];
        if (((name == ".") || (name == ".."))) {
        } else if ((files[i][1] == -2)) {
            printf("Load sub dir: %s\n", (path + name));
            load_all_path((path + name), para);
            sleep(0);
        }

        // --- label L0078 ---
        ++i;
    }  // end loop (verified)
    return ([ ]);

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=3, insts=4
// --------------------------------------------------------------------
void msg_got_server_time(int client_no, string ccs_name, int t) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=88, nargs=2, insts=88
// --------------------------------------------------------------------
mixed get_new_iid_by_old(mixed old_iid, int dist_id) {
    /* ---- Enhanced Pseudocode ---- */
    mixed size;

    if (is_buffer(old_iid)) {
        if ((sizeof(old_iid) != 8)) {
            error("the length of old buffer iid must be 8.");
        }
        return ((old_iid[0..3] + make16(dist_id)) + old_iid[4..<1]);
    }
    if ((!is_string(old_iid))) {
        error("old_iid type must be string or buffer.");
    }
    size = sizeof(old_iid);
    if ((16 == size)) {
        return ((old_iid[0..7] + save_string(make16(dist_id))[1..<2]) + old_iid[8..<1]);
    }
    if ((18 == size)) {
        return ((old_iid[0..8] + save_string(make16(dist_id))[1..<2]) + old_iid[9..<1]);
    }
    error("the length of old string iid must be 16 or 18.");
    return;

}

// --------------------------------------------------------------------
// @code  nc=298, nargs=4, insts=298
// --------------------------------------------------------------------
string convert_mapping_to_sql_cmd(string op, string table, mapping m, mixed key) {
    /* ---- Enhanced Pseudocode ---- */
    int index;
    int i;
    mixed arr;
    mixed key_list;
    mixed value_list;
    mixed q_list;
    mixed set_list;
    mixed cond_list;
    mixed sql_cmd;
    int field;

    switch (op) {
        case "insert":
        case "replace":
        key_list = allocate_array(sizeof(m));
        value_list = allocate_array(sizeof(m));
        q_list = allocate_array(sizeof(m));
        index = 0;
        loop (arr in m) {
            key_list[index] = arr[0];
            q_list[index] = "?";
            value_list[index] = arr[1];
            ++index;
        }  // end loop (verified)
        sql_cmd = sprintf("%s into %s (%s) values (%s)", op, table, implode(key_list, ","), implode(q_list, ","));
        return mysql_bind_parameters_arr(sql_cmd, value_list);
        case "update":
        if ((!sizeof(key))) {
            return "";
        }
        switch (var_type(key)) {
            case "string":
            set_list = allocate_array(sizeof(m));
            value_list = allocate_array((sizeof(m) + 1));
            cond_list = allocate_array(1);
            break;
            case "array":
            set_list = allocate_array(sizeof(m));
            value_list = allocate_array((sizeof(m) + sizeof(key)));
            cond_list = allocate_array(sizeof(key));
            break;
            default:
            return "";
        }  // end switch

        // --- label L0141 ---
        index = 0;
        loop (arr in m) {
            set_list[index] = sprintf("%s=?", (string)arr[0]);
            value_list[index] = arr[1];
            ++index;
        }  // end loop (verified)
        i = 0;
        loop (field in (is_array(key) ? key : ({ key }))) {
            cond_list[i++] = sprintf("%s=?", field);
            value_list[index++] = m[field];
        }  // end loop (verified)
        sql_cmd = sprintf("update %s set %s where %s", table, implode(set_list, " , "), implode(cond_list, " and "));
        return mysql_bind_parameters_arr(sql_cmd, value_list);
        case "delete":
        cond_list = allocate_array(sizeof(m));
        value_list = allocate_array(sizeof(m));
        index = 0;
        loop (arr in m) {
            cond_list[index] = sprintf("%s=?", (string)arr[0]);
            value_list[index] = arr[1];
            ++index;
        }  // end loop (verified)
        sql_cmd = sprintf("delete from %s where %s", table, implode(cond_list, " and "));
        return mysql_bind_parameters_arr(sql_cmd, value_list);
        default:
        return "";
    }  // end switch

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=3, insts=4
// --------------------------------------------------------------------
void msg_celebrity_oper(int client_no, string cmd, array args) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @code  nc=13, nargs=1, insts=13
// --------------------------------------------------------------------
string time_to_string(mixed t) {
    /* ---- Enhanced Pseudocode ---- */
    return ctime((is_int(t) ? t : time()), 3);

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=32, nargs=0, insts=32
// --------------------------------------------------------------------
int is_test() {
    /* ---- Enhanced Pseudocode ---- */
    mixed dist;

    if ((is_test_dist_flag != -1)) {
        return is_test_dist_flag;
    }
    while ((!is_string(dist = "/daemons/configd.c"->get_config("my_dist")))) {
        printf("ªπ√ª”–¥” CONFIG_D »°µ√∑˛ŒÒ∆˜À˘‘⁄«¯◊È√˚£¨«Î…‘∫Ú°≠°≠\n");
        sleep(3);
    }  // end loop (verified)
    return (is_test_dist_flag = is_test_dist(dist));

}

// --------------------------------------------------------------------
// @code  nc=226, nargs=6, insts=226
// --------------------------------------------------------------------
mixed _valid_name(string name, int min_len, int max_len, int may_chinese, mixed include_group, mixed exclude_group) {
    /* ---- Enhanced Pseudocode ---- */
    string str;
    mixed group;
    mixed sz;
    int hasnt_need_word;
    mapping m;
    int i;

    m = ([ ]);
    if (((sz = strlen(name)) < min_len)) {
        m["msg"] = (("√˚◊÷≤ªƒ‹…Ÿ”⁄" + min_len) + "∏ˆ◊÷∑˚");
        m["type"] = -1;
        return m;
    }
    if ((sz > max_len)) {
        m["msg"] = (("√˚◊÷≤ªƒ‹≥¨π˝" + max_len) + "∏ˆ◊÷∑˚");
        m["type"] = -2;
        return m;
    }
    str = "";
    i = 0;

    // --- label L0063 ---
    while (i < sz) {
        if ((isdigit(name[i]) || isalpha(name[i]))) {
        } else {
            if ((!may_chinese)) {
                m["msg"] = "√˚◊÷±ÿ–Î”…”¢Œƒ◊÷ƒ∏ªÚ∞¢¿≠≤Æ ˝◊÷◊È≥…";
                m["type"] = -4;
                return m;
            }
            str += name[i..(i + 1)];
            ++i;
        }
        ++i;
    }  // end loop (verified)
    if ((!include_group)) {
        include_group = ({});
    }
    if ((!exclude_group)) {
        exclude_group = keys(ban_words);
    }
    hasnt_need_word = 1;
    if ((!sizeof(include_group))) {
        hasnt_need_word = 0;
    }
    loop (group in include_group) {
        if (valid_ban_words_by_group(name, group)) {
            hasnt_need_word = 0;
            break;
        }
    }  // end loop (verified)
    if (hasnt_need_word) {
        m["msg"] = "ƒ„ ‰»Îµƒ√˚◊÷÷–»±…Ÿ±ÿ∫¨◊÷∑˚£¨«Î÷ÿ–¬—°‘Ò√˚◊÷";
        m["type"] = -5;
        return m;
    }
    name = tolower_temp(name);
    loop (group in exclude_group) {
        if ((!valid_ban_words_by_group(name, group))) {
            continue;
        }
        m["msg"] = "ƒ„ ‰»Îµƒ√˚◊÷÷–∞¸∫¨√Ù∏–◊÷∑˚£¨«Î÷ÿ–¬—°‘Ò√˚◊÷";
        m["type"] = -6;
        return m;
    }  // end loop (verified)
    m["msg"] = 0;
    m["type"] = 0;
    return m;

}

// --------------------------------------------------------------------
// @brief  predicate: boolean check
// @code  nc=19, nargs=1, insts=19
// --------------------------------------------------------------------
private int is_need_excluded(string cmd_name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed map;

    if ((!is_mapping(map = need_exclude_cmds[4]))) {
        return 0;
    }
    return (int)map[cmd_name];

}

// --------------------------------------------------------------------
// @code  nc=121, nargs=2, insts=121
// --------------------------------------------------------------------
int xdbc_wait_command(int client_no, mixed &para) {
    /* ---- Enhanced Pseudocode ---- */
    mapping client;
    mixed buf;
    int len;

    if ((!is_mapping(client = xdbc_get_client(client_no)))) {
        return -1;
    }
    len = (int)xdbc_pend_read(client["fd"], 4);
    if ((len <= 0)) {
        xdbc_log(2, "Failed to read message header from client(%d), len = %d.\n", client_no, len);
        return -1;
    }
    buf = xdbc_pend_read(client["fd"], len);
    if ((!buf)) {
        xdbc_log(2, "Failed to read message body from client(%d).\n", client_no);
        return -1;
    }
    if (client["key"]) {
        buf = xdbc_decrypt_data(buf, client["key"]);
    }
    try {
        buf = bzip_decompress(buf);
    } catch { /* error handled */ }

    // --- label L0074 ---
    try {
        para = restore_value(buf);
    } catch { /* error handled */ }

    // --- label L0085 ---
    if (((0 || (!is_mapping(para))) || (!is_string(para["cmd"])))) {
        xdbc_log(2, "Bad message from client %d.\n", client_no);
        return -1;
    }
    xdbc_trace(0, "Client(%d) got command %s with key %O\n", client_no, para["cmd"], client["key"]);
    return 0;

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=3, insts=4
// --------------------------------------------------------------------
void msg_draft_oper(int client_no, string cmd, array args) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @code  nc=228, nargs=2, insts=228
// --------------------------------------------------------------------
void load_comm_desc(function callback, int encap_type) {
    /* ---- Enhanced Pseudocode ---- */
    mixed file;
    mixed line;
    mixed attrib;
    mixed cmd_filter;
    int cmd_type;
    int cmd_name;
    int key;
    mixed attrib_map;
    int line_no;
    int interface_flag;

    if ((!is_string(file = read_etc_file("comm_desc.txt")))) {
        return;
    }
    line_no = 0;
    interface_flag = 0;
    loop (line in explode(file)) {
        ++line_no;
        if ((!strlen(line = trim(line)))) {
            continue;
        }
        if ((line[0] == 35)) {
            continue;
        }
        attrib_map = ([ ]);
        if ((sscanf(line, "%s::%s", &line, &attrib) == 2)) {
            loop (key in explode(attrib, "/")) {
                if (strlen(key = trim(key))) {
                    attrib_map[key] = 1;
                }
            }  // end loop (verified)
        }
        if ((sscanf(line, "[%s]", &cmd_filter) == 1)) {
            interface_flag = 0;
            loop (cmd_type in explode(cmd_filter, "/")) {
                if ((!strlen(cmd_type = trim(cmd_type)))) {
                    continue;
                }
                if ((!_cmd_types[cmd_type])) {
                    printf("Bad cmd type: %s\nAt comm description file %s#%d\n", cmd_type, "comm_desc.txt", line_no);
                    continue;
                }
                interface_flag |= _cmd_types[cmd_type];
            }  // end loop (verified)
            continue;
        }
        if ((sscanf(line, "%s %*s", &key) != 2)) {
            printf("Bad line: syntax error.\nAt comm description file %s#%d\n", "comm_desc.txt", line_no);
            continue;
        }
        switch (key) {
            case "encap:none":
            case "encap:mapping":
            case "encap:array":
            try {
                pkt_add_def(line);
            } catch { /* error handled */ }

            // --- label L0156 ---
            continue;
            case "void":
            if ((!interface_flag)) {
                printf("Can't add interface: interface_flag no indicated yet.\nAt comm description file %s#%d\n", "comm_desc.txt", line_no);
                continue;
            }
            if ((sscanf(trim(line[4..<1]), "%s(%*s", &cmd_name) != 2)) {
                printf("Bad syntax: interface missed name.\n", "At comm description file %s#%d\n", "comm_desc.txt", line_no);
            }
            if (is_need_excluded(cmd_name)) {
                continue;
            }
            try {
                pkt_add_interface(interface_flag, line, find_function(callback, cmd_name), encap_type);
            } catch { /* error handled */ }

            // --- label L0204 ---
            if (sizeof(attrib_map)) {
                _cmd_attrib_map[cmd_name] = attrib_map;
            }
            continue;
            default:
            printf("Bad line: lead by %s.\nAt comm description file %s#%d\n", key, "comm_desc.txt", line_no);
            continue;
        }  // end switch
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=159, nargs=3, insts=159
// --------------------------------------------------------------------
mapping get_important_pet_log_info(mixed from, mixed to, mixed pet) {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;

    m = ([ "account_from": "0", "account_to": "0", "gid_from": "0", "gid_to": "0", "lv_from": 0, "lv_to": 0, "pet_iid": "0", "pet_name": "0", "pet_property": "0", "line": 1 ]);
    if (is_object(from)) {
        m["account_from"] = from->query("account");
        m["gid_from"] = from->get_gid();
        m["lv_from"] = from->query("level");
    } else if (is_mapping(from)) {
        m << from;
    } else {
        m["gid_from"] = (string)from;
    }
    if (is_object(to)) {
        m["account_to"] = to->query("account");
        m["gid_to"] = to->get_gid();
        m["lv_to"] = to->query("level");
    } else if (is_mapping(to)) {
        m << to;
    } else {
        m["gid_to"] = (string)to;
    }
    if (is_object(pet)) {
        m["pet_iid"] = save_string(pet->get_iid());
        m["pet_name"] = pet->get_raw_name();
        m["pet_property"] = pet->save_to_text();
    } else if (is_mapping(pet)) {
        m << pet;
    }
    return m;

}

// --------------------------------------------------------------------
// @code  nc=8, nargs=1, insts=8
// --------------------------------------------------------------------
void remove_gid_object(string gid) {
    /* ---- Enhanced Pseudocode ---- */
    map_delete(gid_objects, gid);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=14, nargs=5, insts=14
// --------------------------------------------------------------------
void msg_l_buy_card(int client_no, string cookie, string number, int count, int result) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_buy_card.c"))->main(client_no, cookie, number, count, result);

}

// --------------------------------------------------------------------
// @brief  converter: transform data
// @code  nc=9, nargs=2, insts=9
// --------------------------------------------------------------------
void to_log_file(string file, string log_msg) {
    /* ---- Enhanced Pseudocode ---- */
    log_file(file, "%s", log_msg);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=30, nargs=1, insts=30
// --------------------------------------------------------------------
int get_day_zero_time(int t) {
    /* ---- Enhanced Pseudocode ---- */
    mapping m;

    m = local_time(t);
    m["hour"] = 0;
    m["sec"] = 0;
    m["min"] = 0;
    return mktime(m);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=3, insts=10
// --------------------------------------------------------------------
void msg_moved_content(int clinet_no, int cookie, int result) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->msg_moved_content(cookie, result);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=17, nargs=0, insts=17
// --------------------------------------------------------------------
int get_org_time() {
    /* ---- Enhanced Pseudocode ---- */
    if ((1 && (!defined("LAB")))) {
        return time();
    }
    return (time() - server_shift_time);

}

// --------------------------------------------------------------------
// @code  nc=21, nargs=2, insts=21
// --------------------------------------------------------------------
mixed xdbc_field_value(int vno, mixed index) {
    /* ---- Enhanced Pseudocode ---- */
    xdbc_trace(0, "Call xdbc_field_value(%d, %O)\n", vno, index);
    return xdbc_call_server(xdbc_get_cno_of_view(vno), ([ "cmd": "field_value", "vno": vno, "index": index ]));

}

// --------------------------------------------------------------------
// @code  nc=93, nargs=3, insts=93
// --------------------------------------------------------------------
mixed xdbc_pend_read(int fd, int len, int timeout) {
    /* ---- Enhanced Pseudocode ---- */
    mixed buf;
    mixed temp;
    mixed rarr;
    mixed expired_time;
    int ret;

    xdbc_trace(0, "Pend read %d bytes data for fd %d, timeout = %d\n", len, fd, timeout);
    expired_time = (timeout + time());
    buf = allocate_buffer(0);

    // --- label L0022 ---
    while (len > 0) {
        rarr = ({ fd });
        if ((select(&rarr, 0, 0, ((expired_time - time()) * 1000)) < 0)) {
            xdbc_log(0, "Select read failed to fds ({ %d })\n", implode(rarr, ","));
            return 0;
        }
        xdbc_trace(0, "Select fds ({ %s }) for read\n", implode(rarr, ","));
        ret = recv(fd, &temp, len);
        xdbc_trace(0, "Recv %d bytes from fd %d\n", ret, fd);
        if ((ret <= 0)) {
            return 0;
        }
        buf += temp;
        len -= ret;
    }  // end loop (verified)
    return buf;

}

// --------------------------------------------------------------------
// @code  nc=62, nargs=1, insts=62
// --------------------------------------------------------------------
mapping convert_string_to_complex_data(string str_data) {
    /* ---- Enhanced Pseudocode ---- */
    mixed complex_data;
    mixed str_group;
    int group_name;
    int key;
    mixed value;
    mixed data;

    complex_data = ([ ]);
    loop (str_group in explode(str_data, "\n")) {
        if ((sscanf(str_group, "%s:[%s]", &group_name, &str_group) != 2)) {
            continue;
        }
        data = ([ ]);
        loop (key in explode(str_group, ", ")) {
            if ((sscanf(key, "%s=%s", &key, &value) != 2)) {
                continue;
            }
            data[key] = value;
        }  // end loop (verified)
        complex_data[group_name] = data;
    }  // end loop (verified)
    return complex_data;

}

// --------------------------------------------------------------------
// @brief  data writer: persist information
// @code  nc=26, nargs=4, insts=26
// --------------------------------------------------------------------
mixed write_content_ex(string path, string name, mixed branch, mixed content) {
    /* ---- Enhanced Pseudocode ---- */
    mixed str;
    mixed convert_type;

    convert_type = get_data_convert_type(path, name);
    str = "/daemons/datasaved.c"->convert_to_save_data(convert_type, content);
    return "/daemons/filed.c"->write_content(path, name, branch, str);

}

// --------------------------------------------------------------------
// @code  nc=177, nargs=3, insts=177
// --------------------------------------------------------------------
void create_data_from2(array arr, array &attrib_index, mapping &attrib_list) {
    /* ---- Enhanced Pseudocode ---- */
    mixed section_data;
    mixed mks;
    string section_name;
    mixed attrib;
    mixed attrib_name;
    mixed str;
    mixed matter;
    int i;

    section_name = "";
    section_data = ([ ]);
    i = 0;
    loop (mks in arr) {
        ++i;
        mks = trim(mks);
        if ((strlen(mks) <= 1)) {
            continue;
        }
        if ((mks[0] == 35)) {
            continue;
        }
        if (((mks[0] == 91) && (mks[<1] == 93))) {
            if (((strlen(section_name) > 0) && sizeof(section_data))) {
                attrib_list[section_name] = section_data;
            }
            section_name = mks[1..<2];
            section_name = trim(section_name);
            section_data = ([ ]);
            continue;
        }
        if ((sscanf(mks, "%s:%s", &attrib_name, &attrib) != 2)) {
            continue;
        }
        attrib_name = trim(attrib_name);
        if ((sscanf(attrib, "%*s({%s})%*s", &str) >= 2)) {
            matter = ({});
            if ((strlen(trim(str)) != 0)) {
                arr = explode(str, ",");
                loop (str in arr) {
                    matter += ({ convert_to_value(str) });
                }  // end loop (verified)
            }
            add_to_section(section_data, attrib_name, matter);
            continue;
        }
        add_to_section(section_data, attrib_name, convert_to_value(attrib));
    }  // end loop (verified)
    if (((strlen(section_name) > 0) && sizeof(section_data))) {
        attrib_list[section_name] = section_data;
    }
    attrib_index = keys(attrib_list);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=20, nargs=12, insts=20
// --------------------------------------------------------------------
void msg_l_add_coin(int client_no, string gid, string account, string aaa_cookie, string module, string para, int result, int add_silver_coin, int add_gold_coin, int silver_coin, int gold_coin, int limit_trade_coin) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_add_coin.c"))->main(client_no, gid, account, result, add_silver_coin, add_gold_coin, silver_coin, gold_coin, limit_trade_coin, module, para);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=16, nargs=7, insts=16
// --------------------------------------------------------------------
void msg_l_request_coupon(int client_no, string gid, string coupon_type, int result, string type, string para, string cookie) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_request_coupon.c"))->main(client_no, gid, coupon_type, result, type, para, cookie);

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=18, nargs=2, insts=18
// --------------------------------------------------------------------
string get_data_convert_type(string path, string name) {
    /* ---- Enhanced Pseudocode ---- */
    if (((path == "store") && (strlen(name) > 16))) {
        return "share_store";
    }
    return path;

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=14, nargs=5, insts=14
// --------------------------------------------------------------------
void msg_l_manage_mp_device(int client_no, string gid, int oper, int ret, string para) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_manage_mp_device.c"))->main(client_no, gid, oper, ret, para);

}

// --------------------------------------------------------------------
// @code  nc=69, nargs=1, insts=69
// --------------------------------------------------------------------
void reset_cmd_cost_stat(int client_no) {
    /* ---- Enhanced Pseudocode ---- */
    mapping _cost;
    int cmd;

    if ((!ENABLE_CMDS_STAT)) {
        return;
    }
    loop (cmd in keys(all_cmds_cost)) {
        if ((!is_mapping(_cost = all_cmds_cost[cmd]))) {
            continue;
        }
        _cost["other_count"] += _cost["count"][(client_no % 10000)];
        _cost["other_ticks"] += _cost["ticks"][(client_no % 10000)];
        _cost["count"][(client_no % 10000)] = 0;
        _cost["ticks"][(client_no % 10000)] = 0;
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @code  nc=79, nargs=1, insts=79
// --------------------------------------------------------------------
void size_objects(int output) {
    /* ---- Enhanced Pseudocode ---- */
    mixed m;
    int name;
    mixed ob;

    m = ([ ]);
    loop (ob in objects()) {
        if ((!is_object(ob))) {
            continue;
        }
        name = object_name(ob, 1);
        m[name] = ((int)m[name] + size_object(ob)["summary"]["memory_cost"]);
    }  // end loop (verified)
    printf("Summary of memory usage.\n");
    loop (name in sort(keys(m), (: size_objects_get_usage, m :), -1)) {
        printf("%-40s %dK\n", name, ((m[name] + 512) / 1024));
        --output;
        if ((output <= 0)) {
            break;
        }
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=15, nargs=6, insts=15
// --------------------------------------------------------------------
void msg_l_query_citys(int client_no, string gid, string module, string country, string province, string citys) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_query_citys.c"))->main(client_no, gid, module, country, province, citys);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=14, nargs=5, insts=14
// --------------------------------------------------------------------
void msg_l_update_recharge_return_credit(int client_no, string account, string gid, int total_coin, int max_bonus_id) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_update_recharge_return_credit.c"))->main(client_no, account, gid, total_coin, max_bonus_id);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=11, nargs=4, insts=11
// --------------------------------------------------------------------
void msg_got_gid_arr(int client_no, int cookie, int result, string content) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->msg_got_gid_arr(cookie, result, content);

}

// --------------------------------------------------------------------
// @code  nc=4, nargs=3, insts=4
// --------------------------------------------------------------------
void msg_treasure_trading_oper(int client_no, string cmd, array args) {
    /* ---- Enhanced Pseudocode ---- */
    // (no statements)

}

// --------------------------------------------------------------------
// @code  nc=21, nargs=2, insts=21
// --------------------------------------------------------------------
mixed sql_get_table(mixed db, string table_name) {
    /* ---- Enhanced Pseudocode ---- */
    mixed arr;

    arr = sql_select(db, sprintf("pragma table_info (%s)", table_name), 99);
    if ((!sizeof(arr))) {
        return 0;
    }
    return arr;

}

// --------------------------------------------------------------------
// @brief  accessor: retrieve value
// @code  nc=25, nargs=1, insts=25
// --------------------------------------------------------------------
int get_weeks_in_year(int ti) {
    /* ---- Enhanced Pseudocode ---- */
    mapping time_map;

    time_map = local_time(ti);
    return ((int)ceil(((1 * (time_map["yday"] - time_map["ch_wday"])) / 7)) + 1);

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=1, insts=9
// --------------------------------------------------------------------
void register_just_login_server(function f) {
    /* ---- Enhanced Pseudocode ---- */
    _just_login_server_funcs += ({ f });

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=12, nargs=3, insts=12
// --------------------------------------------------------------------
void msg_l_query_account_state(int client_no, string account, string state) {
    /* ---- Enhanced Pseudocode ---- */
    (find_object("/dba/cmds/intersrv/msg_l_query_account_state.c"))->main(client_no, account, state);

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=1, insts=9
// --------------------------------------------------------------------
void register_leave_world(function f) {
    /* ---- Enhanced Pseudocode ---- */
    _leave_world_funcs += ({ f });

}

// --------------------------------------------------------------------
// @code  nc=12, nargs=2, insts=12
// --------------------------------------------------------------------
private int _size_max_get_memory_cost(mapping m, string name) {
    /* ---- Enhanced Pseudocode ---- */
    return m[name]["summary"]["memory_cost"];

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=19, nargs=6, insts=19
// --------------------------------------------------------------------
void msg_gs_emote_data(int client_no, string verb, string target, int channel, string peer, string self_info) {
    /* ---- Enhanced Pseudocode ---- */
    if (is_object(find_object("/dba/cmds/intersrv/emote_data.c"))) {
        (find_object("/dba/cmds/intersrv/emote_data.c"))->main(client_no, verb, target, channel, peer, self_info);
    }

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=2, insts=10
// --------------------------------------------------------------------
void cmd_cta_to_server(int client_no, buffer buf) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/ctacommandd.c"->cta_command(client_no, buf);

}

// --------------------------------------------------------------------
// @code  nc=9, nargs=1, insts=9
// --------------------------------------------------------------------
void register_post_init(function f) {
    /* ---- Enhanced Pseudocode ---- */
    _post_init_funcs += ({ f });

}

// --------------------------------------------------------------------
// @brief  converter: transform data
// @code  nc=7, nargs=1, insts=7
// --------------------------------------------------------------------
mixed to_get_dir(string path) {
    /* ---- Enhanced Pseudocode ---- */
    return get_dir(path);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=11, nargs=4, insts=11
// --------------------------------------------------------------------
void msg_got_gid(int client_no, int cookie, int result, string gid) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->msg_got_gid(cookie, result, gid);

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=3, insts=10
// --------------------------------------------------------------------
void msg_got_page_data(int client_no, int cookie, string content) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->msg_got_page_data(cookie, content);

}

// --------------------------------------------------------------------
// @code  nc=12, nargs=3, insts=12
// --------------------------------------------------------------------
mixed mysql_select(int cno, string sql_cmd, int max_row) {
    /* ---- Enhanced Pseudocode ---- */
    return mixed_mysql_select(cno, sql_cmd, (: callback_mysql_select :), max_row);

}

// --------------------------------------------------------------------
// @brief  notification: send message
// @code  nc=6, nargs=0, insts=6
// --------------------------------------------------------------------
void notice_shutdown() {
    /* ---- Enhanced Pseudocode ---- */
    release_all_data();

}

// --------------------------------------------------------------------
// @brief  interaction: calls other objects
// @code  nc=10, nargs=3, insts=10
// --------------------------------------------------------------------
void msg_got_catalog(int client_no, int cookie, int result) {
    /* ---- Enhanced Pseudocode ---- */
    "/daemons/filed.c"->msg_got_catalog(cookie, result);

}

/* ================================================================
   DECLARED FUNCTIONS (no bytecode - likely inherited or forward decl)
   ================================================================ */
/*
   _cmd_attrib_map();  // inherited from parent or forward declaration
   f_this_record();  // inherited from parent or forward declaration
   INI_FILE();  // inherited from parent or forward declaration
   startup_id();  // inherited from parent or forward declaration
   new_convert_code();  // inherited from parent or forward declaration
   _server_status();  // inherited from parent or forward declaration
   f_set_auto_commit();  // inherited from parent or forward declaration
   _dist_name();  // inherited from parent or forward declaration
   all_cmds_cost();  // inherited from parent or forward declaration
   ban_words();  // inherited from parent or forward declaration
   all_mutex();  // inherited from parent or forward declaration
   f_get_insert_id();  // inherited from parent or forward declaration
   _cmd_types();  // inherited from parent or forward declaration
   enable_stat_cmds();  // inherited from parent or forward declaration
   _xdbc_output_file();  // inherited from parent or forward declaration
   base_ganzhi();  // inherited from parent or forward declaration
   write_file_cookie();  // inherited from parent or forward declaration
   _heart_beat_funcs();  // inherited from parent or forward declaration
   f_num_records();  // inherited from parent or forward declaration
   _today_zero_time();  // inherited from parent or forward declaration
   _leave_world_funcs();  // inherited from parent or forward declaration
   f_finalize();  // inherited from parent or forward declaration
   lib_pak_path();  // inherited from parent or forward declaration
   f_is_connected();  // inherited from parent or forward declaration
   load_or_compile_all();  // inherited from parent or forward declaration
   cmds_processed();  // inherited from parent or forward declaration
   _just_login_server_funcs();  // inherited from parent or forward declaration
   csl_new_bonus_season();  // inherited from parent or forward declaration
   _internal_comm_close_funcs();  // inherited from parent or forward declaration
   f_disconnect();  // inherited from parent or forward declaration
   f_field_name();  // inherited from parent or forward declaration
   f_fetch_record();  // inherited from parent or forward declaration
   f_num_fields();  // inherited from parent or forward declaration
   is_test_dist_flag();  // inherited from parent or forward declaration
   when_return_user_not_found();  // inherited from parent or forward declaration
   file_dependence();  // inherited from parent or forward declaration
   f_query();  // inherited from parent or forward declaration
   xdbc_func_map();  // inherited from parent or forward declaration
   coupon_charge_type();  // inherited from parent or forward declaration
   server_shift_time();  // inherited from parent or forward declaration
   disconnect_stat();  // inherited from parent or forward declaration
   need_exclude_cmds();  // inherited from parent or forward declaration
   load_or_compile();  // inherited from parent or forward declaration
   _prepare_switch_server_funcs();  // inherited from parent or forward declaration
   _login_timeout();  // inherited from parent or forward declaration
   _mdb();  // inherited from parent or forward declaration
   daemon_data();  // inherited from parent or forward declaration
   _adjust_position_info_funcs();  // inherited from parent or forward declaration
   _regen_iid_funcs();  // inherited from parent or forward declaration
   _enable_log_trace();  // inherited from parent or forward declaration
   online_update_status();  // inherited from parent or forward declaration
   _enter_world_funcs();  // inherited from parent or forward declaration
   _xdbc_output_basic_file();  // inherited from parent or forward declaration
   _switch_server_funcs();  // inherited from parent or forward declaration
   _heart_beat_ex_funcs();  // inherited from parent or forward declaration
   _log_runtime_info_funcs();  // inherited from parent or forward declaration
   _char_disconnected_funcs();  // inherited from parent or forward declaration
   n_msg();  // inherited from parent or forward declaration
   _new_connection_funcs();  // inherited from parent or forward declaration
   load_or_compile_all_path();  // inherited from parent or forward declaration
   _char_reconnected_funcs();  // inherited from parent or forward declaration
   _child_leave_world_funcs();  // inherited from parent or forward declaration
   _server_name();  // inherited from parent or forward declaration
   gid_objects();  // inherited from parent or forward declaration
   queue_map();  // inherited from parent or forward declaration
   tiangan_dizhi();  // inherited from parent or forward declaration
   _post_init_funcs();  // inherited from parent or forward declaration
   last_telnet_login_time();  // inherited from parent or forward declaration
   _xdbc_clients();  // inherited from parent or forward declaration
   _xdbc_debug_level();  // inherited from parent or forward declaration
   csp_new_bonus_season();  // inherited from parent or forward declaration
   month_day();  // inherited from parent or forward declaration
   dizhi();  // inherited from parent or forward declaration
   _xdbc_output_file_suffix();  // inherited from parent or forward declaration
   _auto_save_funcs();  // inherited from parent or forward declaration
   f_field_value();  // inherited from parent or forward declaration
   coupon_charge_type_list();  // inherited from parent or forward declaration
   COMPILE_ACTION();  // inherited from parent or forward declaration
   f_get_error();  // inherited from parent or forward declaration
   ENABLE_CMDS_STAT();  // inherited from parent or forward declaration
   cmds_flow_in();  // inherited from parent or forward declaration
   stat_cmds_interval();  // inherited from parent or forward declaration
   ban_words_match_id();  // inherited from parent or forward declaration
   exchange_cookie();  // inherited from parent or forward declaration
   _xdbc_query();  // inherited from parent or forward declaration
   f_int_field();  // inherited from parent or forward declaration
   f_connect();  // inherited from parent or forward declaration
   f_select_db();  // inherited from parent or forward declaration
   _disconnected_funcs();  // inherited from parent or forward declaration
   telnet_clients();  // inherited from parent or forward declaration
   tiangan();  // inherited from parent or forward declaration
   _enter_world_failed_funcs();  // inherited from parent or forward declaration
   f_execute();  // inherited from parent or forward declaration
   dist_start_time();  // inherited from parent or forward declaration
   tolower_temp_flag();  // inherited from parent or forward declaration
   client_agents();  // inherited from parent or forward declaration
*/

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Constants extracted:         3912 (HEX:2110 INT:421 STR:1160 CODE:204 REF:17)
   Total function names:        416
   Code segments found:         416
   Successfully decoded:        416
   Unresolved/minimal:          0
   Functions without code:      96

   Note: Functions without code are usually inherited from the
   parent object (), forward declarations, or
   simple property accessor methods that the compiler optimizes away.
   ================================================================ */