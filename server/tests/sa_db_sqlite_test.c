#include "sa_db.h"
#include "sa_store.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void load_sql(SA_DB *db, const char *path) {
  FILE *file = fopen(path, "rb");
  long size;
  char *sql;
  assert(file != NULL);
  assert(fseek(file, 0, SEEK_END) == 0);
  size = ftell(file);
  assert(size >= 0);
  rewind(file);
  sql = malloc((size_t)size + 1);
  assert(sql != NULL);
  assert(fread(sql, 1, (size_t)size, file) == (size_t)size);
  sql[size] = '\0';
  fclose(file);
  if (sa_db_query(db, sql) != 0) {
    fprintf(stderr, "%s: %s\n", path, sa_db_error(db));
    abort();
  }
  free(sql);
}

int main(int argc, char **argv) {
  SA_DB *db;
  SA_DB_RESULT *result;
  SA_DB_ROW row;

  assert(argc == 2);
  remove("sa_db_sqlite_test.sqlite3");
  sa_db_configure(SA_DB_SQLITE, "sa_db_sqlite_test.sqlite3");
  db = sa_db_init(NULL);
  assert(db != NULL);
  assert(sa_db_connect(db, NULL, NULL, NULL, NULL, 0) != NULL);
  sa_store_attach(db);
  assert(sa_store_initialize() == 0);
  assert(sa_store_initialize() == 0);
  assert(sa_db_query(db, "CREATE TABLE users(name TEXT, LoginTime TEXT)") == 0);
  assert(sa_db_query(db, "INSERT INTO users VALUES(BINARY'alice', NOW())") == 0);
  assert(sa_db_query(db, "SELECT name, TO_DAYS(NOW())-TO_DAYS(LoginTime) "
                         "FROM users WHERE name=BINARY'alice'") == 0);
  result = sa_db_store_result(db);
  assert(result != NULL);
  assert(sa_db_num_rows(result) == 1);
  row = sa_db_fetch_row(result);
  assert(row != NULL && row[0] != NULL && row[1] != NULL);
  assert(row[0][0] == 'a' && row[1][0] == '0');
  sa_db_free_result(result);
  load_sql(db, argv[1]);
  load_sql(db, argv[1]); /* Initialization is repeatable. */
  assert(sa_db_query(db, "INSERT INTO account(username,password,RegTime,Path) "
                         "VALUES('alice','secret',NOW(),'char/0x1')") == 0);
  assert(sa_db_query(db, "UPDATE account SET Online=1,OnlineName='Alice',"
                         "LoginTime=NOW(),VipPoint=10,PayPoint=20,PKPoint=30,"
                         "SafePasswd='safe' WHERE username='alice'") == 0);
  assert(sa_db_query(db, "SELECT password,TO_DAYS(NOW())-TO_DAYS(LoginTime) "
                         "FROM account WHERE username=BINARY'alice'") == 0);
  result = sa_db_store_result(db);
  assert(result != NULL && sa_db_num_rows(result) == 1);
  row = sa_db_fetch_row(result);
  assert(strcmp(row[0], "secret") == 0 && strcmp(row[1], "0") == 0);
  sa_db_free_result(result);
  assert(sa_db_query(db, "INSERT INTO account(username,password) "
                         "VALUES('alice','duplicate')") != 0);
  assert(sa_db_query(db, "INSERT INTO account_lock(Name) VALUES('alice')") == 0);
  assert(sa_db_query(db, "INSERT INTO online_cost(CostPasswd,CostVal,PayVal) "
                         "VALUES('cost-code',10,20)") == 0);
  assert(sa_db_query(db, "INSERT INTO online_buy(CostPasswd,CostStr) "
                         "VALUES('buy-code','item')") == 0);
  assert(sa_db_query(db, "UPDATE online_cost SET `check`=0,cdkey='alice',"
                         "CostTime=NOW() WHERE CostPasswd='cost-code'") == 0);
  assert(sa_db_query(db, "INSERT INTO safe_data(cdkey,newsafe,oldsafe,flg,FixTime) "
                         "VALUES('alice','device','device',0,NOW())") == 0);
  assert(sa_db_query(db, "INSERT INTO character_info(cdkey,saveindex,name,"
                         "famename,level,trans,alive,fame,TITLE_LV,DUELBATTLECOUNT,"
                         "DUELWINCOUNT,DUELLOSECOUNT,DUELSTWINCOUNT,DUELSTLOSECOUNT,"
                         "DUELMAXSTWINCOUNT,vip,time) VALUES('alice',0,'Alice',"
                         "'',1,0,0,0,0,0,0,0,0,0,0,0,NOW())") == 0);
  {
    const char payload[] = "name'|option|BINARY\\data\n";
    char output[128];
    assert(sa_store_blob_get("character_save", "absent", 0, output, sizeof(output)) == -1);
    assert(sa_db_begin(db) == 0);
    assert(sa_store_blob_put("character_save", "alice'", 0, payload, sizeof(payload)-1) == 0);
    assert(sa_db_begin(db) == 0);
    assert(sa_store_blob_put("character_save", "alice'", 0, "rollback", 8) == 0);
    sa_db_rollback(db);
    assert(sa_db_commit(db) == 0);
    assert(sa_store_blob_get("character_save", "alice'", 0, output, sizeof(output)) == sizeof(payload)-1);
    assert(strcmp(output, payload) == 0);
    assert(sa_store_blob_get("character_save", "alice'", 0, output, 2) == -2);
    assert(sa_db_begin(db) == 0);
    assert(sa_store_blob_put("account_storage", "alice'", 0, "item", 4) == 0);
    assert(sa_store_blob_put("account_storage", "alice'", 1, "pet", 3) == 0);
    assert(sa_db_commit(db) == 0);
    assert(sa_store_blob_get("account_storage", "alice'", 1, output, sizeof(output)) == 3);
    assert(strcmp(output, "pet") == 0);
    assert(sa_db_begin(db) == 0);
    assert(sa_store_character_delete("alice'", 0) == 0);
    sa_db_rollback(db);
    assert(sa_store_blob_get("character_save", "alice'", 0, output, sizeof(output)) > 0);
    assert(sa_db_begin(db) == 0);
    assert(sa_store_character_delete("alice'", 0) == 0);
    assert(sa_db_commit(db) == 0);
    assert(sa_store_blob_get("character_save", "alice'", 0, output, sizeof(output)) == -1);
  }
  sa_db_close(db);
  remove("sa_db_sqlite_test.sqlite3");

  puts("SAAC SQLite compatibility test passed");
  return 0;
}
