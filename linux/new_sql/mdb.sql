/*
Navicat MySQL Data Transfer

Source Server         : 192.168.200.128
Source Server Version : 50173
Source Host           : 192.168.200.128:3306
Source Database       : mdb

Target Server Type    : MYSQL
Target Server Version : 50173
File Encoding         : 936

Date: 2026-07-12 23:02:55
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for char_info
-- ----------------------------
DROP TABLE IF EXISTS `char_info`;
CREATE TABLE `char_info` (
  `update_time` varchar(14) NOT NULL DEFAULT '' COMMENT '更新时间',
  `dist` int(11) NOT NULL DEFAULT '0' COMMENT '区组',
  `gid` varchar(32) NOT NULL DEFAULT '' COMMENT '角色',
  `name` varchar(32) NOT NULL DEFAULT '' COMMENT '姓名',
  `account` varchar(32) NOT NULL DEFAULT '' COMMENT '账号',
  `create_time` varchar(14) NOT NULL DEFAULT '' COMMENT '生日',
  `first_login_time` varchar(14) NOT NULL DEFAULT '' COMMENT '最初登录时间',
  `last_login_time` varchar(14) NOT NULL DEFAULT '' COMMENT '最后登录时间',
  `last_logout_time` varchar(14) NOT NULL DEFAULT '' COMMENT '最后登出时间',
  `first_login_ip` varchar(15) NOT NULL DEFAULT '' COMMENT '最初登录IP',
  `last_login_ip` varchar(15) NOT NULL DEFAULT '' COMMENT '最后登录IP',
  `family` varchar(32) NOT NULL DEFAULT '' COMMENT '门派',
  `last_privilege` int(11) NOT NULL DEFAULT '0' COMMENT '权限',
  `activity_info` text COMMENT '活动资讯',
  `level` int(11) NOT NULL DEFAULT '0' COMMENT '等级',
  `tao` int(11) NOT NULL DEFAULT '0' COMMENT '道行',
  `exp` int(11) NOT NULL DEFAULT '0' COMMENT '经验',
  `char_status` varchar(32) NOT NULL DEFAULT '' COMMENT '角色状态',
  `party_name` varchar(32) NOT NULL DEFAULT '' COMMENT '帮派',
  `party_job` varchar(32) NOT NULL DEFAULT '' COMMENT '帮派职务',
  `cash` int(11) NOT NULL DEFAULT '0' COMMENT '金钱',
  `balance` int(11) NOT NULL DEFAULT '0' COMMENT '存款',
  `gold_coin` int(11) NOT NULL DEFAULT '0',
  `silver_coin` int(11) NOT NULL DEFAULT '0',
  `gender` int(11) NOT NULL DEFAULT '0' COMMENT '性别',
  `title` varchar(32) NOT NULL DEFAULT '' COMMENT '当前称谓',
  `str` int(11) NOT NULL DEFAULT '0' COMMENT '力量',
  `con` int(11) NOT NULL DEFAULT '0' COMMENT '体质',
  `dex` int(11) NOT NULL DEFAULT '0' COMMENT '敏捷',
  `wiz` int(11) NOT NULL DEFAULT '0' COMMENT '灵力',
  `phy_power` int(11) NOT NULL DEFAULT '0' COMMENT '物理攻击',
  `mag_power` int(11) NOT NULL DEFAULT '0' COMMENT '法术攻击',
  `speed` int(11) NOT NULL DEFAULT '0' COMMENT '速度',
  `phy_def` int(11) NOT NULL DEFAULT '0',
  `mag_def` int(11) NOT NULL DEFAULT '0',
  `acc_blocked_time` int(11) NOT NULL DEFAULT '0' COMMENT '封号时间',
  `charge_items_value` int(11) NOT NULL DEFAULT '0' COMMENT '收费道具总价值',
  `deny_login` int(11) NOT NULL DEFAULT '0' COMMENT '禁止登入',
  PRIMARY KEY (`gid`),
  KEY `account` (`account`),
  KEY `tao_level` (`tao`,`level`),
  KEY `level` (`level`),
  KEY `cash` (`cash`,`balance`),
  KEY `phy_power` (`phy_power`),
  KEY `mag_power` (`mag_power`),
  KEY `speed` (`speed`),
  KEY `phy_def` (`phy_def`),
  KEY `mag_def` (`mag_def`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for item_info
-- ----------------------------
DROP TABLE IF EXISTS `item_info`;
CREATE TABLE `item_info` (
  `update_time` varchar(14) NOT NULL DEFAULT '' COMMENT '更新时间',
  `dist` int(11) NOT NULL DEFAULT '0' COMMENT '区组',
  `owner` varchar(32) NOT NULL DEFAULT '0' COMMENT '角色',
  `env` varchar(128) NOT NULL DEFAULT '' COMMENT '位置',
  `pos` int(11) NOT NULL DEFAULT '0' COMMENT '序号',
  `class_id` int(11) NOT NULL DEFAULT '0',
  `owner_name` varchar(32) NOT NULL DEFAULT '' COMMENT '姓名',
  `owner_privilege` int(11) NOT NULL DEFAULT '0',
  `durability` int(11) NOT NULL DEFAULT '0' COMMENT '耐久度',
  `amount` int(11) NOT NULL DEFAULT '0' COMMENT '数量',
  `iid` varchar(32) NOT NULL DEFAULT '',
  `rebuild_level` int(11) NOT NULL DEFAULT '0' COMMENT '改造等级',
  `color` varchar(16) NOT NULL DEFAULT '0' COMMENT '颜色',
  `top_index` int(11) NOT NULL DEFAULT '0',
  `perfect_degree` int(11) NOT NULL DEFAULT '0' COMMENT '完美度',
  `repair_ver` tinyint(4) NOT NULL DEFAULT '0',
  `save_to_text` text NOT NULL COMMENT '存档属性',
  `pet_iid` varchar(32) NOT NULL DEFAULT '',
  PRIMARY KEY (`owner`,`pos`,`env`,`pet_iid`),
  KEY `top_index` (`rebuild_level`,`top_index`,`color`,`perfect_degree`),
  KEY `iid` (`iid`),
  KEY `update_time` (`update_time`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for party_basic_info
-- ----------------------------
DROP TABLE IF EXISTS `party_basic_info`;
CREATE TABLE `party_basic_info` (
  `update_time` varchar(14) NOT NULL DEFAULT '' COMMENT '更新时间',
  `dist` int(11) NOT NULL DEFAULT '0' COMMENT '区组',
  `id` int(11) NOT NULL DEFAULT '0',
  `gid` varchar(20) NOT NULL DEFAULT '' COMMENT '帮派GID',
  `name` varchar(32) NOT NULL DEFAULT '' COMMENT '帮派名',
  `creator` varchar(100) NOT NULL DEFAULT '' COMMENT '帮派创建人GID',
  `level` int(11) NOT NULL DEFAULT '0' COMMENT '帮派等级',
  `prestige` int(11) NOT NULL DEFAULT '0',
  `reputation` int(11) NOT NULL DEFAULT '0',
  `money` int(11) NOT NULL DEFAULT '0' COMMENT '帮派资金',
  `online_msg` varchar(255) NOT NULL DEFAULT '',
  `win_times` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`gid`),
  KEY `win_times` (`win_times`,`level`),
  KEY `money` (`money`,`level`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for party_members_info
-- ----------------------------
DROP TABLE IF EXISTS `party_members_info`;
CREATE TABLE `party_members_info` (
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `dist` int(11) NOT NULL DEFAULT '0',
  `gid` varchar(20) NOT NULL DEFAULT '',
  `party_contribution` int(11) NOT NULL DEFAULT '0',
  `party_job` varchar(100) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `member_gid` varchar(20) NOT NULL DEFAULT '',
  PRIMARY KEY (`gid`,`member_gid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for pet_info
-- ----------------------------
DROP TABLE IF EXISTS `pet_info`;
CREATE TABLE `pet_info` (
  `update_time` varchar(14) NOT NULL DEFAULT '' COMMENT '更新时间',
  `dist` int(11) NOT NULL DEFAULT '0' COMMENT '区组',
  `owner` varchar(32) NOT NULL DEFAULT '' COMMENT '角色',
  `owner_name` varchar(32) NOT NULL DEFAULT '' COMMENT '姓名',
  `owner_privilege` int(11) NOT NULL DEFAULT '0',
  `name` varchar(32) NOT NULL DEFAULT '' COMMENT '宠物名',
  `class_id` int(11) NOT NULL DEFAULT '0',
  `env` varchar(128) NOT NULL DEFAULT '' COMMENT '位置',
  `pos` int(11) NOT NULL DEFAULT '0' COMMENT '序号',
  `longevity` int(11) NOT NULL DEFAULT '0' COMMENT '寿命',
  `iid` varchar(32) NOT NULL DEFAULT '',
  `loyalty` int(11) NOT NULL DEFAULT '0' COMMENT '忠诚度',
  `level` int(11) NOT NULL DEFAULT '0' COMMENT '等级',
  `exp` int(11) NOT NULL DEFAULT '0' COMMENT '经验',
  `str` int(11) NOT NULL DEFAULT '0' COMMENT '力量',
  `con` int(11) NOT NULL DEFAULT '0' COMMENT '体质',
  `dex` int(11) NOT NULL DEFAULT '0' COMMENT '敏捷',
  `wiz` int(11) NOT NULL DEFAULT '0' COMMENT '灵力',
  `talent` int(11) NOT NULL DEFAULT '0',
  `spiritualism` int(11) NOT NULL DEFAULT '0',
  `martial` int(11) NOT NULL DEFAULT '0' COMMENT '武学',
  `f_ride` tinyint(4) NOT NULL DEFAULT '0',
  `f_fly` tinyint(4) NOT NULL DEFAULT '0',
  `f_attrib` tinyint(4) NOT NULL DEFAULT '0',
  `f_ride_with` tinyint(4) NOT NULL DEFAULT '0',
  `phy_power` int(11) NOT NULL DEFAULT '0' COMMENT '物理攻击',
  `mag_power` int(11) NOT NULL DEFAULT '0' COMMENT '法术攻击',
  `speed` int(11) NOT NULL DEFAULT '0' COMMENT '速度',
  `phy_def` int(11) NOT NULL DEFAULT '0',
  `mag_def` int(11) NOT NULL DEFAULT '0',
  `repair_ver` tinyint(4) NOT NULL DEFAULT '0',
  `save_to_text` text NOT NULL COMMENT '存档属性',
  PRIMARY KEY (`owner`,`pos`,`env`),
  KEY `phy_power` (`phy_power`),
  KEY `mag_power` (`mag_power`),
  KEY `speed` (`speed`),
  KEY `phy_def` (`phy_def`),
  KEY `mag_def` (`mag_def`),
  KEY `martial` (`martial`),
  KEY `iid` (`iid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for server_status
-- ----------------------------
DROP TABLE IF EXISTS `server_status`;
CREATE TABLE `server_status` (
  `server` varchar(32) NOT NULL DEFAULT '',
  `dist` int(11) NOT NULL DEFAULT '0',
  `available` int(11) NOT NULL DEFAULT '0',
  `online` int(11) NOT NULL DEFAULT '0',
  `max_user` int(11) NOT NULL DEFAULT '0',
  `cpu_cost` int(11) NOT NULL DEFAULT '0',
  `cpu_satisfy` int(11) NOT NULL DEFAULT '0',
  `report_time` varchar(32) NOT NULL DEFAULT '',
  PRIMARY KEY (`server`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for shop_info
-- ----------------------------
DROP TABLE IF EXISTS `shop_info`;
CREATE TABLE `shop_info` (
  `update_time` varchar(14) NOT NULL DEFAULT '' COMMENT '更新时间',
  `dist` int(11) NOT NULL DEFAULT '0' COMMENT '区组',
  `owner` varchar(32) NOT NULL DEFAULT '' COMMENT '角色',
  `shop_id` varchar(128) NOT NULL DEFAULT '' COMMENT '商店ID',
  `shop_name` varchar(64) NOT NULL DEFAULT '' COMMENT '商店名称',
  `shop_owner` varchar(32) NOT NULL DEFAULT '' COMMENT '商店所有者',
  `shop_counters` int(11) NOT NULL DEFAULT '0' COMMENT '柜台数',
  `shop_base_money` int(11) NOT NULL DEFAULT '0' COMMENT '基础营运资金',
  `shop_run_money` int(11) NOT NULL DEFAULT '0' COMMENT '营业利润',
  `shop_state` varchar(20) NOT NULL DEFAULT '' COMMENT '商店状态',
  `shop_level` int(11) NOT NULL DEFAULT '0' COMMENT '商店等级',
  `shop_type` varchar(20) NOT NULL DEFAULT '' COMMENT '商店类型',
  `shop_credit_rank` int(11) NOT NULL DEFAULT '0' COMMENT '商店信用度等级',
  `shop_credit_value` int(11) NOT NULL DEFAULT '0' COMMENT '商店信用度值',
  `shop_rate` float NOT NULL DEFAULT '0' COMMENT '交易税率',
  PRIMARY KEY (`owner`,`shop_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for villa
-- ----------------------------
DROP TABLE IF EXISTS `villa`;
CREATE TABLE `villa` (
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `dist` int(11) NOT NULL DEFAULT '0',
  `villa_iid` varchar(32) NOT NULL DEFAULT '',
  `villa_id` int(11) NOT NULL DEFAULT '0',
  `gid` varchar(32) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `df_level` int(6) NOT NULL DEFAULT '0',
  `wind` int(11) NOT NULL DEFAULT '0',
  `fire` int(11) NOT NULL DEFAULT '0',
  `earth` int(11) NOT NULL DEFAULT '0',
  `water` int(11) NOT NULL DEFAULT '0',
  `nimbus` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`villa_iid`),
  KEY `gid` (`gid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for child_info
-- ----------------------------
DROP TABLE IF EXISTS `child_info`;
CREATE TABLE `child_info` (
  `dist` int(11) DEFAULT NULL COMMENT '区组',
  `owner` varchar(32) DEFAULT NULL COMMENT '角色gid',
  `owner_name` varchar(32) DEFAULT NULL COMMENT '角色名',
  `iid` varchar(32) DEFAULT NULL COMMENT '娃娃iid',
  `name` varchar(32) DEFAULT NULL COMMENT '娃娃名',
  `level` int(11) DEFAULT NULL COMMENT '等级',
  `exp` int(12) DEFAULT NULL COMMENT '经验',
  `str` int(11) DEFAULT NULL COMMENT '力气',
  `wisdom` int(11) DEFAULT NULL COMMENT '智慧',
  `dex` int(11) DEFAULT NULL COMMENT '灵敏',
  `physique` int(11) DEFAULT NULL,
  `str_effect` int(11) DEFAULT NULL COMMENT '力气成长',
  `wit_effect` int(11) DEFAULT NULL COMMENT '智慧成长',
  `dex_effect` int(11) DEFAULT NULL COMMENT '灵敏成长',
  `phy_effect` int(11) DEFAULT NULL COMMENT '体魄成长',
  `save_to_text` text NOT NULL COMMENT '娃娃数据',
  `update_time` varchar(14) DEFAULT '' COMMENT '更新时间'
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for party_skills_info
-- ----------------------------
DROP TABLE IF EXISTS `party_skills_info`;
CREATE TABLE `party_skills_info` (
  `dist` int(32) DEFAULT NULL COMMENT '区组',
  `gid` int(11) DEFAULT NULL COMMENT '帮派GID',
  `name` varchar(32) DEFAULT NULL COMMENT '帮派名',
  `skill` varchar(32) DEFAULT NULL COMMENT '技能名称',
  `skill_score` int(32) DEFAULT NULL COMMENT '技能研发度',
  `skill_level` tinyint(11) DEFAULT NULL COMMENT '技能等级',
  `skill_next_level_score` int(32) DEFAULT NULL COMMENT '下一级需要的技能研发度',
  `update_time` varchar(14) DEFAULT NULL COMMENT '更新时间'
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for upgrade_char_info
-- ----------------------------
DROP TABLE IF EXISTS `upgrade_char_info`;
CREATE TABLE `upgrade_char_info` (
  `dist` int(11) DEFAULT '0' COMMENT '区组',
  `gid` varchar(20) NOT NULL COMMENT '角色',
  `name` varchar(32) DEFAULT NULL COMMENT '姓名',
  `account` varchar(32) DEFAULT NULL COMMENT '账号',
  `create_time` varchar(14) DEFAULT NULL COMMENT '生日',
  `upgrade_type` int(11) DEFAULT NULL COMMENT '飞升类型',
  `last_login_time` varchar(14) DEFAULT NULL COMMENT '最后登录时间',
  `last_logout_time` varchar(14) DEFAULT NULL COMMENT '最后登出时间',
  `family` varchar(20) DEFAULT NULL COMMENT '门派',
  `level` int(11) DEFAULT NULL COMMENT '等级',
  `exp` int(11) DEFAULT NULL COMMENT '经验',
  `tao` int(11) DEFAULT NULL COMMENT '道行',
  `icon` int(11) DEFAULT NULL COMMENT '图示',
  `str` int(11) DEFAULT NULL COMMENT '力量',
  `con` int(11) DEFAULT NULL COMMENT '体质',
  `dex` int(11) DEFAULT NULL COMMENT '敏捷',
  `wiz` int(11) DEFAULT NULL COMMENT '灵力',
  `update_time` varchar(14) DEFAULT NULL COMMENT '更新时间',
  PRIMARY KEY (`gid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

