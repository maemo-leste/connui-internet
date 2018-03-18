#include <gconf/gconf-client.h>
#include <icd/osso-ic-gconf.h>
#include <connui/connui-log.h>
#include </usr/include/osso-log.h>
#include <string.h>

#include "stage.h"

struct stage_implementation
{
  stage_copy_fn copy;
  stage_rename_fn rename;
  stage_get_fn get;
  stage_set_fn set;
  stage_free_fn free;
};

static void
stage_cache_copy(const struct stage *src, struct stage *dest)
{
  GSList *l;

  for (l = src->entries; l; l = l->next)
  {
    GConfEntry *entry = (GConfEntry *)l->data;
    const gchar *key = strrchr(entry->key, '/') + 1;

    if (entry->value)
      dest->impl->set(dest, key, gconf_value_copy(entry->value));
    else
      dest->impl->set(dest, key, NULL);
  }
}

static GConfValue *
stage_cache_get(const struct stage *s, const gchar *key)
{
  GSList *l;

  for (l = s->entries; l; l = l->next)
  {
    GConfEntry *entry = (GConfEntry *)l->data;

    if (!strcmp(strrchr(entry->key, '/') + 1, key))
    {
      if (entry->value)
        return gconf_value_copy(entry->value);

      break;
    }
  }

  return NULL;
}

static void
stage_cache_set(struct stage *s, const gchar *key, GConfValue *val)
{
  GSList *l;
  GConfEntry *entry;

  for (l = s->entries; l; l = l->next)
  {
    entry = l->data;

    if (!strcmp(strrchr(entry->key, '/') + 1, key))
    {
      gconf_entry_set_value_nocopy(entry, val);
      return;
    }
  }

  entry = gconf_entry_new_nocopy(g_strconcat(s->dir, key, NULL), val);
  s->entries = g_slist_prepend(s->entries, entry);
}

static struct stage_implementation stage_impl_cache = {
  stage_cache_copy,
  NULL,
  stage_cache_get,
  stage_cache_set,
  NULL
};

void
stage_create_cache(struct stage *cache, const struct stage *s)
{
  GError *error;

  memset(cache, 0, sizeof(*cache));

  if (s)
  {
    cache->name = g_strdup(s->name);
    cache->dir = g_strdup(s->dir);
    cache->gconf = (GConfClient *)g_object_ref(s->gconf);
    *strrchr(cache->dir, '/') = 0;
    cache->entries = gconf_client_all_entries(cache->gconf, cache->dir, &error);

    if (error)
    {
      CONNUI_ERR("GConf error: %s", error->message);
      g_clear_error(&error);
    }

    strcat(cache->dir, "/");
  }
  else
  {
    cache->name = g_strdup("");
    cache->dir = g_strdup("/");
  }

  cache->impl = &stage_impl_cache;
}

static void
stage_iap_path_copy(const struct stage *src, struct stage *dest)
{
  GSList *entries;
  GSList *l;
  GError *error = NULL;
  gchar *dir = g_strdup(src->dir);

  *strrchr(dir, '/') = 0;
  entries = gconf_client_all_entries(src->gconf, dir, &error);
  g_free(dir);

  if (error)
  {
    CONNUI_ERR("GConf error: %s", error->message);
    g_clear_error(&error);
  }

  for (l = entries; l; l = l->next)
  {
    GConfEntry *entry = l->data;
    GConfValue *val = entry->value;
    const gchar *key = strrchr(entry->key, '/') + 1;

    if (val)
      val = gconf_value_copy(entry->value);

    dest->impl->set(dest, key, val);
    gconf_entry_free(entry);
  }

  g_slist_free(entries);
}

