#include <hildon/hildon.h>
#include <osso-ic-gconf.h>
#include <connui/connui.h>
#include <connui/wlan-common.h>
#include <connui/connui-log.h>
#include <connui/connui-dbus.h>
#include <connui/wlancond.h>
#include <connui/wlancond-dbus.h>

#include <ctype.h>
#include <string.h>
#include <libintl.h>

#include "stage.h"
#include "widgets.h"
#include "mapper.h"

#include "easy-wlan.h"

#define WPA_SUPP_SERVICE "fi.w1.wpa_supplicant1"
#define WPA_SUPP_PATH "/fi/w1/wpa_supplicant1"
#define WPA_SUPP_IFACE "fi.w1.wpa_supplicant1"

#define WPA_SUPP_INTERFACE_IFACE WPA_SUPP_IFACE ".Interface"
#define WPA_SUPP_INTERFACES_PATH WPA_SUPP_PATH "/Interfaces"
#define WPA_SUPP_INTERFACES_PATH_0 WPA_SUPP_INTERFACES_PATH "/0"

#define WPA_SUPP_INTERFACE_SCAN_DONE_SIG "ScanDone"
#define WPA_SUPP_INTERFACE_BSS_ADDED_SIG "BSSAdded"
#define WPA_SUPP_INTERFACE_SCAN_REQ "Scan"
#define WPA_SUPP_INTERFACE_FLUSH_BSS_REQ "FlushBSS"

#define _(msgid) dgettext("osso-connectivity-ui", msgid)
#define IS_EMPTY(str) (!(str) || !*(str))

struct easy_wlan
{
  GtkWindow *parent;
  GtkWindow *window;
  gchar *network_id;
  gchar *iap_id;
  guint security;
  GHashTable *widgets;
  struct stage stage;
  gint min_width;
};

struct rsn
{
  gboolean wpa_psk : 1;
  gboolean wpa_ft_psk : 1;
  gboolean wpa_psk_sha256 : 1;
  gboolean wpa_eap : 1;
  gboolean wpa_ft_eap : 1;
  gboolean wpa_eap_sha256 : 1;
};

struct wpa
{
  gboolean wpa_psk : 1;
  gboolean wpa_eap : 1;
};

struct bss_info
{
  struct rsn rsn;
  struct wpa wpa;
  gboolean infrastructure : 1;
  gboolean privacy : 1;
};

struct ssid_info
{
  const gchar *ssid;
  guint cap_bits;
  gboolean scan_results_received;
  gboolean ssid_found;
  guint supplicant_timeout_id;
};

static GtkWidget *
iap_easy_wlan_get_widget(gpointer user_data, const gchar *id)
{
  struct easy_wlan *ewlan = (struct easy_wlan *)user_data;

  return GTK_WIDGET(g_hash_table_lookup(ewlan->widgets, id));
}

static void
iap_easy_wlan_h22_entry_activate_cb(HildonCaption *hildoncaption,
                                    gpointer user_data)
{
  GtkDialog *dialog = GTK_DIALOG(user_data);

  if (dialog)
    gtk_dialog_response(dialog, GTK_RESPONSE_OK);
}

static void
iap_easy_wlan_wpa_eap_leap_add_widgets_cb(GtkWidget *vbox,
                                          GtkSizeGroup *size_group,
                                          struct easy_wlan *ewlan)
{
  GtkWidget *entry;
  HildonGtkInputMode im;
  GtkWidget *caption;

  entry = iap_widgets_create_h22_entry();
  im = hildon_gtk_entry_get_input_mode(GTK_ENTRY(entry));
  im &= ~(HILDON_GTK_INPUT_MODE_AUTOCAP | HILDON_GTK_INPUT_MODE_DICTIONARY);
  hildon_gtk_entry_set_input_mode(GTK_ENTRY(entry), im);
  g_hash_table_insert(ewlan->widgets, g_strdup("EAP_USERNAME"), entry);

  caption = hildon_caption_new(size_group, _("conn_set_iap_fi_wlan_username"),
                               entry, NULL, HILDON_CAPTION_OPTIONAL);
  gtk_box_pack_start(GTK_BOX(vbox), caption, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(entry), "activate",
                   G_CALLBACK(iap_easy_wlan_h22_entry_activate_cb),
                   ewlan->window);

  if (!(ewlan->security & IAP_SECURITY_WPA_EAP_GTC))
  {
    entry = iap_widgets_create_h22_entry();
    im = hildon_gtk_entry_get_input_mode(GTK_ENTRY(entry));
    im &= ~HILDON_GTK_INPUT_MODE_AUTOCAP;
    im |= HILDON_GTK_INPUT_MODE_INVISIBLE;
    hildon_gtk_entry_set_input_mode(GTK_ENTRY(entry), im);
    g_signal_connect(G_OBJECT(entry), "activate",
                     G_CALLBACK(iap_easy_wlan_h22_entry_activate_cb),
                     ewlan->window);
    g_hash_table_insert(ewlan->widgets, g_strdup("EAP_PASSWORD"), entry);

    caption = hildon_caption_new(size_group, _("conn_set_iap_fi_wlan_password"),
                                 entry, NULL, HILDON_CAPTION_OPTIONAL);
    gtk_box_pack_start(GTK_BOX(vbox), caption, FALSE, FALSE, 0);
  }
}

static void
iap_easy_wlan_wpa_eap_type_done_cb(struct easy_wlan *ewlan)
{
  GtkWidget *eap_type;
  gint active;

  guint security[] = {IAP_SECURITY_WPA_EAP_PEAP,
                      IAP_SECURITY_WPA_EAP_TLS,
                      IAP_SECURITY_WPA_EAP_TTLS};

  eap_type = iap_easy_wlan_get_widget(ewlan, "EAP_TYPE");
  active = hildon_picker_button_get_active(HILDON_PICKER_BUTTON(eap_type));

  if (active >= 0)
    ewlan->security = security[active];
}

static void
iap_easy_wlan_wpa_eap_ttls_auth_done_cb(struct easy_wlan *ewlan)
{
  GtkWidget *eap_tunneled_type;
  gint active;
  guint security[] = {IAP_SECURITY_WPA_EAP_GTC,
                      IAP_SECURITY_WPA_EAP_MS,
                      IAP_SECURITY_WPA_EAP_TTLS_MS,
                      IAP_SECURITY_WPA_EAP_TTLS_PAP};

  eap_tunneled_type = iap_easy_wlan_get_widget(ewlan, "EAP_TUNNELED_TYPE");
  active =
      hildon_picker_button_get_active(HILDON_PICKER_BUTTON(eap_tunneled_type));

  if (active >= 0)
    ewlan->security |= security[active];
}

