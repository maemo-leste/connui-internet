#include <hildon/hildon.h>
#include <connui/connui.h>

#include <string.h>
#include <libintl.h>

#include "advanced.h"
#include "wizard.h"

void
iap_advanced_show(struct iap_wizard_advanced *adv)
{
  gtk_widget_show_all(adv->dialog);

  if (adv->current_page)
    gtk_notebook_set_current_page(adv->notebook, adv->current_page);
}

void
iap_advanced_destroy(struct iap_wizard_advanced *adv)
{
  gtk_widget_destroy(adv->dialog);
  g_hash_table_destroy(adv->widgets);
  g_free(adv->pages);
  g_free(adv);
}

static GtkWidget *
iap_advanced_gtk_entry_num_special_new()
{
  GtkWidget *entry = gtk_entry_new();

  hildon_gtk_entry_set_input_mode(GTK_ENTRY(entry),
                                  HILDON_GTK_INPUT_MODE_NUMERIC |
                                  HILDON_GTK_INPUT_MODE_SPECIAL);

  return entry;
}

static GtkWidget *
iap_advanced_gtk_entry_full_new()
{
  GtkWidget *entry = gtk_entry_new();

  hildon_gtk_entry_set_input_mode(GTK_ENTRY(entry), HILDON_GTK_INPUT_MODE_FULL);

  return entry;
}

static GtkWidget *
iap_advanced_hildon_number_editor_new()
{
  return hildon_number_editor_new(0, 65535);
}

static struct iap_advanced_widget iap_advanced_wizard_proxies_widgets[] =
{
  {
    NULL,
    "IPV4_USE_PROXY",
    NULL,
    NULL,
    "conn_set_iap_fi_adv_useproxies",
    gtk_check_button_new,
    1
  },
  {
    NULL,
    "IPV4_HTTP_HOST",
    "IPV4_USE_PROXY",
    "IPV4_AUTO_PROXY",
    "conn_set_iap_fi_adv_proxies_http",
    iap_advanced_gtk_entry_full_new,
    0
  },
  {
    NULL,
    "IPV4_HTTP_PORT",
    "IPV4_USE_PROXY",
    "IPV4_AUTO_PROXY",
    "conn_set_iap_fi_adv_proxies_port",
    iap_advanced_hildon_number_editor_new,
    0
  },
  {
    NULL,
    "IPV4_HTTPS_HOST",
    "IPV4_USE_PROXY",
    "IPV4_AUTO_PROXY",
    "conn_set_iap_fi_adv_proxies_https",
    iap_advanced_gtk_entry_full_new,
    0
  },
  {
    NULL,
    "IPV4_HTTPS_PORT",
    "IPV4_USE_PROXY",
    "IPV4_AUTO_PROXY",
    "conn_set_iap_fi_adv_proxies_port",
    iap_advanced_hildon_number_editor_new,
    0
  },
  {
    NULL,
    "IPV4_FTP_HOST",
    "IPV4_USE_PROXY",
    "IPV4_AUTO_PROXY",
    "conn_set_iap_fi_adv_proxies_ftp",
    iap_advanced_gtk_entry_full_new,
    0
  },
  {
    NULL,
    "IPV4_FTP_PORT",
    "IPV4_USE_PROXY",
    "IPV4_AUTO_PROXY",
    "conn_set_iap_fi_adv_proxies_port",
    iap_advanced_hildon_number_editor_new,
    0
  },
  {
    NULL,
    "IPV4_RTSP_HOST",
    "IPV4_USE_PROXY",
    "IPV4_AUTO_PROXY",
    "conn_set_iap_fi_adv_proxies_rtsp",
    iap_advanced_gtk_entry_full_new,
    0
  },
  {
    NULL,
    "IPV4_RTSP_PORT",
    "IPV4_USE_PROXY",
    "IPV4_AUTO_PROXY",
    "conn_set_iap_fi_adv_proxies_port",
    iap_advanced_hildon_number_editor_new,
    0
  },
  {
    NULL,
    "IPV4_OMIT_PROXY",
    "IPV4_USE_PROXY",
    "IPV4_AUTO_PROXY",
    "conn_set_iap_fi_adv_noproxyfor",
    iap_advanced_gtk_entry_full_new,
    0
  },
  {
    NULL,
    "IPV4_AUTO_PROXY",
    "IPV4_USE_PROXY",
    NULL,
    "conn_set_iap_fi_adv_use_autom",
    gtk_check_button_new,
    1
  },
  {
    NULL,
    "IPV4_PROXY_URL",
    "IPV4_AUTO_PROXY",
    NULL,
    "conn_set_iap_fi_adv_autom_url",
    iap_advanced_gtk_entry_full_new,
    0
  },
  { NULL, NULL, NULL, NULL, NULL, NULL, 0 }
};

