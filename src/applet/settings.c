#include <hildon/hildon.h>
#include <libosso.h>
#include <osso-ic-gconf.h>
#include <connui/connui.h>

#include <string.h>
#include <stdlib.h>

#include "mapper.h"
#include "widgets.h"

#include "settings.h"
#include "connections.h"

MAPPER(pickerbutton, int);

struct type_data
{
  gboolean not_empty;
  gboolean dun_found;
  gboolean other_found;
};

enum
{
  IAP_SETTINGS_SAVE = 0,
  IAP_SETTINGS_CONNECTIONS = 1,
  IAP_SETTINGS_CLOSE = 2
};

static int search_intervals[] = {0, 300, 600, 1800, 3600, -1};

static struct stage_widget inet_settings_widgets[] =
{
  {
    NULL,
    NULL,
    "AUTO_CONNECT",
    "network_type/auto_connect",
    NULL,
    &mapper_combo2stringlistfuzzy,
    NULL
  },
  {
    NULL,
    NULL,
    "SEARCH_INTERVAL",
    "network_type/search_interval",
    NULL,
    &mapper_pickerbutton2int,
    search_intervals
  },
  {
    NULL,
    NULL,
    "AUTO_SWITCH",
    "network_type/change_while_connected",
    NULL,
    &mapper_toggle2bool,
    NULL
  },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

static void
int2pickerbutton(const struct stage *s, GtkWidget *entry,
                 const struct stage_widget *sw)
{
  int *priv = sw->priv;
  int val = stage_get_int(s, sw->key);
  int i = 0;

  while (priv[i] >= 0)
  {
    if (val == priv[i])
    {
      hildon_picker_button_set_active(HILDON_PICKER_BUTTON(entry), !!i);
      break;
    }

    i++;
  }
}

static void
pickerbutton2int(struct stage *s, const GtkWidget *entry,
                 const struct stage_widget *sw)
{
  int *priv = sw->priv;
  gint active = hildon_picker_button_get_active(HILDON_PICKER_BUTTON(entry));

  stage_set_int(s, sw->key, priv[active]);
}


static GtkWidget *
inet_settings_get_widget(struct internet_settings *settings, const gchar *id)
{
  if (settings->widgets)
    return (GtkWidget *)g_hash_table_lookup(settings->widgets, id);

  return NULL;
}

static void
update_active_buttons(struct internet_settings *settings)
{
  GtkWidget *auto_connect =
      GTK_WIDGET(g_hash_table_lookup(settings->widgets, "AUTO_CONNECT"));
  GtkWidget *search_int =
      GTK_WIDGET(g_hash_table_lookup(settings->widgets, "SEARCH_INTERVAL"));
  GtkWidget *auto_switch =
      GTK_WIDGET(g_hash_table_lookup(settings->widgets, "AUTO_SWITCH"));
  gint active =
      hildon_picker_button_get_active(HILDON_PICKER_BUTTON(auto_connect));

  if (hildon_picker_button_get_active(HILDON_PICKER_BUTTON(search_int)) > 0)
  {
    gchar **type = settings->network_types[active];

    if (type && *type && (!strcmp(*type, "*") || !strcmp(*type, "WLAN_INFRA")))
    {
      gtk_widget_set_sensitive(auto_switch, TRUE);
      gtk_widget_set_sensitive(search_int , !!active);
      return;
    }
  }

  gtk_widget_set_sensitive(auto_switch, FALSE);
  gtk_widget_set_sensitive(search_int , !!active);
}

static void
search_interval_value_changed_cb(HildonPickerButton *widget,
                                 struct internet_settings *settings)
{
  if (!settings->importing)
    update_active_buttons(settings);
}

void
inet_settings_import(struct internet_settings *settings)
{
  struct stage s;

  inet_settings_widgets[0].priv = settings->network_types;
  settings->importing = TRUE;
  stage_create_for_path(&s, ICD_GCONF_SETTINGS);
  mapper_import_widgets(&s, inet_settings_widgets,
                        (mapper_get_widget_fn)inet_settings_get_widget,
                        settings);
  stage_free(&s);
  settings->importing = FALSE;
  update_active_buttons(settings);
}

void
inet_settings_export(struct internet_settings *settings)
{
  struct stage s;

  stage_create_for_path(&s, ICD_GCONF_SETTINGS);
  mapper_export_widgets(&s, inet_settings_widgets,
                        (mapper_get_widget_fn)inet_settings_get_widget,
                        settings);
  stage_free(&s);
}

void
inet_settings_save_state(struct internet_settings *settings, GByteArray *array)
{
  GtkNotebook *notebook;
  struct stage s;
  guint8 page;

  stage_create_cache(&s, NULL);
  mapper_export_widgets(&s, inet_settings_widgets,
                        (mapper_get_widget_fn)inet_settings_get_widget,
                        settings);
  stage_dump_cache(&s, array);
  stage_free(&s);
  notebook = settings->notebook;

  if (notebook)
  {
    page = gtk_notebook_get_current_page(notebook);
    g_byte_array_append(array, &page, sizeof(page));
    page = settings->conns != 0;
    g_byte_array_append(array, &page, sizeof(page));

    if (page)
      iap_connections_save_state(settings->conns, array);
  }
}

void
inet_settings_destroy(struct internet_settings *settings)
{
  int i = 0;

  if (settings->labels[i])
  {
    g_free(settings->labels[i]);
    g_free(settings->not_found_banners[i]);
    g_strfreev(settings->network_types[i]);
    i++;
  }

  free(settings->labels[i]);
  free(settings->network_types);
  free(settings->not_found_banners);
  g_hash_table_destroy(settings->widgets);
  gtk_widget_destroy(settings->dialog);
  g_free(settings);
}

static void
connections_dialog_response_cb(GtkDialog *dialog, gint response_id,
                               gpointer user_data)
{
  struct internet_settings *settings = user_data;

  if (response_id == IAP_CONNECTIONS_DONE)
  {
    iap_connections_destroy(settings->conns);
    settings->conns = 0;
    update_active_buttons(settings);
  }
}

static void
inet_settings_restore_connections(struct internet_settings *settings,
                                  struct stage_cache *cache)
{
  struct iap_connections *conns =
      iap_connections_create(settings->osso, GTK_WINDOW(settings->dialog));