static void
iap_easy_wlan_wpa_eap_tls_auth_add_widgets_cb(GtkWidget *vbox,
                                              GtkSizeGroup *size_group,
                                              struct easy_wlan *ewlan)
{
  GtkWidget *button;

  button = iap_widgets_create_certificate_picker_button(
        _("conn_set_iap_fi_wlan_sel_cert"));
  g_hash_table_insert(ewlan->widgets, g_strdup("EAP_CERTIFICATE"), button);
  ewlan->min_width = 450;

  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
}

static void
iap_easy_wlan_wpa_eap_ttls_auth_add_widgets_cb(GtkWidget *vbox,
                                               GtkSizeGroup *size_group,
                                               struct easy_wlan *ewlan)
{
  GtkWidget *cert_picker;
  GtkWidget *method_button;
  GtkSizeGroup *title_grp;
  GtkSizeGroup *value_grp;

  title_grp = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
  value_grp = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

  cert_picker = iap_widgets_create_certificate_picker_button(
        _("conn_set_iap_fi_wlan_sel_cert"));
  hildon_button_add_title_size_group(HILDON_BUTTON(cert_picker), title_grp);
  hildon_button_add_value_size_group(HILDON_BUTTON(cert_picker), value_grp);
  g_hash_table_insert(ewlan->widgets, g_strdup("EAP_CERTIFICATE"), cert_picker);

  gtk_box_pack_start(GTK_BOX(vbox), cert_picker, FALSE, FALSE, 0);

  if (ewlan->security & IAP_SECURITY_WPA_EAP_TTLS)
  {
    const gchar *eap_peap;
    GConfClient *gconf = gconf_client_get_default();
    g_object_unref(gconf);

    eap_peap = "EAP PAP";

    method_button = iap_widgets_create_static_picker_button(
          _("conn_set_iap_fi_wlan_ttls_meth"),
          _("conn_set_iap_fi_wlan_ttls_meth_gtc"),
          _("conn_set_iap_fi_wlan_ttls_meth_mschapv2"),
          _("conn_set_iap_fi_wlan_ttls_meth_mschapv2_no_eap"), eap_peap,
          NULL);
  }
  else
  {
    method_button = iap_widgets_create_static_picker_button(
          _("conn_set_iap_fi_wlan_peap_meth"),
          _("conn_set_iap_fi_wlan_peap_meth_gtc"),
          _("conn_set_iap_fi_wlan_peap_meth_mschapv2"),
          NULL);
  }

  hildon_button_add_title_size_group(HILDON_BUTTON(method_button), title_grp);
  hildon_button_add_value_size_group(HILDON_BUTTON(method_button), value_grp);

  g_hash_table_insert(ewlan->widgets, g_strdup("EAP_TUNNELED_TYPE"),
                      method_button);
  ewlan->min_width = 470;
  g_object_unref(G_OBJECT(title_grp));
  g_object_unref(G_OBJECT(value_grp));
  gtk_box_pack_start(GTK_BOX(vbox), method_button, FALSE, FALSE, 0);
}

static void
iap_easy_wlan_wpa_eap_type_add_widgets_cb(GtkWidget *vbox,
                                          GtkSizeGroup *size_group,
                                          struct easy_wlan *ewlan)
{
  GtkWidget *button;

  button = iap_widgets_create_static_picker_button(
        _("conn_set_iap_fi_wlan_wpa_eap_type_txt"),
        _("conn_set_iap_fi_wlan_wpa_eap_type_peap"),
        _("conn_set_iap_fi_wlan_wpa_eap_type_tls"),
        _("conn_set_iap_fi_wlan_wpa_eap_type_ttls"),
        NULL);
  hildon_button_add_title_size_group(HILDON_BUTTON(button), size_group);
  g_hash_table_insert(ewlan->widgets, g_strdup("EAP_TYPE"), button);
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
  ewlan->min_width = 360;
}

static void
iap_easy_wlan_ent_wpa_psk_add_widgets_cb(GtkWidget *vbox,
                                         GtkSizeGroup *size_group,
                                         struct easy_wlan *ewlan)
{
  GtkWidget *entry;
  HildonGtkInputMode im;
  GtkWidget *caption;

  entry = iap_widgets_create_h22_entry();
  im = hildon_gtk_entry_get_input_mode(GTK_ENTRY(entry));
  im &= ~(HILDON_GTK_INPUT_MODE_AUTOCAP | HILDON_GTK_INPUT_MODE_DICTIONARY);
  hildon_gtk_entry_set_input_mode(GTK_ENTRY(entry), im);
  g_object_set(entry, "max-length", 63, NULL);
  g_signal_connect(G_OBJECT(entry), "insert_text",
                   G_CALLBACK(iap_widgets_insert_only_ascii_text),
                   ewlan->window);
  g_signal_connect(G_OBJECT(entry), "insert-text",
                   G_CALLBACK(iap_widgets_insert_text_no_8bit_maxval_reach),
                   ewlan->window);
  g_signal_connect(G_OBJECT(entry), "activate",
                   G_CALLBACK(iap_easy_wlan_h22_entry_activate_cb),
                   ewlan->window);
  g_hash_table_insert(ewlan->widgets, g_strdup("WPA_PSK_KEY"), entry);
  ewlan->min_width = 540;
  caption = hildon_caption_new(size_group,
                               _("conn_set_iap_fi_wlan_wpa_psk_txt"), entry,
                               NULL, HILDON_CAPTION_OPTIONAL);
  gtk_box_pack_start(GTK_BOX(vbox), caption, FALSE, FALSE, 0);
}

static void
iap_easy_wlan_wepkey_add_widgets_cb(GtkWidget *vbox,
                                    GtkSizeGroup *size_group,
                                    struct easy_wlan *ewlan)
{
  GtkWidget *entry;
  HildonGtkInputMode im;
  GtkWidget *caption;