static void
stage_iap_path_rename(struct stage *s, const gchar *new_name)
{
  gchar *escaped;
  GSList *entries;
  GSList *l;
  gchar *dir;
  GError *error = NULL;

  escaped = gconf_escape_key(s->name, -1);
  dir = g_strconcat(ICD_GCONF_PATH, "/", escaped, NULL);
  g_free(escaped);
  g_free(s->name);
  g_free(s->dir);
  s->name = g_strdup(new_name);
  escaped = gconf_escape_key(new_name, -1);
  s->dir = g_strconcat(ICD_GCONF_PATH, "/", escaped, "/", NULL);
  g_free(escaped);

  ULOG_INFO("%s(): Renaming GConf entry '%s' to '%s'", __FUNCTION__,
            dir, s->dir);

  entries = gconf_client_all_entries(s->gconf, dir, &error);

  if (error)
  {
    CONNUI_ERR("GConf error: %s", error->message);
    g_clear_error(&error);
  }

  for (l = entries; l; l = l->next)
  {
    GConfEntry *entry = l->data;
    gchar *key = g_strconcat(s->dir, strrchr(entry->key, '/') + 1, NULL);

    gconf_client_set(s->gconf, key, entry->value, &error);

    if (error)
    {
      CONNUI_ERR("GConf error: %s", error->message);
      g_clear_error(&error);
    }

    gconf_entry_free(entry);
    g_free(key);
  }

  g_slist_free(entries);

  gconf_client_recursive_unset(s->gconf, dir,
                               GCONF_UNSET_INCLUDING_SCHEMA_NAMES, &error);

  if (error)
  {
    CONNUI_ERR("GConf error: %s", error->message);
    g_clear_error(&error);
  }

  g_free(dir);
}

static GConfValue *
stage_iap_path_get(const struct stage *s, const gchar *key)
{
  GConfValue *rv;
  GError *error = NULL;
  gchar *gconf_key = g_strconcat(s->dir, key, NULL);;

  rv = gconf_client_get(s->gconf, gconf_key, &error);

  if (error)
  {
    CONNUI_ERR("GConf error: %s", error->message);
    g_clear_error(&error);
  }

  g_free(gconf_key);

  return rv;
}

static void
stage_iap_path_set(struct stage *s, const gchar *key, GConfValue *val)
{
  gchar *gconf_key = g_strconcat(s->dir, key, NULL);
  GError *error = NULL;

  if (val)
    gconf_client_set(s->gconf, gconf_key, val, &error);
  else
    gconf_client_unset(s->gconf, gconf_key, &error);

  if (error)
  {
    CONNUI_ERR("GConf error: %s", error->message);
    g_clear_error(&error);
  }

  g_free(gconf_key);

  if (val)
    gconf_value_free(val);
}

static void
stage_iap_path_free(struct stage *s)
{
  GError *error = NULL;

  gconf_client_suggest_sync(s->gconf, &error);

  if (error)
  {
    CONNUI_ERR("GConf error: %s", error->message);
    g_clear_error(&error);
  }
}

static struct stage_implementation stage_impl_iap_path =
{
  stage_iap_path_copy,
  stage_iap_path_rename,
  stage_iap_path_get,
  stage_iap_path_set,
  stage_iap_path_free
};

void
stage_create_for_path(struct stage *s, const gchar *path)
{
  memset(s, 0, sizeof(*s));

  s->name = g_strdup(path);
  s->dir = g_strconcat(path, "/", NULL);
  s->impl = &stage_impl_iap_path;
  s->gconf = gconf_client_get_default();
}

void
stage_create_for_iap(struct stage *s, const gchar *iap)
{
  gchar *escaped = gconf_escape_key(iap, -1);;

  memset(s, 0, sizeof(*s));

  s->name = g_strdup(iap);
  s->dir = g_strconcat(ICD_GCONF_PATH, "/", escaped, "/", NULL);
  s->gconf = gconf_client_get_default();
  s->impl = &stage_impl_iap_path;

  g_free(escaped);
}

void
stage_free(struct stage *s)
{
  GSList *l;

  if (s->impl && s->impl->free)
    s->impl->free(s);

  for (l = s->entries; l; l = l->next)
  {
    if (l->data)
      gconf_entry_free((GConfEntry *)l->data);
  }

  g_slist_free(s->entries);

  if (s->gconf)
    g_object_unref(s->gconf);

  g_free(s->name);
  g_free(s->dir);

  memset(s, 0, sizeof(*s));
}

