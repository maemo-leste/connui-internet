#include <hildon/hildon.h>

#include <string.h>
#include <libintl.h>

#include "advanced.h"

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

  iap_advanced_check_proxies(adv, gtk_notebook_get_current_page(adv->notebook));
}

static void
iap_advanced_notebook_switch_page_cb(GtkNotebook *notebook, gpointer page,
                                     guint page_num, gpointer user_data)
{
  struct iap_wizard_advanced *adv = (struct iap_wizard_advanced *)user_data;
  struct iap_advanced_page *p;

  iap_advanced_check_proxies(adv, page_num);

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

static const char *advanced_toggles[] =
{
  "IPV4_AUTO", "IPV4_AUTO", "IPV4_AUTO", "IPV4_AUTO_DNS", "IPV4_AUTO_DNS",
  NULL
};

static const char *advanced_entries[] =
{
  "IPV4_ADDRESS", "IPV4_NETMASK", "IPV4_GATEWAY", "IPV4_DNS1", "IPV4_DNS2",
  NULL
};

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

    while(advanced_entries[i])
    {
      gpointer toggle = g_hash_table_lookup(adv->widgets, advanced_toggles[i]);
      gpointer entry = g_hash_table_lookup(adv->widgets, advanced_entries[i]);

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