  entry = iap_widgets_create_h22_entry();
  g_object_set(entry, "max-length", 26, NULL);
  im = hildon_gtk_entry_get_input_mode(GTK_ENTRY(entry));
  im &= ~(HILDON_GTK_INPUT_MODE_AUTOCAP | HILDON_GTK_INPUT_MODE_DICTIONARY);
  hildon_gtk_entry_set_input_mode(GTK_ENTRY(entry), im);
  g_signal_connect(G_OBJECT(entry), "insert_text",
                   G_CALLBACK(iap_widgets_insert_only_ascii_text),
                   ewlan->window);
  g_signal_connect(G_OBJECT(entry), "insert_text",
                   G_CALLBACK(iap_widgets_insert_text_maxval_reach),
                   ewlan->window);
  g_signal_connect(G_OBJECT(entry), "activate",
                   G_CALLBACK(iap_easy_wlan_h22_entry_activate_cb),
                   ewlan->window);

  g_hash_table_insert(ewlan->widgets, g_strdup("WEP_KEY1"), entry);
  caption = hildon_caption_new(size_group, _("conn_set_iap_fi_wlan_wepkey"),
                               entry, NULL, HILDON_CAPTION_OPTIONAL);
  gtk_box_pack_start(GTK_BOX(vbox), caption, FALSE, FALSE, 0);
}

static void
iap_easy_wlan_ent_wpa_psk_verify_response_cb(GtkDialog *dialog,
                                             gint response_id,
                                             gpointer user_data)
{
  struct easy_wlan *ewlan = (struct easy_wlan *)user_data;
  GtkWidget *widget;

  if (response_id == GTK_RESPONSE_OK)
  {
    widget = iap_easy_wlan_get_widget(ewlan, "WPA_PSK_KEY");

    if (strlen(iap_widgets_h22_entry_get_text(widget)) <= 7)
    {
      hildon_banner_show_information(GTK_WIDGET(ewlan->window), NULL,
                                     _("conn_ib_min8val_req"));
      g_signal_stop_emission_by_name(G_OBJECT(dialog), "response");
    }
  }
}

static void
iap_easy_wlan_wepkey_verify_response_cb(GtkDialog *dialog, gint response_id,
                                        gpointer user_data)
{
  struct easy_wlan *ewlan = (struct easy_wlan *)user_data;

  if (response_id == GTK_RESPONSE_OK)
  {
    GtkWidget *widget = iap_easy_wlan_get_widget(ewlan, "WEP_KEY1");
    const gchar *wep_key = iap_widgets_h22_entry_get_text(widget);
    int len = strlen(wep_key);
    const char *msg;

    if (len == WLANCOND_MIN_KEY_LEN || len == WLANCOND_MAX_KEY_LEN)
      return;

    {
      if (len == 2 * WLANCOND_MIN_KEY_LEN || len == 2 * WLANCOND_MAX_KEY_LEN)
      {
        int i;

        for (i = 0; i < len; i++)
        {
          if (!isxdigit(wep_key[i]))
            break;
        }

        if (i == len)
          return;

        msg = _("conn_ib_wepkey_invalid_characters");
      }
      else
        msg = _("conn_ib_wepkey_invalid_length");

      hildon_banner_show_information(GTK_WIDGET(ewlan->window), NULL, msg);
      g_signal_stop_emission_by_name(G_OBJECT(dialog), "response");
    }
  }
}

typedef void (*iap_easy_wlan_response_fn)(GtkDialog *, gint, gpointer);
typedef void (*iap_easy_wlan_add_widgets_fn)(GtkWidget *, GtkSizeGroup *, struct easy_wlan *);

static gint
iap_easy_wlan_dialog_run(struct easy_wlan *ewlan, const gchar *title,
                         iap_easy_wlan_response_fn response_cb,
                         iap_easy_wlan_add_widgets_fn add_widgets,
                         void (*done_cb)(struct easy_wlan *),
                         struct stage_widget *sw)
{
  GtkDialogFlags flags = GTK_DIALOG_NO_SEPARATOR;
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkSizeGroup *size_group;
  GtkWidget *hbox;
  GtkWidget *label;
  gint resp_id;

  if (ewlan->parent)
    flags |= (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT);

  dialog = hildon_dialog_new_with_buttons(title, ewlan->parent, flags,
                                          dgettext("hildon-libs",
                                                   "wdgt_bd_done"),
                                          GTK_RESPONSE_OK, NULL);
  ewlan->window = GTK_WINDOW(dialog);
  iap_common_set_close_response(dialog, GTK_RESPONSE_CANCEL);

  vbox = GTK_DIALOG(dialog)->vbox;
  size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
  hbox = gtk_hbox_new(0, 16);

  label = gtk_label_new(_("conn_set_iap_fi_wlan_network"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.0);
  gtk_size_group_add_widget(size_group, label);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

  label = gtk_label_new(ewlan->network_id);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  add_widgets(vbox, size_group, ewlan);

  g_object_unref(G_OBJECT(size_group));

  if (ewlan->min_width > 0)
  {
    GdkGeometry geometry;

    geometry.min_width = ewlan->min_width;
    geometry.min_height = -1;
    gtk_window_set_geometry_hints(GTK_WINDOW(dialog), dialog, &geometry,
                                  GDK_HINT_MIN_SIZE);
    ewlan->min_width = 0;
  }

  gtk_widget_show_all(dialog);

  if (response_cb)
  {
    g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(response_cb),
                     ewlan);
  }

  resp_id = gtk_dialog_run(GTK_DIALOG(dialog));

  if (resp_id == GTK_RESPONSE_YES || resp_id == GTK_RESPONSE_OK)
  {
    if (done_cb)
      done_cb(ewlan);

    if (sw)
      mapper_export_widgets(&ewlan->stage, sw, iap_easy_wlan_get_widget, ewlan);
  }

  gtk_widget_destroy(dialog);

  return resp_id;
}

guint
iap_security_from_wlan_security(guint wlancond_capability)
{
  guint iap_security;

  if (wlancond_capability & WLANCOND_ADHOC)
    iap_security = IAP_TYPE_ADHOC;
  else
    iap_security = IAP_TYPE_INFRA;

  if (wlancond_capability & WLANCOND_WPA_EAP)
    return iap_security | IAP_SECURITY_WPA_EAP_UNKNOWN;

  if (wlancond_capability & WLANCOND_WPA_PSK)
    return iap_security | IAP_SECURITY_WPA_PSK;

  if (wlancond_capability & WLANCOND_WEP)
    return iap_security | IAP_SECURITY_WEP;

  return iap_security | IAP_SECURITY_NONE;
}

