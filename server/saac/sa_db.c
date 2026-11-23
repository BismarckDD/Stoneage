#include "sa_db.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sqlite3.h"

#include <mysql/mysql.h>

struct SA_DB_RESULT {
  int backend;
  size_t rows;
  size_t cursor;
  int columns;
  char **sqlite_table;
  MYSQL_RES *mysql_result;
};

struct SA_DB {
  int backend;
  int transaction_depth;
  char error[512];
  char sqlite_path[512];
  sqlite3 *sqlite;
  SA_DB_RESULT *pending;
  MYSQL mysql;
};

static int configured_backend = SA_DB_SQLITE;
static char configured_sqlite_path[512] = "saac.sqlite3";

static void sqlite_now(sqlite3_context *context, int argc,
                       sqlite3_value **argv) {
  time_t current;
  struct tm *parts;
  char value[32];
  (void)argc;
  (void)argv;
  current = time(NULL);
  parts = localtime(&current);
  if (parts == NULL || strftime(value, sizeof(value), "%Y-%m-%d %H:%M:%S", parts) == 0)
    sqlite3_result_null(context);
  else
    sqlite3_result_text(context, value, -1, SQLITE_TRANSIENT);
}

static void sqlite_to_days(sqlite3_context *context, int argc,
                           sqlite3_value **argv) {
  const unsigned char *value;
  int year, month, day;
  static const int before_month[] =
      {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  long days;
  if (argc != 1 || sqlite3_value_type(argv[0]) == SQLITE_NULL) {
    sqlite3_result_null(context);
    return;
  }
  value = sqlite3_value_text(argv[0]);
  if (sscanf((const char *)value, "%d-%d-%d", &year, &month, &day) != 3 ||
      month < 1 || month > 12) {
    sqlite3_result_null(context);
    return;
  }
  days = 365L * year + year / 4 - year / 100 + year / 400 +
         before_month[month] + day;
  if (month <= 2 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
    days--;
  sqlite3_result_int64(context, days);
}

void sa_db_configure(int backend, const char *sqlite_path) {
  configured_backend = backend;
  if (sqlite_path != NULL && sqlite_path[0] != '\0') {
    snprintf(configured_sqlite_path, sizeof(configured_sqlite_path), "%s",
             sqlite_path);
  }
}

static void free_pending(SA_DB *db) {
  if (db != NULL && db->pending != NULL) {
    sa_db_free_result(db->pending);
    db->pending = NULL;
  }
}

SA_DB *sa_db_init(SA_DB *storage) {
  SA_DB *db = storage;
  if (db == NULL) db = (SA_DB *)calloc(1, sizeof(*db));
  if (db == NULL) return NULL;
  memset(db, 0, sizeof(*db));
  db->backend = configured_backend;
  snprintf(db->sqlite_path, sizeof(db->sqlite_path), "%s",
           configured_sqlite_path);
  if (db->backend == SA_DB_MYSQL && mysql_init(&db->mysql) == NULL) return NULL;
  return db;
}

SA_DB *sa_db_connect(SA_DB *db, const char *host, const char *user,
                     const char *password, const char *database,
                     unsigned int port) {
  if (db == NULL) return NULL;
  if (db->backend == SA_DB_SQLITE) {
    int rc = sqlite3_open_v2(db->sqlite_path, &db->sqlite,
                            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
                                SQLITE_OPEN_FULLMUTEX,
                            NULL);
    if (rc != SQLITE_OK) {
      snprintf(db->error, sizeof(db->error), "%s",
               db->sqlite ? sqlite3_errmsg(db->sqlite) : "sqlite open failed");
      return NULL;
    }
    sqlite3_busy_timeout(db->sqlite, 5000);
    sqlite3_exec(db->sqlite, "PRAGMA journal_mode=WAL", NULL, NULL, NULL);
    sqlite3_exec(db->sqlite, "PRAGMA foreign_keys=ON", NULL, NULL, NULL);
    sqlite3_create_function(db->sqlite, "TO_DAYS", 1,
                            SQLITE_UTF8 | SQLITE_DETERMINISTIC, NULL,
                            sqlite_to_days, NULL, NULL);
    sqlite3_create_function(db->sqlite, "NOW", 0, SQLITE_UTF8, NULL,
                            sqlite_now, NULL, NULL);
    return db;
  }
  if (mysql_real_connect(&db->mysql, host, user, password, database, port, NULL,
                         0) == NULL) {
    snprintf(db->error, sizeof(db->error), "%s", mysql_error(&db->mysql));
    return NULL;
  }
  return db;
}

static char *sqlite_sql(const char *sql) {
  const char *needle = "BINARY";
  size_t n = strlen(sql);
  char *out = (char *)malloc(n + 1);
  char *w = out;
  const char *p = sql;
  if (out == NULL) return NULL;
  while (*p) {
    if (strncmp(p, needle, 6) == 0) {
      p += 6;
      continue;
    }
    *w++ = *p++;
  }
  *w = '\0';
  return out;
}

int sa_db_query(SA_DB *db, const char *sql) {
  if (db == NULL) return 1;
  free_pending(db);
  if (db->backend == SA_DB_SQLITE) {
    char *translated = sqlite_sql(sql);
    char *message = NULL;
    char **table = NULL;
    int rows = 0, columns = 0;
    int rc;
    if (translated == NULL) return 1;
    rc = sqlite3_get_table(db->sqlite, translated, &table, &rows, &columns,
                           &message);
    free(translated);
    if (rc != SQLITE_OK) {
      snprintf(db->error, sizeof(db->error), "%s",
               message ? message : sqlite3_errmsg(db->sqlite));
      sqlite3_free(message);
      if (table) sqlite3_free_table(table);
      return 1;
    }
    db->pending = (SA_DB_RESULT *)calloc(1, sizeof(*db->pending));
    if (db->pending == NULL) {
      sqlite3_free_table(table);
      snprintf(db->error, sizeof(db->error), "out of memory");
      return 1;
    }
    db->pending->backend = SA_DB_SQLITE;
    db->pending->rows = (size_t)rows;
    db->pending->columns = columns;
    db->pending->sqlite_table = table;
    return 0;
  }
  if (mysql_query(&db->mysql, sql) != 0) {
    snprintf(db->error, sizeof(db->error), "%s", mysql_error(&db->mysql));
    return 1;
  }
  db->pending = (SA_DB_RESULT *)calloc(1, sizeof(*db->pending));
  if (db->pending == NULL) return 1;
  db->pending->backend = SA_DB_MYSQL;
  db->pending->mysql_result = mysql_store_result(&db->mysql);
  db->pending->rows = db->pending->mysql_result
                          ? (size_t)mysql_num_rows(db->pending->mysql_result)
                          : 0;
  return 0;
}

SA_DB_RESULT *sa_db_store_result(SA_DB *db) {
  SA_DB_RESULT *result;
  if (db == NULL) return NULL;
  result = db->pending;
  db->pending = NULL;
  return result;
}

size_t sa_db_num_rows(SA_DB_RESULT *result) {
  return result ? result->rows : 0;
}

SA_DB_ROW sa_db_fetch_row(SA_DB_RESULT *result) {
  if (result == NULL || result->cursor >= result->rows) return NULL;
  if (result->backend == SA_DB_SQLITE) {
    char **row = result->sqlite_table + result->columns * (result->cursor + 1);
    result->cursor++;
    return row;
  }
  result->cursor++;
  return mysql_fetch_row(result->mysql_result);
}

void sa_db_free_result(SA_DB_RESULT *result) {
  if (result == NULL) return;
  if (result->backend == SA_DB_SQLITE) {
    if (result->sqlite_table) sqlite3_free_table(result->sqlite_table);
  }
  else if (result->mysql_result) mysql_free_result(result->mysql_result);
  free(result);
}

const char *sa_db_error(SA_DB *db) { return db ? db->error : "no database"; }
int sa_db_backend(SA_DB *db) { return db ? db->backend : -1; }

int sa_db_backup(SA_DB *db, const char *path) {
  sqlite3 *destination = NULL;
  sqlite3_backup *backup;
  int rc;
  if (!db || db->backend != SA_DB_SQLITE || db->transaction_depth || !path || strcmp(path, db->sqlite_path) == 0) return 1;
  rc = sqlite3_open(path, &destination);
  if (rc != SQLITE_OK) {
    snprintf(db->error, sizeof(db->error), "Cannot open backup: %s", destination ? sqlite3_errmsg(destination) : "out of memory");
    sqlite3_close(destination); return 1;
  }
  backup = sqlite3_backup_init(destination, "main", db->sqlite, "main");
  if (!backup) {
    snprintf(db->error, sizeof(db->error), "Cannot initialize backup: %s", sqlite3_errmsg(destination));
    sqlite3_close(destination); return 1;
  }
  rc = sqlite3_backup_step(backup, -1);
  if (sqlite3_backup_finish(backup) != SQLITE_OK) rc = SQLITE_ERROR;
  if (rc != SQLITE_DONE) snprintf(db->error, sizeof(db->error), "Database backup failed: %s", sqlite3_errmsg(destination));
  sqlite3_close(destination);
  return rc == SQLITE_DONE ? 0 : 1;
}

void sa_db_close(SA_DB *db) {
  if (db == NULL) return;
  free_pending(db);
  if (db->backend == SA_DB_SQLITE) {
    if (db->sqlite) sqlite3_close(db->sqlite);
    db->sqlite = NULL;
    free(db);
    return;
  }
  mysql_close(&db->mysql);
  free(db);
}

int sa_db_run(SA_DB *db, const char *sql, const SA_DB_PARAM *params, int count,
              SA_DB_VISITOR visitor, void *context) {
  int i, status = 1;
  if (!db || count < 0 || count > 128) return 1;
  if (db->backend == SA_DB_SQLITE) {
    sqlite3_stmt *stmt = NULL;
    int rc, columns;
    const char *values[128];
    size_t sizes[128];
    rc = sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) goto sqlite_done;
    if (sqlite3_bind_parameter_count(stmt) != count) goto sqlite_done;
    for (i = 0; i < count; ++i) {
      if (params[i].size > 2147483647U) goto sqlite_done;
      rc = params[i].blob
          ? sqlite3_bind_blob(stmt, i + 1, params[i].data, (int)params[i].size, SQLITE_TRANSIENT)
          : sqlite3_bind_text(stmt, i + 1, params[i].data, (int)params[i].size, SQLITE_TRANSIENT);
      if (rc != SQLITE_OK) goto sqlite_done;
    }
    columns = sqlite3_column_count(stmt);
    if (columns > 128) goto sqlite_done;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
      for (i = 0; i < columns; ++i) {
        values[i] = (const char *)sqlite3_column_blob(stmt, i);
        sizes[i] = (size_t)sqlite3_column_bytes(stmt, i);
      }
      if (visitor && visitor(context, columns, values, sizes)) goto sqlite_done;
    }
    status = rc == SQLITE_DONE ? 0 : 1;
  sqlite_done:
    if (status) snprintf(db->error, sizeof(db->error), "SQL execution or row validation failed: %s", sqlite3_errmsg(db->sqlite));
    sqlite3_finalize(stmt);
    return status;
  } else {
    MYSQL_STMT *stmt = mysql_stmt_init(&db->mysql);
    MYSQL_BIND bindings[128], output[128];
    unsigned long lengths[128], sizes_out[128];
    const char *values[128] = {0};
    size_t sizes[128];
    unsigned int columns = 0;
    int rc;
    if (!stmt) return 1;
    memset(bindings, 0, sizeof(bindings));
    memset(output, 0, sizeof(output));
    if (mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql))) goto mysql_done;
    if (mysql_stmt_param_count(stmt) != (unsigned long)count) goto mysql_done;
    for (i = 0; i < count; ++i) {
      lengths[i] = (unsigned long)params[i].size;
      bindings[i].buffer_type = params[i].blob ? MYSQL_TYPE_BLOB : MYSQL_TYPE_STRING;
      bindings[i].buffer = (void *)params[i].data;
      bindings[i].buffer_length = lengths[i];
      bindings[i].length = &lengths[i];
    }
    if (count && mysql_stmt_bind_param(stmt, bindings)) goto mysql_done;
    if (mysql_stmt_execute(stmt)) goto mysql_done;
    columns = mysql_stmt_field_count(stmt);
    if (columns > 128) goto mysql_done;
    if (!columns) { status = 0; goto mysql_done; }
    for (i = 0; i < (int)columns; ++i) {
      output[i].buffer_type = MYSQL_TYPE_BLOB;
      output[i].length = &sizes_out[i];
    }
    if (mysql_stmt_bind_result(stmt, output) || mysql_stmt_store_result(stmt)) goto mysql_done;
    while ((rc = mysql_stmt_fetch(stmt)) == 0 || rc == MYSQL_DATA_TRUNCATED) {
      for (i = 0; i < (int)columns; ++i) {
        MYSQL_BIND field;
        char *data = malloc((size_t)sizes_out[i] + 1);
        if (!data) goto mysql_done;
        values[i] = data;
        memset(&field, 0, sizeof(field));
        field.buffer_type = MYSQL_TYPE_BLOB;
        field.buffer = data;
        field.buffer_length = sizes_out[i];
        if (mysql_stmt_fetch_column(stmt, &field, (unsigned int)i, 0)) goto mysql_done;
        data[sizes_out[i]] = 0;
        sizes[i] = sizes_out[i];
      }
      if (visitor && visitor(context, (int)columns, values, sizes)) goto mysql_done;
      for (i = 0; i < (int)columns; ++i) { free((void *)values[i]); values[i] = NULL; }
    }
    status = rc == MYSQL_NO_DATA ? 0 : 1;
  mysql_done:
    if (status) snprintf(db->error, sizeof(db->error), "SQL execution or row validation failed: %s", mysql_stmt_error(stmt));
    for (i = 0; i < 128; ++i) free((void *)values[i]);
    mysql_stmt_close(stmt);
    return status;
  }
}

