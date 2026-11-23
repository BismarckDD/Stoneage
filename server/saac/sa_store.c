#include "sa_store.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include "sa_schema.h"

static SA_DB *connection;
void sa_store_attach(SA_DB *db) { connection = db; }
SA_DB *sa_store_db(void) { return connection; }
int sa_store_initialize(void) {
  if (sa_db_backend(connection) == SA_DB_SQLITE && sa_db_query(connection, SA_SQLITE_SCHEMA)) return -1;
  return sa_db_run(connection, "SELECT save_data FROM character_save WHERE 1=0", NULL, 0, NULL, NULL) ? -1 : 0;
}
SA_DB_PARAM sa_store_text(const char *value) {
  SA_DB_PARAM param = {value, value ? strlen(value) : 0, 0};
  return param;
}

typedef struct { const SA_STORE_LAYOUT *layout; void *record; int found; } Row;
static int read_row(void *ctx, int columns, const char **values, const size_t *sizes) {
  Row *row = ctx;
  int i;
  if (row->found || columns != row->layout->count) return 1;
  for (i = 0; i < columns; ++i) {
    const SA_STORE_FIELD *field = &row->layout->fields[i];
    char *dest = (char *)row->record + field->offset;
    if (!field->size) {
      char text[32], *end;
      long value;
      if (!sizes[i] || sizes[i] >= sizeof(text)) return 1;
      memcpy(text, values[i], sizes[i]); text[sizes[i]] = 0;
      errno = 0; value = strtol(text, &end, 10);
      if (errno || *end || value < INT_MIN || value > INT_MAX) return 1;
      *(int *)dest = (int)value;
    } else {
      if (sizes[i] >= field->size || (sizes[i] && memchr(values[i], 0, sizes[i]))) return 1;
      if (sizes[i]) memcpy(dest, values[i], sizes[i]);
      dest[sizes[i]] = 0;
    }
  }
  row->found = 1;
  return 0;
}

int sa_store_get(const SA_STORE_LAYOUT *layout, const char *key, int slot, void *record) {
  char sql[8192], number[32];
  SA_DB_PARAM params[2];
  Row row = {layout, record, 0};
  int i;
  snprintf(number, sizeof(number), "%d", slot);
  params[0] = sa_store_text(key); params[1] = sa_store_text(number);
  strcpy(sql, "SELECT ");
  for (i = 0; i < layout->count; ++i) {
    if (i) strcat(sql, ",");
    strcat(sql, layout->fields[i].name);
  }
  snprintf(sql + strlen(sql), sizeof(sql) - strlen(sql), " FROM %s WHERE %s=?", layout->table, layout->key1);
  if (layout->key2) snprintf(sql + strlen(sql), sizeof(sql)-strlen(sql), " AND %s=?", layout->key2);
  if (sa_db_run(connection, sql, params, layout->key2 ? 2 : 1, read_row, &row)) return -1;
  return row.found;
}

int sa_store_put(const SA_STORE_LAYOUT *layout, const char *key, int slot, const void *record) {
  char sql[8192], numbers[64][32], slot_text[32];
  SA_DB_PARAM params[68];
  int i, n = layout->count, keys = layout->key2 ? 2 : 1;
  if (n > 64) return -1;
  snprintf(slot_text, sizeof(slot_text), "%d", slot);
  snprintf(sql, sizeof(sql), "UPDATE %s SET ", layout->table);
  for (i = 0; i < n; ++i) {
    const SA_STORE_FIELD *field = &layout->fields[i];
    const char *value = (const char *)record + field->offset;
    if (!field->size) {
      snprintf(numbers[i], sizeof(numbers[i]), "%d", *(const int *)value);
      value = numbers[i];
    } else if (!memchr(value, 0, field->size)) return -1;
    params[i] = sa_store_text(value);
    snprintf(sql + strlen(sql), sizeof(sql) - strlen(sql), "%s%s=?", i ? "," : "", field->name);
  }
  params[n] = sa_store_text(key); params[n+1] = sa_store_text(slot_text);
  snprintf(sql + strlen(sql), sizeof(sql) - strlen(sql), " WHERE %s=?", layout->key1);
  if (layout->key2) snprintf(sql + strlen(sql), sizeof(sql)-strlen(sql), " AND %s=?", layout->key2);
  if (sa_db_run(connection, sql, params, n+keys, NULL, NULL)) return -1;
  snprintf(sql, sizeof(sql), "INSERT INTO %s (", layout->table);
  for (i = 0; i < n; ++i) {
    if (i) strcat(sql, ",");
    strcat(sql, layout->fields[i].name);
  }
  snprintf(sql + strlen(sql), sizeof(sql) - strlen(sql), ",%s", layout->key1);
  if (layout->key2) { strcat(sql, ","); strcat(sql, layout->key2); }
  strcat(sql, ") SELECT ");
  for (i = 0; i < n+keys; ++i) strcat(sql, i ? ",?" : "?");
  snprintf(sql + strlen(sql), sizeof(sql) - strlen(sql), " WHERE NOT EXISTS (SELECT 1 FROM %s WHERE %s=?", layout->table, layout->key1);
  if (layout->key2) snprintf(sql + strlen(sql), sizeof(sql)-strlen(sql), " AND %s=?", layout->key2);
  strcat(sql, ")");
  params[n+keys] = params[n];
  if (keys == 2) params[n+keys+1] = params[n+1];
  return sa_db_run(connection, sql, params, n+2*keys, NULL, NULL) ? -1 : 0;
}
int sa_store_delete(const SA_STORE_LAYOUT *layout, const char *key, int slot) {
  char sql[256], number[32];
  SA_DB_PARAM params[2];
  snprintf(number, sizeof(number), "%d", slot);
  params[0] = sa_store_text(key); params[1] = sa_store_text(number);
  snprintf(sql, sizeof(sql), "DELETE FROM %s WHERE %s=?", layout->table, layout->key1);
  if (layout->key2) snprintf(sql + strlen(sql), sizeof(sql)-strlen(sql), " AND %s=?", layout->key2);
  return sa_db_run(connection, sql, params, layout->key2 ? 2 : 1, NULL, NULL) ? -1 : 0;
}