static const gint eap_types[] = {254, 0, 55, 42, 0, 0, 0, 1, -1};

static struct stage_widget iap_easy_wlan_wepkey_widgets[] =
{
  {
    NULL,
    NULL,
    "WEP_KEY1",
    "wlan_wepkey1",
    NULL,
    &mapper_entry2string,
    NULL
  },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

static struct stage_widget iap_easy_wlan_ent_wpa_psk_widgets[] =
{
  {
    NULL,
    NULL,
    "WPA_PSK_KEY",
    "EAP_wpa_preshared_passphrase",
    NULL,
    &mapper_entry2string,
    NULL
  },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

static gint EAP_default_type[] = {EAP_PEAP, EAP_TLS, EAP_TTLS, -1};

static struct stage_widget iap_easy_wlan_wpa_eap_type_widgets[] =
{
  {
    NULL,
    NULL,
    "EAP_TYPE",
    "EAP_default_type",
    NULL,
    &mapper_combo2int,
    EAP_default_type
  },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

static struct stage_widget iap_easy_wlan_wpa_eap_tls_auth_widgets[] =
{
  {
    NULL,
    NULL,
    "EAP_CERTIFICATE",
    "EAP_TLS_PEAP_client_certificate_file",
    NULL,
    &mapper_combo2string,
    GUINT_TO_POINTER(1)
  },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

static gint PEAP_tunneled_eap_type[] =
{
  EAP_GTC,
  EAP_MS,
  EAP_TTLS_MS,
  EAP_TTLS_PAP,
  -1
};

static struct stage_widget iap_easy_wlan_wpa_eap_ttls_auth_widgets[] =
{
  {
    NULL,
    NULL,
    "EAP_CERTIFICATE",
    "EAP_TLS_PEAP_client_certificate_file",
    NULL,
    &mapper_combo2string,
    GUINT_TO_POINTER(1)
  },
  {
    NULL,
    NULL,
    "EAP_TUNNELED_TYPE",
    "PEAP_tunneled_eap_type",
    NULL,
    &mapper_combo2int,
    PEAP_tunneled_eap_type
  }
};

static gboolean
EAP_MSCHAPV2_validate(const struct stage *s, const gchar *name,
                      const gchar *key)
{
  int eap_type = stage_get_int(s, "PEAP_tunneled_eap_type");

  return
      eap_type == EAP_TTLS_MS || eap_type == EAP_MS || eap_type == EAP_TTLS_PAP;
}

static gboolean
EAP_GTC_validate(const struct stage *s,const gchar *name, const gchar *key)
{
  return stage_get_int(s, "PEAP_tunneled_eap_type") == EAP_GTC;
}

static struct stage_widget iap_easy_wlan_wpa_eap_leap_widgets[] =
{
  {
    NULL,
    EAP_MSCHAPV2_validate,
    "EAP_USERNAME",
    "EAP_MSCHAPV2_username",
    NULL,
    &mapper_entry2string,
    NULL
  },
  {
    NULL,
    EAP_MSCHAPV2_validate,
    "EAP_PASSWORD",
    "EAP_MSCHAPV2_password",
    NULL,
    &mapper_entry2string,
    NULL
  },
  {
    NULL,
    EAP_GTC_validate,
    "EAP_USERNAME",
    "EAP_GTC_identity",
    NULL,
    &mapper_entry2string,
    NULL
  },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

static gint
iap_run_easy_wlan_get_security(struct easy_wlan *ewlan,
                               guint wlancond_capability)
{
  gint resp_id = GTK_RESPONSE_OK;
  const gchar *wlan_security;

  if (wlancond_capability & WLANCOND_WPS_MASK)
  {
    GSList *l = NULL;
    GConfValue *val = gconf_value_new(GCONF_VALUE_LIST);
    int i = 0;

    while (eap_types[i] != -1)
    {
      GConfValue *v = gconf_value_new(GCONF_VALUE_INT);

      gconf_value_set_int(v, eap_types[i]);
      l = g_slist_append(l, v);
      i++;
    };

    gconf_value_set_list_type(val, GCONF_VALUE_INT);
    gconf_value_set_list_nocopy(val, l);
    stage_set_val(&ewlan->stage, "EAP_default_type", val);
    wlan_security = "WPA_EAP";
  }
  else if (ewlan->security == IAP_SECURITY_NONE)
    wlan_security = "NONE";
  else if (ewlan->security == IAP_SECURITY_WPA_EAP_UNKNOWN)
  {
    resp_id =
        iap_easy_wlan_dialog_run(ewlan,
                                 _("conn_set_iap_ti_wlan_sel_wpa_eap_type"),
                                 NULL,
                                 iap_easy_wlan_wpa_eap_type_add_widgets_cb,
                                 iap_easy_wlan_wpa_eap_type_done_cb,
                                 iap_easy_wlan_wpa_eap_type_widgets);
    wlan_security = "WPA_EAP";
  }
  else if (ewlan->security == IAP_SECURITY_WEP)
  {
    resp_id =
        iap_easy_wlan_dialog_run(ewlan,
                                 _("conn_set_iap_ti_wlan_ent_wepkey"),
                                 iap_easy_wlan_wepkey_verify_response_cb,
                                 iap_easy_wlan_wepkey_add_widgets_cb, NULL,
                                 iap_easy_wlan_wepkey_widgets);
    wlan_security = "WEP";
  }
  else if (ewlan->security == IAP_SECURITY_WPA_PSK)
  {
    resp_id =
        iap_easy_wlan_dialog_run(ewlan,
                                 _("conn_set_iap_ti_wlan_ent_wpa_psk"),
                                 iap_easy_wlan_ent_wpa_psk_verify_response_cb,
                                 iap_easy_wlan_ent_wpa_psk_add_widgets_cb,
                                 NULL,
                                 iap_easy_wlan_ent_wpa_psk_widgets);
    wlan_security = "WPA_PSK";
  }
  else
  {
    CONNUI_ERR("invalid security mode! %d", ewlan->security);
    resp_id = GTK_RESPONSE_CANCEL;
  }

  if (resp_id != GTK_RESPONSE_CANCEL)
    stage_set_string(&ewlan->stage, "wlan_security", wlan_security);

  return resp_id;
}

static gint
iap_run_easy_wlan_get_auth(struct easy_wlan *ewlan)
{
  gint resp_id = GTK_RESPONSE_OK;

  if (ewlan->security == IAP_SECURITY_WPA_EAP_TTLS ||
      ewlan->security == IAP_SECURITY_WPA_EAP_PEAP)
  {
    const char *msgeap =
        ewlan->security == IAP_SECURITY_WPA_EAP_TTLS ?
          "conn_set_iap_ti_wlan_wpa_eap_ttls_auth" :
          "conn_set_iap_ti_wlan_wpa_eap_peap_auth";

    resp_id = iap_easy_wlan_dialog_run(
          ewlan, _(msgeap), 0, iap_easy_wlan_wpa_eap_ttls_auth_add_widgets_cb,
          iap_easy_wlan_wpa_eap_ttls_auth_done_cb,
          iap_easy_wlan_wpa_eap_ttls_auth_widgets);
  }
  else if (ewlan->security == IAP_SECURITY_WPA_EAP_TLS)
  {
    resp_id = iap_easy_wlan_dialog_run(
          ewlan, _("conn_set_iap_ti_wlan_wpa_eap_tls_auth"), NULL,
          iap_easy_wlan_wpa_eap_tls_auth_add_widgets_cb, NULL,
          iap_easy_wlan_wpa_eap_tls_auth_widgets);
  }
  else if (ewlan->security &
           (IAP_SECURITY_WPA_EAP_TTLS_PAP | IAP_SECURITY_WPA_EAP_TTLS_MS |
            IAP_SECURITY_WPA_EAP_MS | IAP_SECURITY_WPA_EAP_GTC))
  {
    resp_id = iap_easy_wlan_dialog_run(
          ewlan, ewlan->security & IAP_SECURITY_WPA_EAP_GTC ?
            _("conn_set_iap_ti_wlan_wpa_eap_leap") :
            _("conn_set_iap_ti_wlan_wpa_eap_leap2"),
          NULL,
          iap_easy_wlan_wpa_eap_leap_add_widgets_cb, NULL,
          iap_easy_wlan_wpa_eap_leap_widgets);
  }

  return resp_id;
}

gchar *
iap_run_easy_wlan_dialogs(osso_context_t *libosso, GtkWindow *parent,
                          const gchar *network_id, guint *wlancond_capability)
{
  guint iap_security;
  const gchar *ssid;
  gint resp_id;
  gboolean is_hidden;
  struct easy_wlan ewlan;
  gchar *hidden_ssid = NULL;

  g_return_val_if_fail(wlancond_capability != NULL, NULL);

  memset(&ewlan, 0, sizeof(ewlan));

  if (IS_EMPTY(network_id))
  {
    if (iap_hidden_ssid_dialog(parent, &hidden_ssid, wlancond_capability) ||
        !*wlancond_capability)
    {
      is_hidden = TRUE;
      ewlan.network_id = g_strdup(hidden_ssid);
    }
    else
      return NULL;
  }
  else
  {
    is_hidden = FALSE;
    ewlan.network_id = g_strdup(network_id);
  }

  iap_security = iap_security_from_wlan_security(*wlancond_capability);

  ewlan.parent = parent;
  ewlan.security = iap_security & IAP_SECURITY_MASK;
  ewlan.widgets = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
  wlan_common_mangle_ssid(ewlan.network_id, strlen(ewlan.network_id));
  stage_create_cache(&ewlan.stage, NULL);

  if (iap_security & IAP_TYPE_ADHOC)
    stage_set_string(&ewlan.stage, "type", "WLAN_ADHOC");
  else
    stage_set_string(&ewlan.stage, "type", "WLAN_INFRA");

  if (!hidden_ssid)
    ssid = network_id;
  else
    ssid = hidden_ssid;

  stage_set_bytearray(&ewlan.stage, "wlan_ssid", ssid);
  stage_set_bool(&ewlan.stage, "wlan_hidden", is_hidden);
  stage_set_string(&ewlan.stage, "ipv4_type", "AUTO");
  stage_set_string(&ewlan.stage, "proxytype", "NONE");
  stage_set_bool(&ewlan.stage, "temporary", TRUE);
  g_free(hidden_ssid);

  resp_id = iap_run_easy_wlan_get_security(&ewlan, *wlancond_capability);

  if (resp_id != GTK_RESPONSE_CANCEL)
    resp_id = iap_run_easy_wlan_get_auth(&ewlan);

  if (resp_id != GTK_RESPONSE_CANCEL)
  {
    struct stage s;

    ewlan.iap_id = iap_settings_create_iap_id();
    stage_create_for_iap(&s, ewlan.iap_id);
    stage_copy(&ewlan.stage, &s);
    stage_free(&s);
  }

  stage_free(&ewlan.stage);
  g_free(ewlan.network_id);
  g_hash_table_destroy(ewlan.widgets);

  if (resp_id != GTK_RESPONSE_CANCEL)
    return ewlan.iap_id;

  g_free(ewlan.iap_id);

  return NULL;
}

static void
iap_hidden_ssid_dialog_entry_activate_cb(GtkEntry *entry, gpointer user_data)
{
  if (user_data)
    gtk_dialog_response(GTK_DIALOG(user_data), GTK_RESPONSE_OK);
}

#if 0
static guint
get_wlan_tx_power()
{
  guint power;
  GConfClient *gconf = gconf_client_get_default();
  GError *err = NULL;

  if (gconf)
  {
    power = gconf_client_get_int(gconf, ICD_GCONF_PATH "/wlan_tx_power", &err);

    if (err)
    {
      CONNUI_ERR("Error reading TX power: %s", err->message);
      g_clear_error(&err);
    }

    g_object_unref(G_OBJECT(gconf));

    if (power > 0)
      return power;
  }
  else
    CONNUI_ERR("Unable to get GConfClient for reading TX power");

  return 8;
}
#endif

static gboolean
scan_timeout_cb(gpointer user_data)
{
  ((struct ssid_info *)user_data)->supplicant_timeout_id = 0;

  CONNUI_ERR("Timed out waiting for scan results");

  return FALSE;
}

static gboolean
scan_req_append_type(DBusMessageIter *dict, const char *value)
{
  DBusMessageIter entry, variant;
  const char *key = "Type";

  if (!dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY,
                                        NULL,
                                        &entry))
  {
    return FALSE;
  }