  settings->conns = conns;
  g_signal_connect(G_OBJECT(conns->dialog), "response",
                   G_CALLBACK(connections_dialog_response_cb), settings);

  if (cache)
    iap_connections_restore_state(settings->conns, cache);

  iap_connections_show(conns);
}

gboolean
inet_settings_restore_state(struct internet_settings *settings,
                            struct stage_cache *cache)
{
  guint8 conns_cached;
  guint8 page;
  struct stage s;

  stage_create_cache(&s, NULL);

  if (!stage_restore_cache(&s, cache))
  {
    stage_free(&s);
    return FALSE;
  }

  mapper_import_widgets(&s, inet_settings_widgets,
                        (mapper_get_widget_fn)inet_settings_get_widget,
                        settings);
  stage_free(&s);

  if (cache->processed + sizeof(page) > cache->len)
    return FALSE;

  page = cache->data[cache->processed];
  cache->processed += sizeof(page);

  if (cache->processed + sizeof(conns_cached) > cache->len)
    return FALSE;

  conns_cached = cache->data[cache->processed];
  cache->processed += sizeof(conns_cached);

  gtk_widget_show_all(settings->dialog);
  gtk_notebook_set_current_page(settings->notebook, page);

  if (conns_cached)
    inet_settings_restore_connections(settings, cache);

  return TRUE;
}

/* WTF is with this function?!? */
static gboolean
get_types_cb(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter,
             gpointer user_data)
{
  struct type_data *data = user_data;
  gchar *type;

  data->not_empty = TRUE;
  gtk_tree_model_get(model, iter, 0, &type, -1);

  if (type && !strncmp(type, "DUN_", 4))
  {
    data->dun_found = TRUE;
    g_free(type);

    if (!data->not_empty)
      return FALSE;
  }
  else
  {
    data->other_found = TRUE;
    g_free(type);

    if (!data->not_empty)
      return FALSE;
  }

  if (!data->dun_found)
    return FALSE;

  return data->other_found;
}

static void
auto_connect_value_changed_cb(HildonPickerButton *button,
                              struct internet_settings *settings)
{
  gint active;

  if (settings->importing)
    return;

  update_active_buttons(settings);
  active = hildon_picker_button_get_active(button);

