#include <gconf/gconf-client.h>
#include <hildon/hildon.h>
#include <osso-log.h>
#include <connui/connui.h>

#include <libintl.h>
#include <string.h>

#include "mapper.h"
#include "wizard.h"
#include "stage.h"
#include "advanced.h"

GtkWidget *
iap_wizard_get_dialog(struct iap_wizard *iw)
{
  return iw->dialog;
}

struct stage *
iap_wizard_get_active_stage(struct iap_wizard *iw)
{
  return iw->stage;
}

gchar *
iap_wizard_get_current_page(struct iap_wizard *iw)
{
  gint idx = gtk_notebook_get_current_page(iw->notebook);

  if (idx == -1)
    return NULL;

  return iw->page_ids[idx];
}

gchar *
iap_wizard_get_iap_id(struct iap_wizard *iw)
{
  if (iw)
    return g_strdup(iw->iap_id);

  return NULL;
}

void
iap_wizard_select_plugin_label(struct iap_wizard *iw, gchar *name, guint idx)
{
  gchar *label = g_strdup_printf("%s%d", name, idx);

  if (label)
  {
    GtkWidget *widget = g_hash_table_lookup(iw->widgets, label);

    g_free(label);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_WIDGET(widget)), TRUE);
  }
}

void
iap_wizard_set_completed(struct iap_wizard *iw, gboolean completed)
{
  iw->page.in_progress = !completed;
  iap_wizard_validate_finish_button(iw);
}

static const char *
iap_wizard_get_next_page(struct iap_wizard *iw, const char *page_name,
                         gboolean show_note)
{
  struct iap_wizard_page *page;

  if (!page_name)
    page_name = iap_wizard_get_current_page(iw);

  page = (struct iap_wizard_page *)g_hash_table_lookup(iw->pages, page_name);

  if (page)
  {
    if (page->get_page)
      return page->get_page(page->priv, show_note);
    else
      return page->next_page;
  }
  else
    DLOG_ERR("IAP Wizard page %s not found!", page_name);

  return NULL;
}

void
iap_wizard_validate_finish_button(struct iap_wizard *iw)
{
  if (!iw->import_mode)
  {
    gboolean sensitive = FALSE;

    hildon_helper_set_insensitive_message(iw->button_finish,
                                          dgettext("osso-connectivity-ui",
                                                   "conn_ib_compl_all"));

    if (!iw->page.in_progress)
    {
      const gchar *name_text = gtk_entry_get_text(
            GTK_ENTRY(GTK_WIDGET(g_hash_table_lookup(iw->widgets, "NAME"))));
      const gchar *page_name = iap_wizard_get_current_page(iw);

      if (!page_name)
        page_name = "WELCOME";

      if (!strcmp(page_name, "NAME_AND_TYPE") && name_text && *name_text)
      {
        hildon_helper_set_insensitive_message(
              iw->button_finish, dgettext("osso-connectivity-ui",
                                          "conn_ib_conn_name_in_use"));
      }

      while (!g_str_has_suffix(page_name, "COMPLETE"))
      {
        page_name = iap_wizard_get_next_page(iw, page_name, FALSE);

        if (!page_name)
          break;
      }

      if (page_name)
        sensitive = TRUE;
    }

    gtk_dialog_set_response_sensitive(GTK_DIALOG(iw->dialog), 0, sensitive);
  }
}

static void
iap_wizard_init_stage_widgets(struct iap_wizard *iw,
                              struct stage_widget *widgets)
{
  int stage_widgets_count = 0;
  int widgets_count = 0;
  int i;
  struct stage_widget *w;

  if (iw->stage_widgets)
  {
    w = iw->stage_widgets;

    while (w->name)
    {
      stage_widgets_count++;
      w++;
    }
  }

  if (widgets)
  {
    w = widgets;

    while(w->name)
    {
      widgets_count++;
      w++;
    }
  }

  iw->stage_widgets = (struct stage_widget *)g_realloc_n(
        iw->stage_widgets, widgets_count + stage_widgets_count + 1,
        sizeof(struct stage_widget));

  w = &iw->stage_widgets[stage_widgets_count];

  for (i = 0; i < widgets_count; i++)
    memcpy(w++, widgets++, sizeof(*w));

  w->name = NULL;
}

static void
iap_wizard_load_plugins(struct iap_wizard *iw)
{
  gboolean (*plugin_init)(struct iap_wizard *, struct iap_wizard_plugin *);
  GDir *dir = g_dir_open(IAPSETTINGS_PLUGIN_DIR, 0, NULL);
  GSList *l;
  const gchar *module_name;

  plugin_init = NULL;

  if (!dir)
    return;

  while ((module_name = g_dir_read_name(dir)))
  {
    gchar *module_path =
        g_module_build_path(IAPSETTINGS_PLUGIN_DIR, module_name);
    GModule *module = g_module_open(module_path, G_MODULE_BIND_LOCAL);

    DLOG_INFO("Opening module %s: %p", module_path, module);

    if (module &&
        g_module_symbol(module, "iap_wizard_plugin_init",
                        (gpointer *)&plugin_init))
    {
      struct iap_wizard_plugin *plugin = g_new0(struct iap_wizard_plugin, 1);

      if (plugin && plugin_init(iw, plugin))
      {
        DLOG_INFO("Using IAP settings module %s", module_name);
        iw->plugin_modules = g_slist_append(iw->plugin_modules, module);
        iw->plugins = g_slist_append(iw->plugins, plugin);
      }
      else
      {
        g_module_close(module);
        DLOG_ERR("Unable to initialize module %s", module_name);
      }
    }
    else
      DLOG_ERR("Unable to use module %s: %s", module_name, g_module_error());

    g_free(module_path);
  }

  for (l = iw->plugins; l; l = l->next)
  {
    struct iap_wizard_plugin *plugin = l->data;
    struct iap_wizard_page *page;

    iap_wizard_init_stage_widgets(iw, plugin->stage_widgets);
    page = plugin->pages;

    while (page->id)
    {
      page->priv = plugin->priv;

      g_hash_table_insert(iw->pages, page->id, page);
      page++;
    }
  }

  g_dir_close(dir);
}