  if (!dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key))
    return FALSE;

  if (!dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT,
                                        DBUS_TYPE_STRING_AS_STRING,
                                        &variant))
  {
    return FALSE;
  }

  if (!dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &value))
    return FALSE;

  return
      dbus_message_iter_close_container(&entry, &variant) &&
      dbus_message_iter_close_container(dict, &entry);
}

static gboolean
scan_req_append_ssid(DBusMessageIter *dict, const char *ssid)
{
  DBusMessageIter entry, variant, array, elem;
  const char *key = "SSIDs";

  if (!dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY,
                                        NULL,
                                        &entry))
  {
    return FALSE;
  }

  if (!dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key))
    return FALSE;

  if (!dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT,
                                        DBUS_TYPE_ARRAY_AS_STRING
                                        DBUS_TYPE_ARRAY_AS_STRING
                                        DBUS_TYPE_BYTE_AS_STRING,
                                        &variant))
  {
    return FALSE;
  }

  if (!dbus_message_iter_open_container(&variant, DBUS_TYPE_ARRAY,
                                        DBUS_TYPE_ARRAY_AS_STRING
                                        DBUS_TYPE_BYTE_AS_STRING,
                                        &array))
  {
    return FALSE;
  }

  if (!dbus_message_iter_open_container(&array, DBUS_TYPE_ARRAY,
                                        DBUS_TYPE_BYTE_AS_STRING,
                                        &elem))
  {
    return FALSE;
  }

  if (!dbus_message_iter_append_fixed_array(&elem, DBUS_TYPE_BYTE,
                                            &ssid, strlen(ssid)))
  {
    return FALSE;
  }


  return
      dbus_message_iter_close_container(&array, &elem) &&
      dbus_message_iter_close_container(&variant, &array) &&
      dbus_message_iter_close_container(&entry, &variant) &&
      dbus_message_iter_close_container(dict, &entry);

}