static struct iap_advanced_widget iap_advanced_wizard_ip_widgets[] =
{
  {
    NULL,
    "IPV4_AUTO",
    NULL,
    NULL,
    "conn_set_iap_fi_adv_ip_autoretr",
    gtk_check_button_new,
    TRUE
  },
  {
    NULL,
    "IPV4_ADDRESS",
    NULL,
    "IPV4_AUTO",
    "conn_set_iap_fi_adv_ip_ip",
    iap_advanced_gtk_entry_num_special_new,
    FALSE
  },
  {
    NULL,
    "IPV4_NETMASK",
    NULL,
    "IPV4_AUTO",
    "conn_set_iap_fi_adv_ip_smask",
    iap_advanced_gtk_entry_num_special_new,
    FALSE
  },
  {
    NULL,
    "IPV4_GATEWAY",
    NULL,
    "IPV4_AUTO",
    "conn_set_iap_fi_adv_ip_router",
    iap_advanced_gtk_entry_num_special_new,
    FALSE
  },
  {
    NULL,
    "IPV4_AUTO_DNS",
    "IPV4_AUTO",
    NULL,
    "conn_set_iap_fi_adv_ip_val1",
    gtk_check_button_new,
    TRUE
  },
  {
    NULL,
    "IPV4_DNS1",
    NULL,
    "IPV4_AUTO_DNS",
    "conn_set_iap_fi_adv_ip_dns_prim",
    iap_advanced_gtk_entry_num_special_new,
    FALSE
  },
  {
    NULL,
    "IPV4_DNS2",
    NULL,
    "IPV4_AUTO_DNS",
    "conn_set_iap_fi_adv_ip_dns_sec",
    iap_advanced_gtk_entry_num_special_new,
    FALSE
  },
  { NULL, NULL, NULL, NULL, NULL, NULL, 0 }
};

static const char *advanced_ipv4_toggles[] =
{
  "IPV4_AUTO", "IPV4_AUTO", "IPV4_AUTO", "IPV4_AUTO_DNS", "IPV4_AUTO_DNS",
  NULL
};

static const char *advanced_ipv4_entries[] =
{
  "IPV4_ADDRESS", "IPV4_NETMASK", "IPV4_GATEWAY", "IPV4_DNS1", "IPV4_DNS2",
  NULL
};

static const char *advanced_ipv4_hosts[] =
{
  "IPV4_HTTP_HOST", "IPV4_HTTPS_HOST", "IPV4_FTP_HOST", "IPV4_RTSP_HOST",
  NULL
};

static void
iap_advanced_address_insert_text_cb(GtkEditable *editable, gchar *new_text,
                                    gint new_text_length, gpointer position,
                                    gpointer user_data)
{
  int i;

  for (i = 0; i < new_text_length; i++)
  {
    if ((unsigned char)(new_text[i] - '0') > 9)
      new_text[i] = '.';
  }
}

static gboolean
iap_advanced_host_changed_cb(GtkWidget *widget, struct iap_wizard_advanced *adv)
{
  const char *text;
  gchar **split;
  gchar **s;
  gboolean rv = FALSE;

  text = gtk_entry_get_text(GTK_ENTRY(widget));
  split = g_strsplit_set(text, ".", -1);

  if (strlen(text) > 255)
    goto out;

  s = split;

  while (*s)
  {
    size_t len = strlen(*s);

    if (len > 63 || *s[0] == '-' || *s[len - 1] == '-')
      goto out;

    s++;
  }

  rv = TRUE;

out:
  g_strfreev(split);
  gtk_dialog_set_response_sensitive(
        GTK_DIALOG(adv->dialog), GTK_RESPONSE_OK, rv);

  return rv;
}