GByteArray *
stage_dump_cache(struct stage *s, GByteArray *array)
{
  GSList *l;
  guint16 len;

  for (l = s->entries; l; l = l->next)
  {
    guint8 type;
    const char *key;
    GConfEntry *entry = l->data;

    if (!entry)
      continue;

    if (entry->value)
      type = entry->value->type;
    else
      type = GCONF_VALUE_INVALID;

    key = strrchr(entry->key, '/') + 1;
    len = strlen(key) + 1;
    g_byte_array_append(array, (const guint8 *)&len, sizeof(len));
    g_byte_array_append(array, (const guint8 *)key, len);
    g_byte_array_append(array, &type, sizeof(type));

    switch (type)
    {
      case GCONF_VALUE_STRING:
      {
        const char *s = gconf_value_get_string(entry->value);

        len = strlen(s) + 1;
        g_byte_array_append(array, (const guint8 *)&len, sizeof(len));
        g_byte_array_append(array, (const guint8 *)s, len);
        break;
      }
      case GCONF_VALUE_INT:
      {
        int i = gconf_value_get_int(entry->value);

        g_byte_array_append(array, (const guint8 *)&i, sizeof(i));
        break;
      }
      case GCONF_VALUE_BOOL:
      {
        gboolean b = gconf_value_get_bool(entry->value);

        g_byte_array_append(array, (const guint8 *)&b, sizeof(b));
        break;
      }
      case GCONF_VALUE_LIST:
      {
        GSList *list = gconf_value_get_list(entry->value);
        GSList *l1;

        type = gconf_value_get_list_type(entry->value);
        g_byte_array_append(array, &type, sizeof(type));

        len = g_slist_length(list);
        g_byte_array_append(array, (const guint8 *)&len, sizeof(len));

        if (type == GCONF_VALUE_STRING)
        {
          for (l1 = list; l1; l1 = l1->next)
          {
            const char *s =
                gconf_value_get_string((const GConfValue *)l1->data);

            len = strlen(s) + 1;
            g_byte_array_append(array, (const guint8 *)&len, sizeof(len));
            g_byte_array_append(array, (const guint8 *)s, len);
          }
        }
        else if (type == GCONF_VALUE_INT)
        {
          for (l1 = list; l1; l1 = l1->next)
          {
            guint8 i = gconf_value_get_int((const GConfValue *)list->data);

            g_byte_array_append(array, &i, sizeof(i));
          }
        }
        break;
      }
      default:
        break;
    }
  }

  len = 0;
  return g_byte_array_append(array, (const guint8 *)&len, sizeof(len));
}

/* get/set functions */

/* bytearray */
gchar *
stage_get_bytearray(const struct stage *s, const gchar *key)
{
  gchar *rv;
  GConfValue *val;

  if (!s || !s->impl || !(val = s->impl->get(s, key)))
    return NULL;

  if (val->type == GCONF_VALUE_STRING)
    return g_strdup(gconf_value_get_string(val));
  else if (val->type == GCONF_VALUE_LIST &&
           gconf_value_get_list_type(val) == GCONF_VALUE_INT)
  {
    GSList *l = gconf_value_get_list(val);
    int i;

    rv = g_new0(gchar, g_slist_length(l) + 1);

    for (i = 0; l; l = l->next)
      rv[i++] = gconf_value_get_int((const GConfValue *)l->data);
  }
  else
    rv = NULL;

  gconf_value_free(val);

  return rv;
}

void
stage_set_bytearray(struct stage *s, const gchar *key, const gchar *data)
{
  GSList *l = NULL;
  GConfValue *val;

  while (*data)
  {
    GConfValue *v = gconf_value_new(GCONF_VALUE_INT);

    gconf_value_set_int(v, *data++);
    l = g_slist_prepend(l, v);
  }

  val = gconf_value_new(GCONF_VALUE_LIST);
  gconf_value_set_list_type(val, GCONF_VALUE_INT);
  gconf_value_set_list_nocopy(val, g_slist_reverse(l));
  s->impl->set(s, key, val);
}