static GtkWidget *
iap_wizard_welcome_page_create(gpointer private)
{
  GdkPixbuf *pixbuf;
  GtkWidget *hbox;
  GtkWidget *image;
  GtkWidget *vbox;
  GtkWidget *label;

  hbox = gtk_hbox_new(FALSE, 0);
  pixbuf = connui_pixbuf_load("widgets_wizard", 50);
  image = gtk_image_new_from_pixbuf(pixbuf);
  connui_pixbuf_unref(pixbuf);

  gtk_misc_set_alignment(GTK_MISC(image), 0.0, 0.0);
  gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);

  vbox = gtk_vbox_new(FALSE, 0);

  label = gtk_label_new(dgettext("osso-connectivity-ui",
                                 "conn_set_iap_fi_welcome_text"));

  g_object_set(G_OBJECT(label), "wrap", TRUE, NULL);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);

  gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);


  label = gtk_label_new(dgettext("osso-connectivity-ui",
                                 "conn_set_iap_fi_tap_next"));

  g_object_set(G_OBJECT(label), "wrap", TRUE, NULL);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
  gtk_box_pack_end(GTK_BOX(vbox), label, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

  return GTK_WIDGET(hbox);
}

static void
iap_wizard_complete_page_advanced_clicked_cb(HildonButton *button,
                                             struct iap_wizard *iw)
{
  gtk_dialog_response(GTK_DIALOG(iw->dialog), 4);
}

static GtkWidget *
iap_wizard_complete_page_create(gpointer private)
{
  struct iap_wizard *iw = private;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GdkPixbuf *pixbuf;
  GtkWidget *image;
  GtkWidget *label;
  GtkWidget *adv_button;

  vbox = gtk_vbox_new(FALSE, 0);
  hbox = gtk_hbox_new(FALSE, 0);
  pixbuf = connui_pixbuf_load("widgets_wizard", 50);
  image = gtk_image_new_from_pixbuf(pixbuf);
  connui_pixbuf_unref(pixbuf);

  gtk_misc_set_alignment(GTK_MISC(image), 0.0, 0.0);
  gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);

  label = gtk_label_new(dgettext("osso-connectivity-ui",
                                 "conn_set_iap_fi_finish_text"));
  g_hash_table_insert(iw->widgets, g_strdup("FINISH_LABEL"), label);
  g_object_set(G_OBJECT(label), "wrap", TRUE, NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
  hbox = gtk_hbox_new(FALSE, 0);

  adv_button =
      hildon_button_new_with_text(HILDON_SIZE_FINGER_HEIGHT,
                                  HILDON_BUTTON_ARRANGEMENT_VERTICAL,
                                  dgettext("osso-connectivity-ui",
                                           "conn_set_iap_bd_advanced"),
                                  NULL);
  g_hash_table_insert(iw->widgets, g_strdup("ADVANCED_BUTTON"), adv_button);
  g_signal_connect(GTK_OBJECT(adv_button), "clicked",
                   G_CALLBACK(iap_wizard_complete_page_advanced_clicked_cb),
                   iw);

  gtk_box_pack_end(GTK_BOX(hbox), adv_button, FALSE, FALSE, 0);
  gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  return GTK_WIDGET(vbox);
}

static void
iap_wizard_complete_page_finish(gpointer private)
{
  iap_wizard_set_completed(private, TRUE);
}

static int
iap_wizard_plugins_sort_cb(const struct iap_wizard_plugin *a,
                           const struct iap_wizard_plugin *b)
{
  if (a->prio == b->prio)
    return 0;

  if (a->prio < b->prio)
    return 1;

  return -1;
}

static void
iap_wizard_name_and_type_page_name_changed_cb(GtkEntry *entry,
                                              struct iap_wizard *iw)
{
  iap_wizard_validate_finish_button(iw);
}

static void
iap_wizard_plugins_button_toggled_cb(GtkToggleButton *button,
                                     struct iap_wizard *iw)
{
  if (gtk_toggle_button_get_active(button))
    iap_wizard_validate_finish_button(iw);
}