static void
iap_advanced_check_proxies(struct iap_wizard_advanced *adv)
{
  gpointer use_proxy_widget =
      g_hash_table_lookup(adv->widgets, "IPV4_USE_PROXY");
  gpointer use_auto_proxy_widget =
      g_hash_table_lookup(adv->widgets, "IPV4_AUTO_PROXY");

  g_return_if_fail(use_proxy_widget != NULL && use_auto_proxy_widget != NULL);

  if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(use_proxy_widget)) ||
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(use_auto_proxy_widget)))
  {
    gtk_dialog_set_response_sensitive(GTK_DIALOG(adv->dialog),
                                      GTK_RESPONSE_OK, TRUE);
  }
  else
  {
    const char *host = advanced_ipv4_hosts[0];

    while (host)
    {
      gpointer widget = g_hash_table_lookup(adv->widgets, host);

      if (widget && !iap_advanced_host_changed_cb(GTK_WIDGET(widget), adv))
        break;

      host++;
    }
  }
}

static void
iap_advanced_check_page(struct iap_wizard_advanced *adv, int page)
{
  struct iap_advanced_widget *widget;
  gboolean sensitive;
  GtkWidget *w;

  if (page < 0)
    return;

  widget = adv->pages[page].widgets;

  if (!widget)
    return;

  iap_advanced_check_proxies(adv);

  while (widget->id)
  {
    if (widget->id && (widget->proxy_id || widget->auto_proxy_id))
    {
      GtkWidget *auto_proxy_button = NULL;
      GtkWidget *proxy_button = NULL;

      if (widget->proxy_id)
      {
        proxy_button =
            GTK_WIDGET(g_hash_table_lookup(adv->widgets, widget->proxy_id));
      }

      if (widget->auto_proxy_id)
      {
        auto_proxy_button =
            GTK_WIDGET(g_hash_table_lookup(adv->widgets,
                                           widget->auto_proxy_id));
      }

      sensitive = TRUE;

      if (proxy_button)
      {
        if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(proxy_button)) ||
            !GTK_WIDGET_SENSITIVE(proxy_button) ||
            !GTK_WIDGET_PARENT_SENSITIVE(proxy_button))
        {
          sensitive = FALSE;
        }
      }

      if (auto_proxy_button)
      {
        GtkToggleButton *button = GTK_TOGGLE_BUTTON(auto_proxy_button);

        if (gtk_toggle_button_get_active(button) &&
            GTK_WIDGET_SENSITIVE(auto_proxy_button) &&
            GTK_WIDGET_PARENT_SENSITIVE(auto_proxy_button))
        {
          sensitive = FALSE;
        }
      }

      w = GTK_WIDGET(g_hash_table_lookup(adv->widgets, widget->id));

      if (w)
        gtk_widget_set_sensitive(gtk_widget_get_parent(w), sensitive);
    }

    widget++;
  }
}

static void
iap_advanced_ipv4_auto_toggled_cb(GtkToggleButton *togglebutton,
                                  struct iap_wizard_advanced *user_data)
{
  gpointer auto_dns = g_hash_table_lookup(user_data->widgets, "IPV4_AUTO_DNS");

  gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(GTK_WIDGET(auto_dns)),
        gtk_toggle_button_get_active(togglebutton));
}

static void
iap_advanced_widget_toggled_cb(GtkToggleButton *togglebutton,
                               gpointer user_data)
{
  struct iap_wizard_advanced *adv = (struct iap_wizard_advanced *)user_data;

  iap_advanced_check_page(adv, gtk_notebook_get_current_page(adv->notebook));
}

static void
iap_advanced_notebook_switch_page_cb(GtkNotebook *notebook, gpointer page,
                                     guint page_num, gpointer user_data)
{
  struct iap_wizard_advanced *adv = (struct iap_wizard_advanced *)user_data;
  struct iap_advanced_page *p;

  iap_advanced_check_page(adv, page_num);

  p = &adv->pages[page_num];

  if (p->widgets)
  {
    if (p->activate)
      p->activate(p->priv);
  }
}

static GtkWidget *
iap_advanced_import_get_widget_cb(struct iap_wizard_advanced *adv,
                                  const gchar *id)
{
  if (adv->widgets)
    return g_hash_table_lookup(adv->widgets, id);

  return NULL;
}