  if (active > 0 && settings->not_found_banners[active] &&
      settings->network_types[active])
  {
    struct type_data data = {FALSE, FALSE, FALSE};
    GtkListStore *list_store = gtk_list_store_new(1, G_TYPE_STRING);

    iap_settings_get_iap_list(list_store,
                              (const gchar **)settings->network_types[active],
                              0, -1, -1, -1, -1, -1);
    gtk_tree_model_foreach(GTK_TREE_MODEL(list_store), get_types_cb, &data);
    g_object_unref(G_OBJECT(list_store));

    if (data.not_empty)
    {
      if ( data.dun_found && !data.other_found )
      {
        GConfClient *gconf = gconf_client_get_default();
        gchar *preferred = gconf_client_get_string(
              gconf, ICD_GCONF_SETTINGS "/BT/preferred", NULL);

        g_object_unref(gconf);

        if (!preferred)
        {
          hildon_banner_show_information(GTK_WIDGET(settings->dialog), NULL,
                                         _("conn_ib_net_no_paired_phone"));
        }

        g_free(preferred);
      }
    }
    else
    {
      hildon_banner_show_information(GTK_WIDGET(settings->dialog), NULL,
                                     settings->not_found_banners[active]);
    }
  }
}

static void
dialog_response_cb(GtkDialog *dialog, gint response_id, gpointer user_data)
{
  struct internet_settings *settings = user_data;

  if (response_id == IAP_SETTINGS_CONNECTIONS)
    inet_settings_restore_connections(settings, NULL);
  else
  {
    if(response_id == IAP_SETTINGS_SAVE)
      inet_settings_export(settings);

    gtk_main_quit();
  }
}

static int
compare_strings(const gchar *a, const gchar *b)
{
  if (!a && !b)
    return 0;

  if (!a)
    return 1;
  if (!b)
    return -1;

  return strcmp(a, b);
}
static void
inet_settings_get_gconf_data(struct internet_settings *settings)
{
  GConfClient *gconf = gconf_client_get_default();
  GSList *dirs = gconf_client_all_dirs(gconf,
                                       ICD_GCONF_SETTINGS "/ui/auto_connect",
                                       NULL);
  int len = g_slist_length(dirs) + 1;

  settings->labels = g_new0(gchar *, len);
  settings->network_types = g_new0(gchar **, len);
  settings->not_found_banners = g_new0(gchar *, len);

  if (dirs)
  {
    GSList *l;
    int idx = 0;
    gboolean network_exists = FALSE;

    dirs = g_slist_sort(dirs, (GCompareFunc)compare_strings);

    for (l = dirs; l; l = l->next)
    {
      GSList *l1;
      gchar *not_found_banner;
      gchar *gettext_catalog;
      gchar *label;
      gchar *key;
      GSList *network_types;
      int i;

      key = g_strconcat((const gchar *)l->data, "/gettext_catalog", NULL);
      gettext_catalog = gconf_client_get_string(gconf, key, NULL);
      g_free(key);

      key = g_strconcat((const gchar *)l->data, "/label", NULL);
      label = gconf_client_get_string(gconf, key, NULL);
      g_free(key);

      key = g_strconcat((const gchar *)l->data, "/not_found_banner", NULL);
      not_found_banner = gconf_client_get_string(gconf, key, NULL);
      g_free(key);

      key = g_strconcat((const gchar *)l->data, "/network_types", NULL);
      network_types = gconf_client_get_list(gconf, key, GCONF_VALUE_STRING,
                                            NULL);
      g_free(key);

      if (!label)
      {
        idx--;
        goto next;
      }

      if (network_exists < 2 && network_types &&
          g_slist_length(network_types) == 1)
      {
        gchar *network_type = (gchar *)network_types->data;

        if (network_type && !strcmp(network_type, "*"))
        {
          idx--;
          goto next;
        }
      }

      settings->network_types[idx] =
          g_new0(gchar *, g_slist_length(network_types) + 1);

      i = 0;

      for (l1 = network_types; l1; l1 = l1->next)
        settings->network_types[idx][i++] = (gchar *)l1->data;

      if (!strcmp(label, "%s"))
      {
        const gchar **which_types =
            (const gchar **)settings->network_types[idx];
        GtkTreeIter iter;
        GtkListStore *dun_iaps;

        g_free(label);
        label = NULL;
        dun_iaps = gtk_list_store_new(1, G_TYPE_STRING);
        iap_settings_get_iap_list(dun_iaps,
                                  which_types, -1, -1, 0, -1, -1, -1);

        if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(dun_iaps), &iter))
          gtk_tree_model_get(GTK_TREE_MODEL(dun_iaps), &iter, 0, &label, -1);

        g_object_unref(G_OBJECT(dun_iaps));

        if (!label)
        {
          g_strfreev(settings->network_types[idx]);
          settings->network_types[idx] = NULL;
          idx--;
          goto next;
        }
      }

      if (gettext_catalog)
      {
        settings->labels[idx] = g_strdup(dgettext(gettext_catalog, label));
        settings->not_found_banners[idx] =
            dgettext(gettext_catalog, not_found_banner);
      }
      else
      {
        settings->not_found_banners[idx] = not_found_banner;
        settings->labels[idx] = label;
        label = 0;
        not_found_banner = 0;
      }

      if (g_slist_length(network_types))
        network_exists++;
next:
      if (network_types)
        g_slist_free(network_types);

      g_free(label);
      g_free(not_found_banner);
      g_free(gettext_catalog);
      g_free(l->data);

      idx++;
    }