static GtkWidget *
iap_wizard_name_and_type_page_create(gpointer private)
{
  struct iap_wizard *iw = private;
  GtkWidget *entry;
  GtkWidget *label;
  GtkWidget *caption;
  GtkWidget *vbox;
  GtkSizeGroup *size_group;
  GSList *l;
  GSList *plugins = g_slist_sort(g_slist_copy(iw->plugins),
                                 (GCompareFunc)iap_wizard_plugins_sort_cb);

  vbox = gtk_vbox_new(FALSE, 0);
  size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
  entry = gtk_entry_new();
  g_hash_table_insert(iw->widgets, g_strdup("NAME"), entry);
  g_signal_connect(G_OBJECT(entry), "changed",
                   G_CALLBACK(iap_wizard_name_and_type_page_name_changed_cb),
                   iw);

  caption = hildon_caption_new(size_group,
                               dgettext("osso-connectivity-ui",
                                        "conn_set_iap_fi_conn_name"),
                               entry, NULL, HILDON_CAPTION_OPTIONAL);
  g_hash_table_insert(iw->widgets, g_strdup("NAME_CAPTION"), caption);
  gtk_box_pack_start(GTK_BOX(vbox), caption, FALSE, FALSE, 0);

  label = gtk_label_new("");
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
  g_hash_table_insert(iw->widgets, g_strdup("TYPE"), label);

  caption = hildon_caption_new(size_group,
                               dgettext("osso-connectivity-ui",
                                        "conn_set_iap_bd_conn_type"),
                               label, NULL, HILDON_CAPTION_OPTIONAL);
  g_hash_table_insert(iw->widgets, g_strdup("TYPE_CAPTION"), caption);
  gtk_box_pack_start(GTK_BOX(vbox), caption, FALSE, FALSE, 0);

  for (l = plugins; l; l = l->next)
  {
    struct iap_wizard_plugin *plugin = (struct iap_wizard_plugin *)l->data;

    if (plugin->get_widgets)
    {
      const gchar **plugin_widgets = plugin->get_widgets(plugin->priv);

      if (plugin_widgets)
      {
        int idx = 0;
        GtkWidget *button = NULL;

        while (plugin_widgets[idx])
        {
          gchar *id = g_strdup_printf("%s%d", plugin->name, idx);

          button = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(button));
          g_signal_connect(G_OBJECT(button), "toggled",
                           G_CALLBACK(iap_wizard_plugins_button_toggled_cb),
                           iw);
          g_hash_table_insert(iw->widgets, id, button);

          caption = hildon_caption_new(size_group, plugin_widgets[idx],
                                       button, NULL, HILDON_CAPTION_OPTIONAL);
          gtk_box_pack_start(GTK_BOX(vbox), caption, FALSE, FALSE, 0);
          idx++;
        }
      }
    }
  }

  g_slist_free(plugins);
  g_object_unref(G_OBJECT(size_group));

  return vbox;
}

static GtkWidget *
iap_wizard_find_plugin_widget(struct iap_wizard *iw,
                              struct iap_wizard_plugin *plugin, int idx)
{
  gchar *id = g_strdup_printf("%s%d", plugin->name, idx);
  GtkWidget *widget;

  widget = GTK_WIDGET(g_hash_table_lookup(iw->widgets, id));

  g_free(id);

  return widget;
}

static struct iap_wizard_plugin *
iap_wizard_find_plugin(struct iap_wizard *iw, int *idx)
{
  GSList *l;


  for (l = iw->plugins; l; l = l->next)
  {
    struct iap_wizard_plugin *plugin = l->data;

    if (plugin && plugin->get_widgets)
    {
      const gchar **widgets = plugin->get_widgets(plugin->priv);

      if (widgets && *widgets)
      {
        int i = 0;

        while (widgets[i])
        {
          GtkWidget *button = iap_wizard_find_plugin_widget(iw, plugin, i);

          if (button && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
          {
            if (idx)
              *idx = i;

            return plugin;
          }

          i++;
        }
      }
    }
  }

  return NULL;
}

const char *
iap_wizard_name_and_type_page_next(gpointer private, gboolean show_note)
{
  struct iap_wizard *iw = private;
  GtkWidget *widget;
  const gchar *iap_name;
  gchar *type = NULL;
  const char *page = NULL;
  int idx = 0;

  widget = iap_wizard_get_widget(iw, "NAME");
  iap_name = gtk_entry_get_text(GTK_ENTRY(widget));

  if (iw->stage)
    type = stage_get_string(iw->stage, "type");

  if ((type && !strcmp(type, "GPRS")) || !iap_name || !*iap_name ||
      !iap_settings_iap_exists(iap_name, iw->iap_id))
  {
    struct iap_wizard_plugin *plugin;

    g_free(type);
    plugin = iap_wizard_find_plugin(iw, &idx);
    iw->plugin = plugin;

    if (plugin)
    {
      if (plugin->get_page)
      {
        page = plugin->get_page(plugin->priv, idx, show_note);
      }
      else
        DLOG_INFO("Plugin does not handle this type");
    }
    else
      DLOG_INFO("No plugin found to handle type");

    return page;
  }

  if (show_note)
  {
    hildon_banner_show_information(
          GTK_WIDGET(iw->dialog), NULL,
          dgettext("osso-connectivity-ui", "conn_ib_conn_name_in_use"));
    gtk_widget_grab_focus(widget);
  }

  g_free(type);

  return NULL;
}

static void
iap_wizard_name_and_type_page_finish(gpointer private)
{
  struct iap_wizard *iw = private;
  GSList *l;
  GSList *widget_list = NULL;
  const gchar *type = NULL;
  GSList *plugins;
  gboolean has_active = FALSE;

  plugins = g_slist_sort(g_slist_copy(iw->plugins),
                         (GCompareFunc)iap_wizard_plugins_sort_cb);

  if (plugins)
  {
    for (l = plugins; l; l = l->next)
    {
      const gchar **widgets;
      struct iap_wizard_plugin *plugin = l->data;
      GtkWidget *widget;
      int idx = 0;

      if (!plugin)
        continue;

      if (plugin->get_widgets &&
          (widgets = plugin->get_widgets(plugin->priv)) && *widgets)
      {
        while (widgets[idx])
        {
          widget = iap_wizard_find_plugin_widget(iw, plugin, idx);
          widget_list = g_slist_append(widget_list, widget);
          gtk_widget_show_all(gtk_widget_get_parent(widget));
          type = widgets[idx];
          idx++;
        }
      }

      while (idx < 10 &&
             (widget = iap_wizard_find_plugin_widget(iw, plugin, idx++)))
      {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
        gtk_widget_hide_all(gtk_widget_get_parent(widget));
      }
    }

    g_slist_free(plugins);

    if (widget_list)
    {
      for (l = widget_list; l; l = l->next)
      {
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(l->data)))
        {
          has_active = TRUE;
          break;
        }
      }

      if (!has_active)
      {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget_list->data),
                                     TRUE);
      }

      if (g_slist_length(widget_list) == 1)
      {
        gtk_label_set_text(
              GTK_LABEL(GTK_WIDGET(g_hash_table_lookup(iw->widgets, "TYPE"))),
              type);
        gtk_widget_hide_all(
              gtk_widget_get_parent(GTK_WIDGET(GTK_BUTTON(widget_list->data))));
        return;
      }
    }
  }

  gtk_label_set_text(
        GTK_LABEL(GTK_WIDGET(g_hash_table_lookup(iw->widgets, "TYPE"))), "");
}