void
iap_advanced_import(struct iap_wizard_advanced *adv, struct stage *s)
{
  gchar *proxytype;
  gchar *ipv4_type;
  GtkWidget *widget;

  adv->import_mode = 1;
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(adv->dialog)->vbox),
                    GTK_WIDGET(adv->notebook));

  proxytype = stage_get_string(s, "proxytype");

  if (proxytype)
  {
    if ( !strcmp(proxytype, "MANUAL") )
    {
      widget = GTK_WIDGET(g_hash_table_lookup(adv->widgets, "IPV4_USE_PROXY"));

      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
    }
    else if (!strcmp(proxytype, "AUTOCONF"))
    {
      widget = GTK_WIDGET(g_hash_table_lookup(adv->widgets, "IPV4_USE_PROXY"));
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);

      widget = GTK_WIDGET(g_hash_table_lookup(adv->widgets, "IPV4_AUTO_PROXY"));
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
    }
  }

  g_free(proxytype);

  ipv4_type = stage_get_string(s, "ipv4_type");
  widget = g_hash_table_lookup(adv->widgets, "IPV4_AUTO");

  if (GTK_IS_WIDGET(widget) && (!ipv4_type || !strcmp(ipv4_type, "AUTO")) )
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);

  g_free(ipv4_type);

  mapper_import_widgets(s, adv->sw,
                        (mapper_get_widget_fn)iap_advanced_import_get_widget_cb,
                        adv);
  adv->import_mode = 0;
}

void
iap_advanced_export(struct iap_wizard_advanced *adv, struct stage *s)
{
  gpointer widget;
  const gchar *proxytype;
  const gchar *ipv4_type;

  widget = g_hash_table_lookup(adv->widgets, "IPV4_USE_PROXY");

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(GTK_WIDGET(widget))))
  {
    widget = g_hash_table_lookup(adv->widgets, "IPV4_AUTO_PROXY");

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(GTK_WIDGET(widget))))
      proxytype = "AUTOCONF";
    else
      proxytype = "MANUAL";
  }
  else
  {
    proxytype = "NONE";
  }

  stage_set_string(s, "proxytype", proxytype);

  widget = g_hash_table_lookup(adv->widgets, "IPV4_AUTO");

  if (widget &&
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(GTK_WIDGET(widget))))
  {
    ipv4_type = "AUTO";
  }
  else
    ipv4_type = "STATIC";

  stage_set_string(s, "ipv4_type", ipv4_type);
  mapper_export_widgets(s, adv->sw,
                        (mapper_get_widget_fn)iap_advanced_import_get_widget_cb,
                        adv);
}

void
iap_advanced_save_state(struct iap_wizard_advanced *adv, GByteArray *array)
{
  struct stage s;
  guint8 current_page;

  current_page = gtk_notebook_get_current_page(adv->notebook);
  g_byte_array_append(array, &current_page, sizeof(current_page));
  stage_create_cache(&s, FALSE);
  iap_advanced_export(adv, &s);
  stage_dump_cache(&s, array);
  stage_free(&s);
}

gboolean
iap_advanced_restore_state(struct iap_wizard_advanced *adv,
                           struct stage_cache *data)
{
  gboolean rv;
  struct stage s;

  if (data->processed > data->len )
    return FALSE;

  adv->current_page = data->data[data->processed];
  data->processed++;
  stage_create_cache(&s, NULL);

  if ((rv = stage_restore_cache(&s, data)))
    iap_advanced_import(adv, &s);

  stage_free(&s);

  return rv;
}

static void
iap_advanced_dialog_response_cb(GtkDialog *dialog, gint response_id,
                                gpointer user_data)
{
  struct iap_wizard_advanced *adv = (struct iap_wizard_advanced *)user_data;

  if (response_id != GTK_RESPONSE_OK)
    return;

  if (GTK_IS_WIDGET(g_hash_table_lookup(adv->widgets, "IPV4_AUTO")))
  {
    int i = 0;

    while(advanced_ipv4_entries[i])
    {
      gpointer toggle = g_hash_table_lookup(adv->widgets, advanced_ipv4_toggles[i]);
      gpointer entry = g_hash_table_lookup(adv->widgets, advanced_ipv4_entries[i]);

      if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle)))
        gtk_entry_set_text(GTK_ENTRY(entry), "0.0.0.0");
      else
      {
        int ip1, ip2, ip3, ip4;
        char tmp;

        if (sscanf(gtk_entry_get_text(GTK_ENTRY(entry)), "%d.%d.%d.%d%c",
                   &ip1, &ip2, &ip3, &ip4, &tmp) != 4 ||
            ip1 < 0 || ip1 > 255 || ip2 < 0 || ip2 > 255 ||
            ip3 < 0 || ip3 > 255 || ip4 < 0 || ip4 > 255)
        {
          const char *msg =
              dgettext("osso-connectivity-ui", "conn_ib_enter_valid_ip");

          gtk_widget_grab_focus(GTK_WIDGET(entry));

          if (msg)
            hildon_banner_show_information(GTK_WIDGET(dialog), NULL, msg);

          g_signal_stop_emission_by_name(G_OBJECT(dialog), "response");
          return;
        }
      }

      i++;
    }
  }
}

