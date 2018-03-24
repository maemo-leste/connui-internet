#include <hildon/hildon.h>

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
  g_free(adv->field_18);
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