static gboolean
create_scan_req(DBusMessage *mcall, const char *ssid)
{
  DBusMessageIter array;
  DBusMessageIter dict;

  dbus_message_iter_init_append(mcall, &array);

  if (!dbus_message_iter_open_container(&array, DBUS_TYPE_ARRAY,
                                        DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                                        DBUS_TYPE_STRING_AS_STRING
                                        DBUS_TYPE_VARIANT_AS_STRING
                                        DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
                                        &dict))
  {
    return FALSE;
  }

  return
      scan_req_append_type(&dict, "active") &&
      scan_req_append_ssid(&dict, ssid) &&
      dbus_message_iter_close_container(&array, &dict);
}

static guint
get_cap_bits(struct bss_info *info)
{
  gboolean is_wpa2_psk = info->rsn.wpa_psk ||
                         info->rsn.wpa_ft_psk ||
                         info->rsn.wpa_psk_sha256;
  gboolean is_wpa_psk =  info->wpa.wpa_psk;
  gboolean is_wpa2_eap = info->rsn.wpa_eap ||
                         info->rsn.wpa_ft_eap ||
                         info->rsn.wpa_eap_sha256;
  guint cap = 0;

  if (info->infrastructure)
    cap |= WLANCOND_INFRA;
  else
    cap |= WLANCOND_ADHOC;

  if (info->wpa.wpa_eap || is_wpa2_eap)
    cap |= WLANCOND_WPA_EAP;
  else if (is_wpa_psk || is_wpa2_psk)
    cap |= WLANCOND_WPA_PSK;
  else if (info->privacy)
    cap |= WLANCOND_WEP;
  else
    cap |= WLANCOND_OPEN;

  if (is_wpa2_eap || is_wpa2_psk)
    cap |= WLANCOND_WPA2;

  return cap;
}

static gboolean
process_wpa(DBusMessageIter *entry, struct wpa *wpa)
{
  DBusMessageIter array;
  DBusMessageIter dict;

  dbus_message_iter_recurse(entry, &array);

  if (dbus_message_iter_get_arg_type(&array) != DBUS_TYPE_ARRAY ||
      dbus_message_iter_get_element_type(&array) != DBUS_TYPE_DICT_ENTRY)
  {
    return FALSE;
  }

  dbus_message_iter_recurse(&array, &dict);

  do
  {
    if (dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_DICT_ENTRY)
    {
      DBusMessageIter entry;

      dbus_message_iter_recurse(&dict, &entry);

      if (dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_STRING)
      {
        const char *key;

        dbus_message_iter_get_basic(&entry, &key);

        if (!strcmp(key, "KeyMgmt"))
        {
          DBusMessageIter val;

          dbus_message_iter_next(&entry);
          dbus_message_iter_recurse(&entry, &val);

          if (dbus_message_iter_get_arg_type(&val) == DBUS_TYPE_ARRAY &&
              dbus_message_iter_get_element_type(&val) == DBUS_TYPE_STRING)
          {
            DBusMessageIter strings;

            dbus_message_iter_recurse(&val, &strings);

            do
            {
              if (dbus_message_iter_get_arg_type(&strings) == DBUS_TYPE_STRING)
              {
                const char *keymgmt;

                dbus_message_iter_get_basic(&strings, &keymgmt);

                if (!strcmp(keymgmt, "wpa-psk"))
                  wpa->wpa_psk = TRUE;
                else if (!strcmp(keymgmt, "wpa-eap"))
                  wpa->wpa_eap = TRUE;
              }
            }
            while (dbus_message_iter_next(&strings));
          }
        }
      }
    }
  }
  while (dbus_message_iter_next(&dict));

  return TRUE;
}

