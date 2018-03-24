#ifndef STAGE_H
#define STAGE_H

#include <gconf/gconf-client.h>

struct stage_implementation;

struct stage
{
  int unk1;
  gchar *name;
  gchar *dir;
  GConfClient *gconf;
  const struct stage_implementation *impl;
  GSList *entries;
  int unk2;
};

typedef void (*stage_copy_fn)(const struct stage *src, struct stage *dest);
typedef void (*stage_rename_fn)(struct stage *s, const gchar *key);
typedef GConfValue *(*stage_get_fn)(const struct stage *s, const gchar *key);
typedef void (*stage_set_fn)(struct stage *s, const gchar *key, GConfValue *val);
typedef void (*stage_free_fn)(struct stage *s);

void stage_create_cache(struct stage *cache, const struct stage *s);
void stage_create_for_path(struct stage *s, const gchar *path);
void stage_create_for_iap(struct stage *s, const gchar *iap);
void stage_copy(const struct stage *src, struct stage *dest);
void stage_free(struct stage *s);

/* get/set functions */
gchar *stage_get_bytearray(const struct stage *s, const gchar *key);
void stage_set_bytearray(struct stage *s, const gchar *key, const gchar *data);

gboolean stage_get_bool(const struct stage *s, const gchar *key);
void stage_set_bool(struct stage *s, const gchar *key, gboolean bval);

int stage_get_int(const struct stage *s, const gchar *key);
void stage_set_int(struct stage *s, const gchar *key, int ival);

gchar *stage_get_string(const struct stage *s, const gchar *key);
void stage_set_string(struct stage *s, const gchar *key, const gchar *sval);

gchar **stage_get_stringlist(const struct stage *s, const gchar *key);
void stage_set_stringlist(struct stage *s, const gchar *key, const gchar **lval);

GConfValue *stage_get_val(const struct stage *s, const gchar *key);
void stage_set_val(struct stage *s, const gchar *key, GConfValue *val);

#endif // STAGE_H