static struct iap_wizard_page iap_wizard_pages[] =
{
  {"WELCOME",
   "conn_set_iap_ti_welcome",
   iap_wizard_welcome_page_create,
   NULL,
   NULL,
   NULL,
   "NAME_AND_TYPE",
   "Connectivity_Internetsettings_connectiondialog",
   NULL},
  {
    "NAME_AND_TYPE",
    "conn_set_iap_ti_name_type",
    iap_wizard_name_and_type_page_create,
    iap_wizard_name_and_type_page_next,
    iap_wizard_name_and_type_page_finish,
    NULL,
    NULL,
    "Connectivity_Internetsettings_connectiondialog",
    NULL
  },
  {
    "COMPLETE",
    "conn_set_iap_ti_finish",
    iap_wizard_complete_page_create,
    NULL,
    iap_wizard_complete_page_finish,
    NULL,
    NULL,
    "Connectivity_Internetsettings_IAPsetupfinish",
    NULL
  },
  {0}
};

/* TODO: maybe generate these automatically, at least the IDs, based on existing
 * configs, we want only specific ids for the dummy_provider */
static char* provider_types[] = {"", "DUMMY",  NULL};
static char* provider_ids[] = {"", "dummy-provider",  NULL};

static struct stage_widget iap_wizard_widgets[] =
{
  {NULL, NULL, "NAME", "name", NULL, &mapper_entry2string, NULL},
  {NULL, NULL, "IPV4_HTTP_HOST", "proxy_http", NULL, &mapper_entry2string, NULL},
  {NULL, NULL, "IPV4_HTTP_PORT", "proxy_http_port", NULL, &mapper_numbereditor2int, NULL},
  {NULL, NULL, "IPV4_HTTPS_HOST", "proxy_https", NULL, &mapper_entry2string, NULL},
  {NULL, NULL, "IPV4_HTTPS_PORT", "proxy_https_port", NULL, &mapper_numbereditor2int, NULL},
  {NULL, NULL, "IPV4_FTP_HOST", "proxy_ftp", NULL, &mapper_entry2string, NULL},
  {NULL, NULL, "IPV4_FTP_PORT", "proxy_ftp_port", NULL, &mapper_numbereditor2int, NULL},
  {NULL, NULL, "IPV4_RTSP_HOST", "proxy_rtsp", NULL, &mapper_entry2string, NULL},
  {NULL, NULL, "IPV4_RTSP_PORT", "proxy_rtsp_port", NULL, &mapper_numbereditor2int, NULL},
  {NULL, NULL, "IPV4_OMIT_PROXY", "omit_proxy", NULL, &mapper_entry2stringlist, ", "},
  {NULL, NULL, "IPV4_PROXY_URL", "autoconf_url", NULL, &mapper_entry2string, NULL},
  {NULL, NULL, "IPV4_ADDRESS", "ipv4_address", NULL, &mapper_entry2string, NULL},
  {NULL, NULL, "IPV4_NETMASK", "ipv4_netmask", NULL, &mapper_entry2string, NULL},
  {NULL, NULL, "IPV4_GATEWAY", "ipv4_gateway", NULL, &mapper_entry2string, NULL},
  {NULL, NULL, "IPV4_AUTO_DNS", "ipv4_autodns", NULL, &mapper_toggle2bool, NULL},
  {NULL, NULL, "IPV4_DNS1", "ipv4_dns1", NULL, &mapper_entry2string, NULL},
  {NULL, NULL, "IPV4_DNS2", "ipv4_dns2", NULL, &mapper_entry2string, NULL},
  {NULL, NULL, "PROVIDER_SERVICE_TYPE", "service_type", NULL, &mapper_combo2string, provider_types},
  {NULL, NULL, "PROVIDER_SERVICE_ID", "service_id", NULL, &mapper_combo2string, provider_ids},
  {0, }
};

