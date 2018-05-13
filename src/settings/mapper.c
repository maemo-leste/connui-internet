#include <gconf/gconf-client.h>
#include <connui/iap-network.h>
#include <connui/wlan-common.h>
#include <hildon/hildon.h>

#include <string.h>

#include "widgets.h"

#define MAPPER_STOCK_IMPL

#include "mapper.h"

struct int2combo_foreach_data
{
  int ival;
  gint col;
  GtkWidget *widget;
};

struct string2combo_foreach_data
{
  gchar *str;
  gint col;
  GtkWidget *widget;
};

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
          g_string_insert_c(string, -1, *(gchar *)sw->priv);
        else
        {
          string->str[string->len++] = *(gchar *)sw->priv;
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
  gchar **arr = g_strsplit_set(iap_widgets_h22_entry_get_text(entry),
                               (gchar *)sw->priv, 0);

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

static gint
get_active(const GtkWidget *widget)
{
  if (GTK_IS_COMBO_BOX(widget))
    return gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  else if (HILDON_IS_PICKER_BUTTON(widget))
    return hildon_picker_button_get_active(HILDON_PICKER_BUTTON(widget));

  g_warning("No combobox nor picker button");

  return 0;
}

static gboolean
is_selected(const GtkWidget *widget, GtkTreeIter *iter)
{
  if (GTK_IS_COMBO_BOX(widget))
    return gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), iter);
  else if (HILDON_IS_PICKER_BUTTON(widget))
  {
    return hildon_touch_selector_get_selected(
          hildon_picker_button_get_selector(HILDON_PICKER_BUTTON(widget)), 0,
          iter);
  }

  g_warning("No combobox nor picker button");

  return FALSE;
}

static GtkTreeModel *
get_model(const GtkWidget *widget)
{
  if (GTK_IS_COMBO_BOX(widget))
    return gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
  else if (HILDON_IS_PICKER_BUTTON(widget))
  {
    return hildon_touch_selector_get_model(
          hildon_picker_button_get_selector(HILDON_PICKER_BUTTON(widget)), 0);
  }

  g_warning("No combobox nor picker button");

  return NULL;
}

static void
set_active_iter(GtkWidget *widget, GtkTreeIter *iter)
{
  if (GTK_IS_COMBO_BOX(widget))
  {
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(widget), iter);
  }
  else if (HILDON_IS_PICKER_BUTTON(widget))
  {
    hildon_touch_selector_select_iter(
          hildon_picker_button_get_selector(HILDON_PICKER_BUTTON(widget)), 0,
          iter, 0);
  }
  else
    g_warning("No combobox nor picker button");
}

static void
set_active(GtkWidget *widget, gint index)
{
  if (GTK_IS_COMBO_BOX(widget))
    gtk_combo_box_set_active(GTK_COMBO_BOX(widget), index);
  else if (HILDON_IS_PICKER_BUTTON(widget))
    hildon_picker_button_set_active(HILDON_PICKER_BUTTON(widget), index);
  else
    g_warning("No combobox nor picker button");
}

static gboolean
int2combo_foreach_fn(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter,
                     gpointer userdata)
{
  struct int2combo_foreach_data *data =
      (struct int2combo_foreach_data *)userdata;
  int ival;

  gtk_tree_model_get(model, iter, data->col, &ival, -1);

  if (data->ival != ival)
    return FALSE;

  set_active_iter(data->widget, iter);

  return TRUE;
}

static void
int2combo(const struct stage *s, GtkWidget *entry,
          const struct stage_widget *sw)
{
  gint ival = stage_get_int(s, sw->key);
  gint *priv = (gint *)sw->priv;

  if (GPOINTER_TO_UINT(priv) <= 16)
  {
    if (priv)
    {
      struct int2combo_foreach_data data;

      data.ival = ival;
      data.col = GPOINTER_TO_UINT(priv);
      data.widget = entry;
      set_active(entry, 0);
      gtk_tree_model_foreach(get_model(entry), int2combo_foreach_fn, &data);
    }
    else
    {
      if (ival > gtk_tree_model_iter_n_children(get_model(entry), 0))
        ival = 0;

      set_active(entry, ival);
    }
  }
  else
  {
    int i;

    for (i = 0; priv[i] != -1; i++)
    {
      if (ival == priv[i])
        break;
    }

    if (priv[i] == -1)
      i = 0;

    set_active(entry, i);
  }
}

static void
combo2int(struct stage *s, const GtkWidget *entry,
          const struct stage_widget *sw)
{
  const gint *priv = sw->priv;
  int ival = -1;

  if (GPOINTER_TO_UINT(priv) > 16)
    ival = priv[get_active(entry)];
  else
  {
    if (priv)
    {
      GtkTreeIter iter;

      if (is_selected(entry, &iter))
        gtk_tree_model_get(get_model(entry), &iter, sw->priv, &ival, -1);
    }
    else
      ival = get_active(entry);
  }

  if (ival >= 0)
    stage_set_int(s, sw->key, ival);
  else
    stage_set_val(s, sw->key, NULL);
}

static gboolean
string2combo_foreach_fn(GtkTreeModel *model, GtkTreePath *path,
                        GtkTreeIter *iter, gpointer userdata)
{
  struct string2combo_foreach_data *data =
      (struct string2combo_foreach_data *)userdata;
  gboolean rv;
  gchar *str;

  gtk_tree_model_get(model, iter, data->col, &str, -1);

  if (str && !strcmp(data->str, str))
  {
    set_active_iter(data->widget, iter);
    rv = TRUE;
  }
  else
    rv = FALSE;

  g_free(str);

  return rv;
}