static gboolean
process_rsn(DBusMessageIter *entry, struct rsn *rsn)
{
  DBusMessageIter array;
  DBusMessageIter dict;

  dbus_message_iter_recurse(entry, &array);

  if (dbus_message_iter_get_arg_type(&array) != DBUS_TYPE_ARRAY ||
      dbus_message_iter_get_element_type(&array) != DBUS_TYPE_DICT_ENTRY)
  {
    return FALSE;
  }

  dbus_message_iter_recurse(&array, &dict);

  do
  {
    if (dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_DICT_ENTRY)
    {
      DBusMessageIter entry;

      dbus_message_iter_recurse(&dict, &entry);

      if (dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_STRING)
      {
        const char *key;

        dbus_message_iter_get_basic(&entry, &key);

        if (!strcmp(key, "KeyMgmt"))
        {
          DBusMessageIter val;

          dbus_message_iter_next(&entry);
          dbus_message_iter_recurse(&entry, &val);

          if (dbus_message_iter_get_arg_type(&val) == DBUS_TYPE_ARRAY &&
              dbus_message_iter_get_element_type(&val) == DBUS_TYPE_STRING)
          {
            DBusMessageIter strings;

            dbus_message_iter_recurse(&val, &strings);

            do
            {
              if (dbus_message_iter_get_arg_type(&strings) == DBUS_TYPE_STRING)
              {
                const char *keymgmt;

                dbus_message_iter_get_basic(&strings, &keymgmt);

                if (!strcmp(keymgmt, "wpa-psk"))
                  rsn->wpa_psk = TRUE;
                else if (!strcmp(keymgmt, "wpa-eap"))
                  rsn->wpa_eap = TRUE;
                else if (!strcmp(keymgmt, "wpa-ft-psk"))
                  rsn->wpa_ft_psk = TRUE;
                else if (!strcmp(keymgmt, "wpa-psk-sha256"))
                  rsn->wpa_psk_sha256 = TRUE;
                else if (!strcmp(keymgmt, "wpa-eap-sha256"))
                  rsn->wpa_eap_sha256 = TRUE;
              }
            }
            while (dbus_message_iter_next(&strings));
          }
        }
      }
    }
  }
  while (dbus_message_iter_next(&dict));

  return TRUE;
}

static gboolean
bss_added_get_ssid_caps(DBusMessageIter *array, struct ssid_info *info)
{
  DBusMessageIter dict;
  struct bss_info bss_info = {};

  dbus_message_iter_recurse(array, &dict);

  do
  {
    if (dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_DICT_ENTRY)
    {
      DBusMessageIter entry;

      dbus_message_iter_recurse(&dict, &entry);

      if (dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_STRING)
      {
        const char *key;

        dbus_message_iter_get_basic(&entry, &key);

        if (!strcmp(key, "SSID"))
        {
          DBusMessageIter value;

          dbus_message_iter_next(&entry);
          dbus_message_iter_recurse(&entry, &value);

          if (dbus_message_iter_get_arg_type(&value) == DBUS_TYPE_ARRAY &&
              dbus_message_iter_get_element_type(&value) == DBUS_TYPE_BYTE)
          {
            DBusMessageIter bytes;
            int len = 0;
            gchar *ssid = NULL;

            dbus_message_iter_recurse(&value, &bytes);
            dbus_message_iter_get_fixed_array(&bytes, &ssid, &len);

            ssid = g_strndup(ssid, len);

            DLOG_INFO("Found SSID %s", ssid);

            if (ssid && !strcmp(ssid, info->ssid))
            {
              info->ssid_found = TRUE;
              info->scan_results_received = TRUE;
            }

            g_free(ssid);
          }
        }
        else if (!strcmp(key, "Privacy"))
        {
          DBusMessageIter value;

          dbus_message_iter_next(&entry);
          dbus_message_iter_recurse(&entry, &value);

          if (dbus_message_iter_get_arg_type(&value) == DBUS_TYPE_BOOLEAN)
          {
            dbus_bool_t privacy;

            dbus_message_iter_get_basic(&value, &privacy);
            bss_info.privacy = privacy;
          }
        }
        else if (!strcmp(key, "Mode"))
        {
          DBusMessageIter value;

          dbus_message_iter_next(&entry);
          dbus_message_iter_recurse(&entry, &value);

          if (dbus_message_iter_get_arg_type(&value) == DBUS_TYPE_STRING)
          {
            const char *mode;

            dbus_message_iter_get_basic(&value, &mode);
            bss_info.infrastructure = !strcmp(mode, "infrastructure");
          }
        }
        else if (!strcmp(key, "WPA"))
        {
          dbus_message_iter_next(&entry);
          process_wpa(&entry, &bss_info.wpa);
        }
        else if (!strcmp(key, "RSN"))
        {
          dbus_message_iter_next(&entry);
          process_rsn(&entry, &bss_info.rsn);
        }
      }
    }
  }
  while (dbus_message_iter_next(&dict));

  if (info->ssid_found)
    info->cap_bits = get_cap_bits(&bss_info);

  return info->ssid_found;
}