static void
iap_wizzard_notebook_switch_page_cb(GtkNotebook *notebook, gpointer arg1,
                                    guint idx, struct iap_wizard *iw)
{
  GtkDialog *dialog = GTK_DIALOG(iw->dialog);
  gchar *id = iw->page_ids[idx];

  struct iap_wizard_page *page =
      (struct iap_wizard_page *)g_hash_table_lookup(iw->pages, id);

  if (page)
  {
    gboolean sens;

    gtk_window_set_title(GTK_WINDOW(dialog), dgettext("osso-connectivity-ui",
                                                      page->msgid));
    sens = strcmp(id, "WELCOME") && strcmp(id, "NAME_AND_TYPE");

    gtk_dialog_set_response_sensitive(dialog, WIZARD_BUTTON_PREVIOUS, sens);

    sens = !g_str_has_suffix(id, "COMPLETE") && (page->get_page ||
                                                 page->next_page);

    gtk_dialog_set_response_sensitive(dialog, WIZARD_BUTTON_NEXT, sens);

    if (page->finish)
      page->finish(page->priv);
  }
  else
    DLOG_ERR("Unable to find page %s!", id);
}

static void
iap_wizzard_export_widgets(struct iap_wizard *iw)
{
  mapper_export_widgets(iw->stage, iw->stage_widgets,
                        (mapper_get_widget_fn)iap_wizard_get_widget, iw);
}

static void
iap_wizard_dialog_advanced_settings_response_cb(GtkDialog *dialog,
                                                gint response_id,
                                                gpointer user_data)
{
  struct iap_wizard *iw = (struct iap_wizard *)user_data;

  if (response_id == GTK_RESPONSE_OK)
  {
    struct iap_wizard_plugin *plugin = iw->plugin;

    if (plugin && plugin->advanced_done)
      plugin->advanced_done(plugin->priv);

    iap_advanced_export(iw->advanced, iw->stage);
  }

  iap_advanced_destroy(iw->advanced);
  iw->advanced = NULL;
}

static void
iap_wizard_dialog_activate_advanced_settings(struct iap_wizard *iw)
{
  struct iap_wizard_plugin *plugin;
  struct iap_advanced_page *pages = NULL;

  if (!(plugin = iw->plugin))
  {
    DLOG_ERR("Unable to activate advanced setings while there is no active plugin");
    return;
  }

  if (plugin->get_advanced)
  {
    struct iap_advanced_page *page;

    page = pages = plugin->get_advanced(plugin->priv);

    while (page->msgid)
    {
      pages->priv = plugin->priv;
      page++;
    }
  }

  iw->advanced = iap_advanced_create(iw->osso, GTK_WINDOW(iw->dialog),
                                     pages, iw->stage_widgets, iw->stage);

  iap_advanced_import(iw->advanced, iw->stage);

  if (plugin->advanced_show)
    plugin->advanced_show(plugin->priv, iw->stage);

  g_signal_connect(G_OBJECT(iw->advanced->dialog), "response",
                   G_CALLBACK(iap_wizard_dialog_advanced_settings_response_cb),
                   iw);
  iap_advanced_show(iw->advanced);
}

static void
iap_wizard_dialog_response_cb(GtkDialog *dialog, gint response_id,
                              gpointer user_data)
{
  struct iap_wizard *iw = (struct iap_wizard *)user_data;
  const gchar *id = iap_wizard_get_current_page(iw);
  struct iap_wizard_page *page =
      (struct iap_wizard_page *)g_hash_table_lookup(iw->pages, id);
  const gchar *next_page;

  if (!page)
  {
    DLOG_ERR("Unable to find current page %s!", id);
    return;
  }

  if (response_id != WIZARD_BUTTON_NEXT &&
      response_id != WIZARD_BUTTON_ADVANCED)
  {
    if (page->prev)
      page->prev(page->priv);
  }

  iw->response_id = response_id;

  switch (response_id)
  {
    case WIZARD_BUTTON_FINISH:
      iap_wizard_get_next_page(iw, NULL, FALSE);
      break;
    case WIZARD_BUTTON_PREVIOUS:
      iap_wizard_get_next_page(iw, NULL, FALSE);
      iw->page.current--;
      gtk_notebook_set_current_page(iw->notebook,
                                    iw->page.index[iw->page.current]);
      break;
    case WIZARD_BUTTON_NEXT:
      next_page = iap_wizard_get_next_page(iw, id, TRUE);

      if (next_page)
      {
        if (page->prev)
          page->prev(page->priv);

        if (iw->stage)
          iap_wizzard_export_widgets(iw);

        iw->page.current++;
        iap_wizard_set_current_page(iw, next_page);
      }
      break;
    case WIZARD_BUTTON_ADVANCED:
      iap_wizzard_export_widgets(iw);
      iap_wizard_dialog_activate_advanced_settings(iw);
      break;
    default:
      break;
  }
}

