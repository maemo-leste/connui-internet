#include <gconf/gconf-client.h>
#include <connui/iap-network.h>
#include <connui/wlan-common.h>
#include <hildon/hildon.h>

#include <string.h>

#include "widgets.h"

#define MAPPER_STOCK_IMPL

#include "mapper.h"

static void
entry2string(struct stage *s, const GtkWidget *entry,
             const struct stage_widget *sw)
{
  stage_set_string(s, sw->key, iap_widgets_h22_entry_get_text(entry));
}

static void
string2entry(const struct stage *s, GtkWidget *entry,
             const struct stage_widget *sw)
{
  gchar *str = stage_get_string(s, sw->key);

  if (str)
  {
    iap_widgets_h22_entry_set_text(entry, str);
    g_free(str);
  }
  else
    iap_widgets_h22_entry_set_text(entry, "");
}

static void
entry2bytearray(struct stage *s, const GtkWidget *entry,
                const struct stage_widget *sw)
{
  if (GTK_WIDGET_SENSITIVE(entry) && GTK_WIDGET_PARENT_SENSITIVE(entry))
    stage_set_bytearray(s, sw->key, iap_widgets_h22_entry_get_text(entry));
}

static void
bytearray2entry(const struct stage *s, GtkWidget *entry,
                const struct stage_widget *sw)
{
  gchar *str = stage_get_bytearray(s, sw->key);

  if (str)
  {
    gtk_widget_set_sensitive(entry, wlan_common_mangle_ssid(str, strlen(str)));
    iap_widgets_h22_entry_set_text(entry, str);
    g_free(str);
  }
  else
    iap_widgets_h22_entry_set_text(entry, "");
}

static void
numbereditor2int(struct stage *s, const GtkWidget *entry,
                 const struct stage_widget *sw)
{
  stage_set_int(s, sw->key,
                hildon_number_editor_get_value(HILDON_NUMBER_EDITOR(entry)));
}

static void
int2numbereditor(const struct stage *s, GtkWidget *entry,
                 const struct stage_widget *sw)
{
  hildon_number_editor_set_value(HILDON_NUMBER_EDITOR(entry),
                                 stage_get_int(s, sw->key));
}

static void
toggle2int(struct stage *s, const GtkWidget *entry,
           const struct stage_widget *sw)
{
  stage_set_int(s, sw->key,
                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(entry)));
}

static void
int2toggle(const struct stage *s, GtkWidget *entry,
           const struct stage_widget *sw)
{
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(entry),
                               stage_get_int(s, sw->key));
}

static void
toggle2bool(struct stage *s, const GtkWidget *entry,
            const struct stage_widget *sw)
{
  if (GTK_IS_TOGGLE_BUTTON(entry))
  {
    stage_set_bool(s, sw->key,
                   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(entry)));
  }
  else if (HILDON_IS_CHECK_BUTTON(entry))
  {
    stage_set_bool(s, sw->key,
                   hildon_check_button_get_active(HILDON_CHECK_BUTTON(entry)));
  }
}

static void
bool2toggle(const struct stage *s, GtkWidget *entry,
            const struct stage_widget *sw)
{
  gboolean bval = stage_get_bool(s, sw->key);

  if (GTK_IS_TOGGLE_BUTTON(entry))
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(entry), bval);
  else if (HILDON_IS_CHECK_BUTTON(entry))
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(entry), bval);
}

static void
stringlist2entry(const struct stage *s, GtkWidget *entry,
                 const struct stage_widget *sw)
{

  GString *string = string = g_string_new(NULL);
  GConfValue *val = stage_get_val(s, sw->key);
  gchar *str;

  if (val)
  {
    if (val->type == GCONF_VALUE_LIST &&
        gconf_value_get_list_type(val) == GCONF_VALUE_STRING)
    {
      GSList *l;

      for (l = gconf_value_get_list(val); l; l = l->next)
      {
        if (string->len + 1 >= string->allocated_len)
          g_string_insert_c(string, -1, *sw->sep);
        else
        {
          string->str[string->len++] = *sw->sep;
          string->str[string->len] = 0;
        }

        g_string_append(string,
                        gconf_value_get_string((const GConfValue *)l->data));
      }
    }

    gconf_value_free(val);
  }

  str = g_string_free(string, FALSE);

  if (str && strlen(str) > 1)
    iap_widgets_h22_entry_set_text(entry, str + 1);
  else
    iap_widgets_h22_entry_set_text(entry, "");

  g_free(str);
}

static void
entry2stringlist(struct stage *s, const GtkWidget *entry,
                 const struct stage_widget *sw)
{
  GSList *l = NULL;
  GConfValue *val;
  gchar **arr =
      g_strsplit_set(iap_widgets_h22_entry_get_text(entry), sw->sep, 0);

  while (*arr)
  {
    val = gconf_value_new(GCONF_VALUE_STRING);

    gconf_value_set_string(val, *arr);
    l = g_slist_prepend(l, val);
    arr++;
  }

  g_strfreev(arr);

  val = gconf_value_new(GCONF_VALUE_LIST);
  gconf_value_set_list_type(val, GCONF_VALUE_STRING);
  gconf_value_set_list_nocopy(val, g_slist_reverse(l));

  stage_set_val(s, sw->key, val);
}

void
mapper_import_widgets(struct stage *s, struct stage_widget *sw,
                      mapper_get_widget_fn get_widget, gpointer user_data)
{
  const gchar *id;

  while ((id = sw->name))
  {
    const GtkWidget *widget = get_widget(user_data, id);

    if (widget)
    {
      if (!sw->needs_sync || sw->needs_sync(s, sw->name, sw->key))
      {
        if (sw->import)
          sw->import(user_data, s, sw);

        sw->mapper->widget2stage(s, widget, sw);
      }
    }

    sw++;
  }
}

void
mapper_export_widgets(struct stage *s, struct stage_widget *sw,
                      mapper_get_widget_fn get_widget, gpointer user_data)
{
  GHashTable *hash = g_hash_table_new(g_str_hash, g_str_equal);
  const gchar *id;

  while ((id = sw->name))
  {
    GtkWidget *widget = get_widget(user_data, id);

    if (widget)
    {
      if (sw->needs_sync && !sw->needs_sync(s, sw->name, sw->key))
      {
        if ((!sw->export || sw->export(s, sw->name, sw->key)) &&
            !g_hash_table_lookup(hash, sw->key))
        {
          stage_set_val(s, sw->key, NULL);
        }
      }
      else
      {
        sw->mapper->stage2widget(s, widget, sw);
        g_hash_table_insert(hash, (gpointer)sw->key, GINT_TO_POINTER(TRUE));
      }
    }

    sw++;
  }

  g_hash_table_destroy(hash);
}