    g_slist_free(dirs);

  }

  g_object_unref(gconf);
}

struct internet_settings *
inet_settings_create(osso_context_t *osso, GtkWindow *parent)
{
  struct internet_settings *settings = g_new0(struct internet_settings, 1);
  GtkWidget *dialog;
  GtkWidget *touch_selector;
  GtkWidget *auto_connect_button;
  GtkWidget *search_interval_button;
  GtkWidget *auto_switch_button;
  GtkSizeGroup *size_group;
  int i;

  settings->osso = osso;
  settings->widgets =
      g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
  settings->parent = parent;

  inet_settings_get_gconf_data(settings);

  /* dialog */
  dialog = gtk_dialog_new_with_buttons(
        _("conn_set_ti_conn_set"), parent,
        GTK_DIALOG_NO_SEPARATOR | GTK_DIALOG_DESTROY_WITH_PARENT |
        GTK_DIALOG_MODAL,
        _("conn_set_bd_conn_set_conn"), IAP_SETTINGS_CONNECTIONS,
        dgettext("hildon-libs", "wdgt_bd_save"), IAP_SETTINGS_SAVE,
        NULL);
  settings->dialog = dialog;
  iap_common_set_close_response(dialog, IAP_SETTINGS_CLOSE);

  size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
  touch_selector = hildon_touch_selector_new_text();

  /* auto connect */
  auto_connect_button = hildon_picker_button_new(HILDON_SIZE_FINGER_HEIGHT, 0);
  hildon_picker_button_set_selector(HILDON_PICKER_BUTTON(auto_connect_button),
                                    HILDON_TOUCH_SELECTOR(touch_selector));
  hildon_button_set_title(HILDON_BUTTON(auto_connect_button),
                          _("conn_set_fi_conn_set_conn_no_ask"));
  hildon_button_set_alignment(HILDON_BUTTON(auto_connect_button),
                              0.0, 0.5, 1.0, 1.0);

  i = 0;
  while (settings->labels[i])
  {
    hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR(touch_selector),
                                      settings->labels[i++]);
  }

  g_hash_table_insert(settings->widgets, g_strdup("AUTO_CONNECT"),
                      auto_connect_button);

  hildon_button_add_title_size_group(HILDON_BUTTON(auto_connect_button),
                                     size_group);
  g_signal_connect(G_OBJECT(auto_connect_button), "value-changed",
                   G_CALLBACK(auto_connect_value_changed_cb), settings);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                     auto_connect_button, FALSE, FALSE, 0);

  /* search interval */
  search_interval_button =
      iap_widgets_create_static_picker_button(
        _("conn_set_fi_conn_set_search"),
        _("conn_set_va_conn_set_search_1"),
        _("conn_set_va_conn_set_search_2"),
        _("conn_set_va_conn_set_search_3"),
        _("conn_set_va_conn_set_search_4"),
        _("conn_set_va_conn_set_search_5"),
        NULL);
  g_hash_table_insert(settings->widgets, g_strdup("SEARCH_INTERVAL"),
                      search_interval_button);
  hildon_button_add_title_size_group(HILDON_BUTTON(search_interval_button),
                                     size_group);
  g_signal_connect(G_OBJECT(search_interval_button), "value-changed",
                   G_CALLBACK(search_interval_value_changed_cb), settings);
  hildon_helper_set_insensitive_message(search_interval_button,
                                        _("conn_ib_net_auto_conn_off"));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), search_interval_button,
                     FALSE, FALSE, 0);

  /* auto switch */
  auto_switch_button = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
  gtk_button_set_label(GTK_BUTTON(auto_switch_button),
                       _("conn_set_fi_conn_set_switch"));
  g_hash_table_insert(settings->widgets, g_strdup("AUTO_SWITCH"),
                      auto_switch_button);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), auto_switch_button,
                     FALSE, FALSE, 0);
  g_object_unref(G_OBJECT(size_group));

  g_signal_connect(G_OBJECT(dialog), "response",
                   G_CALLBACK(dialog_response_cb), settings);

  return settings;
}