struct iap_wizard *
iap_wizard_create(osso_context_t *osso, GtkWindow *parent)
{
  struct iap_wizard *iw = g_new0(struct iap_wizard, 1);
  GtkDialogFlags dialog_flags = GTK_DIALOG_NO_SEPARATOR;
  struct iap_wizard_page *page;
  GtkDialog *dialog;

  iw->osso = osso;
  iw->pages = g_hash_table_new(g_str_hash, g_str_equal);
  iw->widgets = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

  if (parent)
    dialog_flags |= (GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL);

  iw->parent = parent;
  iw->page.in_progress = TRUE;

  iw->dialog = gtk_dialog_new_with_buttons(NULL, parent, dialog_flags, NULL);
  dialog = GTK_DIALOG(iw->dialog);

  iw->button_finish = gtk_dialog_add_button(dialog,
                                            dgettext("hildon-libs",
                                                     "wdgt_bd_finish"),
                                            WIZARD_BUTTON_FINISH);
  gtk_dialog_add_button(dialog,
                        dgettext("hildon-libs",
                                 "wdgt_bd_previous"),
                        WIZARD_BUTTON_PREVIOUS);
  iw->button_next = gtk_dialog_add_button(dialog,
                                          dgettext("hildon-libs",
                                                   "wdgt_bd_next"),
                                          WIZARD_BUTTON_NEXT);
  iap_common_set_close_response(iw->dialog, WIZARD_BUTTON_CLOSE);

  iw->notebook = GTK_NOTEBOOK(gtk_notebook_new());

  gtk_notebook_set_show_tabs(iw->notebook, FALSE);
  gtk_notebook_set_show_border(iw->notebook, FALSE);

  gtk_container_add(GTK_CONTAINER(dialog->vbox), GTK_WIDGET(iw->notebook));
  iap_wizard_init_stage_widgets(iw, iap_wizard_widgets);

  page = iap_wizard_pages;

  while(page->id)
  {
    page->priv = iw;
    g_hash_table_insert(iw->pages, page->id, page);
    page++;
  }

  iap_wizard_load_plugins(iw);

  iw->page_ids = g_new0(gchar *, g_hash_table_size(iw->pages) + 1);
  g_hash_table_foreach(iw->pages, (GHFunc)iap_wizzard_create_page, iw);
  g_signal_connect(GTK_OBJECT(iw->notebook), "switch-page",
                   G_CALLBACK(iap_wizzard_notebook_switch_page_cb), iw);
  g_signal_connect(GTK_OBJECT(iw->dialog), "response",
                   G_CALLBACK(iap_wizard_dialog_response_cb), iw);

  return iw;
}

void
iap_wizard_destroy(struct iap_wizard *iw)
{
  GSList *m;
  GSList *l;

  gtk_widget_destroy(iw->dialog);
  for (l = iw->plugins, m = iw->plugin_modules; l && m;
       l = l->next, m = m->next)
  {
    void (*iap_wizard_plugin_destroy)(struct iap_wizard *,
                                      struct iap_wizard_plugin *);

    if (g_module_symbol((GModule *)m->data, "iap_wizard_plugin_destroy",
                        (gpointer *)&iap_wizard_plugin_destroy) )
    {
      if (iap_wizard_plugin_destroy)
        iap_wizard_plugin_destroy(iw, (struct iap_wizard_plugin *)l->data);
    }

    g_module_close((GModule *)m->data);
    g_free(l->data);
  }

  g_slist_free(iw->plugins);
  g_slist_free(iw->plugin_modules);
  g_hash_table_destroy(iw->pages);
  g_hash_table_destroy(iw->widgets);
  g_strfreev(iw->page_ids);
  g_free(iw->stage_widgets);
  g_free(iw->iap_id);
  g_free(iw);
}

GtkWidget *
iap_wizard_get_widget(struct iap_wizard *iw, const gchar *id)
{
  gpointer widget;
  GSList *l;

  if ((widget = g_hash_table_lookup(iw->widgets, id)))
    return GTK_WIDGET(widget);

  for (l = iw->plugins; l; l = l->next )
  {
    struct iap_wizard_plugin *plugin = (struct iap_wizard_plugin *)l->data;

    if ((widget = GTK_WIDGET(g_hash_table_lookup(plugin->widgets, id))))
      return GTK_WIDGET(widget);
  }

  if (iw->advanced && (widget = g_hash_table_lookup(iw->advanced->widgets, id)))
    return GTK_WIDGET(widget);

  return NULL;
}

void
iap_wizzard_create_page(const gchar *id, struct iap_wizard_page *wp,
                        struct iap_wizard *iw)
{
  GtkWidget *page;
  GtkWidget *viewport;
  GtkWidget *scrolled_window;
  gint idx;
  GtkRequisition requisition;

  page = wp->create(wp->priv);
  viewport = gtk_viewport_new(NULL, NULL);
  gtk_viewport_set_shadow_type(GTK_VIEWPORT(viewport), GTK_SHADOW_NONE);
  gtk_container_add(GTK_CONTAINER(viewport), GTK_WIDGET(page));

  scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),
                                      GTK_SHADOW_NONE);
  gtk_container_add(GTK_CONTAINER(scrolled_window), viewport);

  if (GTK_IS_CONTAINER(page))
  {
    gtk_container_set_focus_vadjustment(
          GTK_CONTAINER(page), gtk_scrolled_window_get_vadjustment(
                                         GTK_SCROLLED_WINDOW(scrolled_window)));
  }

  idx = gtk_notebook_append_page(iw->notebook, scrolled_window, NULL);

  if (idx >= 0)
    iw->page_ids[idx] = g_strdup(wp->id);

  gtk_widget_show_all(page);
  gtk_widget_size_request(page, &requisition);
  gtk_widget_set_size_request(scrolled_window, -1, requisition.height);
}