static struct iap_advanced_page iap_advanced_pages[] =
{
  {
    TRUE,
    "conn_set_iap_ti_adv_proxies",
    iap_advanced_wizard_proxies_widgets,
    NULL,
    "Connectivity_Internetsettings_IAPsetupAdvancedproxies",
    NULL
  },
  {
    TRUE,
    "conn_set_iap_ti_adv_ip",
    iap_advanced_wizard_ip_widgets,
    NULL,
    "Connectivity_Internetsettings_IAPsetupAdvancedIPaddresses",
    NULL
  },
  {FALSE, NULL, NULL, NULL, NULL, NULL}
};

static gboolean
iap_advanced_address_key_press(GtkWidget *widget, GdkEventKey *event,
                               gpointer user_data)
{
  gboolean rv = FALSE;
  int i;
  gint n_entries;
  guint *keyvals = NULL;
  GdkKeymapKey *keys = NULL;

  if (!gdk_keymap_get_entries_for_keycode(gdk_keymap_get_default(),
                                          event->hardware_keycode, &keys,
                                          &keyvals, &n_entries))
  {
    return rv;
  }

  for (i = 0; i < n_entries; i++)
  {
    guint32 c = gdk_keyval_to_unicode(keyvals[i]);

    if (g_unichar_isdigit(c))
    {
      GtkEditable *editable = GTK_EDITABLE(user_data);
      gint position = gtk_editable_get_position(editable);
      gchar *name = gdk_keyval_name(keyvals[i]);

      gtk_editable_insert_text(editable, name, strlen(name), &position);
      gtk_editable_set_position(editable, position + 1);
      rv = TRUE;
      break;
    }
  }


  if (keys)
    g_free(keys);

  if (keyvals)
    g_free(keyvals);

  return rv;
}

static void
iap_advanced_host_insert_text_cb(GtkEditable *editable, gchar *new_text,
                                 gint new_text_length, gpointer position,
                                 gpointer user_data)
{
  int i;

  for (i = 0; i < new_text_length; i++)
  {
    gchar c = new_text[i];

    if (c == '?' || c == ',' || c == '=')
    {
      new_text[i] = '.';
      continue;
    }

    if ((c < 'a' || c > 'z') && (c < '0' || c > '9') && c != '-' && c != '.')
    {
      g_signal_stop_emission_by_name(G_OBJECT(editable), "insert_text");
      return;
    }
  }
}

struct iap_wizard_advanced *
iap_advanced_create(gpointer user_data, GtkWindow *parent,
                    struct iap_advanced_page *pages, struct stage_widget *sw,
                    struct stage *s)
{
  int adv_page_cnt;
  int plugin_page_cnt = 0;
  int i;
  struct iap_advanced_page *page;
  gpointer ipv4_auto;
  GtkNotebook *notebook;
  struct iap_wizard_advanced *adv = g_new0(struct iap_wizard_advanced, 1);
  GtkDialogFlags flags = GTK_DIALOG_NO_SEPARATOR;

  if (parent)
    flags |= (GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL);

  adv->user_data = user_data;
  adv->stage = s;
  adv->sw = sw;
  adv->widgets = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
  adv->dialog = gtk_dialog_new_with_buttons(
        dgettext("osso-connectivity-ui", "conn_set_iap_ti_adv"), parent, flags,
        dgettext("hildon-libs", "wdgt_bd_save"), GTK_RESPONSE_OK,
        NULL);

  iap_common_set_close_response(adv->dialog, GTK_RESPONSE_CANCEL);
  gtk_window_set_default_size(GTK_WINDOW(adv->dialog), 640, 270);

  adv->notebook = notebook = GTK_NOTEBOOK(gtk_notebook_new());
  gtk_notebook_set_show_tabs(notebook, TRUE);
  gtk_notebook_set_show_border(notebook, FALSE);

  for (adv_page_cnt = 0; iap_advanced_pages[adv_page_cnt].msgid;
       adv_page_cnt++);

  if (pages)
  {
    for (plugin_page_cnt = 0; pages[plugin_page_cnt].msgid;
         plugin_page_cnt++);
  }

