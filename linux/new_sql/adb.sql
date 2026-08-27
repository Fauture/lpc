/*
Navicat MySQL Data Transfer

Source Server         : 192.168.200.100
Source Server Version : 50173
Source Host           : 192.168.200.100:3306
Source Database       : adb

Target Server Type    : MYSQL
Target Server Version : 50173
File Encoding         : 936

Date: 2026-07-13 22:56:11
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for aaa
-- ----------------------------
DROP TABLE IF EXISTS `aaa`;
CREATE TABLE `aaa` (
  `aaa` varchar(32) NOT NULL DEFAULT '',
  `ip` varchar(15) NOT NULL DEFAULT '',
  `port` int(11) NOT NULL DEFAULT '0',
  `attrib` varchar(255) DEFAULT NULL,
  `num` int(11) NOT NULL DEFAULT '0',
  `id` int(11) NOT NULL DEFAULT '0',
  `server_key` varchar(32) NOT NULL DEFAULT '',
  `error_password_times` int(11) NOT NULL DEFAULT '5',
  `busy_time` int(11) NOT NULL DEFAULT '600',
  `client_per_ip` int(11) NOT NULL DEFAULT '0',
  `activate_url` varchar(255) NOT NULL DEFAULT '',
  `activate_aaa_ip` varchar(15) NOT NULL DEFAULT '',
  `ecard_fill_url` varchar(255) NOT NULL DEFAULT '',
  `ecard_resend_url` varchar(255) NOT NULL DEFAULT '',
  `ecard_web_status_url` varchar(255) NOT NULL DEFAULT '',
  `antibot_server` varchar(255) DEFAULT NULL,
  `antibot_action` varchar(255) DEFAULT NULL,
  `passpod_server` varchar(255) DEFAULT NULL,
  `memo` text,
  `auth_server` varchar(255) DEFAULT NULL,
  `coin_trade_server` varchar(255) DEFAULT 'interface.extend.huuc.cn',
  `recharge_url` varchar(255) NOT NULL DEFAULT '',
  `inquiry_url` varchar(255) NOT NULL DEFAULT '',
  `sms_auth_url` varchar(255) NOT NULL DEFAULT 'http://interface.message.huuc.cn/v1/Send/Account',
  PRIMARY KEY (`aaa`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Records of aaa
-- ----------------------------
INSERT INTO `aaa` VALUES ('aaa_2017', '127.0.0.1', '8001', null, '1', '1000', 'network', '5', '600', '0', 'http://interface.account.huuc.cn:8066/wendaoservice/register.ashx', '', 'http://interface.pay.huuc.cn:8066/v1/game/AskTaoFill/CardTrading', 'http://interface.pay.huuc.cn:8066/v1/game/AskTaoValid/ValidOrder', 'http://interface.pay.huuc.cn:8066/v1/game/AskTaoFill/IsValidAskTaoService', '192.168.121.75:32666', 'ignore_but_check', 'http://interface.security.huuc.cn:8066/Api/ekeyverifi', null, 'http://verify.huuc.cn:81/PasswordProtectionService/SecurityValid.ashx', 'interface.extend.huuc.cn:8066', 'http://interface.pay.huuc.cn:8066/v1/game/GyyxCardGamePay/CardDirectPay', 'http://interface.pay.huuc.cn:8066/v1/game/GyyxCardGamePay/Query', 'http://interface.message.huuc.cn:8066/v1/Send/Account');

-- ----------------------------
-- Table structure for acc_comp
-- ----------------------------
DROP TABLE IF EXISTS `acc_comp`;
CREATE TABLE `acc_comp` (
  `dist` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `branch` varchar(128) NOT NULL DEFAULT '',
  `comp` varchar(128) NOT NULL DEFAULT '',
  `time` varchar(14) NOT NULL DEFAULT '',
  `gid` varchar(128) NOT NULL DEFAULT '',
  `name` varchar(128) NOT NULL DEFAULT '0',
  PRIMARY KEY (`dist`,`account`,`branch`,`comp`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of acc_comp
-- ----------------------------

-- ----------------------------
-- Table structure for acc_log
-- ----------------------------
DROP TABLE IF EXISTS `acc_log`;
CREATE TABLE `acc_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `ip` varchar(15) NOT NULL DEFAULT '',
  `time` varchar(14) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `type` varchar(32) NOT NULL DEFAULT '',
  `msg` text,
  `memo` text,
  PRIMARY KEY (`id`),
  KEY `ip` (`ip`),
  KEY `account` (`account`),
  KEY `time` (`time`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of acc_log
-- ----------------------------

-- ----------------------------
-- Table structure for account
-- ----------------------------
DROP TABLE IF EXISTS `account`;
CREATE TABLE `account` (
  `account` varchar(32) NOT NULL DEFAULT '' COMMENT '账号',
  `blocked_time` varchar(14) NOT NULL DEFAULT '0' COMMENT '封号时间',
  `blocked_reason` varchar(128) NOT NULL DEFAULT '' COMMENT '封号原因',
  `temp_blocked_time` varchar(14) NOT NULL DEFAULT '' COMMENT '临时封停时间',
  `temp_blocked_reason` varchar(128) NOT NULL DEFAULT '' COMMENT '临时封停时间',
  `name` varchar(32) NOT NULL DEFAULT '' COMMENT '真实姓名',
  `birthday` varchar(8) NOT NULL DEFAULT '' COMMENT '出生日期',
  `id_type` varchar(32) NOT NULL DEFAULT '' COMMENT '证件类型',
  `id_num` varchar(32) NOT NULL DEFAULT '' COMMENT '证件号码',
  `tel` varchar(32) NOT NULL DEFAULT '' COMMENT '联系电话',
  `mobile` varchar(32) NOT NULL DEFAULT '' COMMENT '手机',
  `email` varchar(32) NOT NULL DEFAULT '' COMMENT '电子信箱',
  `password` varchar(32) NOT NULL DEFAULT '',
  `protect` varchar(32) NOT NULL DEFAULT '',
  `auto_lock` int(11) NOT NULL DEFAULT '0',
  `locked` varchar(14) NOT NULL DEFAULT '',
  `gold_coin` int(11) NOT NULL DEFAULT '0',
  `silver_coin` int(11) NOT NULL DEFAULT '0',
  `limit_trade_coin` int(11) NOT NULL DEFAULT '0',
  `trade_lock_time` varchar(14) NOT NULL DEFAULT '',
  `time` int(11) NOT NULL DEFAULT '0',
  `active_time` varchar(14) NOT NULL DEFAULT '',
  `first_login_time` varchar(14) NOT NULL DEFAULT '' COMMENT '首次登录时间',
  `privilege` int(11) unsigned NOT NULL DEFAULT '0',
  `account_id` varchar(32) NOT NULL DEFAULT '',
  `permit_ip` varchar(128) NOT NULL DEFAULT '',
  `permit_id` varchar(128) NOT NULL DEFAULT '',
  `ip` varchar(15) NOT NULL DEFAULT '',
  `adult` tinyint(4) NOT NULL DEFAULT '1' COMMENT '成年标记',
  `checksum` varchar(32) NOT NULL DEFAULT '',
  `coin_password` varchar(32) NOT NULL DEFAULT '',
  `unlock_coin_password_time` varchar(14) NOT NULL DEFAULT '',
  `org_password` varchar(32) NOT NULL DEFAULT '',
  `org_permit_ip` varchar(128) NOT NULL DEFAULT '',
  `memo` text COMMENT '备注',
  `last_login_ip` varchar(128) NOT NULL DEFAULT '',
  `last_login_time` varchar(14) NOT NULL DEFAULT '',
  `last_login_id` varchar(32) NOT NULL DEFAULT '',
  `presentee` tinyint(4) NOT NULL DEFAULT '0',
  `reg_date` varchar(14) NOT NULL DEFAULT '',
  `active_path` tinyint(4) NOT NULL DEFAULT '0',
  `trade_coin` int(11) NOT NULL DEFAULT '0',
  `last_trade_coin` varchar(14) NOT NULL DEFAULT '',
  `consum_coin` int(11) NOT NULL DEFAULT '0',
  `last_consum_coin` varchar(14) NOT NULL DEFAULT '',
  PRIMARY KEY (`account`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of account
-- ----------------------------
INSERT INTO `account` VALUES ('1', '', '', '', '', '', '', '', '', '', '', '', 'A1DCB9CC45C150193FBB4611FAC6DBE5', '', '0', '', '19952050', '20000000', '0', '', '0', '', '20260714053654', '300', '', '', '', '', '1', '51C79C8A4FC8103463FB5A4A1CAD9999', '', '', '', '', '', '192.168.200.1', '20260714065227', '0000b445067b15f4', '0', '', '0', '0', '', '47950', '20260714065243');
INSERT INTO `account` VALUES ('a', '', '', '', '', '', '', '', '', '', '', '', '42929920580FDA6B48A4DBBEAC729791', '', '0', '', '20000000', '20000000', '0', '', '0', '', '', '300', '', '', '', '', '1', '7AEBF7B95B19BFD3C0B029A16C5F92C7', '', '', '', '', null, '', '', '', '0', '', '0', '0', '', '0', '');
INSERT INTO `account` VALUES ('b', '', '', '', '', '', '', '', '', '', '', '', '0E249716C69F0CB31EB3BD638C30F57F', '', '0', '', '20000000', '20000000', '0', '', '0', '', '', '300', '', '', '', '', '1', '5C9C2385212EF4282B17B56B0BE170B3', '', '', '', '', null, '', '', '', '0', '', '0', '0', '', '0', '');
INSERT INTO `account` VALUES ('q', '', '', '', '', '', '', '', '', '', '', '', 'BFA1AF25984AEF0EAC96FC722AF11F91', '', '0', '', '20000000', '20000000', '0', '', '0', '', '', '0', '', '', '', '', '1', 'F5A19BD1D9B62C6A547E3B1808449603', '', '', '', '', null, '', '', '', '0', '', '0', '0', '', '0', '');
INSERT INTO `account` VALUES ('2', '', '', '', '', '', '', '', '', '', '', '', '148B3900E39FA02CFE65B6C845E42074', '', '0', '', '20000000', '20000000', '0', '', '0', '', '', '0', '', '', '', '', '1', '5189DCE5F684BA5C9E0218331F37CF79', '', '', '', '', null, '', '', '', '0', '', '0', '0', '', '0', '');
INSERT INTO `account` VALUES ('3', '', '', '', '', '', '', '', '', '', '', '', '18166C37F437E76F37CD0B4FCFF7DB65', '', '0', '', '20000000', '20000000', '0', '', '0', '', '', '0', '', '', '', '', '1', '23E247A869C9C1303AE2F38F0BA4E294', '', '', '', '', null, '', '', '', '0', '', '0', '0', '', '0', '');

-- ----------------------------
-- Table structure for account_coin_trade
-- ----------------------------
DROP TABLE IF EXISTS `account_coin_trade`;
CREATE TABLE `account_coin_trade` (
  `account` varchar(32) NOT NULL DEFAULT '',
  `gold_coin` int(11) NOT NULL DEFAULT '0',
  `checksum` varchar(32) NOT NULL DEFAULT '0',
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `memo` text NOT NULL,
  PRIMARY KEY (`account`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of account_coin_trade
-- ----------------------------

-- ----------------------------
-- Table structure for add_coin_log
-- ----------------------------
DROP TABLE IF EXISTS `add_coin_log`;
CREATE TABLE `add_coin_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT COMMENT '日志编号',
  `gid` varchar(32) NOT NULL DEFAULT '' COMMENT '角色',
  `account` varchar(32) NOT NULL DEFAULT '' COMMENT '账号',
  `server` varchar(32) NOT NULL DEFAULT '' COMMENT '获得服务器',
  `dist` varchar(32) NOT NULL DEFAULT '' COMMENT '区组',
  `add_time` varchar(14) NOT NULL DEFAULT '' COMMENT '获得时间',
  `silver_coin_left` int(11) NOT NULL DEFAULT '0' COMMENT '获得前银元宝数',
  `gold_coin_left` int(11) NOT NULL DEFAULT '0' COMMENT '获得前金元宝数',
  `add_silver_coin` int(11) unsigned zerofill NOT NULL DEFAULT '00000000000' COMMENT '获得银元宝数',
  `add_gold_coin` int(11) unsigned NOT NULL DEFAULT '0' COMMENT '获得金元宝数',
  `cur_gold_coin` int(11) NOT NULL DEFAULT '0' COMMENT '获得后银元宝数',
  `cur_silver_coin` int(11) NOT NULL DEFAULT '0' COMMENT '获得后金元宝数',
  `gs_memo` text COMMENT 'GS备注',
  `memo` text COMMENT '备注',
  PRIMARY KEY (`id`),
  KEY `account` (`account`),
  KEY `add_time` (`add_time`)
) ENGINE=MyISAM AUTO_INCREMENT=3 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of add_coin_log
-- ----------------------------

-- ----------------------------
-- Table structure for add_gold_coin_log
-- ----------------------------
DROP TABLE IF EXISTS `add_gold_coin_log`;
CREATE TABLE `add_gold_coin_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `gid` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `server` varchar(32) NOT NULL DEFAULT '',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `cost_time` varchar(14) NOT NULL DEFAULT '',
  `cost_platform` tinyint(4) NOT NULL DEFAULT '0',
  `cost_coin` int(11) NOT NULL DEFAULT '0',
  `item_price` int(11) NOT NULL DEFAULT '0',
  `buy_item` varchar(32) NOT NULL DEFAULT '',
  `buy_item_amount` int(11) NOT NULL DEFAULT '0',
  `silver_coin_cost` int(11) NOT NULL DEFAULT '0',
  `silver_coin_left` int(11) NOT NULL DEFAULT '0',
  `gold_coin_cost` int(11) NOT NULL DEFAULT '0',
  `gold_coin_left` int(11) NOT NULL DEFAULT '0',
  `cur_state` int(11) NOT NULL DEFAULT '0',
  `uid` varchar(32) NOT NULL DEFAULT '',
  `para1` varchar(32) NOT NULL DEFAULT '',
  `para2` varchar(32) NOT NULL DEFAULT '',
  `para3` varchar(32) NOT NULL DEFAULT '',
  `gs_memo` text,
  `memo` text,
  PRIMARY KEY (`id`),
  KEY `account` (`account`),
  KEY `cost_time` (`cost_time`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of add_gold_coin_log
-- ----------------------------

-- ----------------------------
-- Table structure for admin_acc
-- ----------------------------
DROP TABLE IF EXISTS `admin_acc`;
CREATE TABLE `admin_acc` (
  `account` varchar(32) NOT NULL DEFAULT '',
  `s_account` varchar(32) NOT NULL DEFAULT '',
  `privilege` text NOT NULL,
  `permit_ip` varchar(128) NOT NULL DEFAULT '',
  `permit_id` varchar(128) NOT NULL DEFAULT '',
  `permit_dist` text,
  `checksum` varchar(128) NOT NULL DEFAULT '',
  `memo` text,
  PRIMARY KEY (`account`),
  UNIQUE KEY `s_account` (`s_account`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Records of admin_acc
-- ----------------------------

-- ----------------------------
-- Table structure for ahserver
-- ----------------------------
DROP TABLE IF EXISTS `ahserver`;
CREATE TABLE `ahserver` (
  `ahserver` varchar(32) NOT NULL DEFAULT '',
  `ip` varchar(15) NOT NULL DEFAULT '',
  `port` int(11) NOT NULL DEFAULT '0',
  `memo` text,
  PRIMARY KEY (`ahserver`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Records of ahserver
-- ----------------------------
INSERT INTO `ahserver` VALUES ('apex1', '127.0.0.1', '15001', '');
INSERT INTO `ahserver` VALUES ('apex10', '127.0.0.1', '15010', '');
INSERT INTO `ahserver` VALUES ('apex11', '127.0.0.1', '15011', '');
INSERT INTO `ahserver` VALUES ('apex12', '127.0.0.1', '15012', '');
INSERT INTO `ahserver` VALUES ('apex13', '127.0.0.1', '15013', '');
INSERT INTO `ahserver` VALUES ('apex14', '127.0.0.1', '15014', '');
INSERT INTO `ahserver` VALUES ('apex15', '127.0.0.1', '15015', '');
INSERT INTO `ahserver` VALUES ('apex16', '127.0.0.1', '15016', '');
INSERT INTO `ahserver` VALUES ('apex17', '127.0.0.1', '15017', '');
INSERT INTO `ahserver` VALUES ('apex18', '127.0.0.1', '15018', '');
INSERT INTO `ahserver` VALUES ('apex19', '127.0.0.1', '15019', '');
INSERT INTO `ahserver` VALUES ('apex2', '127.0.0.1', '15002', '');
INSERT INTO `ahserver` VALUES ('apex20', '127.0.0.1', '15020', '');
INSERT INTO `ahserver` VALUES ('apex21', '127.0.0.1', '15021', '');
INSERT INTO `ahserver` VALUES ('apex22', '127.0.0.1', '15022', '');
INSERT INTO `ahserver` VALUES ('apex23', '127.0.0.1', '15023', '');
INSERT INTO `ahserver` VALUES ('apex24', '127.0.0.1', '15024', '');
INSERT INTO `ahserver` VALUES ('apex25', '127.0.0.1', '15025', '');
INSERT INTO `ahserver` VALUES ('apex26', '127.0.0.1', '15026', '');
INSERT INTO `ahserver` VALUES ('apex27', '127.0.0.1', '15027', '');
INSERT INTO `ahserver` VALUES ('apex28', '127.0.0.1', '15028', '');
INSERT INTO `ahserver` VALUES ('apex29', '127.0.0.1', '15029', '');
INSERT INTO `ahserver` VALUES ('apex3', '127.0.0.1', '15003', '');
INSERT INTO `ahserver` VALUES ('apex30', '127.0.0.1', '15030', '');
INSERT INTO `ahserver` VALUES ('apex31', '127.0.0.1', '15031', '');
INSERT INTO `ahserver` VALUES ('apex32', '127.0.0.1', '15032', '');
INSERT INTO `ahserver` VALUES ('apex33', '127.0.0.1', '15033', '');
INSERT INTO `ahserver` VALUES ('apex34', '127.0.0.1', '15034', '');
INSERT INTO `ahserver` VALUES ('apex35', '127.0.0.1', '15035', '');
INSERT INTO `ahserver` VALUES ('apex36', '127.0.0.1', '15036', '');
INSERT INTO `ahserver` VALUES ('apex37', '127.0.0.1', '15037', '');
INSERT INTO `ahserver` VALUES ('apex38', '127.0.0.1', '15038', '');
INSERT INTO `ahserver` VALUES ('apex39', '127.0.0.1', '15039', '');
INSERT INTO `ahserver` VALUES ('apex4', '127.0.0.1', '15004', '');
INSERT INTO `ahserver` VALUES ('apex40', '127.0.0.1', '15040', '');
INSERT INTO `ahserver` VALUES ('apex41', '127.0.0.1', '15041', '');
INSERT INTO `ahserver` VALUES ('apex42', '127.0.0.1', '15042', '');
INSERT INTO `ahserver` VALUES ('apex43', '127.0.0.1', '15043', '');
INSERT INTO `ahserver` VALUES ('apex44', '127.0.0.1', '15044', '');
INSERT INTO `ahserver` VALUES ('apex45', '127.0.0.1', '15045', '');
INSERT INTO `ahserver` VALUES ('apex46', '127.0.0.1', '15046', '');
INSERT INTO `ahserver` VALUES ('apex47', '127.0.0.1', '15047', '');
INSERT INTO `ahserver` VALUES ('apex48', '127.0.0.1', '15048', '');
INSERT INTO `ahserver` VALUES ('apex49', '127.0.0.1', '15049', '');
INSERT INTO `ahserver` VALUES ('apex5', '127.0.0.1', '15005', '');
INSERT INTO `ahserver` VALUES ('apex50', '127.0.0.1', '15050', '');
INSERT INTO `ahserver` VALUES ('apex51', '127.0.0.1', '15051', '');
INSERT INTO `ahserver` VALUES ('apex52', '127.0.0.1', '15052', '');
INSERT INTO `ahserver` VALUES ('apex53', '127.0.0.1', '15053', '');
INSERT INTO `ahserver` VALUES ('apex54', '127.0.0.1', '15054', '');
INSERT INTO `ahserver` VALUES ('apex55', '127.0.0.1', '15055', '');
INSERT INTO `ahserver` VALUES ('apex56', '127.0.0.1', '15056', '');
INSERT INTO `ahserver` VALUES ('apex6', '127.0.0.1', '15006', '');
INSERT INTO `ahserver` VALUES ('apex7', '127.0.0.1', '15007', '');
INSERT INTO `ahserver` VALUES ('apex8', '127.0.0.1', '15008', '');
INSERT INTO `ahserver` VALUES ('apex9', '127.0.0.1', '15009', '');

-- ----------------------------
-- Table structure for alipay_account
-- ----------------------------
DROP TABLE IF EXISTS `alipay_account`;
CREATE TABLE `alipay_account` (
  `account` varchar(32) NOT NULL DEFAULT '' COMMENT '账号',
  `alipay_account` varchar(255) NOT NULL DEFAULT '' COMMENT '支付宝账号',
  `bind_time` varchar(14) NOT NULL DEFAULT '' COMMENT '绑定时间',
  `state` tinyint(4) unsigned NOT NULL DEFAULT '0' COMMENT '绑定状态',
  `update_time` varchar(14) NOT NULL DEFAULT '' COMMENT '更新时间',
  PRIMARY KEY (`account`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of alipay_account
-- ----------------------------

-- ----------------------------
-- Table structure for antiaddiction
-- ----------------------------
DROP TABLE IF EXISTS `antiaddiction`;
CREATE TABLE `antiaddiction` (
  `account` varchar(32) NOT NULL DEFAULT '',
  `last_offline` int(10) NOT NULL DEFAULT '0',
  `total_offline` int(10) NOT NULL DEFAULT '0',
  `total_online` int(10) NOT NULL DEFAULT '0',
  PRIMARY KEY (`account`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of antiaddiction
-- ----------------------------

-- ----------------------------
-- Table structure for antibot_log
-- ----------------------------
DROP TABLE IF EXISTS `antibot_log`;
CREATE TABLE `antibot_log` (
  `no` int(11) NOT NULL AUTO_INCREMENT,
  `ip` varchar(16) NOT NULL DEFAULT '',
  `account` varchar(32) DEFAULT NULL,
  `bot_type` varchar(16) DEFAULT NULL,
  `bot_content` text,
  `time` varchar(14) DEFAULT NULL,
  `memo` text,
  PRIMARY KEY (`no`),
  KEY `bot_type` (`bot_type`),
  KEY `ip` (`ip`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of antibot_log
-- ----------------------------

-- ----------------------------
-- Table structure for arena_account
-- ----------------------------
DROP TABLE IF EXISTS `arena_account`;
CREATE TABLE `arena_account` (
  `account` varchar(32) NOT NULL DEFAULT '' COMMENT '账号',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  PRIMARY KEY (`account`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of arena_account
-- ----------------------------

-- ----------------------------
-- Table structure for card_info
-- ----------------------------
DROP TABLE IF EXISTS `card_info`;
CREATE TABLE `card_info` (
  `number` varchar(32) NOT NULL DEFAULT '',
  `password` varchar(32) NOT NULL DEFAULT '',
  `count` int(11) NOT NULL DEFAULT '0',
  `used` int(11) NOT NULL DEFAULT '0',
  `locked` int(11) NOT NULL DEFAULT '0',
  `memo` text,
  PRIMARY KEY (`number`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of card_info
-- ----------------------------

-- ----------------------------
-- Table structure for ccs
-- ----------------------------
DROP TABLE IF EXISTS `ccs`;
CREATE TABLE `ccs` (
  `ccs` varchar(32) NOT NULL DEFAULT '',
  `ip` varchar(31) NOT NULL DEFAULT '',
  `port` int(11) NOT NULL DEFAULT '0',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `server_key` varchar(32) NOT NULL DEFAULT '',
  `id` int(11) NOT NULL DEFAULT '0',
  `memo` text,
  PRIMARY KEY (`ccs`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Records of ccs
-- ----------------------------
INSERT INTO `ccs` VALUES ('ccs_2017', '192.168.200.100', '9020', '98WenDao', 'network', '0', null);

-- ----------------------------
-- Table structure for charge
-- ----------------------------
DROP TABLE IF EXISTS `charge`;
CREATE TABLE `charge` (
  `account` varchar(32) NOT NULL DEFAULT '',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `count` int(11) NOT NULL DEFAULT '0',
  `end_time` varchar(14) NOT NULL DEFAULT '',
  `card_bonus` int(11) NOT NULL DEFAULT '0',
  `gender` int(11) NOT NULL DEFAULT '0',
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `memo` text,
  PRIMARY KEY (`account`,`dist`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of charge
-- ----------------------------

-- ----------------------------
-- Table structure for charge_encourage
-- ----------------------------
DROP TABLE IF EXISTS `charge_encourage`;
CREATE TABLE `charge_encourage` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `gold_coin` int(11) NOT NULL DEFAULT '0',
  `encourage` varchar(128) NOT NULL DEFAULT '',
  `start_time` varchar(14) NOT NULL DEFAULT '',
  `end_time` varchar(14) NOT NULL DEFAULT '',
  `checksum` varchar(32) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`),
  KEY `gold_coin` (`gold_coin`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of charge_encourage
-- ----------------------------

-- ----------------------------
-- Table structure for charge_log
-- ----------------------------
DROP TABLE IF EXISTS `charge_log`;
CREATE TABLE `charge_log` (
  `id` int(10) NOT NULL AUTO_INCREMENT,
  `src_ip` varchar(15) NOT NULL DEFAULT '',
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `type` varchar(32) NOT NULL DEFAULT '',
  `number` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `delta_count` int(11) NOT NULL DEFAULT '0',
  `delta_time` int(11) NOT NULL DEFAULT '0',
  `count` int(11) NOT NULL DEFAULT '0',
  `end_time` varchar(14) NOT NULL DEFAULT '',
  `memo` text,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of charge_log
-- ----------------------------

-- ----------------------------
-- Table structure for citywar_account
-- ----------------------------
DROP TABLE IF EXISTS `citywar_account`;
CREATE TABLE `citywar_account` (
  `account` varchar(32) NOT NULL DEFAULT '',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  PRIMARY KEY (`account`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of citywar_account
-- ----------------------------

-- ----------------------------
-- Table structure for coin_trade_log
-- ----------------------------
DROP TABLE IF EXISTS `coin_trade_log`;
CREATE TABLE `coin_trade_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `server` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `name` varchar(12) NOT NULL DEFAULT '',
  `order_id` varchar(15) NOT NULL DEFAULT '',
  `type` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `amount` int(11) NOT NULL DEFAULT '0',
  `gold_coin_before` int(11) NOT NULL DEFAULT '0',
  `gold_coin_after` int(11) NOT NULL DEFAULT '0',
  `store_coin_before` int(11) NOT NULL DEFAULT '0',
  `store_coin_after` int(11) NOT NULL DEFAULT '0',
  `para1` int(11) NOT NULL DEFAULT '0',
  `para2` varchar(32) NOT NULL DEFAULT '0',
  `para3` varchar(128) NOT NULL DEFAULT '0',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`)
) ENGINE=MyISAM AUTO_INCREMENT=3 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of coin_trade_log
-- ----------------------------

-- ----------------------------
-- Table structure for config
-- ----------------------------
DROP TABLE IF EXISTS `config`;
CREATE TABLE `config` (
  `version` varchar(32) NOT NULL DEFAULT '',
  `dist` varchar(255) NOT NULL DEFAULT '',
  `section` varchar(32) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `value` text NOT NULL,
  `checksum` varchar(32) NOT NULL DEFAULT '',
  `memo` text,
  PRIMARY KEY (`version`,`dist`,`section`,`name`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Records of config
-- ----------------------------
INSERT INTO `config` VALUES ('', '98WenDao', 'global', '门派比武大会', '98WenDao:金系/98WenDao:木系/98WenDao:水系/98WenDao:火系/98WenDao:土系', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'baishou_shengyan', '([\"98WenDao\":({40,79,}),\"98WenDao\":({80,99,}),\"98WenDao\":({100,119,}),\"98WenDao\":({120,159,}),])', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'bbgs_2011', '([\"98WenDao\":({60,79,}),\"98WenDao\":({80,99,}),\"98WenDao\":({100,119,}),\"98WenDao\":({120,149,}),])', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'bianfang_gaoren', '98WenDao:60-79/98WenDao:80-99/98WenDao:100-119/98WenDao:120-149', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'bobing_pk_servers', '({ \"98WenDao\", \"98WenDao\", \"98WenDao\" })', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'bzn_yaoshou_ruqin', '({\"98WenDao\",\"98WenDao\",\"98WenDao\",\"98WenDao\"})', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'challenge_match', '([\"150-169\":\"98WenDao\",\"140-149\":\"98WenDao\",\"130-139\":\"98WenDao\",\"120-129\":\"98WenDao\",\"110-119\":\"98WenDao\",\"100-109\":\"98WenDao\",\"90-99\":\"98WenDao\",\"80-89\":\"98WenDao\",\"70-79\":\"98WenDao\",\"60-69\":\"98WenDao\"])', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'chaofan_tiyan', '({\"2016-12-01-00:00:00\",\"2097-01-03-23:59:59\"})', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'charge_rate', '60', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'chengjiu_daren', '({\"2017-01-09-00:00:00\",\"2019-02-08-23:59:59\",\"2097-03-08-23:59:59\"})', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'chongzhi_fanli', '({\"2016-11-11-00:00:00\",\"2096-11-27-23:59:59\", 1})', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'CITY_WAR_SERVER', '98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'coin_trade_gs', '([\"min_id\":1,\"max_id\":10,])', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'common_guess_server', '98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'convenant_info', '98WenDao,98WenDao,98WenDao,98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'custom_package', '([\"start_time\":\"2016-11-11-00:00:00\",\"end_time\":\"2096-11-27-23:59:59\",])', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'defense_city', '98WenDao|98WenDao|98WenDao|98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'dongfu_tiaozhan', '98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'douchong_dahui', '98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'duobao_qibing', '([1:\"98WenDao\",2:\"98WenDao\",])', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', '试道大会', '98WenDao:60-69,70-79,80-89/98WenDao:90-99,100-109,110-119/98WenDao:120-129,130-139,140-149,150-159,160-169', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'european_cup_guess_server', '98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'exchange_count', '500', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'exchange_day', '30', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'fengyin_zhifa', '([\"98WenDao\": \"40-69\", \"98WenDao\":\"70-99\", \"98WenDao\": \"100-119\", \"98WenDao\": \"120-149\"])', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'fishing_server', '98WenDao', '', null);
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'group_buying_server', '98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'gwgc', '([\"end_time\":\"2016-12-18-18:01:00\",\"start_time\":\"2096-12-17-16:00:00\",\"interval\":86400,\"duration\":7200,])', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'hero_boss_server', '98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'jingbing_jianyue', '98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'jiuli_xiangyao', '([ \"start_time\": \"2017-01-06-00:00:00\", \"end_time\" : \"2097-02-05-23:59:59\", \"version\" : 3, ])', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'laobing_busi', '({\"98WenDao\"})', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'ldb_host', '127.0.0.1:3306', '', null);
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'lgxy_server', '98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'limit_login', 'limit_level=0,login_num=5', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'longxue_boss_server', '98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'luandou', '98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'lzn_xianzi_cifu', '({\"98WenDao\",\"98WenDao\",\"98WenDao\",\"98WenDao\"})', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'manage_server', '98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'map_decorate_server', '98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'mdb_host', '127.0.0.1:3306', '', null);
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'menpai_dazhan', '([\"98WenDao\":({60,79}),\"98WenDao\":({80,99}),\"98WenDao\":({100,129}),\"98WenDao\":({130,MAX_LEVEL_CEIL})])', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'menpai_hubao', '([\"98WenDao\":\"130-MAX_LEVEL_CEIL\",\"98WenDao\":\"100-129\",\"98WenDao\":\"70-99\",\"98WenDao\":\"40-69\"])', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'myzz', '([\"98WenDao\":({40,59,}),\"98WenDao\":({60,79,}),\"98WenDao\":({80,99,}),\"98WenDao\":({100,119,}),\"98WenDao\":({120,155,}),])', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'pantaoyuan_zhengduozhan', '([ 100 : \"98WenDao\", 110 : \"98WenDao\", 120 : \"98WenDao\", 130 : \"98WenDao\", 140 : \"98WenDao\", 150 : \"98WenDao\", 160 : \"98WenDao\" ])', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'party_qkbgz', '([4:\"98WenDao\",3:\"98WenDao\",2:\"98WenDao\",])', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'party_qql', '98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'party_sell_servers', '98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'phah_effect_timeout', '30', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'present_day', '3', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'purchase_gs', '([\"min_id\":1,\"max_id\":10,])', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'race_server', '98WenDao,98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'reduce_server_place', '98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'red_envelope', '([ \"start_time\" : \"2017-04-20-00:00:00\", \"end_time\" : \"2017-04-27-23:59:59\", \"sort_end_time\" : \"2017-04-26-23:59:59\", \"envelope_list\" : ({ ([ \"time\" : \"98WenDao-04-22-20:00:00\",\"coin\" : 2000000,\"amount\" : 2000,]),([ \"time\" : \"2097-04-22-22:00:00\",\"coin\" : 4000000,\"amount\" : 2000,]) }), \"sys_name\" : \"九周年\", \"version\" : 1 ])', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'send_exp_online', '([\"end_time\":\"2016-12-17-22:00:00\",\"start_time\":\"2096-12-17-20:00:00\",\"interval\":0,\"duration\":7200,])', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'shenbin_liqi_task', '([ \"start_time\" : \"2017-01-09-00:00:00\", \"end_time\" : \"2017-02-08-23:59:59\"])', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'shengsidou', '0', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'shequ_xuyuan', '({\"2016-04-08-00:00:00\",\"2096-05-08-23:59:59\"})', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'shop_gs', '([\"min_id\":1,\"max_id\":10,])', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'shouci_chongzhi', '([ \"start_time\" : \"2017-1-5-00:00:00\", \"end_time\" : \"2097-2-8-23:59:00\"])', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'skill_workshop_gs', '([\"min_id\":1,\"max_id\":6,])', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'sncbc_open_gs', '({ \"98WenDao\", \"98WenDao\", \"98WenDao\", \"98WenDao\", \"98WenDao\", \"98WenDao\", \"98WenDao\", \"98WenDao\" })', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'stall_gs', '([\"min_id\":1,\"max_id\":10,])', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'super_boss_server', '98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'task_control_server', '98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'wedding_server', '98WenDao', '', null);
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'world_boss_server', '98WenDao', '', null);
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'wsboss_wzz_server', '98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'wuxing_zhengba', '({ \"98WenDao\",\"98WenDao\" })', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'wuzhuangyuan', '98WenDao:80-89,120-129/98WenDao:90-99,130-139/98WenDao:100-109,140-149/98WenDao:110-119,150-159,160-169', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'xianshou_cifu', '({\"98WenDao\",\"98WenDao\",\"98WenDao\",\"98WenDao\"})', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'xinmlw_servers', '98WenDao:60-79|98WenDao:80-99|98WenDao:100-119|98WenDao:120-MAX_LEVEL_CEIL', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'xixunling_server', '98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'xiygc_zhenyz', '98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'xiying_zhounian', '({\"98WenDao\",\"98WenDao\",\"98WenDao\",\"98WenDao\"})', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'xyhc_server', '98WenDao,98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'year_insider', '1', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'ymdx_server', '98WenDao,98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'yuqing_shandian_challenge', '98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'zhaojiling', '([\"副本任务\":({\"98WenDao\"}),\"昆仑神镜\":({\"98WenDao\"})])', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'zhengdou', '98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', 'zhongxian_baoxi', '({\"98WenDao\",\"98WenDao\",\"98WenDao\",\"98WenDao\"})', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', '比武大会', '98WenDao:60-69,70-79/98WenDao:80-89,90-99/98WenDao:100-109,110-119/98WenDao:120-129,130-139,140-149', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', '夺宝之战', '70 : 98WenDao/80 : 98WenDao/90 : 98WenDao/100 : 98WenDao/110 : 98WenDao/120 : 98WenDao/130 : 98WenDao/140 :98WenDao', '', '');
INSERT INTO `config` VALUES ('', '98WenDao', 'global', '节节高升', '70 : 98WenDao/80 : 98WenDao/90 : 98WenDao/100 : 98WenDao/110 : 98WenDao/120 : 98WenDao/130 : 98WenDao/140 : 98WenDao', '', '');

-- ----------------------------
-- Table structure for cost_log
-- ----------------------------
DROP TABLE IF EXISTS `cost_log`;
CREATE TABLE `cost_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `gid` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `server` varchar(32) NOT NULL DEFAULT '',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `cost_time` varchar(14) NOT NULL DEFAULT '',
  `cost_platform` tinyint(4) NOT NULL DEFAULT '0',
  `cost_coin` int(11) NOT NULL DEFAULT '0',
  `item_price` int(11) NOT NULL DEFAULT '0',
  `buy_item` varchar(32) NOT NULL DEFAULT '',
  `buy_item_amount` int(11) NOT NULL DEFAULT '0',
  `silver_coin_cost` int(11) NOT NULL DEFAULT '0',
  `silver_coin_left` int(11) NOT NULL DEFAULT '0',
  `gold_coin_cost` int(11) NOT NULL DEFAULT '0',
  `gold_coin_left` int(11) NOT NULL DEFAULT '0',
  `cur_state` int(11) NOT NULL DEFAULT '0',
  `uid` varchar(32) NOT NULL DEFAULT '',
  `para1` varchar(32) NOT NULL DEFAULT '',
  `para2` varchar(32) NOT NULL DEFAULT '',
  `para3` varchar(32) NOT NULL DEFAULT '',
  `gs_memo` text,
  `memo` text,
  PRIMARY KEY (`id`),
  KEY `account` (`account`),
  KEY `cost_time` (`cost_time`)
) ENGINE=MyISAM AUTO_INCREMENT=1759 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for coupon_info
-- ----------------------------
DROP TABLE IF EXISTS `coupon_info`;
CREATE TABLE `coupon_info` (
  `code` varchar(32) NOT NULL DEFAULT '',
  `gid` bigint(20) NOT NULL DEFAULT '0',
  `type` varchar(32) NOT NULL DEFAULT '',
  `used` int(11) NOT NULL DEFAULT '0',
  `memo` text,
  PRIMARY KEY (`code`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of coupon_info
-- ----------------------------

-- ----------------------------
-- Table structure for credit_log
-- ----------------------------
DROP TABLE IF EXISTS `credit_log`;
CREATE TABLE `credit_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `transaction_id` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gold_coin` int(11) NOT NULL DEFAULT '0',
  `silver_coin` int(11) NOT NULL DEFAULT '0',
  `coupon_gold_used` int(11) NOT NULL DEFAULT '0',
  `coupon_silver_used` int(11) NOT NULL DEFAULT '0',
  `checksum` varchar(32) NOT NULL DEFAULT '',
  `time` varchar(14) NOT NULL DEFAULT '',
  `server` varchar(32) NOT NULL DEFAULT '',
  `source` tinyint(4) NOT NULL DEFAULT '0',
  `para1` int(11) NOT NULL DEFAULT '0',
  `para2` int(11) NOT NULL DEFAULT '0',
  `para3` varchar(32) NOT NULL DEFAULT '',
  `para4` varchar(32) NOT NULL DEFAULT '',
  `para5` varchar(255) NOT NULL DEFAULT '',
  `memo` text,
  PRIMARY KEY (`id`),
  KEY `account` (`account`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of credit_log
-- ----------------------------

-- ----------------------------
-- Table structure for cs_ccs
-- ----------------------------
DROP TABLE IF EXISTS `cs_ccs`;
CREATE TABLE `cs_ccs` (
  `ccs` varchar(32) NOT NULL DEFAULT '',
  `role` text NOT NULL,
  `privilege` text NOT NULL,
  `ip` varchar(31) NOT NULL DEFAULT '',
  `port` int(11) NOT NULL DEFAULT '0',
  `id` int(11) NOT NULL DEFAULT '0',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `server_key` varchar(32) NOT NULL DEFAULT '',
  `memo` text,
  PRIMARY KEY (`ccs`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of cs_ccs
-- ----------------------------

-- ----------------------------
-- Table structure for csa
-- ----------------------------
DROP TABLE IF EXISTS `csa`;
CREATE TABLE `csa` (
  `csa` varchar(32) NOT NULL DEFAULT '',
  `ip` varchar(32) NOT NULL DEFAULT '',
  `port` varchar(15) NOT NULL DEFAULT '',
  `id` int(11) NOT NULL DEFAULT '0',
  `cs_dist` varchar(255) NOT NULL DEFAULT '',
  `http_ip` varchar(255) NOT NULL DEFAULT '',
  `http_port` int(11) NOT NULL DEFAULT '0',
  `http_plat` varchar(255) NOT NULL DEFAULT '',
  `httpd_enable` int(1) NOT NULL DEFAULT '0',
  `memo` text NOT NULL,
  PRIMARY KEY (`csa`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Records of csa
-- ----------------------------
INSERT INTO `csa` VALUES ('csa_league', '192.168.200.100', '9999', '0', '98WenDao', '', '0', '', '0', '');

-- ----------------------------
-- Table structure for csc_account
-- ----------------------------
DROP TABLE IF EXISTS `csc_account`;
CREATE TABLE `csc_account` (
  `account` varchar(32) NOT NULL DEFAULT '',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  PRIMARY KEY (`account`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of csc_account
-- ----------------------------

-- ----------------------------
-- Table structure for cta
-- ----------------------------
DROP TABLE IF EXISTS `cta`;
CREATE TABLE `cta` (
  `cta` varchar(32) NOT NULL DEFAULT '',
  `privilege` varchar(255) NOT NULL DEFAULT '',
  `ip` varchar(255) NOT NULL DEFAULT '',
  `port` int(11) NOT NULL DEFAULT '0',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `access_adb_user` varchar(32) NOT NULL DEFAULT '',
  `access_adb_password` varchar(32) NOT NULL DEFAULT '',
  `memo` text,
  PRIMARY KEY (`cta`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Records of cta
-- ----------------------------
INSERT INTO `cta` VALUES ('cta', '*', '192.168.200.100', '0', '98WenDao', 'cta', '5416D7CD6EF195A0F7622A9C56B55E82', '');

-- ----------------------------
-- Table structure for dba
-- ----------------------------
DROP TABLE IF EXISTS `dba`;
CREATE TABLE `dba` (
  `dba` varchar(32) NOT NULL DEFAULT '',
  `ip` varchar(15) NOT NULL DEFAULT '',
  `port` int(11) NOT NULL DEFAULT '0',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `server_key` varchar(32) NOT NULL DEFAULT '',
  `id` int(11) NOT NULL DEFAULT '0',
  `memo` text,
  PRIMARY KEY (`dba`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Records of dba
-- ----------------------------
INSERT INTO `dba` VALUES ('dba_2017', '192.168.200.100', '9030', '98WenDao', 'network', '0', null);

-- ----------------------------
-- Table structure for ddb
-- ----------------------------
DROP TABLE IF EXISTS `ddb`;
CREATE TABLE `ddb` (
  `ddb` varchar(32) NOT NULL DEFAULT '',
  `user` varchar(32) NOT NULL DEFAULT '',
  `password` varchar(64) NOT NULL DEFAULT '',
  `driver` varchar(64) NOT NULL DEFAULT '',
  `ip` varchar(15) NOT NULL DEFAULT '',
  `port` int(11) NOT NULL DEFAULT '0',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `memo` text,
  PRIMARY KEY (`ddb`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Records of ddb
-- ----------------------------
INSERT INTO `ddb` VALUES ('ddb', 'root', '112233', 'MySQL ODBC 3.51 Driver', '127.0.0.1', '3306', '98WenDao', null);

-- ----------------------------
-- Table structure for district
-- ----------------------------
DROP TABLE IF EXISTS `district`;
CREATE TABLE `district` (
  `dist` varchar(32) NOT NULL DEFAULT '',
  `alias` varchar(32) NOT NULL DEFAULT '',
  `service` varchar(255) NOT NULL DEFAULT '',
  `id` int(11) NOT NULL DEFAULT '0',
  `spa` varchar(32) NOT NULL DEFAULT '',
  `csa` varchar(32) NOT NULL DEFAULT '',
  `party_war_server` varchar(32) NOT NULL DEFAULT '',
  `memo` text,
  PRIMARY KEY (`dist`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Records of district
-- ----------------------------
INSERT INTO `district` VALUES ('98WenDao', '', 'login/permit_wizard/new_char/new_account/new_party', '1', 'spa_cnhk', 'csa_league', '98WenDao', null);

-- ----------------------------
-- Table structure for ecard_business
-- ----------------------------
DROP TABLE IF EXISTS `ecard_business`;
CREATE TABLE `ecard_business` (
  `id` mediumint(9) NOT NULL AUTO_INCREMENT,
  `ecard_no` varchar(30) DEFAULT NULL COMMENT '逍遥点卡号',
  `ecard_id` varchar(100) DEFAULT NULL COMMENT '吉比特点卡号',
  `ecard_server` varchar(30) CHARACTER SET utf8 COLLATE utf8_bin DEFAULT NULL COMMENT '交易服务器',
  `ecard_seller` varchar(128) DEFAULT NULL COMMENT '卖家角色',
  `ecard_buyer` varchar(128) DEFAULT NULL COMMENT '买家角色',
  `ecard_buyer_account` varchar(128) DEFAULT NULL COMMENT '买家账号',
  `ecard_price` int(11) unsigned DEFAULT NULL COMMENT '成交价',
  `ecard_type` int(11) DEFAULT NULL COMMENT '点卡类型',
  `buyer_done` int(11) DEFAULT NULL COMMENT '买家确认',
  `seller_done` int(11) DEFAULT NULL COMMENT '卖家确认',
  `ecard_time` varchar(30) DEFAULT '' COMMENT '交易时间',
  `action` varchar(10) DEFAULT NULL,
  `insert_time` varchar(14) DEFAULT NULL,
  `update_time` varchar(14) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of ecard_business
-- ----------------------------

-- ----------------------------
-- Table structure for ecard_log
-- ----------------------------
DROP TABLE IF EXISTS `ecard_log`;
CREATE TABLE `ecard_log` (
  `id` mediumint(9) NOT NULL AUTO_INCREMENT,
  `ecard_no` varchar(30) DEFAULT NULL,
  `ecard_pwd` varchar(20) DEFAULT NULL,
  `ecard_id` varchar(100) DEFAULT NULL,
  `ecard_server` varchar(30) DEFAULT NULL,
  `ecard_seller` varchar(128) DEFAULT NULL,
  `ecard_buyer` varchar(128) DEFAULT NULL,
  `ecard_buyer_account` varchar(128) DEFAULT NULL,
  `ecard_price` int(11) DEFAULT NULL,
  `ecard_type` int(11) DEFAULT NULL,
  `ecard_time` varchar(30) DEFAULT NULL,
  `direction` varchar(10) DEFAULT NULL,
  `result` varchar(10) DEFAULT NULL,
  `insert_time` varchar(14) DEFAULT NULL,
  `update_time` varchar(14) DEFAULT NULL,
  `memo` text,
  PRIMARY KEY (`id`),
  KEY `ecard_id` (`ecard_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of ecard_log
-- ----------------------------

-- ----------------------------
-- Table structure for ecard_need_to_resend
-- ----------------------------
DROP TABLE IF EXISTS `ecard_need_to_resend`;
CREATE TABLE `ecard_need_to_resend` (
  `id` mediumint(9) NOT NULL AUTO_INCREMENT,
  `ecard_no` varchar(30) DEFAULT NULL,
  `ecard_pwd` varchar(20) DEFAULT NULL,
  `ecard_id` varchar(100) DEFAULT NULL,
  `ecard_server` varchar(30) DEFAULT NULL,
  `ecard_seller` varchar(128) DEFAULT NULL,
  `ecard_buyer` varchar(128) DEFAULT NULL,
  `ecard_buyer_account` varchar(128) DEFAULT NULL,
  `ecard_price` int(11) DEFAULT NULL,
  `ecard_type` int(11) DEFAULT NULL,
  `ecard_dist` varchar(30) DEFAULT NULL,
  `ecard_time` varchar(30) DEFAULT NULL,
  `ecard_order_id` text,
  `send_result` int(11) DEFAULT '0',
  `insert_time` varchar(14) DEFAULT NULL,
  `update_time` varchar(14) DEFAULT NULL,
  `memo` text,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of ecard_need_to_resend
-- ----------------------------

-- ----------------------------
-- Table structure for encourage
-- ----------------------------
DROP TABLE IF EXISTS `encourage`;
CREATE TABLE `encourage` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `account` varchar(32) NOT NULL DEFAULT '' COMMENT '账号',
  `encourage_type` varchar(255) NOT NULL DEFAULT '' COMMENT '奖励类型',
  `dist` varchar(32) NOT NULL DEFAULT '' COMMENT '区组',
  `expired_time` varchar(14) NOT NULL DEFAULT '' COMMENT '终止时间',
  `used` int(11) NOT NULL DEFAULT '0' COMMENT '领取标识',
  `request_time` varchar(14) NOT NULL DEFAULT '' COMMENT '申请时间',
  `locked` int(11) NOT NULL DEFAULT '0' COMMENT '锁定标识',
  `insert_time` varchar(14) NOT NULL DEFAULT '',
  `checksum` varchar(32) NOT NULL DEFAULT '',
  `memo` text COMMENT '备注',
  PRIMARY KEY (`id`),
  KEY `account` (`account`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of encourage
-- ----------------------------

-- ----------------------------
-- Table structure for encourage_log
-- ----------------------------
DROP TABLE IF EXISTS `encourage_log`;
CREATE TABLE `encourage_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `dist` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `encourage_type` varchar(128) NOT NULL DEFAULT '',
  `request_time` varchar(14) NOT NULL DEFAULT '',
  `encourage_id` int(11) NOT NULL DEFAULT '0',
  `memo` text,
  PRIMARY KEY (`id`),
  KEY `account` (`account`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of encourage_log
-- ----------------------------

-- ----------------------------
-- Table structure for express_recharge
-- ----------------------------
DROP TABLE IF EXISTS `express_recharge`;
CREATE TABLE `express_recharge` (
  `order_id` varchar(32) NOT NULL DEFAULT '',
  `result` tinyint(2) DEFAULT NULL,
  `result_code` int(11) DEFAULT NULL,
  `gold` int(11) DEFAULT NULL,
  `inquiry_times` int(11) DEFAULT NULL,
  `card_no` varchar(30) DEFAULT NULL,
  `card_pwd` varchar(20) DEFAULT NULL,
  `user_name` varchar(128) DEFAULT NULL,
  `char_name` varchar(32) DEFAULT NULL,
  `server_name` varchar(30) DEFAULT NULL,
  `sales_name` varchar(128) DEFAULT NULL,
  `user_ip` varchar(15) DEFAULT NULL,
  `par_value` int(11) DEFAULT NULL,
  `pay_type` varchar(30) DEFAULT NULL,
  `card_type` tinyint(4) DEFAULT NULL,
  `update_time` varchar(14) DEFAULT NULL,
  `insert_time` varchar(14) DEFAULT NULL,
  `memo` text,
  PRIMARY KEY (`order_id`),
  KEY `result_inquiry` (`result`,`inquiry_times`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of express_recharge
-- ----------------------------

-- ----------------------------
-- Table structure for gbuy_coin_log
-- ----------------------------
DROP TABLE IF EXISTS `gbuy_coin_log`;
CREATE TABLE `gbuy_coin_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  `server` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `type` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `gold_coin` int(11) NOT NULL DEFAULT '0',
  `silver_coin` int(11) NOT NULL DEFAULT '0',
  `gcoin_before` int(11) NOT NULL DEFAULT '0',
  `gcoin_after` int(11) NOT NULL DEFAULT '0',
  `scoin_before` int(11) NOT NULL DEFAULT '0',
  `scoin_after` int(11) NOT NULL DEFAULT '0',
  `gbuy_gcoin_before` int(11) NOT NULL DEFAULT '0',
  `gbuy_gcoin_after` int(11) NOT NULL DEFAULT '0',
  `gbuy_scoin_before` int(11) NOT NULL DEFAULT '0',
  `gbuy_scoin_after` int(11) NOT NULL DEFAULT '0',
  `para1` int(11) NOT NULL DEFAULT '0',
  `para2` varchar(32) NOT NULL DEFAULT '0',
  `para3` varchar(128) NOT NULL DEFAULT '0',
  `memo` text NOT NULL,
  PRIMARY KEY (`id`),
  KEY `time` (`update_time`),
  KEY `gid` (`gid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of gbuy_coin_log
-- ----------------------------

-- ----------------------------
-- Table structure for gift_coin_log
-- ----------------------------
DROP TABLE IF EXISTS `gift_coin_log`;
CREATE TABLE `gift_coin_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `transaction_id` varchar(32) NOT NULL DEFAULT '',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gold_coin` int(11) NOT NULL DEFAULT '0',
  `silver_coin` int(11) NOT NULL DEFAULT '0',
  `coupon_gold_used` int(11) NOT NULL DEFAULT '0',
  `coupon_silver_used` int(11) NOT NULL DEFAULT '0',
  `checksum` varchar(32) NOT NULL DEFAULT '',
  `time` varchar(14) NOT NULL DEFAULT '',
  `server` varchar(32) NOT NULL DEFAULT '',
  `source` tinyint(4) NOT NULL DEFAULT '0',
  `para1` int(11) NOT NULL DEFAULT '0',
  `para2` int(11) NOT NULL DEFAULT '0',
  `para3` varchar(32) NOT NULL DEFAULT '',
  `para4` varchar(32) NOT NULL DEFAULT '',
  `para5` varchar(255) NOT NULL DEFAULT '',
  `memo` text,
  PRIMARY KEY (`id`,`transaction_id`),
  UNIQUE KEY `transaction_id` (`transaction_id`),
  KEY `account` (`account`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of gift_coin_log
-- ----------------------------

-- ----------------------------
-- Table structure for gift_valid_time
-- ----------------------------
DROP TABLE IF EXISTS `gift_valid_time`;
CREATE TABLE `gift_valid_time` (
  `gift_type` varchar(30) NOT NULL DEFAULT '',
  `start_time` varchar(14) NOT NULL DEFAULT '0',
  `end_time` varchar(14) NOT NULL DEFAULT '0',
  `checksum` varchar(32) NOT NULL DEFAULT '',
  PRIMARY KEY (`gift_type`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of gift_valid_time
-- ----------------------------

-- ----------------------------
-- Table structure for ip_card
-- ----------------------------
DROP TABLE IF EXISTS `ip_card`;
CREATE TABLE `ip_card` (
  `card_no` varchar(32) NOT NULL DEFAULT '',
  `type` varchar(32) NOT NULL DEFAULT '',
  `used` int(11) NOT NULL DEFAULT '0',
  `account` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `request_time` varchar(14) NOT NULL DEFAULT '',
  `memo` text,
  PRIMARY KEY (`card_no`,`type`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of ip_card
-- ----------------------------

-- ----------------------------
-- Table structure for ip_info
-- ----------------------------
DROP TABLE IF EXISTS `ip_info`;
CREATE TABLE `ip_info` (
  `start_ip_desc` varchar(16) NOT NULL DEFAULT '',
  `end_ip_desc` varchar(16) NOT NULL DEFAULT '',
  `start_ip` int(11) unsigned NOT NULL DEFAULT '0',
  `end_ip` int(11) unsigned NOT NULL DEFAULT '0',
  `country` varchar(64) DEFAULT NULL,
  `province` varchar(64) DEFAULT NULL,
  `city` varchar(64) DEFAULT NULL,
  `memo` text,
  PRIMARY KEY (`start_ip_desc`,`end_ip_desc`,`start_ip`,`end_ip`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of ip_info
-- ----------------------------

-- ----------------------------
-- Table structure for league_account
-- ----------------------------
DROP TABLE IF EXISTS `league_account`;
CREATE TABLE `league_account` (
  `account` varchar(32) NOT NULL DEFAULT '',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `level_section` varchar(32) NOT NULL DEFAULT '',
  PRIMARY KEY (`account`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of league_account
-- ----------------------------

-- ----------------------------
-- Table structure for local_encrypt
-- ----------------------------
DROP TABLE IF EXISTS `local_encrypt`;
CREATE TABLE `local_encrypt` (
  `account` varchar(32) NOT NULL DEFAULT '',
  `file_name` varchar(64) NOT NULL DEFAULT '',
  `local_encrypt_info` text NOT NULL,
  `state` int(11) NOT NULL DEFAULT '0',
  `time` varchar(32) NOT NULL DEFAULT '0',
  `checksum` varchar(32) NOT NULL DEFAULT '',
  `memo` text,
  PRIMARY KEY (`account`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of local_encrypt
-- ----------------------------

-- ----------------------------
-- Table structure for matrix
-- ----------------------------
DROP TABLE IF EXISTS `matrix`;
CREATE TABLE `matrix` (
  `account` varchar(32) NOT NULL DEFAULT '',
  `matrix` text NOT NULL,
  `bind_time` varchar(32) NOT NULL DEFAULT '',
  `expired_time` varchar(32) NOT NULL DEFAULT '',
  `claim_lost_off_time` varchar(32) NOT NULL DEFAULT '',
  `enable` int(11) NOT NULL DEFAULT '1',
  `checksum` varchar(32) NOT NULL DEFAULT '',
  `update_time` varchar(32) NOT NULL DEFAULT '',
  `memo` text,
  PRIMARY KEY (`account`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of matrix
-- ----------------------------

-- ----------------------------
-- Table structure for partywar_account
-- ----------------------------
DROP TABLE IF EXISTS `partywar_account`;
CREATE TABLE `partywar_account` (
  `account` varchar(32) NOT NULL DEFAULT '',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `gid` varchar(16) NOT NULL DEFAULT '',
  `name` varchar(32) NOT NULL DEFAULT '',
  `party_name` varchar(32) NOT NULL DEFAULT '',
  PRIMARY KEY (`account`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of partywar_account
-- ----------------------------

-- ----------------------------
-- Table structure for phone_auth
-- ----------------------------
DROP TABLE IF EXISTS `phone_auth`;
CREATE TABLE `phone_auth` (
  `account` varchar(32) NOT NULL DEFAULT '',
  `bind_time` varchar(14) NOT NULL DEFAULT '',
  `state` tinyint(4) NOT NULL DEFAULT '0',
  `bonus_info` text,
  `update_time` varchar(14) NOT NULL DEFAULT '',
  PRIMARY KEY (`account`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of phone_auth
-- ----------------------------

-- ----------------------------
-- Table structure for privilege_group
-- ----------------------------
DROP TABLE IF EXISTS `privilege_group`;
CREATE TABLE `privilege_group` (
  `privilege_group` varchar(15) NOT NULL DEFAULT '',
  `class` varchar(15) NOT NULL DEFAULT '',
  `privilege` varchar(255) NOT NULL DEFAULT '',
  `memo` text,
  PRIMARY KEY (`privilege_group`,`class`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Records of privilege_group
-- ----------------------------
INSERT INTO `privilege_group` VALUES ('CS1', 'admintool', 'block_character/fetch_keywords/chat_attr/channel_silence/set_inspect_area/query_ddb/query_adb/query_mdb/query_ldb/query_tdb/monitor_signature/query_online_char', null);
INSERT INTO `privilege_group` VALUES ('CS2', 'admintool', 'polling_char/ip_query/trace_iid/restrict_char/polling_account/throw_in_jail/jail_and_silence', null);
INSERT INTO `privilege_group` VALUES ('CS3', 'admintool', 'control_account/oper_check/release_protected/transfer_pet/transfer_item/bind_roll_back/delete_pet/delete_item/polling_account/transfer_cash/delete_cash', null);
INSERT INTO `privilege_group` VALUES ('CS4', 'admintool', 'update_keywords/add_keywords/delete_keywords', null);
INSERT INTO `privilege_group` VALUES ('G1', 'admintool', 'query_ddb/query_adb/query_mdb/oper_character/block_account/block_character/delete_character/recover_character/release_character/delete_item/control_account/delete_pet/polling_char', '');

-- ----------------------------
-- Table structure for protect_bonus
-- ----------------------------
DROP TABLE IF EXISTS `protect_bonus`;
CREATE TABLE `protect_bonus` (
  `account` varchar(32) NOT NULL DEFAULT '',
  `branch` varchar(7) NOT NULL DEFAULT '',
  `dist` varchar(32) DEFAULT NULL,
  `matrix_time` text,
  `passpod_time` text,
  `phone_time` text,
  `sm_time` text,
  `bonus_time` varchar(14) DEFAULT NULL,
  `bonus_type` varchar(10) DEFAULT NULL,
  `bonus_info` text,
  `insert_time` varchar(14) NOT NULL DEFAULT '',
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `memo` text,
  PRIMARY KEY (`account`,`branch`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of protect_bonus
-- ----------------------------

-- ----------------------------
-- Table structure for protect_range
-- ----------------------------
DROP TABLE IF EXISTS `protect_range`;
CREATE TABLE `protect_range` (
  `account` varchar(32) NOT NULL DEFAULT '',
  `p_range` tinyint(4) NOT NULL DEFAULT '0',
  PRIMARY KEY (`account`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of protect_range
-- ----------------------------

-- ----------------------------
-- Table structure for pwd_protect
-- ----------------------------
DROP TABLE IF EXISTS `pwd_protect`;
CREATE TABLE `pwd_protect` (
  `account` varchar(32) NOT NULL DEFAULT '',
  `protect_type` varchar(32) NOT NULL DEFAULT '',
  `protect_info` text,
  `checksum` varchar(32) DEFAULT NULL,
  `bind_time` varchar(14) DEFAULT NULL,
  `memo` text,
  PRIMARY KEY (`account`,`protect_type`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of pwd_protect
-- ----------------------------

-- ----------------------------
-- Table structure for relative_acc
-- ----------------------------
DROP TABLE IF EXISTS `relative_acc`;
CREATE TABLE `relative_acc` (
  `account` varchar(32) NOT NULL DEFAULT '',
  `last_add_coin_cookie` varchar(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`account`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of relative_acc
-- ----------------------------

-- ----------------------------
-- Table structure for safe_ctrl
-- ----------------------------
DROP TABLE IF EXISTS `safe_ctrl`;
CREATE TABLE `safe_ctrl` (
  `account` varchar(32) NOT NULL DEFAULT '',
  `protect_type` varchar(32) NOT NULL DEFAULT '',
  `protect_info` text,
  `checksum` varchar(32) DEFAULT NULL,
  `memo` text,
  PRIMARY KEY (`account`,`protect_type`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of safe_ctrl
-- ----------------------------
INSERT INTO `safe_ctrl` VALUES ('gm01', 'total_switch', '0', '71E3F9D4268C364BBF3889048FA9BC1B', '');
INSERT INTO `safe_ctrl` VALUES ('test01', 'total_switch', '0', '1F516ECCEA7F114CE63A1C8B06431500', '(undefined)');

-- ----------------------------
-- Table structure for server
-- ----------------------------
DROP TABLE IF EXISTS `server`;
CREATE TABLE `server` (
  `id` int(11) NOT NULL DEFAULT '0',
  `server` varchar(32) CHARACTER SET utf8 NOT NULL DEFAULT '' COMMENT '服务器名称',
  `alias` varchar(32) NOT NULL DEFAULT '',
  `ip` varchar(15) NOT NULL DEFAULT '',
  `port` int(11) NOT NULL DEFAULT '0',
  `max_user` int(11) NOT NULL DEFAULT '0' COMMENT '最大玩家数',
  `dist` varchar(32) NOT NULL DEFAULT '' COMMENT '区组名',
  `enable` int(11) NOT NULL DEFAULT '0' COMMENT '是否开放本线',
  `server_key` varchar(32) NOT NULL DEFAULT '',
  `when_iid_duplicated` varchar(32) NOT NULL DEFAULT '',
  `when_challenge_failed` varchar(32) NOT NULL DEFAULT 'punish_ex',
  `when_found_cheater` varchar(32) NOT NULL DEFAULT '',
  `punish_id` int(11) NOT NULL DEFAULT '0',
  `save_when_coin_cost` int(11) NOT NULL DEFAULT '0',
  `charge_level` int(11) NOT NULL DEFAULT '0',
  `charge_interval` int(11) NOT NULL DEFAULT '0',
  `ahserver` varchar(32) NOT NULL DEFAULT '',
  `ip_wt` varchar(15) NOT NULL DEFAULT '',
  `ip_dx` varchar(15) NOT NULL DEFAULT '',
  `enable_multi_ip` tinyint(4) unsigned NOT NULL DEFAULT '0',
  `memo` text,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Records of server
-- ----------------------------
INSERT INTO `server` VALUES ('1', '98WenDao', '', '192.168.200.100', '9010', '400', '98WenDao', '1', 'network', '', 'punish_ex', '', '0', '0', '0', '600', 'apex1', '', '', '0', '');

-- ----------------------------
-- Table structure for server_status
-- ----------------------------
DROP TABLE IF EXISTS `server_status`;
CREATE TABLE `server_status` (
  `server` varchar(32) NOT NULL DEFAULT '',
  `available` int(11) NOT NULL DEFAULT '0',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `online` int(11) NOT NULL DEFAULT '0',
  `max_user` int(11) NOT NULL DEFAULT '0',
  `cpu_cost` int(11) NOT NULL DEFAULT '0',
  `cpu_satisfy` int(11) NOT NULL DEFAULT '0',
  `report_time` varchar(32) NOT NULL DEFAULT '',
  `memo` text,
  PRIMARY KEY (`server`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of server_status
-- ----------------------------

-- ----------------------------
-- Table structure for service
-- ----------------------------
DROP TABLE IF EXISTS `service`;
CREATE TABLE `service` (
  `account` varchar(32) NOT NULL DEFAULT '',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `permit` varchar(255) NOT NULL DEFAULT '',
  `forbid` varchar(255) NOT NULL DEFAULT '',
  `memo` text,
  PRIMARY KEY (`account`,`dist`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of service
-- ----------------------------

-- ----------------------------
-- Table structure for spa
-- ----------------------------
DROP TABLE IF EXISTS `spa`;
CREATE TABLE `spa` (
  `spa` varchar(32) NOT NULL DEFAULT '',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `ip` varchar(15) NOT NULL DEFAULT '',
  `port` int(11) NOT NULL DEFAULT '0',
  `server_key` varchar(32) NOT NULL DEFAULT '',
  `id` int(11) NOT NULL DEFAULT '0',
  `http_plat` varchar(255) DEFAULT 'NULL',
  `http_ip` varchar(255) DEFAULT NULL,
  `httpd_enable` int(1) DEFAULT NULL,
  `httpd_port` int(16) DEFAULT NULL,
  `memo` text,
  PRIMARY KEY (`spa`,`dist`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Records of spa
-- ----------------------------
INSERT INTO `spa` VALUES ('spa_cnhk', '98WenDao', '192.168.200.100', '9055', 'network', '0', '98WenDao', '', '0', '82', null);

-- ----------------------------
-- Table structure for tts
-- ----------------------------
DROP TABLE IF EXISTS `tts`;
CREATE TABLE `tts` (
  `tts` varchar(32) NOT NULL DEFAULT '',
  `ip` varchar(32) NOT NULL DEFAULT '',
  `port` int(11) NOT NULL DEFAULT '0',
  `id` int(11) NOT NULL DEFAULT '0',
  `dist` varchar(32) NOT NULL DEFAULT '',
  `trading_url` varchar(255) NOT NULL DEFAULT '',
  `web_status_url` varchar(32) NOT NULL DEFAULT '',
  `http_ip` varchar(255) NOT NULL DEFAULT '',
  `http_plat` varchar(255) NOT NULL DEFAULT '',
  `httpd_enable` int(1) NOT NULL DEFAULT '0',
  `httpd_port` int(16) NOT NULL DEFAULT '0',
  `check_sum` varchar(32) NOT NULL DEFAULT '',
  PRIMARY KEY (`tts`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Records of tts
-- ----------------------------
INSERT INTO `tts` VALUES ('tts_2017', '192.168.200.100', '9050', '0', '98WenDao', '', '', '', '', '1', '9150', '');

-- ----------------------------
-- Table structure for vip_sms_protect
-- ----------------------------
DROP TABLE IF EXISTS `vip_sms_protect`;
CREATE TABLE `vip_sms_protect` (
  `account` varchar(32) NOT NULL DEFAULT '',
  `p_coin_trade` int(11) NOT NULL DEFAULT '0',
  `p_coin_consum` int(11) NOT NULL DEFAULT '0',
  `p_prop_trade` int(11) NOT NULL DEFAULT '0',
  `p_block_account` int(11) NOT NULL DEFAULT '0',
  `p_oper_role` int(11) NOT NULL DEFAULT '0',
  `update_time` varchar(14) NOT NULL DEFAULT '',
  PRIMARY KEY (`account`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of vip_sms_protect
-- ----------------------------

-- ----------------------------
-- Table structure for wba
-- ----------------------------
DROP TABLE IF EXISTS `wba`;
CREATE TABLE `wba` (
  `user` varchar(32) NOT NULL DEFAULT '',
  `password` varchar(32) NOT NULL DEFAULT '',
  `ip` varchar(15) NOT NULL DEFAULT '',
  `money` int(11) NOT NULL DEFAULT '0',
  `force_count` int(11) NOT NULL DEFAULT '0',
  `memo` text,
  PRIMARY KEY (`user`,`ip`,`password`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of wba
-- ----------------------------

-- ----------------------------
-- Table structure for zone
-- ----------------------------
DROP TABLE IF EXISTS `zone`;
CREATE TABLE `zone` (
  `zone` varchar(32) NOT NULL DEFAULT '',
  `alias` varchar(32) NOT NULL DEFAULT '',
  `memo` text,
  PRIMARY KEY (`zone`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of zone
-- ----------------------------