void
iap_wizard_set_start_page(struct iap_wizard *iw, const gchar *page_id)
{
  const gchar *id = iw->page_ids[0];
  int idx = 0;

  if (!id)
    return;

  while (strcmp(id, page_id))
  {
    id = iw->page_ids[++idx];

    if (!id)
      return;
  }

  iw->page.index[0] = idx;
}

void
iap_wizard_set_empty_values(struct iap_wizard *iw)
{
  int idx = 1;
  gpointer widget;
  char iap[100];

  do
  {
    const char *s = dgettext("osso-connectivity-ui",
                             "conn_set_iap_fi_conn_name_default");

    sprintf(iap, s, idx++);
  }
  while (strcmp(iap, "conn_set_iap_fi_conn_name_default") &&
         iap_settings_iap_exists(iap, NULL));

  widget = g_hash_table_lookup(iw->widgets, "NAME");
  gtk_entry_set_text(GTK_ENTRY(GTK_WIDGET(widget)), iap);

  iap_wizard_set_start_page(iw, "WELCOME");
  iap_wizard_validate_finish_button(iw);
}

void
iap_wizard_show(struct iap_wizard *iw)
{
  const char *id;

  gtk_widget_show_all(iw->dialog);
  gtk_notebook_set_current_page(iw->notebook, iw->page.index[iw->page.current]);

  id = iw->page_ids[iw->page.index[iw->page.current]];

  if (iw->advanced)
    iap_advanced_show(iw->advanced);
  else if (id && !strcmp(id, "WELCOME"))
    gtk_widget_grab_focus(iw->button_next);

  iap_wizard_validate_finish_button(iw);
}

gboolean
iap_wizard_set_current_page(struct iap_wizard *iw, const gchar *id)
{
  int idx = 0;

  while (iw->page_ids[idx] && strcmp(iw->page_ids[idx], id))
    idx++;

  if (iw->page_ids[idx])
  {
    iw->page.index[iw->page.current] = idx;
    gtk_notebook_set_current_page(iw->notebook, idx);
    iap_wizard_validate_finish_button(iw);
    return TRUE;
  }

  DLOG_ERR("Unable to find page %s!", id);

  return FALSE;
}

int
iap_wizard_get_import_mode(struct iap_wizard *iw)
{
  if (iw->advanced)
    return iw->advanced->import_mode;

  return iw->import_mode;
}

void
iap_wizard_set_import_mode(struct iap_wizard *iw, int mode)
{
  if (iw->advanced)
    iw->advanced->import_mode = mode;
  else
    iw->import_mode = mode;
}

GtkWidget *
iap_wizard_export(struct iap_wizard *iw, struct stage *s, gboolean reconnect)
{
  const char *msg;


  if (iw->plugin && iw->plugin->advanced_done)
    iw->plugin->advanced_done(iw->plugin->priv);

  iap_wizzard_export_widgets(iw);
  stage_copy(iw->stage, s);

  if (reconnect)
    msg = dgettext("osso-connectivity-ui", "conn_ib_settings_saved_reconnect");
  else
    msg = dgettext("osso-connectivity-ui", "conn_ib_settings_saved");

  return hildon_banner_show_information(GTK_WIDGET(iw->parent), 0, msg);
}

void
iap_wizard_save_state(struct iap_wizard *iw, GByteArray *state)
{
  GSList *l;
  struct iap_wizard_plugin *plugin;
  int idx = 0;
  guint8 len = 0;

  iap_wizzard_export_widgets(iw);
  plugin = iap_wizard_find_plugin(iw, &idx);

  if (plugin)
  {
    len = strlen(plugin->name) + 1;
    g_byte_array_append(state, &len, sizeof(len));
    g_byte_array_append(state, (guint8 *)plugin->name, len);
    g_byte_array_append(state, (guint8 *)&idx, sizeof(idx));
  }
  else
    g_byte_array_append(state, &len, sizeof(len));

  for (l = iw->plugins; l; l = l->next)
  {
    plugin = l->data;

    if (plugin->save_state)
      plugin->save_state(plugin->priv, state);
  }

  if (iw->iap_id)
    len = strlen(iw->iap_id) + 1;
  else
    len = 0;

  g_byte_array_append(state, &len, sizeof(len));

  if (len)
    g_byte_array_append(state, (guint8 *)iw->iap_id, len);

  g_byte_array_append(state, (guint8 *)&iw->page, sizeof(iw->page));

  len = iw->advanced != 0 ? 1 : 0;
  g_byte_array_append(state, &len, sizeof(len));

  if (len)
    iap_advanced_save_state(iw->advanced, state);
}

static void
iap_wizard_import_widgets(struct iap_wizard *sw)
{
  sw->import_mode = 1;
  mapper_import_widgets(sw->stage, sw->stage_widgets,
                        (mapper_get_widget_fn)iap_wizard_get_widget, sw);
  sw->import_mode = 0;
  iap_wizard_validate_finish_button(sw);
}

