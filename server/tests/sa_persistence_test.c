/* Exercise the actual family serializers, including feature-guarded fields. */
#include "../saac/acfamily.c"
#include "char.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

int main(void) {
  SA_DB *db;
  static char data[CHARDATASIZE];
  remove("sa_persistence_test.sqlite3");
  sa_db_configure(SA_DB_SQLITE, "sa_persistence_test.sqlite3");
  db = sa_db_init(NULL);
  assert(sa_db_connect(db, NULL, NULL, NULL, NULL, 0));
  sa_store_attach(db);
  assert(sa_store_initialize() == 0);
  assert(readFamily(NULL) == 0 && fmnownum == 0);
  assert(loadCharOne("alice", 0, data, sizeof(data)) == -1);
  assert(saveCharOne("alice", 0, "Alice|option|DATAEND=1\\nBINARY'foo") == 0);
  assert(loadCharOne("alice", 0, data, sizeof(data)) == 0);
  assert(strcmp(data, "Alice|option|DATAEND=1\\nBINARY'foo") == 0);
  assert(loadCharOne("alice", 0, data, 4) == -2);
#ifdef _CHAR_POOLITEM
  assert(saveCharPoolItem("alice", "item1", 5) == 5);
  assert(InsertCharPoolItem("alice", "|item2", 6) == 6);
  assert(loadCharPoolItemOne("alice", data, sizeof(data)) == 11);
  assert(strcmp(data, "item1|item2") == 0);
#endif
#ifdef _CHAR_POOLPET
  assert(saveCharPoolPet("alice", "pet1", 4) == 4);
  assert(InsertCharPoolPet("alice", "|pet2", 5) == 5);
  assert(loadCharPoolPetOne("alice", data, sizeof(data)) == 9);
  assert(strcmp(data, "pet1|pet2") == 0);
#endif
  family[0].fmindex = 1;
  strcpy(family[0].fmname, "family'|name");
  strcpy(family[0].fmleaderid, "alice");
  strcpy(family[0].fmleadername, "Alice");
  family[0].fmmemberindexmaxnum = MAX_MEMBERNUM;
  family[0].fmnum = family[0].fmjoinnum = 1;
  family[0].fmgold = 12345;
  family[0].memonum = 1;
  family[0].memonowwritenum = 1;
  strcpy(family[0].memolist[0], "hello | ' BINARY \\n");
  strcpy(family[0].fmmemberindex[0].charid, "alice");
  strcpy(family[0].fmmemberindex[0].charname, "Alice");
  family[0].fmmemberindex[0].charflag = FMMEMBER_LEADER;
  family[0].fmmemberindex[0].onlineflag = 42;
  family[0].fmmemberindex[0].charfdid = 321;
  family[0].fmmemberindex[0].popular = 7;
  db_familyupdate[0] = 1;
  fmpoint[0].fl = 100;
  fmpoint[0].hadfmindex = 1;
  strcpy(fmpoint[0].hadfmname, family[0].fmname);
  db_fmpointupdate = 1;
  fmsmemo.num = 1; fmsmemo.fmsnowwritenum = 1;
  strcpy(fmsmemo.memodata[0], "public message");
  db_fmsmemoupdate = 1;
  assert(sa_db_begin(db) == 0);
  assert(writeFamily(NULL) == 0);
  assert(writeFMPoint(NULL) == 0);
  assert(writeFMSMemo(NULL) == 0);
  assert(sa_db_commit(db) == 0);
  assert(db_familyupdate[0] == 0 && db_fmpointupdate == 0 && db_fmsmemoupdate == 0);
  sa_store_attach(NULL); sa_db_close(db);
  db = sa_db_init(NULL);
  assert(sa_db_connect(db, NULL, NULL, NULL, NULL, 0));
  sa_store_attach(db);
  assert(loadCharOne("alice", 0, data, sizeof(data)) == 0);
  assert(readFamily(NULL) == 0 && fmnownum == 1);
  assert(strcmp(family[0].fmname, "family'|name") == 0);
  assert(family[0].fmgold == 12345);
  assert(family[0].fmmemberindex[0].charflag == FMMEMBER_LEADER);
  assert(family[0].fmmemberindex[0].onlineflag == 0 && family[0].fmmemberindex[0].charfdid == 0);
  assert(strcmp(family[0].memolist[0], "hello | ' BINARY \\n") == 0);
  assert(readFMPoint(NULL) == 0 && fmpoint[0].fl == 100 && fmpoint[0].hadfmindex == 1);
  assert(readFMSMemo(NULL) == 0 && strcmp(fmsmemo.memodata[0], "public message") == 0);
  /* Failed writes preserve the dirty flag and the last committed family. */
  assert(sa_db_run(db, "CREATE TRIGGER fail_family BEFORE UPDATE ON family BEGIN SELECT RAISE(ABORT,'test failure'); END", NULL, 0, NULL, NULL) == 0);
  family[0].fmgold = 999;
  db_familyupdate[0] = 1;
  assert(writeFamily(NULL) == -1 && db_familyupdate[0] == 1);
  assert(readOneFamilyFromTi(0) == 1 && family[0].fmgold == 12345);
  assert(sa_db_run(db, "DROP TRIGGER fail_family", NULL, 0, NULL, NULL) == 0);
  family[0].fmindex = -1; family[0].fmname[0] = 0;
  assert(writeFamily(NULL) == 0);
  assert(readFamily(NULL) == 0 && fmnownum == 0);
  sa_store_attach(NULL); sa_db_close(db);
  remove("sa_persistence_test.sqlite3");
  puts("Character and family persistence test passed");
  return 0;
}