  adv->pages = g_new0(struct iap_advanced_page,
                      adv_page_cnt +  plugin_page_cnt + 1);

  for (i = 0; i < adv_page_cnt; i++)
  {
    memcpy(&adv->pages[i], &iap_advanced_pages[i],
           sizeof(struct iap_advanced_page));
  }

  if (pages)
  {
    for (i = 0; i < plugin_page_cnt; i++)
    {
      memcpy(&adv->pages[i + adv_page_cnt], &pages[i],
             sizeof(struct iap_advanced_page));
    }
  }

  for (page = adv->pages; page->msgid; page++)
  {
    GtkWidget *note_page;

    if (page->widgets)
    {
      GtkSizeGroup *size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
      struct iap_advanced_widget *widget = page->widgets;

      note_page = gtk_vbox_new(0, 0);

      while (widget->id)
      {
        if (!widget->init || widget->init(adv->stage))
        {
          GtkWidget *w = widget->create();
          GtkWidget *caption;

          g_hash_table_insert(adv->widgets, g_strdup(widget->id), w);

          if (widget->is_toggle)
          {
            g_signal_connect(G_OBJECT(w), "toggled",
                             G_CALLBACK(iap_advanced_widget_toggled_cb), adv);
          }

          caption = hildon_caption_new(
                size_group, dgettext("osso-connectivity-ui", widget->msgid), w,
                NULL, HILDON_CAPTION_OPTIONAL);
          gtk_box_pack_start(GTK_BOX(note_page), caption, FALSE, FALSE, 0);
        }

        widget++;
      }

      g_object_unref(G_OBJECT(size_group));

      if (page->has_content)
      {
        GtkAdjustment *adj;
        GtkWidget *scrolled_window;
        GtkWidget *viewport = gtk_viewport_new(NULL, NULL);

        gtk_viewport_set_shadow_type(GTK_VIEWPORT(viewport), GTK_SHADOW_NONE);

        gtk_container_add(GTK_CONTAINER(viewport), note_page);
        scrolled_window = gtk_scrolled_window_new(NULL, NULL);

        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                       GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

        gtk_scrolled_window_set_shadow_type(
              GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_NONE);

        adj = gtk_scrolled_window_get_vadjustment(
              GTK_SCROLLED_WINDOW(scrolled_window));
        gtk_container_set_focus_vadjustment(
              GTK_CONTAINER(note_page), adj);
        note_page = scrolled_window;

        gtk_container_add(GTK_CONTAINER(scrolled_window), viewport);
      }
    }
    else
      note_page = gtk_label_new("Empty");

    gtk_notebook_append_page(
          notebook, note_page,
          gtk_label_new(dgettext("osso-connectivity-ui", page->msgid)));
  }

  g_signal_connect(G_OBJECT(adv->dialog), "response",
                   G_CALLBACK(iap_advanced_dialog_response_cb), adv);
  g_signal_connect(G_OBJECT(adv->notebook), "switch-page",
                   G_CALLBACK(iap_advanced_notebook_switch_page_cb), adv);

  ipv4_auto = GTK_WIDGET(g_hash_table_lookup(adv->widgets, "IPV4_AUTO"));

  if (ipv4_auto)
  {
    g_signal_connect(G_OBJECT(ipv4_auto), "toggled",
                     G_CALLBACK(iap_advanced_ipv4_auto_toggled_cb), adv);
  }

  for (i = 0; advanced_ipv4_hosts[i]; i++)
  {
    GtkWidget *host =
        GTK_WIDGET(g_hash_table_lookup(adv->widgets, advanced_ipv4_hosts[i]));

    if (host)
    {
      g_signal_connect(G_OBJECT(host), "insert_text",
                       G_CALLBACK(iap_advanced_host_insert_text_cb), NULL);
      g_signal_connect(G_OBJECT(host), "changed",
                       G_CALLBACK(iap_advanced_host_changed_cb), adv);
    }
  }

  for (i = 0; advanced_ipv4_entries[i]; i++)
  {
    GtkWidget *entry =
        GTK_WIDGET(g_hash_table_lookup(adv->widgets, advanced_ipv4_entries[i]));

    if (entry)
    {
      g_signal_connect(G_OBJECT(entry), "insert_text",
                       G_CALLBACK(iap_advanced_address_insert_text_cb), NULL);
      g_signal_connect(G_OBJECT(entry), "key-press-event",
                       G_CALLBACK(iap_advanced_address_key_press), entry);
    }
  }

  return adv;
}