void
iap_wizard_import(struct iap_wizard *iw, struct stage *s)
{
  GSList *l;
  gchar *name;

  iw->import_mode = 1;

  if (s)
  {
    iap_wizard_set_active_stage(iw, s);
    iw->page.in_progress = FALSE;
    iw->iap_id = g_strdup(s->name);
  }

  for (l = iw->plugins; l; l = l->next)
  {
    struct iap_wizard_plugin *plugin = l->data;

    if (plugin && plugin->get_page && plugin->get_page(plugin->priv, -1, FALSE))
    {
      iw->plugin = plugin;
      break;
    }
  }

  iap_wizard_import_widgets(iw);

  if (iw->plugin && iw->plugin->advanced_show)
    iw->plugin->advanced_show(iw->plugin->priv, s);

  iw->import_mode = 1;
  name = stage_get_string(iw->stage, "name");

  if ((!name || !*name) && s && s->name&& *s->name)
  {
    gpointer widget = g_hash_table_lookup(iw->widgets, "NAME");
    gchar *stage_name = iap_settings_get_name(s->name);

    gtk_entry_set_text(GTK_ENTRY(GTK_WIDGET(widget)), stage_name);
    g_free(stage_name);
  }

  g_free(name);
  iw->import_mode = 0;
}

static const struct
{
  const char *key;
  const char *id;
} stage_defaults[] =
{
  {"ipv4_type", "AUTO"},
  {"ipv4_address", "0.0.0.0"},
  {"ipv4_netmask", "0.0.0.0"},
  {"ipv4_gateway", "0.0.0.0"},
  {"ipv4_autodns", GUINT_TO_POINTER(-1)},
  {"ipv4_dns1", "0.0.0.0"},
  {"ipv4_dns2", "0.0.0.0"},
  {"proxytype", "NONE"}
};


void
iap_wizard_set_active_stage(struct iap_wizard *iw, struct stage *new_stage)
{
  struct stage *s = iw->stage;
  gpointer widget;
  const gchar *name;

  if (s == new_stage)
    return;

  iw->import_mode = 1;

  if ( s && !s->gconf )
    iap_wizzard_export_widgets(iw);

  widget = g_hash_table_lookup(iw->widgets, "NAME");

  name = gtk_entry_get_text(GTK_ENTRY(GTK_WIDGET(widget)));

  iw->stage = new_stage;

  if (new_stage)
  {
    int i;

    for (i = 0; i < G_N_ELEMENTS(stage_defaults); i++)
    {
      const char *key = stage_defaults[i].key;
      GConfValue *val = stage_get_val(new_stage, stage_defaults[i].key);

      if (!val)
      {
        const char *id = stage_defaults[i].id;

        if (id == GUINT_TO_POINTER(-2))
          stage_set_bool(iw->stage, key, FALSE);
        else if (id == GUINT_TO_POINTER(-1))
          stage_set_bool(iw->stage, key, TRUE);
        else if (GPOINTER_TO_UINT(id) > 255)
          stage_set_string(iw->stage, key, id);
        else
          stage_set_int(iw->stage, key, GPOINTER_TO_UINT(id));
      }
      else
        gconf_value_free(val);
    }
  }

  ULOG_DEBUG("Wizard stage changed: %s(%p)", new_stage->name, new_stage);

  if (name && *name)
    stage_set_string(iw->stage, "name", name);

  iap_wizard_import_widgets(iw);
}

gboolean
iap_wizard_restore_state(struct iap_wizard *iw, struct stage_cache *data)
{
  GSList *l;
  guint8 len;
  int page_idx = 0;

  if (data->processed + sizeof(len) > data->len)
    return 0;

  len = data->data[data->processed];
  data->processed += sizeof(len);

  if (len)
  {
    const char *plugin_id;
    if (data->processed + len > data->len)
      return FALSE;

    plugin_id = (const char *)&data->data[data->processed];

    data->processed += len;

    if (!plugin_id || data->processed + sizeof(page_idx) > data->len)
      return FALSE;

    memcpy(&page_idx, &data->data[data->processed], sizeof(page_idx));

    data->processed += sizeof(page_idx);

    for (l = iw->plugins; l; l = l->next)
    {
      struct iap_wizard_plugin *plugin = l->data;

      if (!strcmp(plugin->name, plugin_id))
      {
        iw->plugin = plugin;
        break;
      }
    }

    if (!iw->plugin)
    {
      DLOG_ERR("Active plugin was not found, when restoring state!");
      return FALSE;
    }
  }

  for (l = iw->plugins; l; l = l->next)
  {
    struct iap_wizard_plugin *plugin = l->data;

    if (plugin->restore)
      plugin->restore(plugin->priv, data);
  }

  if (data->processed + sizeof(len) > data->len)
    return FALSE;

  len = data->data[data->processed];
  data->processed += sizeof(len);

  if (len)
  {
    const char *iap_id;

    if (data->processed + len > data->len)
      return FALSE;

    iap_id = (const char *)&data->data[data->processed];
    data->processed += len;

    if (!iap_id)
      return FALSE;

    iw->iap_id = g_strdup(iap_id);
  }

  if (data->processed + sizeof(iw->page) > data->len)
    return FALSE;

  if (!&data->data[data->processed])
    return FALSE;

  memcpy(&iw->page, &data->data[data->processed], sizeof(iw->page));

  data->processed += sizeof(iw->page);

  iw->stage = NULL;

  if (iw->plugin&& iw->plugin->get_page)
    iw->plugin->get_page(iw->plugin->priv, page_idx, FALSE);

  iap_wizard_import(iw, NULL);

  if (data->processed + sizeof(len) > data->len)
    return FALSE;

  len = data->data[data->processed];
  data->processed++;

  if (len)
    iap_wizard_dialog_activate_advanced_settings(iw);

  return TRUE;
}
