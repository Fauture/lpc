/*
Navicat MySQL Data Transfer

Source Server         : 192.168.200.128
Source Server Version : 50173
Source Host           : 192.168.200.128:3306
Source Database       : ldb

Target Server Type    : MYSQL
Target Server Version : 50173
File Encoding         : 936

Date: 2026-07-12 23:02:48
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for activity_log
-- ----------------------------
DROP TABLE IF EXISTS `activity_log`;
CREATE TABLE `activity_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` char(14) NOT NULL DEFAULT '',
  `server` varchar(32) NOT NULL DEFAULT '',
  `activity` varchar(32) NOT NULL DEFAULT '',
  `start_time` char(14) NOT NULL DEFAULT '',
  `end_time` char(14) NOT NULL DEFAULT '',
  `para1` varchar(32) NOT NULL DEFAULT '',
  `para2` varchar(128) NOT NULL DEFAULT '',
  `para3` varchar(255) NOT NULL DEFAULT '',
  `memo` text,
  PRIMARY KEY (`id`),
  UNIQUE KEY `activity_time` (`activity`,`start_time`,`end_time`),
  KEY `update_time` (`update_time`),
  KEY `activity` (`activity`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for anti_cheater_log
-- ----------------------------
DROP TABLE IF EXISTS `anti_cheater_log`;
CREATE TABLE `anti_cheater_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `type` varchar(32) NOT NULL DEFAULT '',
  `action` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `lv` int(11) NOT NULL DEFAULT '0',
  `gold_coin` int(11) NOT NULL DEFAULT '0',
  `ip` varchar(15) NOT NULL DEFAULT '',
  `mac` varchar(32) NOT NULL DEFAULT '',
  `para1` varchar(32) DEFAULT '',
  `para2` varchar(32) DEFAULT '',
  `para3` varchar(128) DEFAULT '',
  `memo` text,
  PRIMARY KEY (`id`),
  KEY `update_time` (`update_time`),
  KEY `type` (`type`,`action`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for antiaddiction_log
-- ----------------------------
DROP TABLE IF EXISTS `antiaddiction_log`;
CREATE TABLE `antiaddiction_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `online_time` int(11) NOT NULL DEFAULT '0',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `update_time` (`update_time`),
  KEY `account` (`account`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for apex_log
-- ----------------------------
DROP TABLE IF EXISTS `apex_log`;
CREATE TABLE `apex_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` char(14) NOT NULL DEFAULT '',
  `line` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `kill_action` varchar(32) NOT NULL DEFAULT '',
  `task` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` char(16) NOT NULL DEFAULT '',
  `char_name` varchar(32) NOT NULL DEFAULT '',
  `lv` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `tao` int(11) NOT NULL DEFAULT '0',
  `gold_coin` int(11) NOT NULL DEFAULT '0',
  `error_id` int(11) NOT NULL DEFAULT '0',
  `str_n` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `ip` varchar(16) NOT NULL DEFAULT '',
  `mac` varchar(16) NOT NULL DEFAULT '',
  `ip_gids` int(11) NOT NULL DEFAULT '0',
  `mac_gids` int(11) NOT NULL DEFAULT '0',
  `memo` text,
  PRIMARY KEY (`id`),
  KEY `update_time` (`update_time`),
  KEY `gid` (`gid`),
  KEY `error_id` (`error_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for apply_log
-- ----------------------------
DROP TABLE IF EXISTS `apply_log`;
CREATE TABLE `apply_log` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `update_time` char(14) NOT NULL DEFAULT '',
  `line` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `type` tinyint(3) unsigned DEFAULT '0',
  `action` varchar(32) NOT NULL DEFAULT '',
  `upgrade_state` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` char(16) DEFAULT NULL,
  `lv` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `gold_coin` int(10) unsigned NOT NULL DEFAULT '0',
  `insider` int(10) unsigned NOT NULL DEFAULT '0',
  `item_type` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `item_name` varchar(32) NOT NULL DEFAULT '',
  `item_price` int(11) NOT NULL DEFAULT '0',
  `item_life` int(11) NOT NULL DEFAULT '0',
  `item_source` tinyint(4) NOT NULL DEFAULT '0',
  `iid` varchar(32) NOT NULL DEFAULT '',
  `lv_equipment` smallint(5) unsigned DEFAULT NULL,
  `para1` int(10) unsigned DEFAULT NULL,
  `para2` varchar(32) NOT NULL DEFAULT '',
  `para3` varchar(32) NOT NULL DEFAULT '',
  `memo` text NOT NULL,
  KEY `id` (`id`),
  KEY `time` (`update_time`),
  KEY `name` (`item_name`),
  KEY `gid` (`gid`)
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for apply_log_bak
-- ----------------------------
DROP TABLE IF EXISTS `apply_log_bak`;
CREATE TABLE `apply_log_bak` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `update_time` char(14) NOT NULL DEFAULT '',
  `line` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `type` tinyint(3) unsigned DEFAULT '0',
  `action` varchar(32) NOT NULL DEFAULT '',
  `upgrade_state` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` char(16) DEFAULT NULL,
  `lv` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `gold_coin` int(10) unsigned NOT NULL DEFAULT '0',
  `insider` int(10) unsigned NOT NULL DEFAULT '0',
  `item_type` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `item_name` varchar(32) NOT NULL DEFAULT '',
  `item_price` int(11) NOT NULL DEFAULT '0',
  `item_life` int(11) NOT NULL DEFAULT '0',
  `item_source` tinyint(4) NOT NULL DEFAULT '0',
  `iid` varchar(32) NOT NULL DEFAULT '',
  `lv_equipment` smallint(5) unsigned DEFAULT NULL,
  `para1` int(10) unsigned DEFAULT NULL,
  `para2` varchar(32) NOT NULL DEFAULT '',
  `para3` varchar(32) NOT NULL DEFAULT '',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `name` (`item_name`),
  KEY `gid` (`gid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for apply_log_old2
-- ----------------------------
DROP TABLE IF EXISTS `apply_log_old2`;
CREATE TABLE `apply_log_old2` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `line` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `type` tinyint(3) unsigned DEFAULT '0',
  `action` varchar(32) NOT NULL DEFAULT '',
  `upgrade_state` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) DEFAULT NULL,
  `lv` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `gold_coin` int(10) unsigned NOT NULL DEFAULT '0',
  `insider` int(10) unsigned NOT NULL DEFAULT '0',
  `item_type` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `item_name` varchar(32) NOT NULL DEFAULT '',
  `iid` varchar(32) NOT NULL DEFAULT '',
  `lv_equipment` smallint(5) unsigned DEFAULT NULL,
  `para1` int(10) unsigned DEFAULT NULL,
  `para2` varchar(32) NOT NULL DEFAULT '',
  `para3` varchar(32) NOT NULL DEFAULT '',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `name` (`item_name`),
  KEY `gid` (`gid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for auto_talk_log
-- ----------------------------
DROP TABLE IF EXISTS `auto_talk_log`;
CREATE TABLE `auto_talk_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `line` tinyint(2) unsigned NOT NULL DEFAULT '0',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `mac` varchar(16) NOT NULL DEFAULT '',
  `ip` varchar(15) NOT NULL DEFAULT '',
  `level` int(11) NOT NULL DEFAULT '0',
  `max_lv` int(11) NOT NULL DEFAULT '0',
  `gold_coin` int(11) NOT NULL DEFAULT '0',
  `map` varchar(255) NOT NULL DEFAULT '',
  `call_times` int(11) NOT NULL DEFAULT '0',
  `publish` tinyint(2) NOT NULL DEFAULT '0',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `account` (`account`),
  KEY `gid` (`gid`),
  KEY `mac` (`mac`),
  KEY `update_time` (`update_time`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for behavior_log
-- ----------------------------
DROP TABLE IF EXISTS `behavior_log`;
CREATE TABLE `behavior_log` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `time_used` smallint(5) unsigned NOT NULL DEFAULT '0',
  `time_used_fd` smallint(5) unsigned NOT NULL DEFAULT '0',
  `lv` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `tao` int(10) unsigned NOT NULL DEFAULT '0',
  `upgrade_type` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `addup_exp` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `addup_tao` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `addup_play` smallint(5) unsigned NOT NULL DEFAULT '0',
  `addup_round` smallint(5) unsigned NOT NULL DEFAULT '0',
  `addup_talk_times` smallint(5) unsigned NOT NULL DEFAULT '0',
  `addup_friend_times` smallint(5) unsigned NOT NULL DEFAULT '0',
  `addup_click_npc` smallint(5) unsigned NOT NULL DEFAULT '0',
  `addup_exchange` smallint(5) unsigned NOT NULL DEFAULT '0',
  `addup_click_mouse` smallint(5) unsigned NOT NULL DEFAULT '0',
  `addup_move_order` smallint(5) unsigned NOT NULL DEFAULT '0',
  `time_used_stall` smallint(5) unsigned NOT NULL DEFAULT '0',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `gid` (`gid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for block_log
-- ----------------------------
DROP TABLE IF EXISTS `block_log`;
CREATE TABLE `block_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `mode` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `type` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `action` varchar(32) NOT NULL DEFAULT '',
  `due` int(11) NOT NULL DEFAULT '0',
  `reason_type` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `operator` varchar(32) NOT NULL DEFAULT '',
  `para1` varchar(32) NOT NULL DEFAULT '',
  `para2` varchar(255) NOT NULL DEFAULT '',
  `para3` int(11) NOT NULL DEFAULT '0',
  `memo` text,
  PRIMARY KEY (`id`),
  KEY `operator` (`operator`),
  KEY `account` (`account`),
  KEY `action` (`action`),
  KEY `time_gid` (`update_time`,`gid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for campaign_log
-- ----------------------------
DROP TABLE IF EXISTS `campaign_log`;
CREATE TABLE `campaign_log` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `line` varchar(15) NOT NULL DEFAULT '',
  `type` tinyint(4) NOT NULL DEFAULT '0',
  `action` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(32) NOT NULL DEFAULT '',
  `lv` int(11) NOT NULL DEFAULT '0',
  `party_gid` varchar(32) NOT NULL DEFAULT '',
  `gold_coin` int(11) NOT NULL DEFAULT '0',
  `insider` tinyint(1) NOT NULL DEFAULT '0',
  `para1` int(11) DEFAULT NULL,
  `para2` varchar(32) DEFAULT NULL,
  `para3` varchar(255) DEFAULT NULL,
  `bonus_type` tinyint(4) NOT NULL DEFAULT '0',
  `bonus_name` varchar(32) NOT NULL DEFAULT '',
  `bonus_prop` varchar(32) DEFAULT NULL,
  `memo` text NOT NULL,
  KEY `id` (`id`),
  KEY `time` (`update_time`),
  KEY `gid` (`gid`),
  KEY `action` (`action`)
) ENGINE=InnoDB AUTO_INCREMENT=4 DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for channel_log
-- ----------------------------
DROP TABLE IF EXISTS `channel_log`;
CREATE TABLE `channel_log` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `update_time` char(14) NOT NULL DEFAULT '',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(32) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `lv` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `channel` int(11) NOT NULL DEFAULT '0',
  `target` varchar(32) NOT NULL DEFAULT '',
  `msg` text NOT NULL,
  KEY `id` (`id`),
  KEY `time` (`update_time`),
  KEY `gid` (`gid`),
  KEY `sender` (`name`),
  KEY `target` (`target`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for chat_log
-- ----------------------------
DROP TABLE IF EXISTS `chat_log`;
CREATE TABLE `chat_log` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(32) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `lv` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `chat_type` varchar(32) NOT NULL DEFAULT '',
  `chat_channel` int(11) NOT NULL DEFAULT '0',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `gid` (`gid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for client_action_log
-- ----------------------------
DROP TABLE IF EXISTS `client_action_log`;
CREATE TABLE `client_action_log` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `account` varchar(32) DEFAULT NULL,
  `gid` varchar(32) DEFAULT NULL,
  `mac` varchar(64) DEFAULT NULL,
  `ip` varchar(32) DEFAULT NULL,
  `insider` tinyint(4) unsigned DEFAULT NULL,
  `lv` int(11) unsigned DEFAULT NULL,
  `tao` int(11) unsigned DEFAULT NULL,
  `gold_coin` int(11) unsigned DEFAULT NULL,
  `in_party` tinyint(4) unsigned DEFAULT NULL,
  `send_num` int(11) unsigned DEFAULT NULL,
  `send_gids` int(11) unsigned DEFAULT NULL,
  `receive_num` int(11) unsigned DEFAULT NULL,
  `receive_gids` int(11) unsigned DEFAULT NULL,
  `current_channel` int(11) unsigned DEFAULT NULL,
  `troop_channel` int(11) unsigned DEFAULT NULL,
  `party_channel` int(11) unsigned DEFAULT NULL,
  `team_channel` int(11) unsigned DEFAULT NULL,
  `world_channel` int(11) unsigned DEFAULT NULL,
  `whoop_channel` int(11) unsigned DEFAULT NULL,
  `para1` int(11) DEFAULT NULL,
  `para2` int(11) DEFAULT NULL,
  `para3` int(11) DEFAULT NULL,
  `memo` text,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `mac` (`mac`),
  KEY `gid` (`gid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for client_performance_log
-- ----------------------------
DROP TABLE IF EXISTS `client_performance_log`;
CREATE TABLE `client_performance_log` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `record_interval` int(11) DEFAULT NULL,
  `account` varchar(32) DEFAULT NULL,
  `gid` varchar(32) DEFAULT NULL,
  `mac` varchar(64) DEFAULT NULL,
  `ip` varchar(32) DEFAULT NULL,
  `insider` tinyint(4) unsigned DEFAULT NULL,
  `lv` int(11) unsigned DEFAULT NULL,
  `type` tinyint(4) unsigned DEFAULT NULL,
  `cpu_cost` int(11) DEFAULT NULL,
  `mem_cost` varchar(128) DEFAULT NULL,
  `render_frame_rate` int(11) unsigned DEFAULT NULL,
  `cur_scene` tinyint(4) unsigned DEFAULT NULL,
  `cur_room_user` int(11) unsigned DEFAULT NULL,
  `cur_room_name` varchar(32) DEFAULT NULL,
  `cur_process_num` int(11) unsigned DEFAULT NULL,
  `minimize_window` tinyint(4) unsigned DEFAULT NULL,
  `button_type` varchar(32) DEFAULT NULL,
  `button_name` varchar(32) DEFAULT NULL,
  `timeslice` varchar(32) DEFAULT NULL,
  `button_times` int(11) unsigned DEFAULT NULL,
  `directx_version` varchar(19) DEFAULT NULL,
  `prever_download` varchar(19) DEFAULT NULL,
  `para1` int(11) DEFAULT NULL,
  `para2` varchar(32) DEFAULT NULL,
  `para3` varchar(32) DEFAULT NULL,
  `para4` varchar(64) DEFAULT NULL,
  `para5` varchar(128) DEFAULT NULL,
  `memo` text,
  KEY `id` (`id`),
  KEY `time` (`update_time`),
  KEY `mac` (`mac`),
  KEY `gid` (`gid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for coin_consume_log
-- ----------------------------
DROP TABLE IF EXISTS `coin_consume_log`;
CREATE TABLE `coin_consume_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `coin` int(11) unsigned NOT NULL DEFAULT '0',
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `consume_info` varchar(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`),
  KEY `account` (`account`),
  KEY `time` (`update_time`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for coin_order_log
-- ----------------------------
DROP TABLE IF EXISTS `coin_order_log`;
CREATE TABLE `coin_order_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `server` varchar(32) NOT NULL DEFAULT '',
  `order_id` varchar(15) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `name` varchar(12) NOT NULL DEFAULT '',
  `mac` varchar(16) NOT NULL DEFAULT '',
  `ip` varchar(15) NOT NULL DEFAULT '',
  `insider` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `lv` int(11) NOT NULL DEFAULT '0',
  `gold_coin` int(11) NOT NULL DEFAULT '0',
  `store_coin` int(11) NOT NULL DEFAULT '0',
  `type` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `amount` int(11) NOT NULL DEFAULT '0',
  `price` int(11) NOT NULL DEFAULT '0',
  `tax` int(11) NOT NULL DEFAULT '0',
  `para1` int(11) NOT NULL DEFAULT '0',
  `para2` varchar(32) NOT NULL DEFAULT '0',
  `para3` varchar(128) NOT NULL DEFAULT '0',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `server` (`server`),
  KEY `order_id` (`order_id`),
  KEY `gid` (`gid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for collect_data_log
-- ----------------------------
DROP TABLE IF EXISTS `collect_data_log`;
CREATE TABLE `collect_data_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `cpu_id` varchar(64) NOT NULL DEFAULT '',
  `cpu_info` varchar(64) NOT NULL DEFAULT '',
  `disk_sequence` varchar(64) NOT NULL DEFAULT '',
  `mac_address` varchar(64) NOT NULL DEFAULT '',
  `video_card` varchar(64) NOT NULL DEFAULT '',
  `memory_size` int(11) NOT NULL DEFAULT '0',
  `video_memory` int(11) NOT NULL DEFAULT '0',
  `bios` varchar(64) NOT NULL DEFAULT '',
  `privilege` int(11) NOT NULL DEFAULT '0',
  `para1` varchar(64) NOT NULL DEFAULT '',
  `para2` varchar(64) NOT NULL DEFAULT '',
  `para3` varchar(64) NOT NULL DEFAULT '',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`)
) ENGINE=InnoDB AUTO_INCREMENT=267 DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for combat_in_map_log
-- ----------------------------
DROP TABLE IF EXISTS `combat_in_map_log`;
CREATE TABLE `combat_in_map_log` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `update_time` char(14) NOT NULL DEFAULT '',
  `line` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `account` varchar(32) NOT NULL DEFAULT '',
  `char_name` varchar(32) NOT NULL DEFAULT '',
  `gid` char(16) NOT NULL DEFAULT '',
  `level` int(11) NOT NULL DEFAULT '0',
  `gold_coin` int(11) NOT NULL DEFAULT '0',
  `insider` int(11) NOT NULL DEFAULT '0',
  `ip` varchar(32) NOT NULL DEFAULT '',
  `mac` varchar(16) NOT NULL DEFAULT '',
  `tao` int(11) NOT NULL DEFAULT '0',
  `family` varchar(32) NOT NULL DEFAULT '',
  `create_time` char(14) NOT NULL DEFAULT '',
  `map` varchar(32) NOT NULL DEFAULT '',
  `combat_times` int(11) NOT NULL DEFAULT '0',
  `combat_pet` varchar(32) NOT NULL DEFAULT '',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `account` (`account`),
  KEY `char_name` (`char_name`),
  KEY `gid` (`gid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for combat_log
-- ----------------------------
DROP TABLE IF EXISTS `combat_log`;
CREATE TABLE `combat_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `mac` varchar(32) NOT NULL DEFAULT '',
  `ip` varchar(15) NOT NULL DEFAULT '',
  `insider` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `lv` int(11) NOT NULL DEFAULT '0',
  `tao` int(11) NOT NULL DEFAULT '0',
  `gold_coin` int(11) NOT NULL DEFAULT '0',
  `in_party` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `all_round` int(11) NOT NULL DEFAULT '0',
  `abnormal_round` int(11) NOT NULL DEFAULT '0',
  `abnormal_degree` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `main_attribute` varchar(32) NOT NULL DEFAULT '0',
  `ratio_attribute` int(11) NOT NULL DEFAULT '0',
  `para1` int(11) NOT NULL DEFAULT '0',
  `para2` int(11) NOT NULL DEFAULT '0',
  `para3` varchar(32) NOT NULL DEFAULT '0',
  `para4` varchar(128) NOT NULL DEFAULT '0',
  `para5` varchar(128) NOT NULL DEFAULT '0',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for common_hist
-- ----------------------------
DROP TABLE IF EXISTS `common_hist`;
CREATE TABLE `common_hist` (
  `id` int(11) NOT NULL DEFAULT '0',
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `ip` varchar(15) NOT NULL DEFAULT '',
  `type` varchar(32) NOT NULL DEFAULT '',
  `severity` int(11) unsigned NOT NULL DEFAULT '0',
  `action` varchar(32) NOT NULL DEFAULT '',
  `para1` varchar(128) NOT NULL DEFAULT '',
  `para2` varchar(128) NOT NULL DEFAULT '',
  `para3` varchar(128) NOT NULL DEFAULT '',
  `memo` text NOT NULL,
  KEY `id` (`id`),
  KEY `time` (`update_time`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for common_log
-- ----------------------------
DROP TABLE IF EXISTS `common_log`;
CREATE TABLE `common_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `server` varchar(32) NOT NULL DEFAULT '',
  `type` varchar(32) NOT NULL DEFAULT '',
  `severity` int(11) unsigned NOT NULL DEFAULT '0',
  `action` varchar(32) NOT NULL DEFAULT '',
  `para1` varchar(128) NOT NULL DEFAULT '',
  `para2` varchar(128) NOT NULL DEFAULT '',
  `para3` varchar(128) NOT NULL DEFAULT '',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`)
) ENGINE=InnoDB AUTO_INCREMENT=2214 DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for cost_coin_log
-- ----------------------------
DROP TABLE IF EXISTS `cost_coin_log`;
CREATE TABLE `cost_coin_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `server` varchar(64) NOT NULL DEFAULT '',
  `type` varchar(32) NOT NULL DEFAULT '',
  `action` varchar(32) NOT NULL DEFAULT '',
  `cost_type` varchar(32) NOT NULL DEFAULT '',
  `cost` int(11) NOT NULL DEFAULT '0',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(32) NOT NULL DEFAULT '',
  `item_name` varchar(32) NOT NULL DEFAULT '',
  `amount` int(11) NOT NULL DEFAULT '0',
  `uid` varchar(32) NOT NULL DEFAULT '',
  `para1` varchar(32) NOT NULL DEFAULT '',
  `para2` varchar(32) NOT NULL DEFAULT '',
  `para3` varchar(64) NOT NULL DEFAULT '',
  `memo` text,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=761 DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for csc_combat_log
-- ----------------------------
DROP TABLE IF EXISTS `csc_combat_log`;
CREATE TABLE `csc_combat_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `server` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `mac` varchar(16) NOT NULL DEFAULT '',
  `ip` varchar(15) NOT NULL DEFAULT '',
  `insider` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `lv` int(11) NOT NULL DEFAULT '0',
  `tao` int(11) NOT NULL DEFAULT '0',
  `g_coin` int(11) NOT NULL DEFAULT '0',
  `s_coin` int(11) NOT NULL DEFAULT '0',
  `score` int(11) NOT NULL DEFAULT '0',
  `result` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `para1` int(11) NOT NULL DEFAULT '0',
  `para2` int(11) NOT NULL DEFAULT '0',
  `para3` varchar(128) NOT NULL DEFAULT '',
  `para4` varchar(128) NOT NULL DEFAULT '',
  `para5` varchar(128) NOT NULL DEFAULT '',
  `memo` text,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `server_account` (`server`,`account`),
  KEY `server_gid` (`server`,`gid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for csl_combat_log
-- ----------------------------
DROP TABLE IF EXISTS `csl_combat_log`;
CREATE TABLE `csl_combat_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `server` varchar(32) NOT NULL DEFAULT '',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `mac` varchar(15) NOT NULL DEFAULT '',
  `ip` varchar(15) NOT NULL DEFAULT '',
  `lv` int(11) NOT NULL DEFAULT '0',
  `tao` int(11) NOT NULL DEFAULT '0',
  `league_id` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `season_id` tinyint(11) unsigned NOT NULL DEFAULT '0',
  `stage_id` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `round_id` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `matchday_id` tinyint(4) NOT NULL DEFAULT '0',
  `match_id` varchar(32) NOT NULL DEFAULT '',
  `level_section` varchar(15) NOT NULL DEFAULT '',
  `result` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `score` int(11) NOT NULL DEFAULT '0',
  `score_left` int(11) NOT NULL DEFAULT '0',
  `para1` int(11) NOT NULL DEFAULT '0',
  `para2` varchar(32) NOT NULL DEFAULT '0',
  `para3` varchar(128) NOT NULL DEFAULT '',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `combat_index` (`league_id`,`season_id`,`stage_id`,`round_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for csl_match_log
-- ----------------------------
DROP TABLE IF EXISTS `csl_match_log`;
CREATE TABLE `csl_match_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `server` varchar(32) NOT NULL DEFAULT '',
  `home_dist` varchar(32) NOT NULL DEFAULT '',
  `away_dist` varchar(32) NOT NULL DEFAULT '',
  `league_id` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `season_id` int(11) unsigned NOT NULL DEFAULT '0',
  `stage_id` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `round_id` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `matchday_id` tinyint(4) NOT NULL DEFAULT '0',
  `result` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `home_score` int(11) NOT NULL DEFAULT '0',
  `away_score` int(11) NOT NULL DEFAULT '0',
  `home_gid_num` int(11) NOT NULL DEFAULT '0',
  `away_gid_num` int(11) NOT NULL DEFAULT '0',
  `home_avg_lv` int(11) NOT NULL DEFAULT '0',
  `away_avg_lv` int(11) NOT NULL DEFAULT '0',
  `para1` int(11) NOT NULL DEFAULT '0',
  `para2` varchar(32) NOT NULL DEFAULT '',
  `para3` varchar(128) NOT NULL DEFAULT '',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `combat_index` (`league_id`,`season_id`,`stage_id`,`round_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for csp_combat_log
-- ----------------------------
DROP TABLE IF EXISTS `csp_combat_log`;
CREATE TABLE `csp_combat_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `server` varchar(32) NOT NULL DEFAULT '',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `mac` varchar(15) NOT NULL DEFAULT '',
  `ip` varchar(15) NOT NULL DEFAULT '',
  `lv` int(11) NOT NULL DEFAULT '0',
  `tao` int(11) NOT NULL DEFAULT '0',
  `league_id` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `season_id` tinyint(11) unsigned NOT NULL DEFAULT '0',
  `round_id` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `matchday_id` tinyint(4) NOT NULL DEFAULT '0',
  `match_id` varchar(32) NOT NULL DEFAULT '',
  `party_id` varchar(32) NOT NULL DEFAULT '',
  `result` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `party_contrib` int(11) NOT NULL DEFAULT '0',
  `para1` int(11) NOT NULL DEFAULT '0',
  `para2` varchar(32) NOT NULL DEFAULT '0',
  `para3` varchar(128) NOT NULL DEFAULT '',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `combat_index` (`league_id`,`season_id`,`round_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for csp_match_log
-- ----------------------------
DROP TABLE IF EXISTS `csp_match_log`;
CREATE TABLE `csp_match_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `server` varchar(32) NOT NULL DEFAULT '',
  `home_dist` varchar(32) NOT NULL DEFAULT '',
  `away_dist` varchar(32) NOT NULL DEFAULT '',
  `league_id` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `season_id` int(11) unsigned NOT NULL DEFAULT '0',
  `round_id` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `matchday_id` tinyint(4) NOT NULL DEFAULT '0',
  `result` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `home_party_name` varchar(32) NOT NULL DEFAULT '0',
  `away_party_name` varchar(32) NOT NULL DEFAULT '0',
  `home_gid_num` int(11) NOT NULL DEFAULT '0',
  `away_gid_num` int(11) NOT NULL DEFAULT '0',
  `home_avg_lv` int(11) NOT NULL DEFAULT '0',
  `away_avg_lv` int(11) NOT NULL DEFAULT '0',
  `para1` int(11) NOT NULL DEFAULT '0',
  `para2` varchar(32) NOT NULL DEFAULT '',
  `para3` varchar(128) NOT NULL DEFAULT '',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `combat_index` (`league_id`,`season_id`,`round_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for debug_log
-- ----------------------------
DROP TABLE IF EXISTS `debug_log`;
CREATE TABLE `debug_log` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) DEFAULT '',
  `server` varchar(32) DEFAULT '',
  `type` varchar(32) DEFAULT '',
  `action` varchar(32) DEFAULT '',
  `para1` varchar(32) DEFAULT '',
  `para2` varchar(32) DEFAULT '',
  `para3` varchar(128) DEFAULT '',
  `memo` text,
  KEY `id` (`id`),
  KEY `update_time` (`update_time`),
  KEY `type_action` (`type`,`action`)
) ENGINE=InnoDB AUTO_INCREMENT=1524 DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for encourage_detail_log
-- ----------------------------
DROP TABLE IF EXISTS `encourage_detail_log`;
CREATE TABLE `encourage_detail_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `dist` varchar(16) DEFAULT NULL,
  `account` varchar(32) DEFAULT '',
  `gid` varchar(16) DEFAULT '',
  `update_time` varchar(14) DEFAULT '',
  `encourage_type` varchar(64) DEFAULT '',
  `bonus_name` varchar(32) DEFAULT '',
  `iid` varchar(32) NOT NULL DEFAULT '',
  `encourage_id` int(11) DEFAULT '0',
  `ip` varchar(16) DEFAULT '',
  `para1` varchar(32) DEFAULT '',
  `para2` varchar(255) DEFAULT '',
  `memo` text,
  PRIMARY KEY (`id`),
  KEY `update_time` (`update_time`),
  KEY `account` (`account`),
  KEY `gid` (`gid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for equipment_log
-- ----------------------------
DROP TABLE IF EXISTS `equipment_log`;
CREATE TABLE `equipment_log` (
  `id` int(10) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `action` varchar(32) NOT NULL DEFAULT '',
  `item_name` varchar(32) NOT NULL DEFAULT '',
  `item_iid` varchar(32) NOT NULL DEFAULT '',
  `cost_item_name` varchar(32) DEFAULT '',
  `cost_item_iid` varchar(32) NOT NULL DEFAULT '',
  `cost` int(10) NOT NULL DEFAULT '0',
  `oper_result` tinyint(4) NOT NULL DEFAULT '0',
  `para1` varchar(32) DEFAULT '',
  `para2` varchar(128) DEFAULT '',
  `para3` varchar(255) DEFAULT '',
  `item_prop` text,
  KEY `id` (`id`),
  KEY `time` (`update_time`),
  KEY `gid` (`gid`),
  KEY `action` (`action`),
  KEY `equip_iid` (`item_iid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for errand_log
-- ----------------------------
DROP TABLE IF EXISTS `errand_log`;
CREATE TABLE `errand_log` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `line` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `type` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `action` varchar(32) NOT NULL DEFAULT '0',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(32) NOT NULL DEFAULT '',
  `lv` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `party_gid` varchar(32) NOT NULL DEFAULT '',
  `gold_coin` int(11) NOT NULL DEFAULT '0',
  `insider` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `para1` int(11) DEFAULT NULL,
  `para2` varchar(32) DEFAULT NULL,
  `para3` varchar(255) DEFAULT NULL,
  `bonus_type` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `bonus_name` varchar(32) NOT NULL DEFAULT '0',
  `bonus_prop` varchar(32) NOT NULL DEFAULT '0',
  `memo` text NOT NULL,
  KEY `time` (`update_time`),
  KEY `gid` (`gid`),
  KEY `action` (`action`),
  KEY `id` (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=1622 DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for error_log
-- ----------------------------
DROP TABLE IF EXISTS `error_log`;
CREATE TABLE `error_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `server` varchar(32) DEFAULT '',
  `update_time` char(14) NOT NULL DEFAULT '',
  `date` varchar(32) NOT NULL DEFAULT '',
  `err_num` int(11) NOT NULL DEFAULT '0',
  `err_reason` varchar(128) NOT NULL DEFAULT '',
  `err_file` varchar(128) NOT NULL DEFAULT '',
  `err_detail` text,
  `memo` text,
  PRIMARY KEY (`id`),
  KEY `update_time` (`update_time`),
  KEY `date` (`date`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for exchange_appraise_log
-- ----------------------------
DROP TABLE IF EXISTS `exchange_appraise_log`;
CREATE TABLE `exchange_appraise_log` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `update_time` char(14) NOT NULL DEFAULT '',
  `server` varchar(32) NOT NULL DEFAULT '',
  `exchange_time` char(14) NOT NULL DEFAULT '',
  `account1` varchar(32) NOT NULL DEFAULT '',
  `name1` varchar(32) NOT NULL DEFAULT '',
  `gid1` varchar(32) NOT NULL DEFAULT '',
  `lv1` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `tao1` int(11) unsigned NOT NULL DEFAULT '0',
  `gold_coin1` int(11) NOT NULL DEFAULT '0',
  `account2` varchar(32) NOT NULL DEFAULT '',
  `name2` varchar(32) NOT NULL DEFAULT '',
  `gid2` varchar(32) NOT NULL DEFAULT '',
  `lv2` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `tao2` int(11) unsigned NOT NULL DEFAULT '0',
  `gold_coin2` int(11) NOT NULL DEFAULT '0',
  `report` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `star` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `content` varchar(255) NOT NULL DEFAULT '',
  `phone` varchar(32) NOT NULL DEFAULT '',
  `para1` varchar(32) DEFAULT '',
  `para2` varchar(32) DEFAULT '',
  `para3` varchar(255) DEFAULT '',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `account1` (`account1`),
  KEY `account2` (`account2`),
  KEY `gid1` (`gid1`),
  KEY `gid2` (`gid2`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for gbuy_action_log
-- ----------------------------
DROP TABLE IF EXISTS `gbuy_action_log`;
CREATE TABLE `gbuy_action_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `server` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `mac` varchar(16) NOT NULL DEFAULT '',
  `ip` varchar(15) NOT NULL DEFAULT '',
  `insider` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `lv` int(11) NOT NULL DEFAULT '0',
  `gold_coin` int(11) NOT NULL DEFAULT '0',
  `silver_coin` int(11) NOT NULL DEFAULT '0',
  `gbuy_gcoin` int(11) NOT NULL DEFAULT '0',
  `gbuy_scoin` int(11) NOT NULL DEFAULT '0',
  `action` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `coin_type` tinyint(4) NOT NULL DEFAULT '0',
  `coin` int(11) NOT NULL DEFAULT '0',
  `gbg_id` varchar(32) NOT NULL DEFAULT '',
  `gbg_name` varchar(32) NOT NULL DEFAULT '0',
  `gbg_amount` int(11) NOT NULL DEFAULT '0',
  `para1` int(11) NOT NULL DEFAULT '0',
  `para2` varchar(32) NOT NULL DEFAULT '0',
  `para3` varchar(128) NOT NULL DEFAULT '0',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `gid` (`gid`),
  KEY `gbg_id` (`gbg_id`),
  KEY `action` (`action`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for gift_log
-- ----------------------------
DROP TABLE IF EXISTS `gift_log`;
CREATE TABLE `gift_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '' COMMENT '记录时间',
  `server` varchar(32) NOT NULL DEFAULT '' COMMENT '服务器',
  `type` varchar(32) NOT NULL DEFAULT '0' COMMENT '赠品类型',
  `action` varchar(32) NOT NULL DEFAULT '0' COMMENT '产出系统',
  `account` varchar(32) NOT NULL DEFAULT '0' COMMENT '账号',
  `gid` varchar(32) NOT NULL DEFAULT '0' COMMENT 'GID',
  `lv` int(11) unsigned NOT NULL DEFAULT '0' COMMENT '等级',
  `tao` int(11) unsigned NOT NULL DEFAULT '0' COMMENT '道行',
  `login_ip` varchar(32) NOT NULL DEFAULT '0' COMMENT '登录IP',
  `mac` varchar(32) NOT NULL DEFAULT '0' COMMENT 'MAC位址',
  `item_name` varchar(32) NOT NULL DEFAULT '0' COMMENT '物品名称',
  `iid` varchar(32) NOT NULL DEFAULT '0' COMMENT '物品iid或银元宝数量',
  `gold_coin` int(11) unsigned NOT NULL DEFAULT '0' COMMENT '金元宝',
  `insider` varchar(32) NOT NULL DEFAULT '0' COMMENT '玩家状态',
  `para1` varchar(128) NOT NULL DEFAULT '0',
  `para2` varchar(128) NOT NULL DEFAULT '0',
  `para3` varchar(128) NOT NULL DEFAULT '0',
  `memo` text NOT NULL COMMENT '备注',
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for gm_log
-- ----------------------------
DROP TABLE IF EXISTS `gm_log`;
CREATE TABLE `gm_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `lv` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `tao` int(11) NOT NULL DEFAULT '0',
  `cash` int(11) NOT NULL DEFAULT '0',
  `mode` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `type` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `action` int(11) NOT NULL DEFAULT '0',
  `operator` varchar(32) NOT NULL DEFAULT '',
  `para1` varchar(32) NOT NULL DEFAULT '',
  `para2` varchar(255) NOT NULL DEFAULT '',
  `para3` varchar(32) NOT NULL DEFAULT '',
  `memo` text,
  PRIMARY KEY (`id`),
  KEY `operator` (`operator`),
  KEY `account` (`account`),
  KEY `action` (`action`),
  KEY `time_gid` (`update_time`,`gid`),
  KEY `name` (`name`)
) ENGINE=InnoDB AUTO_INCREMENT=330 DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for gs_log
-- ----------------------------
DROP TABLE IF EXISTS `gs_log`;
CREATE TABLE `gs_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `server` varchar(32) NOT NULL DEFAULT '',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `type` varchar(32) NOT NULL DEFAULT '',
  `time` int(11) NOT NULL DEFAULT '0',
  `ctime` varchar(32) NOT NULL DEFAULT '',
  `count` int(11) NOT NULL DEFAULT '0',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `server` (`server`),
  KEY `time` (`time`)
) ENGINE=InnoDB AUTO_INCREMENT=4149 DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for history_credit_log
-- ----------------------------
DROP TABLE IF EXISTS `history_credit_log`;
CREATE TABLE `history_credit_log` (
  `account` varchar(32) NOT NULL DEFAULT '',
  `coin` int(11) NOT NULL DEFAULT '0',
  `used` tinyint(4) NOT NULL DEFAULT '0',
  PRIMARY KEY (`account`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for important_action_log
-- ----------------------------
DROP TABLE IF EXISTS `important_action_log`;
CREATE TABLE `important_action_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `action` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `lv` int(11) NOT NULL DEFAULT '0',
  `gold_coin` int(11) NOT NULL DEFAULT '0',
  `ip` varchar(15) NOT NULL DEFAULT '',
  `mac` varchar(32) NOT NULL DEFAULT '',
  `bonus_type` tinyint(4) NOT NULL DEFAULT '0',
  `amount` int(11) NOT NULL DEFAULT '0',
  `party_gid` varchar(32) NOT NULL DEFAULT '',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `update_time` (`update_time`),
  KEY `action` (`action`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for important_log
-- ----------------------------
DROP TABLE IF EXISTS `important_log`;
CREATE TABLE `important_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `server` varchar(32) NOT NULL DEFAULT '',
  `type` varchar(32) NOT NULL DEFAULT '',
  `severity` int(11) unsigned NOT NULL DEFAULT '0',
  `action` varchar(32) NOT NULL DEFAULT '',
  `para1` varchar(128) NOT NULL DEFAULT '',
  `para2` varchar(128) NOT NULL DEFAULT '',
  `para3` varchar(128) NOT NULL DEFAULT '',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `type_action` (`type`,`action`),
  KEY `time` (`update_time`)
) ENGINE=InnoDB AUTO_INCREMENT=305 DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for important_pet_log
-- ----------------------------
DROP TABLE IF EXISTS `important_pet_log`;
CREATE TABLE `important_pet_log` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `line` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `type` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `action` varchar(32) NOT NULL DEFAULT '',
  `account_from` varchar(32) NOT NULL DEFAULT '',
  `account_to` varchar(32) NOT NULL DEFAULT '',
  `gid_from` varchar(16) NOT NULL DEFAULT '',
  `gid_to` varchar(16) NOT NULL DEFAULT '',
  `lv_from` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `lv_to` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `pet_iid` varchar(32) NOT NULL DEFAULT '',
  `pet_name` varchar(32) NOT NULL DEFAULT '',
  `pet_property` text NOT NULL,
  `para1` varchar(255) NOT NULL DEFAULT '',
  `para2` varchar(255) NOT NULL DEFAULT '',
  `para3` int(11) NOT NULL DEFAULT '0',
  `memo` text,
  PRIMARY KEY (`id`),
  KEY `update_time` (`update_time`),
  KEY `account_to` (`account_to`),
  KEY `gid_to` (`gid_to`),
  KEY `pet_iid` (`pet_iid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for ip_task_log
-- ----------------------------
DROP TABLE IF EXISTS `ip_task_log`;
CREATE TABLE `ip_task_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `time` varchar(14) NOT NULL DEFAULT '',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `line` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `ip` varchar(32) NOT NULL DEFAULT '',
  `task` varchar(32) NOT NULL DEFAULT '',
  `num` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `memo` text,
  PRIMARY KEY (`id`),
  KEY `time` (`time`),
  KEY `ip` (`ip`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for item_transfer_log
-- ----------------------------
DROP TABLE IF EXISTS `item_transfer_log`;
CREATE TABLE `item_transfer_log` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `line` tinyint(3) unsigned DEFAULT NULL,
  `type` tinyint(3) unsigned DEFAULT NULL,
  `action` varchar(32) NOT NULL DEFAULT '',
  `account_from` varchar(32) DEFAULT NULL,
  `account_to` varchar(32) DEFAULT NULL,
  `gid_from` varchar(16) DEFAULT NULL,
  `gid_to` varchar(16) DEFAULT NULL,
  `lv_from` tinyint(3) unsigned DEFAULT NULL,
  `lv_to` tinyint(3) unsigned DEFAULT NULL,
  `gold_coin_from` int(10) unsigned DEFAULT NULL,
  `gold_coin_to` int(10) unsigned DEFAULT NULL,
  `mac_from` varchar(16) DEFAULT NULL,
  `mac_to` varchar(16) DEFAULT NULL,
  `ip_from` varchar(15) DEFAULT NULL,
  `ip_to` varchar(15) DEFAULT NULL,
  `locality_from` varchar(64) DEFAULT NULL,
  `locality_to` varchar(64) DEFAULT NULL,
  `item_iid` varchar(32) NOT NULL DEFAULT '',
  `item_name` varchar(32) DEFAULT NULL,
  `item_amount` int(10) unsigned DEFAULT NULL,
  `transfer_type` tinyint(3) unsigned DEFAULT NULL,
  `transfer_id` varchar(32) DEFAULT NULL,
  `para1` varchar(32) DEFAULT NULL,
  `para2` varchar(255) DEFAULT NULL,
  `para3` int(10) unsigned DEFAULT NULL,
  `para4` char(16) DEFAULT NULL,
  `memo` text,
  KEY `id` (`id`),
  KEY `account` (`account_from`),
  KEY `gid` (`gid_from`),
  KEY `update_time` (`update_time`),
  KEY `iid` (`item_iid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for item_transfer_log_snap
-- ----------------------------
DROP TABLE IF EXISTS `item_transfer_log_snap`;
CREATE TABLE `item_transfer_log_snap` (
  `snap_id` varchar(32) NOT NULL DEFAULT '',
  `snap_time` char(14) NOT NULL DEFAULT '',
  `id` int(10) unsigned NOT NULL,
  `update_time` char(14) NOT NULL DEFAULT '',
  `line` tinyint(3) unsigned DEFAULT NULL,
  `type` tinyint(3) unsigned DEFAULT NULL,
  `action` varchar(32) NOT NULL DEFAULT '',
  `account_from` varchar(32) DEFAULT NULL,
  `account_to` varchar(32) DEFAULT NULL,
  `gid_from` char(16) DEFAULT NULL,
  `gid_to` char(16) DEFAULT NULL,
  `lv_from` tinyint(3) unsigned DEFAULT NULL,
  `lv_to` tinyint(3) unsigned DEFAULT NULL,
  `gold_coin_from` int(10) unsigned DEFAULT NULL,
  `gold_coin_to` int(10) unsigned DEFAULT NULL,
  `mac_from` char(16) DEFAULT NULL,
  `mac_to` char(16) DEFAULT NULL,
  `ip_from` varchar(15) DEFAULT NULL,
  `ip_to` varchar(15) DEFAULT NULL,
  `locality_from` varchar(64) DEFAULT NULL,
  `locality_to` varchar(64) DEFAULT NULL,
  `item_iid` varchar(32) DEFAULT NULL,
  `item_name` varchar(32) DEFAULT NULL,
  `item_amount` int(10) unsigned DEFAULT NULL,
  `transfer_type` tinyint(3) unsigned DEFAULT NULL,
  `transfer_id` varchar(32) DEFAULT NULL,
  `para1` varchar(32) DEFAULT NULL,
  `para2` varchar(255) DEFAULT NULL,
  `para3` int(10) unsigned DEFAULT NULL,
  `para4` char(16) DEFAULT NULL,
  `memo` text,
  PRIMARY KEY (`snap_id`,`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for level_up_log
-- ----------------------------
DROP TABLE IF EXISTS `level_up_log`;
CREATE TABLE `level_up_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `line` varchar(16) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(32) NOT NULL DEFAULT '',
  `lv` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `gold_coin` int(11) NOT NULL DEFAULT '0',
  `insider` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `upgrade_type` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `ip` varchar(32) NOT NULL DEFAULT '',
  `mac` varchar(32) NOT NULL DEFAULT '',
  `time_used` bigint(20) DEFAULT NULL,
  `last_lv_up` varchar(14) NOT NULL DEFAULT '',
  `create_time` varchar(14) NOT NULL DEFAULT '',
  `newbie` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `para1` int(11) DEFAULT NULL,
  `para2` varchar(32) DEFAULT NULL,
  `para3` varchar(255) DEFAULT NULL,
  `para4` bigint(20) DEFAULT NULL,
  `para5` varchar(255) DEFAULT NULL,
  `memo` text,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `gid` (`gid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for login_log
-- ----------------------------
DROP TABLE IF EXISTS `login_log`;
CREATE TABLE `login_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `ip` varchar(18) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `char_name` varchar(32) NOT NULL DEFAULT '',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `server` varchar(32) NOT NULL DEFAULT '',
  `privilege` int(11) NOT NULL DEFAULT '0',
  `login_mode` int(11) NOT NULL DEFAULT '0',
  `time_used` int(11) NOT NULL DEFAULT '0',
  `login_ip` varchar(15) NOT NULL DEFAULT '',
  `last_login_ip` varchar(15) NOT NULL DEFAULT '',
  `gold_coin` int(11) NOT NULL DEFAULT '0',
  `silver_coin` int(11) NOT NULL DEFAULT '0',
  `insider` int(11) NOT NULL DEFAULT '0',
  `level` int(11) NOT NULL DEFAULT '0',
  `tao` int(11) NOT NULL DEFAULT '0',
  `party` varchar(32) NOT NULL DEFAULT '',
  `family` varchar(32) NOT NULL DEFAULT '',
  `create_time` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(32) NOT NULL DEFAULT '',
  `protect_type` varchar(32) NOT NULL DEFAULT '',
  `upgrade_state` int(11) NOT NULL DEFAULT '0',
  `memo` text NOT NULL,
  KEY `time` (`update_time`),
  KEY `account` (`account`),
  KEY `login_ip` (`login_ip`),
  KEY `id` (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=432 DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for material_log
-- ----------------------------
DROP TABLE IF EXISTS `material_log`;
CREATE TABLE `material_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `server` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '0',
  `gid` varchar(32) NOT NULL DEFAULT '0',
  `lv` int(11) unsigned NOT NULL DEFAULT '0',
  `tao` int(11) unsigned NOT NULL DEFAULT '0',
  `login_ip` varchar(32) NOT NULL DEFAULT '0',
  `mac` varchar(32) NOT NULL DEFAULT '0',
  `gold_coin` int(11) unsigned NOT NULL DEFAULT '0',
  `silver_coin` int(11) unsigned NOT NULL DEFAULT '0',
  `insider` varchar(32) NOT NULL DEFAULT '0',
  `map` varchar(32) NOT NULL DEFAULT '0',
  `item_name` varchar(32) NOT NULL DEFAULT '0',
  `amount` int(11) unsigned NOT NULL DEFAULT '0',
  `lasted_time` int(11) unsigned NOT NULL DEFAULT '0',
  `para1` varchar(128) NOT NULL DEFAULT '0',
  `para2` varchar(128) NOT NULL DEFAULT '0',
  `para3` varchar(128) NOT NULL DEFAULT '0',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for material1_log
-- ----------------------------
DROP TABLE IF EXISTS `material1_log`;
CREATE TABLE `material1_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `server` varchar(32) NOT NULL DEFAULT '',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `line` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(32) NOT NULL DEFAULT '0',
  `lv` int(11) unsigned NOT NULL DEFAULT '0',
  `tao` int(11) unsigned NOT NULL DEFAULT '0',
  `login_ip` varchar(32) NOT NULL DEFAULT '0',
  `mac` varchar(32) NOT NULL DEFAULT '0',
  `gold_coin` int(11) unsigned NOT NULL DEFAULT '0',
  `insider` varchar(32) NOT NULL DEFAULT '0',
  `map` varchar(32) NOT NULL DEFAULT '0',
  `amount` int(11) unsigned NOT NULL DEFAULT '0',
  `lasted_time` int(11) unsigned NOT NULL DEFAULT '0',
  `para1` varchar(128) NOT NULL DEFAULT '0',
  `para2` varchar(128) NOT NULL DEFAULT '0',
  `para3` varchar(128) NOT NULL DEFAULT '0',
  `memo` text,
  PRIMARY KEY (`id`),
  KEY `update_time` (`update_time`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for money_log
-- ----------------------------
DROP TABLE IF EXISTS `money_log`;
CREATE TABLE `money_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `type` tinyint(4) NOT NULL DEFAULT '0',
  `action` tinyint(4) NOT NULL DEFAULT '0',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `gid_op` varchar(16) DEFAULT NULL,
  `cash` int(11) DEFAULT NULL,
  `voucher` int(11) DEFAULT NULL,
  `after_value` varchar(64) DEFAULT NULL,
  `memo` text,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `type` (`type`),
  KEY `action` (`action`),
  KEY `gid` (`gid`)
) ENGINE=InnoDB AUTO_INCREMENT=365 DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for money1_log
-- ----------------------------
DROP TABLE IF EXISTS `money1_log`;
CREATE TABLE `money1_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `action` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `lv` int(11) NOT NULL DEFAULT '0',
  `gold_coin` int(11) NOT NULL DEFAULT '0',
  `ip` varchar(15) NOT NULL DEFAULT '',
  `mac` varchar(32) NOT NULL DEFAULT '',
  `cash_type` tinyint(4) NOT NULL DEFAULT '0',
  `amount` int(11) NOT NULL DEFAULT '0',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `update_time` (`update_time`),
  KEY `action` (`action`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for nummary_log
-- ----------------------------
DROP TABLE IF EXISTS `nummary_log`;
CREATE TABLE `nummary_log` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `type` tinyint(4) unsigned DEFAULT NULL,
  `action` varchar(32) NOT NULL DEFAULT '',
  `totality` bigint(20) unsigned NOT NULL DEFAULT '0',
  `times` mediumint(9) unsigned DEFAULT NULL,
  `gids` mediumint(9) unsigned DEFAULT NULL,
  `para1` int(11) DEFAULT NULL,
  `para2` varchar(32) DEFAULT NULL,
  `para3` varchar(255) DEFAULT NULL,
  `memo` text,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `type_action` (`type`,`action`)
) ENGINE=InnoDB AUTO_INCREMENT=66 DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for official_stat_log
-- ----------------------------
DROP TABLE IF EXISTS `official_stat_log`;
CREATE TABLE `official_stat_log` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `update_time` char(14) NOT NULL DEFAULT '',
  `type` tinyint(4) NOT NULL DEFAULT '0',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(32) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `para1` varchar(128) DEFAULT '',
  `para2` varchar(128) DEFAULT '',
  `para3` varchar(255) DEFAULT '',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `type` (`type`),
  KEY `gid` (`gid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for pet_level_up_log
-- ----------------------------
DROP TABLE IF EXISTS `pet_level_up_log`;
CREATE TABLE `pet_level_up_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `iid` varchar(32) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `lv` int(4) NOT NULL DEFAULT '0',
  `last_lv_up` varchar(14) NOT NULL DEFAULT '',
  `para1` int(11) NOT NULL DEFAULT '0',
  `para2` varchar(32) NOT NULL DEFAULT '',
  `para3` varchar(255) NOT NULL DEFAULT '',
  `memo` text NOT NULL,
  KEY `id` (`id`),
  KEY `time` (`update_time`),
  KEY `iid` (`iid`),
  KEY `gid` (`gid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for pet_log
-- ----------------------------
DROP TABLE IF EXISTS `pet_log`;
CREATE TABLE `pet_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `type` varchar(32) NOT NULL DEFAULT '',
  `action` varchar(32) NOT NULL DEFAULT '',
  `pet_name` varchar(16) NOT NULL DEFAULT '',
  `pet_iid` varchar(32) NOT NULL DEFAULT '',
  `cost_item` varchar(16) NOT NULL DEFAULT '',
  `item_iid` varchar(32) NOT NULL DEFAULT '',
  `para1` varchar(32) NOT NULL DEFAULT '',
  `para2` varchar(32) NOT NULL DEFAULT '',
  `para3` varchar(128) NOT NULL DEFAULT '',
  `pet_memo` text,
  `memo` text,
  KEY `id` (`id`),
  KEY `update_time` (`update_time`),
  KEY `pet_iid` (`pet_iid`),
  KEY `item_iid` (`item_iid`),
  KEY `action` (`action`),
  KEY `gid` (`gid`)
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for plug_log
-- ----------------------------
DROP TABLE IF EXISTS `plug_log`;
CREATE TABLE `plug_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `tool` int(11) NOT NULL DEFAULT '0',
  `time` varchar(14) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `ip` varchar(15) NOT NULL DEFAULT '',
  `level` int(11) NOT NULL DEFAULT '0',
  `coins` int(11) NOT NULL DEFAULT '0',
  `relevance` int(11) NOT NULL DEFAULT '0',
  `action` varchar(32) NOT NULL DEFAULT '',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `tool` (`tool`),
  KEY `time` (`time`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for property_bind_log
-- ----------------------------
DROP TABLE IF EXISTS `property_bind_log`;
CREATE TABLE `property_bind_log` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `type` varchar(32) NOT NULL DEFAULT '',
  `owner` varchar(16) NOT NULL DEFAULT '',
  `iid` varchar(32) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `para` varchar(32) DEFAULT NULL,
  `main` tinyint(4) unsigned DEFAULT '0',
  `data` text,
  `memo` text,
  KEY `id` (`id`),
  KEY `update_time` (`update_time`),
  KEY `iid` (`iid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for purchase_platform_log
-- ----------------------------
DROP TABLE IF EXISTS `purchase_platform_log`;
CREATE TABLE `purchase_platform_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `server` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `insider` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `lv` int(11) NOT NULL DEFAULT '0',
  `type` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `order_id` varchar(32) NOT NULL DEFAULT '',
  `order_time` int(11) unsigned NOT NULL DEFAULT '0',
  `commodity_type` int(11) unsigned NOT NULL DEFAULT '0',
  `commodity_name` varchar(64) NOT NULL DEFAULT '',
  `commodity_iid` varchar(32) NOT NULL DEFAULT '0',
  `commodity_amount` int(11) NOT NULL DEFAULT '0',
  `unit_price` int(11) NOT NULL DEFAULT '0',
  `turnover` int(11) NOT NULL DEFAULT '0',
  `tax` int(11) NOT NULL DEFAULT '0',
  `para1` int(11) NOT NULL DEFAULT '0',
  `para2` varchar(32) NOT NULL DEFAULT '0',
  `para3` varchar(32) NOT NULL DEFAULT '0',
  `para4` varchar(128) NOT NULL DEFAULT '0',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `gid` (`gid`),
  KEY `order_id` (`order_id`),
  KEY `time_account` (`update_time`,`account`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for report_log
-- ----------------------------
DROP TABLE IF EXISTS `report_log`;
CREATE TABLE `report_log` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `server` varchar(32) NOT NULL DEFAULT '',
  `account1` varchar(32) NOT NULL DEFAULT '',
  `name1` varchar(32) NOT NULL DEFAULT '',
  `gid1` varchar(32) NOT NULL DEFAULT '',
  `lv1` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `tao1` int(11) unsigned NOT NULL DEFAULT '0',
  `gold_coin1` int(11) NOT NULL DEFAULT '0',
  `account2` varchar(32) NOT NULL DEFAULT '',
  `name2` varchar(32) NOT NULL DEFAULT '',
  `gid2` varchar(32) NOT NULL DEFAULT '',
  `lv2` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `tao2` int(11) unsigned NOT NULL DEFAULT '0',
  `gold_coin2` int(11) NOT NULL DEFAULT '0',
  `para1` varchar(32) DEFAULT '',
  `para2` varchar(32) DEFAULT '',
  `para3` varchar(255) DEFAULT '',
  `para4` varchar(255) DEFAULT '',
  `para5` varchar(255) DEFAULT '',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `account1` (`account1`),
  KEY `account2` (`account2`),
  KEY `name2` (`name2`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for safe_log
-- ----------------------------
DROP TABLE IF EXISTS `safe_log`;
CREATE TABLE `safe_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `type` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `action` varchar(32) NOT NULL DEFAULT '',
  `ip` varchar(15) NOT NULL DEFAULT '',
  `lv` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `tao` int(10) unsigned NOT NULL DEFAULT '0',
  `ob_iid` varchar(32) NOT NULL DEFAULT '',
  `ob_name` varchar(32) NOT NULL DEFAULT '',
  `para1` varchar(32) DEFAULT NULL,
  `para2` varchar(255) DEFAULT NULL,
  `para3` int(11) DEFAULT NULL,
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `update_time` (`update_time`),
  KEY `gid` (`gid`),
  KEY `action` (`action`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for sale_log
-- ----------------------------
DROP TABLE IF EXISTS `sale_log`;
CREATE TABLE `sale_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `dist` varchar(16) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `type` tinyint(4) NOT NULL DEFAULT '0',
  `item_name` varchar(32) NOT NULL DEFAULT '',
  `amount` int(11) NOT NULL DEFAULT '0',
  `money` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `update_time` (`update_time`),
  KEY `gid` (`gid`),
  KEY `type` (`type`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for stat
-- ----------------------------
DROP TABLE IF EXISTS `stat`;
CREATE TABLE `stat` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) DEFAULT NULL,
  `pid` varchar(6) DEFAULT NULL,
  `server` varchar(20) DEFAULT NULL,
  `cpu` varchar(6) DEFAULT NULL,
  `vmem` varchar(6) DEFAULT NULL,
  `rmem` varchar(6) DEFAULT NULL,
  `timer` varchar(10) DEFAULT '',
  `online` varchar(6) DEFAULT '',
  `combat` varchar(10) DEFAULT '',
  `pend` varchar(10) DEFAULT '',
  `memo` text,
  PRIMARY KEY (`id`),
  KEY `update_time` (`update_time`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for switch_log
-- ----------------------------
DROP TABLE IF EXISTS `switch_log`;
CREATE TABLE `switch_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `line` tinyint(2) unsigned NOT NULL DEFAULT '0',
  `account` varchar(32) NOT NULL DEFAULT '',
  `char_name` varchar(32) NOT NULL DEFAULT '',
  `login_mode` tinyint(2) NOT NULL DEFAULT '0',
  `time_used` int(11) NOT NULL DEFAULT '0',
  `login_ip` varchar(15) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `memo` text NOT NULL,
  KEY `id` (`id`),
  KEY `time` (`update_time`),
  KEY `account` (`account`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for system_monitor_log
-- ----------------------------
DROP TABLE IF EXISTS `system_monitor_log`;
CREATE TABLE `system_monitor_log` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `type` varchar(32) NOT NULL DEFAULT '',
  `action` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `number_ih` int(11) NOT NULL DEFAULT '0',
  `number_il` int(11) NOT NULL DEFAULT '0',
  `degree` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(32) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `lv` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `gold_coin` int(11) unsigned NOT NULL DEFAULT '0',
  `ip` varchar(32) NOT NULL DEFAULT '',
  `mac` varchar(32) NOT NULL DEFAULT '',
  `para1` varchar(32) NOT NULL DEFAULT '',
  `para2` varchar(32) NOT NULL DEFAULT '',
  `para3` varchar(255) NOT NULL DEFAULT '',
  `memo` text,
  PRIMARY KEY (`id`),
  KEY `update_time` (`update_time`),
  KEY `degree` (`degree`),
  KEY `type` (`type`),
  KEY `action` (`action`),
  KEY `gid` (`gid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for tao_stat_log
-- ----------------------------
DROP TABLE IF EXISTS `tao_stat_log`;
CREATE TABLE `tao_stat_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` char(14) NOT NULL DEFAULT '',
  `finish_date` char(8) NOT NULL DEFAULT '',
  `dist_id` int(11) NOT NULL DEFAULT '0',
  `dist_name` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` char(16) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `lv` int(11) NOT NULL DEFAULT '0',
  `gold_coin` int(11) NOT NULL DEFAULT '0',
  `tao` int(11) NOT NULL DEFAULT '0',
  `ip` char(15) NOT NULL DEFAULT '',
  `gid_times` int(11) NOT NULL DEFAULT '0',
  `gid_charge_times` int(11) NOT NULL DEFAULT '0',
  `lv_charge_times` int(11) NOT NULL DEFAULT '0',
  `ip_times` int(11) NOT NULL DEFAULT '0',
  `ip_gids` int(11) NOT NULL DEFAULT '0',
  `punish` tinyint(4) NOT NULL DEFAULT '0',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `gid` (`gid`),
  KEY `ip` (`ip`),
  KEY `finish_date` (`finish_date`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for tao_task_log
-- ----------------------------
DROP TABLE IF EXISTS `tao_task_log`;
CREATE TABLE `tao_task_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` char(14) NOT NULL DEFAULT '',
  `dist_id` int(11) NOT NULL DEFAULT '0',
  `dist_name` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` char(16) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `lv` int(11) NOT NULL DEFAULT '0',
  `gold_coin` int(11) NOT NULL DEFAULT '0',
  `tao` int(11) NOT NULL DEFAULT '0',
  `ip` char(15) NOT NULL DEFAULT '',
  `mac` char(16) NOT NULL DEFAULT '',
  `action` varchar(32) NOT NULL DEFAULT '',
  `use_charge` tinyint(4) NOT NULL DEFAULT '0',
  `lv_charge` tinyint(4) NOT NULL DEFAULT '0',
  `lv_cha` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `update_time` (`update_time`),
  KEY `gid` (`gid`),
  KEY `ip` (`ip`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for task_log
-- ----------------------------
DROP TABLE IF EXISTS `task_log`;
CREATE TABLE `task_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(16) NOT NULL DEFAULT '',
  `dist` varchar(16) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `task_type` varchar(16) NOT NULL DEFAULT '',
  `task_name` varchar(32) NOT NULL DEFAULT '',
  `action` varchar(64) NOT NULL DEFAULT '',
  `ip` varchar(16) NOT NULL DEFAULT '',
  `line` varchar(16) NOT NULL DEFAULT '',
  `level` int(11) NOT NULL DEFAULT '0',
  `tao` int(11) NOT NULL DEFAULT '0',
  `gold_coin` int(11) NOT NULL DEFAULT '0',
  `insider` int(11) NOT NULL DEFAULT '0',
  `para1` varchar(32) NOT NULL DEFAULT '',
  `para2` varchar(32) NOT NULL DEFAULT '',
  `para3` varchar(32) NOT NULL DEFAULT '',
  `memo` text,
  PRIMARY KEY (`id`),
  KEY `update_time` (`update_time`),
  KEY `gid` (`gid`),
  KEY `task_type` (`task_type`)
) ENGINE=InnoDB AUTO_INCREMENT=51 DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for user_log
-- ----------------------------
DROP TABLE IF EXISTS `user_log`;
CREATE TABLE `user_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `server` varchar(32) NOT NULL DEFAULT '',
  `type` varchar(32) NOT NULL DEFAULT '0',
  `severity` int(11) unsigned NOT NULL DEFAULT '0',
  `action` varchar(32) NOT NULL DEFAULT '0',
  `para1` varchar(128) NOT NULL DEFAULT '',
  `para2` varchar(128) NOT NULL DEFAULT '0',
  `para3` varchar(128) NOT NULL DEFAULT '0',
  `memo` text NOT NULL,
  KEY `time` (`update_time`),
  KEY `id` (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=716 DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for user_report_ip_log
-- ----------------------------
DROP TABLE IF EXISTS `user_report_ip_log`;
CREATE TABLE `user_report_ip_log` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `server` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(32) NOT NULL DEFAULT '',
  `lv` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `para1` varchar(255) NOT NULL DEFAULT '',
  `para2` varchar(32) NOT NULL DEFAULT '',
  `para3` varchar(32) NOT NULL DEFAULT '',
  `para4` varchar(32) NOT NULL DEFAULT '',
  `para5` varchar(32) NOT NULL DEFAULT '',
  `memo` text,
  PRIMARY KEY (`id`),
  KEY `gid` (`gid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for wishing_log
-- ----------------------------
DROP TABLE IF EXISTS `wishing_log`;
CREATE TABLE `wishing_log` (
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `score` int(11) NOT NULL DEFAULT '0',
  `memo` text NOT NULL,
  PRIMARY KEY (`gid`),
  KEY `update_time` (`update_time`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for zq_add_point_log
-- ----------------------------
DROP TABLE IF EXISTS `zq_add_point_log`;
CREATE TABLE `zq_add_point_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `line` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `char_name` varchar(32) NOT NULL DEFAULT '',
  `create_time` varchar(14) NOT NULL DEFAULT '',
  `ip` varchar(18) NOT NULL DEFAULT '',
  `mac` varchar(16) NOT NULL DEFAULT '',
  `point_before` int(11) NOT NULL DEFAULT '0',
  `point_after` int(11) NOT NULL DEFAULT '0',
  `point_add` int(11) NOT NULL DEFAULT '0',
  `action` varchar(32) NOT NULL DEFAULT '',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `account` (`account`),
  KEY `action` (`action`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for zq_card_log
-- ----------------------------
DROP TABLE IF EXISTS `zq_card_log`;
CREATE TABLE `zq_card_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `line` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `char_name` varchar(32) NOT NULL DEFAULT '',
  `create_time` varchar(14) NOT NULL DEFAULT '',
  `ip` varchar(18) NOT NULL DEFAULT '',
  `mac` varchar(16) NOT NULL DEFAULT '',
  `action` varchar(32) NOT NULL DEFAULT '',
  `bonus_name` varchar(32) NOT NULL DEFAULT '',
  `bonus_prop` varchar(32) DEFAULT NULL,
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `account` (`account`),
  KEY `action` (`action`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for zq_combat_log
-- ----------------------------
DROP TABLE IF EXISTS `zq_combat_log`;
CREATE TABLE `zq_combat_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `line` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `char_name` varchar(32) NOT NULL DEFAULT '',
  `create_time` varchar(14) NOT NULL DEFAULT '',
  `ip` varchar(18) NOT NULL DEFAULT '',
  `mac` varchar(16) NOT NULL DEFAULT '',
  `action` varchar(32) NOT NULL DEFAULT '',
  `total_round` int(11) NOT NULL DEFAULT '0',
  `hero_name` varchar(32) NOT NULL DEFAULT '',
  `hero_lv` int(11) NOT NULL DEFAULT '0',
  `result` tinyint(4) NOT NULL DEFAULT '0',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `account` (`account`),
  KEY `action` (`action`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for zq_cost_point_log
-- ----------------------------
DROP TABLE IF EXISTS `zq_cost_point_log`;
CREATE TABLE `zq_cost_point_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `line` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `char_name` varchar(32) NOT NULL DEFAULT '',
  `create_time` varchar(14) NOT NULL DEFAULT '',
  `ip` varchar(18) NOT NULL DEFAULT '',
  `mac` varchar(16) NOT NULL DEFAULT '',
  `point_before` int(11) NOT NULL DEFAULT '0',
  `point_after` int(11) NOT NULL DEFAULT '0',
  `point_cost` int(11) NOT NULL DEFAULT '0',
  `action` varchar(32) NOT NULL DEFAULT '',
  `item_name` varchar(32) NOT NULL DEFAULT '',
  `amount` int(11) NOT NULL DEFAULT '0',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `account` (`account`),
  KEY `action` (`action`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for zq_errand_log
-- ----------------------------
DROP TABLE IF EXISTS `zq_errand_log`;
CREATE TABLE `zq_errand_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `line` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `char_name` varchar(32) NOT NULL DEFAULT '',
  `create_time` varchar(14) NOT NULL DEFAULT '',
  `ip` varchar(18) NOT NULL DEFAULT '',
  `mac` varchar(16) NOT NULL DEFAULT '',
  `type` varchar(32) NOT NULL DEFAULT '',
  `action` varchar(32) NOT NULL DEFAULT '',
  `bonus_type` tinyint(4) NOT NULL DEFAULT '0',
  `bonus_name` varchar(32) NOT NULL DEFAULT '',
  `bonus_prop` varchar(32) DEFAULT NULL,
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `account` (`account`),
  KEY `action` (`action`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for zq_login_log
-- ----------------------------
DROP TABLE IF EXISTS `zq_login_log`;
CREATE TABLE `zq_login_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `line` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `char_name` varchar(32) NOT NULL DEFAULT '',
  `create_time` varchar(14) NOT NULL DEFAULT '',
  `ip` varchar(18) NOT NULL DEFAULT '',
  `mac` varchar(16) NOT NULL DEFAULT '',
  `privilege` int(11) NOT NULL DEFAULT '0',
  `login_mode` int(11) NOT NULL DEFAULT '0',
  `time_used` int(11) NOT NULL DEFAULT '0',
  `hero_lv` int(11) NOT NULL DEFAULT '0',
  `xg_point` int(11) NOT NULL DEFAULT '0',
  `card_num` int(11) NOT NULL DEFAULT '0',
  `fragment_num` int(11) NOT NULL DEFAULT '0',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `account` (`account`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for zq_lv_up_log
-- ----------------------------
DROP TABLE IF EXISTS `zq_lv_up_log`;
CREATE TABLE `zq_lv_up_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `line` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `char_name` varchar(32) NOT NULL DEFAULT '',
  `create_time` varchar(14) NOT NULL DEFAULT '',
  `ip` varchar(18) NOT NULL DEFAULT '',
  `mac` varchar(16) NOT NULL DEFAULT '',
  `hero_name` varchar(32) NOT NULL DEFAULT '',
  `lv` int(11) NOT NULL DEFAULT '0',
  `bonus_name` varchar(32) NOT NULL DEFAULT '',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `account` (`account`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for channel_log_20161229
-- ----------------------------
DROP TABLE IF EXISTS `channel_log_20161229`;
CREATE TABLE `channel_log_20161229` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `update_time` char(14) NOT NULL DEFAULT '',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(32) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `lv` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `channel` int(11) NOT NULL DEFAULT '0',
  `target` varchar(32) NOT NULL DEFAULT '',
  `msg` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `gid` (`gid`),
  KEY `sender` (`name`),
  KEY `target` (`target`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