static void
string2combo(const struct stage *s, GtkWidget *entry,
             const struct stage_widget *sw)
{
  gchar *str = stage_get_string(s, sw->key);
  const gchar **strlist = (const gchar **)sw->priv;

  if (GPOINTER_TO_UINT(sw->priv) < 16)
  {
    if (str)
    {
      struct string2combo_foreach_data data;

      data.str = str;
      data.col = GPOINTER_TO_UINT(sw->priv);
      data.widget = entry;

      gtk_tree_model_foreach(get_model(entry), string2combo_foreach_fn, &data);
    }
    else
      set_active(entry, 0);
  }
  else
  {
    int i = 0;

    if (str)
    {
      while (strlist[i] && strcmp(strlist[i], str))
        i++;
    }

    if (!strlist[i])
      i = 0;

    set_active(entry, i);
  }

  g_free(str);
}

static void
combo2string(struct stage *s, const GtkWidget *entry,
             const struct stage_widget *sw)
{
  const gchar **strlist = (const gchar **)sw->priv;
  gchar *str = NULL;
  GtkTreeIter iter;

  if (GPOINTER_TO_UINT(sw->priv) > 15)
  {
    gint active = get_active(entry);

    if (active >= 0)
      str = g_strdup(strlist[active]);
  }
  else if (is_selected(entry, &iter))
    gtk_tree_model_get(get_model(entry), &iter, sw->priv, &str, -1);

  if (str)
    stage_set_string(s, sw->key, str);
  else
    stage_set_val(s, sw->key, NULL);

  g_free(str);
}

static gboolean strlist_equal(gchar **sa1, gchar **sa2)
{
  int i = 0;

  if (g_strv_length(sa1) != g_strv_length(sa2))
    return FALSE;

  while (sa1[i])
  {
    if (strcmp(sa1[i], sa2[i]))
      return FALSE;

    i++;
  }

  return TRUE;
}

static void
stringlist2combo(const struct stage *s, GtkWidget *entry,
                 const struct stage_widget *sw)
{
  gchar ***priv = (gchar ***)sw->priv;
  gchar **strlist = stage_get_stringlist(s, sw->key);;
  gint active = 0;

  if (!strlist)
  {
    strlist = g_new0(gchar *, 1);
    *strlist = NULL;
  }

  while (priv[active])
  {
    if (strlist_equal(priv[active], strlist))
      break;
  }

  g_strfreev(strlist);

  set_active(entry, active);
}

static void
combo2stringlist(struct stage *s, const GtkWidget *entry,
                 const struct stage_widget *sw)
{
  const gchar ***priv = (const gchar ***)sw->priv;
  gint active = get_active(entry);
  const gchar **strlist = NULL;

  if (active >= 0)
    strlist = priv[active];

  if (strlist)
    stage_set_stringlist(s, sw->key, strlist);
  else
    stage_set_val(s, sw->key, NULL);
}

static void
stringlistfuzzy2combo(const struct stage *s, GtkWidget *entry,
                      const struct stage_widget *sw)
{
  gchar ***priv = (gchar ***)sw->priv;
  gchar **strlist = stage_get_stringlist(s, sw->key);
  gint active = 0;
  int max = 0;
  int i = 0;

  if (!strlist)
  {
    strlist = g_new0(gchar *, 1);
    *strlist = NULL;
  }

  if (!*strlist)
  {
    set_active(entry, 0);
    g_strfreev(strlist);
    return;
  }

  while (priv[i])
  {
    if (strlist_equal(priv[i], strlist))
    {
      set_active(entry, i);
      g_strfreev(strlist);
      return;
    }

    i++;
  }

  i = 0;

  while (priv[i])
  {
    gchar **p = priv[i];
    int match = 0;
    int j = 0;

    while (p[j])
    {
      int k = 0;

      while (strlist[k])
      {
        if (!strcmp(p[j], strlist[k]))
          match++;

        k++;
      }
    }

    if (match > max)
    {
      active = i;
      max = match;
    }

    i++;
  }

  set_active(entry, active);

  g_strfreev(strlist);
}

static void
combo2stringlistfuzzy(struct stage *s, const GtkWidget *entry,
                      const struct stage_widget *sw)
{
  combo2stringlist(s, entry, sw);
}

void
mapper_import_widgets(struct stage *s, struct stage_widget *sw,
                      mapper_get_widget_fn get_widget, gpointer user_data)
{
  const gchar *id;

  while ((id = sw->name))
  {
    GtkWidget *widget = get_widget(user_data, id);

    if (widget)
    {
      if (!sw->validate || sw->validate(s, sw->name, sw->key))
      {
        if (sw->import)
          sw->import(user_data, s, sw);

        sw->mapper->stage2widget(s, widget, sw);
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
    const GtkWidget *widget = get_widget(user_data, id);

    if (widget)
    {
      if (sw->validate && !sw->validate(s, sw->name, sw->key))
      {
        if ((!sw->export || sw->export(s, sw->name, sw->key)) &&
            !g_hash_table_lookup(hash, sw->key))
        {
          stage_set_val(s, sw->key, NULL);
        }
      }
      else
      {
        sw->mapper->widget2stage(s, widget, sw);
        g_hash_table_insert(hash, (gpointer)sw->key, GUINT_TO_POINTER(TRUE));
      }
    }

    sw++;
  }

  g_hash_table_destroy(hash);
}