/* bool */
gboolean
stage_get_bool(const struct stage *s, const gchar *key)
{
  GConfValue *val;
  gboolean rv;

  if (!s || !s->impl || !(val = s->impl->get(s, key)))
    return FALSE;

  if (val->type == GCONF_VALUE_BOOL)
    rv = gconf_value_get_bool(val);
  else
    rv = FALSE;

  gconf_value_free(val);
  return rv;
}

void
stage_set_bool(struct stage *s, const gchar *key, gboolean bval)
{
  GConfValue *val = gconf_value_new(GCONF_VALUE_BOOL);

  gconf_value_set_bool(val, bval);
  s->impl->set(s, key, val);
}

/* int */
int
stage_get_int(const struct stage *s, const gchar *key)
{
  GConfValue *val;
  int rv;

  if (!s || !s->impl || !(val = s->impl->get(s, key)))
    return 0;

  if (val->type == GCONF_VALUE_INT)
    rv = gconf_value_get_int(val);
  else
    rv = 0;

  gconf_value_free(val);

  return rv;
}

void
stage_set_int(struct stage *s, const gchar *key, int ival)
{
  GConfValue *val = gconf_value_new(GCONF_VALUE_INT);

  gconf_value_set_int(val, ival);
  s->impl->set(s, key, val);
}

/* string */
gchar *
stage_get_string(const struct stage *s, const gchar *key)
{
  GConfValue *val;
  gchar *rv;

  if (!s || !s->impl || !(val = s->impl->get(s, key)))
    return NULL;

  if (val->type == GCONF_VALUE_STRING)
    rv = g_strdup(gconf_value_get_string(val));
  else
    rv = NULL;

  gconf_value_free(val);

  return rv;
}

void
stage_set_string(struct stage *s, const gchar *key, const gchar *sval)
{
  GConfValue *val = gconf_value_new(GCONF_VALUE_STRING);

  gconf_value_set_string(val, sval);
  s->impl->set(s, key, val);
}

/* stringlist */
gchar **
stage_get_stringlist(const struct stage *s, const gchar *key)
{
  gchar **rv;
  GConfValue *val;

  if (!s || !s->impl || !(val = s->impl->get(s, key)))
    return NULL;

  if (val->type == GCONF_VALUE_LIST &&
      gconf_value_get_list_type(val) == GCONF_VALUE_STRING)
  {
    GSList *l = gconf_value_get_list(val);
    int i = 0;

    rv = g_new0(gchar *, g_slist_length(l) + 1);

    while (l)
      rv[i++] = g_strdup(gconf_value_get_string((const GConfValue *)l->data));

    rv[i] = NULL;
  }
  else
    rv = NULL;

  gconf_value_free(val);

  return rv;
}

void
stage_set_stringlist(struct stage *s, const gchar *key, const gchar **lval)
{
  GSList *l = NULL;
  GConfValue *val;

  while(*lval)
  {
    val = gconf_value_new(GCONF_VALUE_STRING);

    gconf_value_set_string(val, *lval++);
    l = g_slist_prepend(l, val);
  }

  l = g_slist_reverse(l);
  val = gconf_value_new(GCONF_VALUE_LIST);
  gconf_value_set_list_type(val, GCONF_VALUE_STRING);
  gconf_value_set_list(val, l);
  s->impl->set(s, key, val);
  g_slist_free(l);
}

GConfValue *
stage_get_val(const struct stage *s, const gchar *key)
{
  GConfValue *val;

  if (!s && !s->impl && (val = s->impl->get(s, key)))
    return gconf_value_copy(val);

  return NULL;
}

void
stage_set_val(struct stage *s, const gchar *key, GConfValue *val)
{
  s->impl->set(s, key, val);
}
