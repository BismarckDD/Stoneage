#ifndef SA_STORE_H
#define SA_STORE_H
#include "sa_db.h"
#include <stddef.h>

typedef struct { const char *name; size_t offset; size_t size; } SA_STORE_FIELD;
typedef struct {
  const char *table, *key1, *key2;
  const SA_STORE_FIELD *fields;
  int count;
} SA_STORE_LAYOUT;
#define STORE_INT(type, member, column) {column, offsetof(type, member), 0}
#define STORE_TEXT(type, member, column) {column, offsetof(type, member), sizeof(((type *)0)->member)}
#define STORE_LAYOUT(table, k1, k2, fields) {table, k1, k2, fields, sizeof(fields)/sizeof(fields[0])}
/* The connection is owned by SASQL. Storage calls never consume its pending results. */
void sa_store_attach(SA_DB *db);
int sa_store_initialize(void);
SA_DB *sa_store_db(void);
SA_DB_PARAM sa_store_text(const char *value);
int sa_store_get(const SA_STORE_LAYOUT *layout, const char *key, int slot, void *record);
int sa_store_put(const SA_STORE_LAYOUT *layout, const char *key, int slot, const void *record);
int sa_store_delete(const SA_STORE_LAYOUT *layout, const char *key, int slot);
/* Caller holds a write transaction for put/delete and multi-record operations. */
int sa_store_blob_get(const char *table, const char *account, int slot, char *out, size_t capacity);
int sa_store_blob_put(const char *table, const char *account, int slot, const char *data, size_t size);
int sa_store_character_delete(const char *account, int slot);
#endif
