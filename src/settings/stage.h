#ifndef STAGE_H
#define STAGE_H

struct stage_implementation;

struct stage
{
  int ink1;
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
void stage_free(struct stage *s);

/* get/set functions */
guint8 *stage_get_bytearray(const struct stage *s, const gchar *key);
void stage_set_bytearray(struct stage *s, const gchar *key, const guint8 *data);

gboolean stage_get_bool(const struct stage *s, const gchar *key);
void stage_set_bool(struct stage *s, const gchar *key, gboolean bval);

int stage_get_int(const struct stage *s, const gchar *key);
void stage_set_int(struct stage *s, const gchar *key, int ival);

#endif // STAGE_H
