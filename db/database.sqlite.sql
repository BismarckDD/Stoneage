-- SAAC local database schema for SQLite.
-- Usage: sqlite3 saac.sqlite3 < database.sqlite.sql

PRAGMA journal_mode = WAL;
PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS `account` (
  `id` INTEGER PRIMARY KEY AUTOINCREMENT,
  `username` TEXT NOT NULL UNIQUE,
  `password` TEXT NOT NULL,
  `IP` TEXT NOT NULL DEFAULT '-',
  `MAC` TEXT NOT NULL DEFAULT '',
  `RegTime` TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `LoginTime` TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `OnlineName` TEXT,
  `Online` INTEGER DEFAULT 0,
  `Path` TEXT DEFAULT '',
  `VipPoint` INTEGER DEFAULT 0,
  `PayPoint` INTEGER DEFAULT 0,
  `SafePasswd` TEXT DEFAULT '',
  `ServerId` INTEGER NOT NULL DEFAULT 0,
  `ServerName` TEXT NOT NULL DEFAULT '',
  `Offline` INTEGER NOT NULL DEFAULT 0,
  `PKPoint` INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS `account_lock` (
  `Name` TEXT NOT NULL PRIMARY KEY
);

CREATE TABLE IF NOT EXISTS `online_cost` (
  `CostPasswd` TEXT NOT NULL PRIMARY KEY,
  `CostVal` INTEGER NOT NULL DEFAULT 0,
  `PayVal` INTEGER NOT NULL DEFAULT 0,
  `check` INTEGER NOT NULL DEFAULT 1,
  `cdkey` TEXT DEFAULT '',
  `CostTime` TEXT
);

CREATE TABLE IF NOT EXISTS `online_buy` (
  `CostPasswd` TEXT NOT NULL PRIMARY KEY,
  `CostStr` TEXT NOT NULL,
  `check` INTEGER NOT NULL DEFAULT 1,
  `cdkey` TEXT DEFAULT '',
  `CostTime` TEXT
);

CREATE TABLE IF NOT EXISTS `character_info` (
  `cdkey` TEXT NOT NULL,
  `saveindex` INTEGER NOT NULL DEFAULT 0,
  `name` TEXT NOT NULL,
  `famename` TEXT NOT NULL,
  `level` INTEGER NOT NULL DEFAULT 1,
  `trans` INTEGER NOT NULL DEFAULT 0,
  `alive` INTEGER NOT NULL DEFAULT 0,
  `fame` INTEGER NOT NULL,
  `TITLE_LV` INTEGER NOT NULL,
  `DUELBATTLECOUNT` INTEGER NOT NULL,
  `DUELWINCOUNT` INTEGER NOT NULL,
  `DUELLOSECOUNT` INTEGER NOT NULL,
  `DUELSTWINCOUNT` INTEGER NOT NULL,
  `DUELSTLOSECOUNT` INTEGER NOT NULL,
  `DUELMAXSTWINCOUNT` INTEGER NOT NULL,
  `vip` INTEGER NOT NULL,
  `time` TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS `safe_data` (
  `cdkey` TEXT NOT NULL DEFAULT '',
  `newsafe` TEXT DEFAULT NULL,
  `oldsafe` TEXT DEFAULT NULL,
  `flg` INTEGER NOT NULL,
  `FixTime` TEXT NOT NULL,
  PRIMARY KEY (`cdkey`)
);


-- Full character saves and account-wide warehouses. slot is 0=item, 1=pet in account_storage.
CREATE TABLE IF NOT EXISTS character_save (
  account_name VARCHAR(64) NOT NULL,
  slot INTEGER NOT NULL,
  save_data BLOB NOT NULL,
  format_version INTEGER NOT NULL DEFAULT 1,
  revision INTEGER NOT NULL DEFAULT 1,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (account_name, slot)
);
CREATE TABLE IF NOT EXISTS account_storage (
  account_name VARCHAR(64) NOT NULL,
  slot INTEGER NOT NULL,
  save_data BLOB NOT NULL,
  revision INTEGER NOT NULL DEFAULT 1,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (account_name, slot)
);
CREATE TABLE IF NOT EXISTS character_archive (
  archive_id INTEGER PRIMARY KEY AUTOINCREMENT,
  account_name VARCHAR(64) NOT NULL,
  slot INTEGER NOT NULL,
  save_data BLOB NOT NULL,
  revision INTEGER NOT NULL,
  archived_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

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
  name TEXT NOT NULL DEFAULT '',
  leader_name TEXT NOT NULL DEFAULT '',
  leader_account TEXT NOT NULL DEFAULT '',
  guardian_name TEXT NOT NULL DEFAULT '',
  guardian_data TEXT NOT NULL DEFAULT '',
  rules TEXT NOT NULL DEFAULT '',
  totem TEXT NOT NULL DEFAULT '',
  PRIMARY KEY (family_id)
);

CREATE TABLE IF NOT EXISTS family_member (
  family_id INTEGER NOT NULL,
  member_slot INTEGER NOT NULL,
  level INTEGER NOT NULL DEFAULT 0,
  role INTEGER NOT NULL DEFAULT 0,
  delete_time INTEGER NOT NULL DEFAULT 0,
  reputation INTEGER NOT NULL DEFAULT 0,
  leader_eligible INTEGER NOT NULL DEFAULT 0,
  momentum INTEGER NOT NULL DEFAULT 0,
  character_name TEXT NOT NULL DEFAULT '',
  account_name TEXT NOT NULL DEFAULT '',
  PRIMARY KEY (family_id, member_slot)
);

CREATE TABLE IF NOT EXISTS family_message (
  family_id INTEGER NOT NULL,
  message_slot INTEGER NOT NULL,
  content TEXT NOT NULL DEFAULT '',
  PRIMARY KEY (family_id, message_slot)
);

CREATE TABLE IF NOT EXISTS family_public_message (
  board_id INTEGER NOT NULL,
  message_slot INTEGER NOT NULL,
  content TEXT NOT NULL DEFAULT '',
  PRIMARY KEY (board_id, message_slot)
);

CREATE TABLE IF NOT EXISTS family_message_board (
  board_id INTEGER NOT NULL,
  message_count INTEGER NOT NULL DEFAULT 0,
  message_cursor INTEGER NOT NULL DEFAULT 0,
  PRIMARY KEY (board_id)
);

CREATE TABLE IF NOT EXISTS family_manor (
  manor_id INTEGER NOT NULL,
  map_id INTEGER NOT NULL DEFAULT 0,
  x INTEGER NOT NULL DEFAULT 0,
  y INTEGER NOT NULL DEFAULT 0,
  leader_map_id INTEGER NOT NULL DEFAULT 0,
  village_id INTEGER NOT NULL DEFAULT 0,
  owner_family_id INTEGER NOT NULL DEFAULT 0,
  owner_name TEXT NOT NULL DEFAULT '',
  owner_reputation INTEGER NOT NULL DEFAULT 0,
  occupied_at INTEGER NOT NULL DEFAULT 0,
  PRIMARY KEY (manor_id)
);

CREATE TABLE IF NOT EXISTS family_pk_schedule (
  schedule_kind VARCHAR(32) NOT NULL,
  schedule_slot INTEGER NOT NULL,
  flag INTEGER NOT NULL DEFAULT 0,
  schedule_data TEXT NOT NULL DEFAULT '',
  PRIMARY KEY (schedule_kind, schedule_slot)
);

CREATE TABLE IF NOT EXISTS schema_migration (
  version INTEGER PRIMARY KEY,
  applied_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);
INSERT INTO schema_migration(version) SELECT 1 WHERE NOT EXISTS (SELECT 1 FROM schema_migration WHERE version=1);
