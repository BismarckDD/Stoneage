-- Optional MySQL schema. Default SAAC backend: SQLite (database.sqlite.sql).
CREATE DATABASE IF NOT EXISTS sa;
USE sa;

CREATE TABLE IF NOT EXISTS `character_info` (
  `cdkey` varchar(32) CHARACTER SET utf8mb4 NOT NULL,
  `saveindex` tinyint(1) NOT NULL DEFAULT 0 COMMENT '1个账号可以建2个人物.',
  `name` varchar(32) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '角色名称',
  `famename` varchar(32) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `level` int(11) NOT NULL DEFAULT 1 COMMENT '',
  `trans` int(11) NOT NULL DEFAULT 0 COMMENT '',
  `alive` int(11) NOT NULL DEFAULT 0 COMMENT '',
  `fame` int(11) NOT NULL,
  `TITLE_LV` int(11) NOT NULL,
  `DUELBATTLECOUNT` int(11) NOT NULL,
  `DUELWINCOUNT` int(11) NOT NULL,
  `DUELLOSECOUNT` int(11) NOT NULL,
  `DUELSTWINCOUNT` int(11) NOT NULL,
  `DUELSTLOSECOUNT` int(11) NOT NULL,
  `DUELMAXSTWINCOUNT` int(11) NOT NULL,
  `vip` int(11) NOT NULL,
  `time` datetime NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `account` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `username` varchar(32) NOT NULL COMMENT '用户名称',
  `password` varchar(32) NOT NULL COMMENT '用户密码',
  `IP` varchar(16) NOT NULL DEFAULT '-' COMMENT '用户上次登录的IP地址',
  `MAC` varchar(64) NOT NULL DEFAULT '' COMMENT '用户上次登录的MAC地址',
  `RegTime` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '用户注册的时间',
  `LoginTime` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '用户上一次登录的时间',
  `OnlineName` varchar(32) CHARACTER SET utf8mb4 DEFAULT NULL,
  `Online` tinyint(4) DEFAULT '0',
  `Path` varchar(16) DEFAULT '',
  `VipPoint` int(11) DEFAULT '0',
  `PayPoint` int(11) DEFAULT '0',
  `SafePasswd` varchar(32) DEFAULT '',
  `ServerId` int(4) NOT NULL DEFAULT '0',
  `ServerName` varchar(32) NOT NULL DEFAULT '' COMMENT '???',
  `Offline` tinyint(4) NOT NULL DEFAULT '0',
  `PKPoint` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  UNIQUE KEY `username` (`username`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `account_lock` (
  `Name` varchar(64) NOT NULL,
  PRIMARY KEY (`Name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `online_buy` (
  `CostPasswd` varchar(32) NOT NULL,
  `CostStr` varchar(64) DEFAULT NULL,
  `cdkey` varchar(32) DEFAULT NULL,
  `CostTime` datetime DEFAULT NULL,
  `check` tinyint(1) DEFAULT '1',
  PRIMARY KEY (`CostPasswd`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `online_cost` (
  `CostPasswd` varchar(32) NOT NULL,
  `CostVal` int(11) NOT NULL,
  `PayVal` int(11) NOT NULL,
  `cdkey` varchar(32) DEFAULT '',
  `CostTime` datetime DEFAULT NULL,
  `check` tinyint(1) NOT NULL DEFAULT '1',
  UNIQUE KEY `CostPasswd` (`CostPasswd`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `safe_data` (
  `cdkey` varchar(32) NOT NULL DEFAULT '',
  `newsafe` varchar(64) DEFAULT NULL,
  `oldsafe` varchar(64) DEFAULT NULL,
  `flg` int(1) NOT NULL,
  `FixTime` datetime NOT NULL,
  PRIMARY KEY (`cdkey`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;


-- Full character saves and account-wide warehouses. slot is 0=item, 1=pet in account_storage.
CREATE TABLE IF NOT EXISTS character_save (
  account_name VARCHAR(64) NOT NULL,
  slot INTEGER NOT NULL,
  save_data LONGBLOB NOT NULL,
  format_version INTEGER NOT NULL DEFAULT 1,
  revision INTEGER NOT NULL DEFAULT 1,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (account_name, slot)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin;
CREATE TABLE IF NOT EXISTS account_storage (
  account_name VARCHAR(64) NOT NULL,
  slot INTEGER NOT NULL,
  save_data LONGBLOB NOT NULL,
  revision INTEGER NOT NULL DEFAULT 1,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (account_name, slot)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin;
CREATE TABLE IF NOT EXISTS character_archive (
  archive_id BIGINT PRIMARY KEY AUTO_INCREMENT,
  account_name VARCHAR(64) NOT NULL,
  slot INTEGER NOT NULL,
  save_data LONGBLOB NOT NULL,
  revision INTEGER NOT NULL,
  archived_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin;

CREATE TABLE IF NOT EXISTS family (
  family_id INTEGER NOT NULL,
  legacy_index INTEGER NOT NULL DEFAULT 0,
  leader_image INTEGER NOT NULL DEFAULT 0,
  member_count INTEGER NOT NULL DEFAULT 0,
  joined_count INTEGER NOT NULL DEFAULT 0,
  accept_members INTEGER NOT NULL DEFAULT 0,
  established INTEGER NOT NULL DEFAULT 0,
  sprite INTEGER NOT NULL DEFAULT 0,
  manor_index INTEGER NOT NULL DEFAULT 0,
  reputation INTEGER NOT NULL DEFAULT 0,
  gold INTEGER NOT NULL DEFAULT 0,
  member_limit INTEGER NOT NULL DEFAULT 0,
  apply_time INTEGER NOT NULL DEFAULT 0,
  setup_time INTEGER NOT NULL DEFAULT 0,
  delete_time INTEGER NOT NULL DEFAULT 0,
  message_cursor INTEGER NOT NULL DEFAULT 0,
  message_count INTEGER NOT NULL DEFAULT 0,
  adventure_reputation INTEGER NOT NULL DEFAULT 0,
  breeding_reputation INTEGER NOT NULL DEFAULT 0,
  craft_reputation INTEGER NOT NULL DEFAULT 0,
  cooking_reputation INTEGER NOT NULL DEFAULT 0,
  pk_reputation INTEGER NOT NULL DEFAULT 0,
  momentum INTEGER NOT NULL DEFAULT 0,
  manor_apply_time INTEGER NOT NULL DEFAULT 0,
  pay_point INTEGER NOT NULL DEFAULT 0,
  badge INTEGER NOT NULL DEFAULT 0,
  name TEXT NOT NULL,
  leader_name TEXT NOT NULL,
  leader_account TEXT NOT NULL,
  guardian_name TEXT NOT NULL,
  guardian_data TEXT NOT NULL,
  rules TEXT NOT NULL,
  totem varchar(649) NOT NULL DEFAULT '',
  PRIMARY KEY (family_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin;

CREATE TABLE IF NOT EXISTS family_member (
  family_id INTEGER NOT NULL,
  member_slot INTEGER NOT NULL,
  level INTEGER NOT NULL DEFAULT 0,
  role INTEGER NOT NULL DEFAULT 0,
  delete_time INTEGER NOT NULL DEFAULT 0,
  reputation INTEGER NOT NULL DEFAULT 0,
  leader_eligible INTEGER NOT NULL DEFAULT 0,
  momentum INTEGER NOT NULL DEFAULT 0,
  character_name TEXT NOT NULL,
  account_name TEXT NOT NULL,
  PRIMARY KEY (family_id, member_slot)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin;

CREATE TABLE IF NOT EXISTS family_message (
  family_id INTEGER NOT NULL,
  message_slot INTEGER NOT NULL,
  content TEXT NOT NULL,
  PRIMARY KEY (family_id, message_slot)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin;

CREATE TABLE IF NOT EXISTS family_public_message (
  board_id INTEGER NOT NULL,
  message_slot INTEGER NOT NULL,
  content TEXT NOT NULL,
  PRIMARY KEY (board_id, message_slot)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin;

CREATE TABLE IF NOT EXISTS family_message_board (
  board_id INTEGER NOT NULL,
  message_count INTEGER NOT NULL DEFAULT 0,
  message_cursor INTEGER NOT NULL DEFAULT 0,
  PRIMARY KEY (board_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin;

CREATE TABLE IF NOT EXISTS family_manor (
  manor_id INTEGER NOT NULL,
  map_id INTEGER NOT NULL DEFAULT 0,
  x INTEGER NOT NULL DEFAULT 0,
  y INTEGER NOT NULL DEFAULT 0,
  leader_map_id INTEGER NOT NULL DEFAULT 0,
  village_id INTEGER NOT NULL DEFAULT 0,
  owner_family_id INTEGER NOT NULL DEFAULT 0,
  owner_name TEXT NOT NULL,
  owner_reputation INTEGER NOT NULL DEFAULT 0,
  occupied_at INTEGER NOT NULL DEFAULT 0,
  PRIMARY KEY (manor_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin;

CREATE TABLE IF NOT EXISTS family_pk_schedule (
  schedule_kind VARCHAR(32) NOT NULL,
  schedule_slot INTEGER NOT NULL,
  flag INTEGER NOT NULL DEFAULT 0,
  schedule_data TEXT NOT NULL,
  PRIMARY KEY (schedule_kind, schedule_slot)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin;

CREATE TABLE IF NOT EXISTS schema_migration (
  version INTEGER PRIMARY KEY,
  applied_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin;
INSERT INTO schema_migration(version) SELECT 1 WHERE NOT EXISTS (SELECT 1 FROM schema_migration WHERE version=1);
