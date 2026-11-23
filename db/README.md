# 数据库结构

SAAC 默认使用内嵌 SQLite3，文件路径相对于 SAAC 工作目录。首次运行前：

```sh
sqlite3 saac.sqlite3 < database.sqlite.sql
```

PowerShell 可使用 `sqlite3 saac.sqlite3 ".read database.sqlite.sql"`。
将 `acserv.cf` 放入运行目录；省略 `sql_backend` 时也使用 SQLite。
MySQL 仍可通过 `sql_backend mysql` 显式启用，初始化脚本为 `database.sql`。
当前构建仍链接 MySQL 客户端库，SQLite 模式不需要 MySQL 服务。

表名统一为小写 snake_case，避免大小写敏感平台上的差异和 SQL 关键字。
这是常见约定，并非 SQL 强制规则。本次保留已使用字段的名称，避免扩大
SAAC、Lua 和外部工具的字段接口变更；后续新增字段也建议使用 snake_case。

| 旧表名 | 新表名 | 使用位置 |
| --- | --- | --- |
| CSAlogin | account | SAAC 认证、在线状态、积分、安全密码；Lua 登录/登出 |
| lock / LOCK | account_lock | SAAC 账号/IP 锁定 |
| OnlineCost | online_cost | SAAC 充值码 |
| OnlineBuy | online_buy | SAAC 兑换码 |
| CSAinfo | character_info | freecharlogout.lua 角色信息 |
| safedata | safe_data | freecharlogin.lua 安全设备信息 |

Lua 支持表在两种初始化脚本中均保留；这不意味着 GMSV 的旧 MySQL Lua
接口已迁移到 SQLite。角色存档、家族、邮件仍沿用原文件存储。

## 清理依据

核对 server 和 server_lua 中的 SQL，包括条件编译功能；remove_code 是历史
代码，不作为现行表的消费者。未发现有效 SQL 引用的 21 张表从初始化脚本删除：
capture_pet、cash_draw、check、consignment、counter、dice、dicelog、loan、
OnlineCard、OnlinePlayer（只有注释）、PauctionInfo（只有遗留函数调用，无 SQL 实现）、
petbilling、pklist、SaleVipPoint、SuperStar、transpointlog、VipShop、weekpay、
YamaKing、yuxiaxie、yuxiaxielog。

account 删除 OnlineName1、OnlineName2、GamblePoint、TrialPoint、GoldPoint、
SQLPoint、Regip、qq、tel、Recommender、Lock、LockTime、LockInfo、Trust；
补齐 SAAC PK 积分代码使用的 PKPoint。保留 id 主键和 username 唯一约束。
character_info 删除未使用的 cameo 及 RESETPETCOUNT 至 HUNTBIGFISHCOUNT
这 14 个扩展计数。account_lock 删除未读写的 LockTime，online_cost 删除 creator；
SQLite 充值/兑换表删除未使用的 id，以 CostPasswd 为主键。
MySQL online_cost.CostTime 改为可空，允许生成未使用的充值码。
删除自动创建的测试账号及无人读取、参数格式不匹配的 sql.cf；SAAC 读取 acserv.cf。
其中 sql_RegIP 等五个未被配置解析器读取的选项也已移除。

## 已有数据库

初始化脚本只用于新库，不会自动修改旧表。停止服务、备份数据库后，在副本上
执行对应的 migrate_legacy 脚本，再导入对应的初始化脚本，最后更新 acserv.cf
及 Lua 脚本。迁移脚本针对仓库原始结构，只执行一次；自定义过表名或已添加
PKPoint 的库需先调整脚本。MySQL 脚本在所选数据库中执行，注意原 lock 表大小写。
迁移保留旧库的多余表和字段，不删除已有数据；清理存量数据应单独审核。
MySQL 到 SQLite 的数据搬迁不属于这两个同后端升级脚本的用途。

仓库外的运营后台或 Lua 扩展无法由本次引用审计覆盖；使用此类扩展时应先核对
其 SQL。运行中的数据库文件未作修改。