static DBusHandlerResult
scan_cb(DBusConnection *connection, DBusMessage *message, gpointer user_data)
{
  struct ssid_info *info = user_data;
  const char *member;

  /* FIXME - hardcoded interface path */
  if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_SIGNAL ||
      strcmp(dbus_message_get_path(message), WPA_SUPP_INTERFACES_PATH_0))
  {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  member = dbus_message_get_member(message);

  if (!strcmp(member, WPA_SUPP_INTERFACE_BSS_ADDED_SIG))
  {
    DBusMessageIter iter;

    dbus_message_iter_init(message, &iter);

    do
    {
      if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY)
      {
        if (bss_added_get_ssid_caps(&iter, info))
          DLOG_INFO("SSID %s caps 0x%x", info->ssid, info->cap_bits);
      }
    }
    while (!info->ssid_found && dbus_message_iter_next(&iter));
  }
  else if (!strcmp(member, WPA_SUPP_INTERFACE_SCAN_DONE_SIG))
  {
    if (!info->ssid_found)
      CONNUI_ERR("Unable to find the given SSID %s", info->ssid);

    info->scan_results_received = TRUE;
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static gboolean
get_capability_for_ssid(const gchar *ssid, guint *capability)
{
  struct ssid_info info;
  //dbus_int32_t tx_power;
  gulong timeout = 500000;
  DBusMessage *mcall;
  DBusMessage *reply;
  dbus_uint32_t expiry = 0;

  g_return_val_if_fail(capability != NULL, FALSE);

  info.ssid = ssid;
  info.cap_bits = 0;
  info.scan_results_received = FALSE;
  info.ssid_found = FALSE;

  if (!connui_dbus_connect_system_bcast_signal(WPA_SUPP_INTERFACE_IFACE,
                                               scan_cb, &info, NULL))
  {
    CONNUI_ERR("Unable to connect to wpa_supplicant '"
               WPA_SUPP_INTERFACE_SCAN_DONE_SIG "' signal");
    return FALSE;
  }

  /*
     Remove all of the aready cached APs, as BSSAdded is fired only once
     otherwise.
  */
  mcall = connui_dbus_create_method_call(WPA_SUPP_SERVICE,
                                         WPA_SUPP_INTERFACES_PATH_0,
                                         WPA_SUPP_INTERFACE_IFACE,
                                         WPA_SUPP_INTERFACE_FLUSH_BSS_REQ,
                                         DBUS_TYPE_UINT32, &expiry,
                                         DBUS_TYPE_INVALID);
  if (mcall)
  {

      reply = connui_dbus_recv_reply_system_mcall(mcall);

      if (reply)
      {
        if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR)
        {
          DLOG_ERR("wpa_supplicant replied with error %s",
                   dbus_message_get_error_name(reply));
          dbus_message_unref(reply);
        }

        dbus_message_unref(reply);
      }
      else
        DLOG_ERR("Unable to receive reply from wpa_supplicant!");

      dbus_message_unref(mcall);
  }
  else
  {
    DLOG_ERR("Unable to create wpa_supplicant '"
             WPA_SUPP_INTERFACE_FLUSH_BSS_REQ
             "' request!");
  }

  //tx_power = get_wlan_tx_power();

  while (1)
  {
    mcall = dbus_message_new_method_call(WPA_SUPP_SERVICE,
                                         WPA_SUPP_INTERFACES_PATH_0,
                                         WPA_SUPP_INTERFACE_IFACE,
                                         WPA_SUPP_INTERFACE_SCAN_REQ);

    if (mcall)
    {

      if (create_scan_req(mcall, ssid))
      {
        reply = connui_dbus_recv_reply_system_mcall(mcall);

        if (reply)
        {
          if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR)
          {
            DLOG_ERR("wpa_supplicant replied with error %s",
                     dbus_message_get_error_name(reply));
            dbus_message_unref(reply);
          }
          else
            break;
        }
        else
          DLOG_ERR("Unable to receive reply from wpa_supplicant!");
      }
      else
        DLOG_ERR("Unable to send wpa_supplicant scan request!");
    }
    else
      DLOG_ERR("Unable to create wpa_supplicant scan request!");

    g_usleep(timeout);

    if (mcall)
      dbus_message_unref(mcall);

    timeout += 500000;

    if (timeout == 3000000)
      goto out;
  }

  dbus_message_unref(reply);
  dbus_message_unref(mcall);

  info.supplicant_timeout_id = g_timeout_add(30000, scan_timeout_cb, &info);

  while (!info.scan_results_received && info.supplicant_timeout_id)
    g_main_context_iteration(NULL, TRUE);

  if (info.supplicant_timeout_id)
  {
    g_source_remove(info.supplicant_timeout_id);
    info.supplicant_timeout_id = 0;
    *capability = info.cap_bits;
  }
  else
    DLOG_ERR("Unable to receive scan results from wpa_supplicant!");

out:
  connui_dbus_disconnect_system_bcast_signal(WPA_SUPP_INTERFACE_IFACE,
                                             scan_cb, &info, NULL);

  return info.ssid_found;
}

gboolean
iap_hidden_ssid_dialog(GtkWindow *parent, gchar **ssid, guint *wlan_capability)
{
  GtkWidget *dialog;
  GtkWidget *entry;
  int im;
  GtkWidget *label;
  GtkWidget *hbox;

  g_return_val_if_fail(ssid != NULL && wlan_capability != NULL, FALSE);

  dialog = hildon_dialog_new_with_buttons(
        _("conn_set_iap_ti_wlan_hidden"),
        parent,
        GTK_DIALOG_NO_SEPARATOR | GTK_DIALOG_DESTROY_WITH_PARENT |
        GTK_DIALOG_MODAL,
        dgettext("hildon-libs", "wdgt_bd_done"),
        GTK_RESPONSE_OK,
        NULL);

  entry = iap_widgets_create_h22_entry();
  im = hildon_gtk_entry_get_input_mode(GTK_ENTRY(entry));
  im &= ~HILDON_GTK_INPUT_MODE_AUTOCAP;
  hildon_gtk_entry_set_input_mode(GTK_ENTRY(entry), im);

  label = gtk_label_new(_("conn_set_iap_fi_wlan_hidden_ssid"));
  hbox = gtk_hbox_new(0, 8);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, FALSE, 0);

  g_signal_connect(G_OBJECT(entry), "activate",
                   G_CALLBACK(iap_hidden_ssid_dialog_entry_activate_cb),
                   dialog);
  iap_common_set_close_response(dialog, GTK_RESPONSE_CANCEL);
  gtk_widget_show_all(dialog);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_CANCEL)
  {
    gtk_widget_destroy(dialog);
    return FALSE;
  }

  *ssid = g_strdup(iap_widgets_h22_entry_get_text(entry));
  gtk_widget_destroy(dialog);

  return get_capability_for_ssid(*ssid, wlan_capability);
}

guint
iap_wlan_to_iap_security(const char *security, guint iap_security, int iap_auth)
{
  if (!security || !strcmp(security, "NONE"))
    return IAP_SECURITY_NONE;

  if (!strcmp(security, "WEP"))
    return IAP_SECURITY_WEP;

  if (!strcmp(security, "WPA_PSK"))
    return IAP_SECURITY_WPA_PSK;

  if (!strcmp(security, "WPA_EAP"))
  {
    switch (iap_security)
    {
      case EAP_TLS:
        return IAP_SECURITY_WPA_EAP_TLS;
      case 17:
        return 0x4000000;
      case 18:
        return 0x200000;
      case EAP_TTLS:
      {
        guint iap_sec = IAP_SECURITY_WPA_EAP_TTLS;

        if (iap_auth == EAP_GTC)
          iap_sec |= IAP_SECURITY_WPA_EAP_GTC;
        else if (iap_auth == EAP_MS)
          iap_sec |= IAP_SECURITY_WPA_EAP_MS;
        else if (iap_auth == EAP_TTLS_PAP)
          iap_sec |= IAP_SECURITY_WPA_EAP_TTLS_PAP;
        else if (iap_auth == EAP_TTLS_MS)
          iap_sec |= IAP_SECURITY_WPA_EAP_TTLS_MS;

        return iap_sec;
      }
      case 23:
        return 0x100000;
      case EAP_PEAP:
      {
        guint iap_sec = IAP_SECURITY_WPA_EAP_PEAP;

        if (iap_auth == EAP_GTC)
          iap_sec |= IAP_SECURITY_WPA_EAP_GTC;
        else if (iap_auth == EAP_MS)
          iap_sec |= IAP_SECURITY_WPA_EAP_MS;

        return iap_sec;
      }
      default:
        return IAP_SECURITY_WPA_EAP_UNKNOWN;
    }
  }

  return IAP_SECURITY_NONE;
}