typedef struct { char *data; size_t capacity; int size; } Blob;
static int read_blob(void *ctx, int columns, const char **values, const size_t *sizes) {
  Blob *blob = ctx;
  if (columns != 1 || blob->size >= 0 || sizes[0] >= blob->capacity || sizes[0] > INT_MAX) return 1;
  if (sizes[0]) memcpy(blob->data, values[0], sizes[0]);
  blob->data[sizes[0]] = 0;
  blob->size = (int)sizes[0];
  return 0;
}
static int blob_table(const char *table) {
  return strcmp(table, "character_save") == 0 || strcmp(table, "account_storage") == 0;
}
/* >=0 byte count; -1 absent; -2 failure (never treated as an empty slot). */
int sa_store_blob_get(const char *table, const char *account, int slot, char *out, size_t capacity) {
  char sql[256], number[32];
  SA_DB_PARAM params[2];
  Blob blob = {out, capacity, -1};
  if (!blob_table(table) || !capacity) return -2;
  out[0] = 0;
  snprintf(number, sizeof(number), "%d", slot);
  params[0] = sa_store_text(account); params[1] = sa_store_text(number);
  snprintf(sql, sizeof(sql), "SELECT save_data FROM %s WHERE account_name=? AND slot=?", table);
  if (sa_db_run(connection, sql, params, 2, read_blob, &blob)) { out[0] = 0; return -2; }
  return blob.size;
}
int sa_store_blob_put(const char *table, const char *account, int slot, const char *data, size_t size) {
  char sql[512], number[32];
  SA_DB_PARAM params[5];
  if (!blob_table(table) || !data || !account || !account[0] || slot < 0) return -1;
  snprintf(number, sizeof(number), "%d", slot);
  params[0].data = data; params[0].size = size; params[0].blob = 1;
  params[1] = sa_store_text(account); params[2] = sa_store_text(number);
  params[3] = params[1]; params[4] = params[2];
  snprintf(sql, sizeof(sql), "UPDATE %s SET save_data=?,revision=revision+1,updated_at=CURRENT_TIMESTAMP WHERE account_name=? AND slot=?", table);
  if (sa_db_run(connection, sql, params, 3, NULL, NULL)) return -1;
  snprintf(sql, sizeof(sql), "INSERT INTO %s(save_data,account_name,slot) SELECT ?,?,? WHERE NOT EXISTS(SELECT 1 FROM %s WHERE account_name=? AND slot=?)", table, table);
  return sa_db_run(connection, sql, params, 5, NULL, NULL) ? -1 : 0;
}
int sa_store_character_delete(const char *account, int slot) {
  char number[32];
  SA_DB_PARAM params[2];
  snprintf(number, sizeof(number), "%d", slot);
  params[0] = sa_store_text(account); params[1] = sa_store_text(number);
  /* Transaction ownership remains with the character/family operation. */
  if (sa_db_run(connection, "INSERT INTO character_archive(account_name,slot,save_data,revision) SELECT account_name,slot,save_data,revision FROM character_save WHERE account_name=? AND slot=?", params, 2, NULL, NULL)) return -1;
  if (sa_db_run(connection, "DELETE FROM character_info WHERE cdkey=? AND saveindex=?", params, 2, NULL, NULL)) return -1;
  return sa_db_run(connection, "DELETE FROM character_save WHERE account_name=? AND slot=?", params, 2, NULL, NULL) ? -1 : 0;
}
