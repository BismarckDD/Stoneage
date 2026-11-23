#define __CHAR_C__
#include "char.h"
//
#include "main.h"
#include "saac_config.h"
#include "saac_server.h"
#include "util.h"
#include "sa_store.h"


// CoolFish: Family 2001/6/12
#include "acfamily.h"

// Arminius 7.17 memory lock
#include "lock.h"
#include "recv.h"

char char_buf[MAXCHAR_PER_USER][CHARDATASIZE];
char name_work[CHARDATASIZE];
char option_work[CHARDATASIZE];
char nm_work[CHARDATASIZE];

static void getCharNameFromString(const char *in, char *output);
static void getCharOptionFromString(const char *in, char *output);
// Nuke *1*1
static int makeSaveCharString(char *output, int outputlen, char *nm, char *opt,
                              char *info);
static int findBlankCharIndex(char *id);

static int unlinkCharFile(char *id, int num);
#ifdef _SLEEP_CHAR // 取得非活跃人物档名
#endif
#ifdef _CHAR_POOLITEM
#endif
#define SPACE '|'
#define SPACESTRING "|"

#define USERLOG(id, format, args...)                                           \
  {                                                                            \
    char body[1000];                                                           \
    char fn[1000];                                                             \
    if (id[0]) {                                                               \
      snprintf(body, sizeof(body), "%s.log.%d", id, get_rotate_count());       \
      makeDirFilename(fn, sizeof(fn), g_saac_config.logdir, getHash(id),       \
                      body);                                                   \
      logFile(fn, "%u ", (unsigned int)time(NULL));                            \
      logFile(fn, format, ##args);                                             \
    }                                                                          \
  }

char load_buf[CHARDATASIZE];
char info_buf[CHARDATASIZE];
char result[100];
char retdata[100];
void charLoadCallback(int ti,    // connection_fd
                      int auth,  // 0, 必须是0，也必然是0
                      char *id,  // username
                      char *passwd,  // password
                      char *charname,  // char_name
                      char *process_no,  // process_no
                      char *deadline,  // ""(empty)
                      int lock,
                      int mesgid) {
  // Spock deleted 2000/11/2
  // CoolFish: Init char_index 2001/10/16
#ifdef _NewSave
  int char_index = -1;
#else
  int char_index;
#endif
  // Spock deleted 2000/11/2
  /* if (auth != 0) {
    char data[100];
    snprintf(data, sizeof(data), "%d", auth);
#ifdef _NewSave
    SaacServer_ACCharLoad_send(ti, FAILED, data, mesgid, char_index);
#else
    SaacServer_ACCharLoad_send(ti, FAILED, data, mesgid);
#endif
    return;
  } */
  if (isLocked(id)) {
    int process = atoi(process_no);
#ifdef _NewSave
    SaacServer_ACCharLoad_send(ti, FAILED, "locked", mesgid, char_index);
#else
    SaacServer_ACCharLoad_send(ti, FAILED, "locked", mesgid);
#endif
    DeleteMemLock(getHash(id) & 0xff, id, &process); // 如果AP无锁则AC解锁
    logErr("\n (%s) 同一星系(SAAC)重复登入，踢人!!\n", id);
    SaacServer_ACKick_recv(ti, id, 1, -1); // 踢人
    checkGSUCheck(id);
    return;
  }
  char_index = getCharIndexByName(id, charname);
  logErr("\n档案装载序号:%d, 账号:%s, 名字:%s\n", char_index, id, charname);
  if (char_index < 0) {
#ifdef _NewSave
    SaacServer_ACCharLoad_send(ti, FAILED, "读取档案装载序号错误.", mesgid,
                               char_index);
#else
    SaacServer_ACCharLoad_send(ti, FAILED, "读取档案装在序号错误.", mesgid);
#endif
    return;
  }

  if (loadCharOne(id, char_index, load_buf, sizeof(load_buf)) < 0) {
#ifdef _NewSave
    SaacServer_ACCharLoad_send(ti, FAILED, "cannot load ( disk i/o error?)",
                               mesgid, char_index);
#else
    SaacServer_ACCharLoad_send(ti, FAILED, "cannot load ( disk i/o error?)",
                               mesgid);
#endif
    return;
  } else {
#ifdef _CHAR_POOLITEM
    checkCharPoolItem(id);
#endif
#ifdef _CHAR_POOLPET
    checkCharPoolPet(id);
#endif
  }
  if (lock) {
#ifdef _LOCK_ADD_NAME
    if (lockUser(gs[ti].name, id, charname, passwd, 1, result, sizeof(result),
                 retdata, sizeof(retdata), process_no, deadline) < 0) {
#else
    // Spock 2000/11/2
    if (lockUser(gs[ti].name, id, passwd, 1, result, sizeof(result), retdata,
                 sizeof(retdata), process, deadline) < 0) {
#endif
      // 获取用户锁失败.
      SaacServer_ACCharLoad_send(ti, FAILED, "LOCK USER FAILED.", mesgid, char_index);
      return;
    }
  }
  memset(info_buf, 0, sizeof(info_buf));
  getCharInfoFromString(load_buf, info_buf);
  makeStringFromEscaped(info_buf);
#ifdef _NewSave
  SaacServer_ACCharLoad_send(ti, SUCCESSFUL, info_buf, mesgid, char_index);
#else
  SaacServer_ACCharLoad_send(ti, SUCCESSFUL, info_buf, mesgid);
#endif

#ifdef _WAEI_KICK
  SaacServer_ACKick_recv(ti, id, 10, -1); // 踢其他星系
#endif
}

#ifdef _NewSave
int charSave(int ti, char *id, char *charname, char *opt, char *charinfo,
             int unlock, int mesgid, int char_index)
#else
int charSave(int ti, char *id, char *charname, char *opt, char *charinfo,
             int unlock, int mesgid)
#endif
{
#ifndef _NewSave
  int char_index;
#endif
  /* SAAC dispatches requests on one thread.  Keep MiB-sized scratch storage
   * out of the small native Windows thread stack. */
  static char savebuf[CHARDATASIZE];
  int ret = -1;
  memset(savebuf, 0, sizeof(savebuf));
  // andy_log
  if (strstr(charinfo, "DATAEND=") == NULL) {
    FILE *fp;
    if ((fp = fopen("badpetstring.txt", "a+")) != NULL) {
      fprintf(fp, "%s\n", charinfo);
      fclose(fp);
    }
    logErr("err add batpetstring.txt:%s[%s] !\n", id, charname);
  }

  // Nuke *1 add escape
  if (makeSaveCharString(savebuf, sizeof(savebuf), charname, opt, charinfo) <
      0) {
    logErr("\n AC存档:太长.");
    SaacServer_ACCharSave_send(ti, FAILED, "too long", mesgid);
    // Spock fixed 2000/11/1
    return ret;
  }
#ifdef _NewSave
  if (char_index == -1)
    char_index = getCharIndexByName(id, charname);
#else
  // Nuke *1-
  char_index = getCharIndexByName(id, charname);
#endif
  if (char_index == -2) {
    SaacServer_ACCharSave_send(ti, FAILED, "database read failed", mesgid);
    return -1;
  }
  if (char_index < 0) {
    int blank_index = findBlankCharIndex(id);
    if (blank_index < 0) {
      logErr("\n ACCharSave:char full.");
      SaacServer_ACCharSave_send(ti, FAILED, "char full", mesgid);
      return ret;
    } else {
      char_index = blank_index;
    }
  }

  logErr("账号:[%s] 人物:[%s]\n", id, charname);
  if (saveCharOne(id, char_index, savebuf) < 0) {
    logErr("\n ACCharSave:disk I/O error or a bug  ");
    SaacServer_ACCharSave_send(ti, FAILED, "disk I/O error or a bug", mesgid);
    return ret;
  }

  if (unlock) {
    char result[100];
    char retdata[100];
#ifdef _LOCK_ADD_NAME
    if ((ret = lockUser(gs[ti].name, id, "", "0", 0, result, sizeof(result),
                        retdata, sizeof(retdata), "0", "0")) < 0) {
#else
    if ((ret = lockUser(gs[ti].name, id, "0", 0, result, sizeof(result),
                        retdata, sizeof(retdata), "0", "0")) < 0) {
#endif
      logErr("解锁:%s 失败!!\n", id);
    }
  }

  SaacServer_ACCharSave_send(ti, SUCCESSFUL, "", mesgid);
  return ret;
}

void charListCallback(int ti, int auth, char *c0, char *c1, char *c2, char *c3,
                      char *c4, int i0, int i1, int charlistflg) {
  static char listbuf[CHARDATASIZE];
  char *id = c0;
  int mesgid = i0;
  if (auth != 0) {
    char data[100];
    snprintf(data, sizeof(data), "%d", auth);
    SaacServer_ACCharList_send(ti, FAILED, "你的账号暂时无法登陆！", mesgid);
    total_ng_charlist++;
    return;
  }
  // 取消下列 unlock 动作
  if (isLocked(id)) {
    SaacServer_ACCharList_send(
        ti, FAILED,
        "你的账号处理锁定状态，请重新登陆，如果三次都不能"
        "进入，请联系管理员为您解锁！",
        mesgid);
    SaacServer_ACKick_recv(ti, id, 1, -1);
    checkGSUCheck(id);
    total_ng_charlist++;
    return;
  }
  if (loadCharNameAndOption(id, listbuf, sizeof(listbuf)) < 0) {
    SaacServer_ACCharList_send(ti, FAILED, "database read failed", mesgid);
    return;
  }
  // Arminius
  SaacServer_ACCharList_send(ti, SUCCESSFUL, listbuf, mesgid);
  total_ok_charlist++;
}

void charDeleteCallback(int ti, int auth, char *c0, char *c1, char *c2,
                        char *c3, char *c4, int i0, int i1) {
  int char_index;
  char *id = c0;
  char *charname = c2;
  int mesgid = i0;
  struct tm *now;
  time_t timenow;
  char logfile[128];

  if (auth != 0) {
    char data[100];
    snprintf(data, sizeof(data), "%d", auth);
    SaacServer_ACCharDelete_send(ti, FAILED, data, mesgid);
    return;
  }

  char_index = getCharIndexByName(id, charname);
  if (char_index < 0) {
    SaacServer_ACCharDelete_send(ti, FAILED, "nochar", mesgid);
    return;
  }

  if (unlinkCharFile(id, char_index) < 0) {
    SaacServer_ACCharDelete_send(ti, FAILED, "fileI/O", mesgid);
    return;
  }
#ifdef _FAMILY
  // CoolFish: Family 2001/6/12
  {
    int index = 0, fmindex = 0, fmchar_index = 0;
    char fmname[32];
    index = ACgetFMInfoFromChar(fmname, &fmindex, charname, id, &fmchar_index);
    if (index != -1) {
      if (fmchar_index == 0) {
#ifdef _FMVER21
        if (ChangeFMLeader(index, fmname, fmindex) >= 0) {
          if (ACDelFM(index, fmname, fmindex) >= 0)
            logErr("删除家族成员:%d 家族名:%s 家族索引:%d 人物:%s, 账号:%s, "
                   "家族人物索引:%d\n",
                   index, fmname, fmindex, charname, id, fmchar_index);
        }
        logErr("ChangeFMLeader_3 index:%d fmname:%s fmindex:%d\n", index,
               fmname, fmindex);
#else
        if (ACDelFM(index, fmname, fmindex) >= 0)
          logErr("删除家族成员:%d 家族名:%s 家族索引:%d 人物:%s, 账号:%s, "
                 "家族人物索引:%d\n",
                 index, fmname, fmindex, charname, id, fmchar_index);
#endif
      } else {
#ifdef _FMVER21
        if (ACMemberLeaveFM(index, fmname, fmindex, charname, 0, 0,
                            fmchar_index) >= 0)
          logErr("ACMemberLeaveFM index:%d 家族名:%s 家族索引:%d 人物:%s, "
                 "账号:%s, 家族人物索引:%d\n",
                 index, fmname, fmindex, charname, id, fmchar_index);
#else
        if (ACMemberLeaveFM(index, fmname, fmindex, charname, 0,
                            fmchar_index) >= 0)
          logErr("ACMemberLeaveFM index:%d 家族名:%s 家族索引:%d 人物:%s, "
                 "账号:%s, 家族人物索引:%d\n",
                 index, fmname, fmindex, charname, id, fmchar_index);
#endif
      }
    }
  }
#endif
  // Spock 2000/12/6
  time(&timenow);
  now = localtime(&timenow);
  sprintf(logfile, "log/chardel/%04d%02d%02d.log", now->tm_year + 1900,
          now->tm_mon + 1, now->tm_mday);
  logFile(logfile,
          "%04d/%02d/%02d %02d:%02d:%02d id:[%s] char:[%s] index:[%d]\n",
          now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour,
          now->tm_min, now->tm_sec, id, charname, char_index);
  // Spock end
  USERLOG(id, "账号删除: 成功\n");
  SaacServer_ACCharDelete_send(ti, SUCCESSFUL, "ok", mesgid);
}

void dummyCallback(int ti, int auth, char *c0, char *c1, char *c2, char *c3,
                   char *c4, int i0, int i1) {
  return;
}

static void getCharNameFromString(const char *input, char *output) {
  int i;
  output[0] = '\0';
  for (i = 0;; ++i) {
    output[i] = input[i];
    if (input[i] == '\0')
      break;
    if (input[i] == SPACE) {
      output[i] = '\0';
      break;
    }
  }
}

static void getCharOptionFromString(const char *input, char *output) {
  int ri, wi = 0;
  output[0] = '\0';
  for (ri = 0;; ++ri) {
    if (input[ri] == '\0')
      break;
    if (input[ri] == SPACE) {
      ++ri;
      for (;; ++ri) {
        output[wi] = input[ri];
        if (input[ri] == '\0')
          break;
        if (input[ri] == SPACE) {
          output[wi] = '\0';
          break;
        }
        wi++;
      }
      return;
    }
  }
}

void getCharInfoFromString(char *input, char *output) {
  int ri = 0, wi = 0;
  int space_counter = 0;

  output[wi] = '\0';
  for (ri = 0;; ++ri) {
    if (input[ri] == '\0')
      break;
    if (input[ri] == SPACE) {
      space_counter++;
    }
    if (space_counter == 2 && input[ri] == SPACE) {
      ++ri;
      for (;; ++ri) {
        output[wi] = input[ri];
        if (input[ri] == '\0')
          break;
        if (input[ri] == SPACE) {
          output[wi] = '\0';
          break;
        }
        ++wi;
      }
      return;
    }
  }
}



// 2026.08.24 在output里填入dir_path


#ifdef _SLEEP_CHAR // 取得非活跃人物档名

#endif

int loadCharNameAndOption(char *id, char *output, int outputlen) {
  int i, count = 0;
  int so_far_bytes = 0;
  output[0] = 0;

  for (i = 0; i < MAXCHAR_PER_USER; i++) {
    int l;
    char delim[2];
    char_buf[i][0] = 0;
    if (loadCharOne(id, i, char_buf[i], sizeof(char_buf[i])) == -2) return -1;
    getCharNameFromString(char_buf[i], name_work);
    getCharOptionFromString(char_buf[i], option_work);
    if (char_buf[i][0] == 0)
      continue;
    if (strlen(output) > 0) {
      delim[0] = SPACE;
      delim[1] = 0;
    } else {
      delim[0] = 0;
    }
    snprintf(nm_work, sizeof(nm_work), "%s%s|%s", delim, name_work,
             option_work);
    l = strlen(nm_work);
    if ((so_far_bytes + l) < outputlen) {
      strcat(output, nm_work);
      so_far_bytes += l;
      count++;
    }
  }
  return count;
}

int loadCharOne(char *id, int num, char *output, int outputlen) {
  int size;
  if (num < 0 || num >= MAXCHAR_PER_USER || outputlen <= 0) return -2;
  size = sa_store_blob_get("character_save", id, num, output, (size_t)outputlen);
  if (size < 0) return size;
  if (!size || (output[0] == '|' && output[1] == '|')) { output[0] = 0; return -2; }
  return 0;
}

int saveCharOne(char *id, int num, char *input) {
  if (!input || !input[0] || num < 0 || num >= MAXCHAR_PER_USER) return -1;
  if (sa_db_begin(sa_store_db())) return -1;
  if (sa_store_blob_put("character_save", id, num, input, strlen(input)) < 0 || sa_db_commit(sa_store_db())) {
    sa_db_rollback(sa_store_db()); return -1;
  }
  return 0;
}

static int makeSaveCharString(char *output, int outputlen, char *nm, char *opt,
                              char *info) {
  static char nmwork[CHARDATASIZE];
  static char optwork[CHARDATASIZE];
  static char infowork[CHARDATASIZE];
  char *nmwork_p, *optwork_p, *infowork_p;

  if (outputlen <= 0)
    return -1;

  nmwork_p = makeEscapeString(nm, nmwork, sizeof(nmwork));
  optwork_p = makeEscapeString(opt, optwork, sizeof(optwork));
  infowork_p = makeEscapeString(info, infowork, sizeof(infowork));

  const int written = snprintf(output, outputlen,
                               "%s" SPACESTRING "%s" SPACESTRING "%s",
                               nmwork_p, optwork_p, infowork_p);
  if (written < 0 || written >= outputlen) {
    output[0] = '\0';
    return -1;
  }
  return 0;
}

char output[CHARDATASIZE];
char cn[CHARDATASIZE];

int getCharIndexByName(char *id, char *charname) {
  int i;
  for (i = 0; i < MAXCHAR_PER_USER; i++) {
    // 如果没有读取到角色，就跳过
    int rc = loadCharOne(id, i, output, sizeof(output));
    if (rc == -2) return -2;
    if (rc < 0) {
      continue;
    } else {
      // 如果读取到角色信息，比对角色数据中的角色名称和传入的charname
      getCharNameFromString(output, cn);
      if (strcmp(charname, makeStringFromEscaped(cn)) == 0) {
        return i;
      }
    }
  }
  return -1; // 没有找到这个角色
}

static int findBlankCharIndex(char *id) {
  int i;
  static char output[CHARDATASIZE];
  for (i = 0; i < MAXCHAR_PER_USER; i++) {
    int rc = loadCharOne(id, i, output, sizeof(output));
    if (rc == -2) return -2;
    if (rc == -1) {
      return i;
    }
  }
  return -1;
}

static int unlinkCharFile(char *id, int num) {
  return sa_store_character_delete(id, num);
}

#ifdef _LOCK_ADD_NAME
int lockUser(char *gmsvname, char *id, char *name, char *passwd, int lock,
             char *result, int resultlen, char *retdata, int retdatalen,
             char *process, char *deadline)
#else
int lockUser(char *gmsvname, char *id, char *passwd, int lock, char *result,
             int resultlen, char *retdata, int retdatalen, char *process,
             char *deadline)
#endif
{
  int ret = -1;

  if (!id[0]) {
    snprintf(result, resultlen, FAILED);
    snprintf(retdata, retdatalen, "bad id");
    return -1;
  }
  retdata[0] = 0;
  if (lock) {
    if (isLocked(id)) {
      snprintf(result, resultlen, FAILED);
      snprintf(retdata, retdatalen, "already locked");
      logErr("写入内存信息: 用户:%x/%s 已经同意锁定 !!\n", getHash(id), id);
      return -1;
    } else {
#ifdef _LOCK_ADD_NAME
      if (InsertMemLock(getHash(id) & 0xff, id, name, passwd, gmsvname,
                        atoi(process), deadline))
#else
      if (InsertMemLock(getHash(id) & 0xff, id, passwd, gmsvname, atoi(process),
                        deadline))
#endif
        return 0;
      else
        return -1;
    }
  } else {
    if (!isLocked(id)) {
      logErr("删除内存信息: 用户:%x/%s 没有锁定!!\n", getHash(id), id);
    }
    if (DeleteMemLock(getHash(id) & 0xff, id, &ret)) {
      snprintf(result, resultlen, SUCCESSFUL);
      snprintf(retdata, retdatalen, "removed");
      return ret;
    } else {
      snprintf(result, resultlen, FAILED);
      snprintf(retdata, retdatalen, "不能移除锁定");

      logErr("不能解锁 %x:%s !\n", getHash(id), id);
      return ret;
    }
  }
}

int isLocked(char *id) {
  if (!id[0])
    return 1; // invalid id: lock it
  return isMemLocked(getHash(id) & 0xff, id);
}

int checkValidCharName(char *a) {
  int i;
  for (i = 0;; i++) {
    if (a[i] == 0)
      break;
    if (a[i] == SPACE)
      return -1;
  }
  return 0;
}

#ifdef _CHAR_POOLITEM


#ifdef _SLEEP_CHAR // 取得非活跃人物仓库档名

#endif

int InsertCharPoolItem(char *id, char *input, int sizes) {
  char *data; int length, rc = -1;
  if (sizes <= 0) return sizes;
  if (!input || strlen(input) >= CHARDATASIZE) return -1;
  data = malloc(CHARDATASIZE);
  if (!data) return -1;
  if (sa_db_begin(sa_store_db())) { free(data); return -1; }
  length = sa_store_blob_get("account_storage", id, 0, data, CHARDATASIZE);
  if (length == -1) length = 0;
  if (length >= 0 && (size_t)length + strlen(input) < CHARDATASIZE) {
    strcpy(data + length, input);
    if (sa_store_blob_put("account_storage", id, 0, data, strlen(data)) == 0 && !sa_db_commit(sa_store_db())) rc = sizes;
  }
  if (rc < 0) sa_db_rollback(sa_store_db());
  free(data); return rc;
}

int saveCharPoolItem(char *id, char *input, int sizes) {
  if (sizes < 0 || !input) return -1;
  if (sa_db_begin(sa_store_db())) return -1;
  if (sa_store_blob_put("account_storage", id, 0, input, strlen(input)) < 0 || sa_db_commit(sa_store_db())) {
    sa_db_rollback(sa_store_db()); return -1;
  }
  return sizes;
}

int loadCharPoolItemOne(char *id, char *output, int outputlen) {
  int size;
  if (outputlen <= 0) return -1;
  size = sa_store_blob_get("account_storage", id, 0, output, (size_t)outputlen);
  return size == -1 ? 0 : (size < 0 ? -1 : size);
}

int checkCharPoolItem(char *id) {
  /* A missing warehouse is empty; the first save creates it. */
  return id && id[0] ? 1 : -1;
}
#endif

#ifdef _CHAR_POOLPET


#ifdef _SLEEP_CHAR // 取得非活跃人物仓库档名

#endif

int InsertCharPoolPet(char *id, char *input, int sizes) {
  char *data; int length, rc = -1;
  if (sizes <= 0) return sizes;
  if (!input || strlen(input) >= CHARDATASIZE) return -1;
  data = malloc(CHARDATASIZE);
  if (!data) return -1;
  if (sa_db_begin(sa_store_db())) { free(data); return -1; }
  length = sa_store_blob_get("account_storage", id, 1, data, CHARDATASIZE);
  if (length == -1) length = 0;
  if (length >= 0 && (size_t)length + strlen(input) < CHARDATASIZE) {
    strcpy(data + length, input);
    if (sa_store_blob_put("account_storage", id, 1, data, strlen(data)) == 0 && !sa_db_commit(sa_store_db())) rc = sizes;
  }
  if (rc < 0) sa_db_rollback(sa_store_db());
  free(data); return rc;
}

int saveCharPoolPet(char *id, char *input, int sizes) {
  if (sizes < 0 || !input) return -1;
  if (sa_db_begin(sa_store_db())) return -1;
  if (sa_store_blob_put("account_storage", id, 1, input, strlen(input)) < 0 || sa_db_commit(sa_store_db())) {
    sa_db_rollback(sa_store_db()); return -1;
  }
  return sizes;
}

int loadCharPoolPetOne(char *id, char *output, int outputlen) {
  int size;
  if (outputlen <= 0) return -1;
  size = sa_store_blob_get("account_storage", id, 1, output, (size_t)outputlen);
  return size == -1 ? 0 : (size < 0 ? -1 : size);
}

int checkCharPoolPet(char *id) {
  /* A missing warehouse is empty; the first save creates it. */
  return id && id[0] ? 1 : -1;
}
#endif

int filecopy(char *oldfilename, char *newfilename) {
  FILE *oldfile;
  FILE *newfile;
  char c_temp;

  if ((oldfile = fopen(oldfilename, "r")) == NULL) {
    return -1;
  }
  if ((newfile = fopen(newfilename, "w")) == NULL) {
    return -1;
  }

  while (1) {
    c_temp = fgetc(oldfile);
    if (c_temp == EOF)
      break;
    fputc(c_temp, newfile);
  };

  fclose(oldfile);
  fclose(newfile);

  return 0;
}