int sa_db_begin(SA_DB *db) {
  char sql[64];
  if (!db) return 1;
  if (db->transaction_depth) snprintf(sql, sizeof(sql), "SAVEPOINT sa_tx_%d", db->transaction_depth);
  else snprintf(sql, sizeof(sql), "%s", db->backend == SA_DB_SQLITE ? "BEGIN IMMEDIATE" : "START TRANSACTION");
  if (sa_db_run(db, sql, NULL, 0, NULL, NULL)) return 1;
  ++db->transaction_depth;
  return 0;
}
int sa_db_commit(SA_DB *db) {
  char sql[64];
  if (!db || !db->transaction_depth) return 1;
  if (db->transaction_depth > 1) snprintf(sql, sizeof(sql), "RELEASE SAVEPOINT sa_tx_%d", db->transaction_depth - 1);
  else strcpy(sql, "COMMIT");
  if (sa_db_run(db, sql, NULL, 0, NULL, NULL)) return 1;
  --db->transaction_depth;
  return 0;
}
void sa_db_rollback(SA_DB *db) {
  char sql[64];
  if (!db || !db->transaction_depth) return;
  if (db->transaction_depth > 1) {
    snprintf(sql, sizeof(sql), "ROLLBACK TO SAVEPOINT sa_tx_%d", db->transaction_depth - 1);
    sa_db_run(db, sql, NULL, 0, NULL, NULL);
    snprintf(sql, sizeof(sql), "RELEASE SAVEPOINT sa_tx_%d", db->transaction_depth - 1);
    sa_db_run(db, sql, NULL, 0, NULL, NULL);
  } else sa_db_run(db, "ROLLBACK", NULL, 0, NULL, NULL);
  --db->transaction_depth;
}
