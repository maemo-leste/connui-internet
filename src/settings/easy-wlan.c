#include <hildon/hildon.h>
#include <osso-ic-gconf.h>
#include <connui/connui.h>
#include <connui/wlan-common.h>
#include <connui/connui-log.h>
#include <wlancond.h>

#include <ctype.h>
#include <string.h>
#include <libintl.h>

#include "stage.h"
#include "widgets.h"
#include "mapper.h"

#include "easy-wlan.h"

#define _(msgid) dgettext("osso-connectivity-ui", msgid)

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

  /* FIXME 0x800000 */
  if (!(ewlan->security & 0x800000))
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
  /* not sure about those are the corerect defines, FIXME */
  guint security[] = {0x10000000, 0x400000, 0x20000000};

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
  guint security[] = {0x800000, 0x1000000, 0x2000000, 0x8000000};

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

  if (ewlan->security & 0x20000000)
  {
    const gchar *eap_peap;
    GConfClient *gconf = gconf_client_get_default();
    gboolean pap_enabled =
        gconf_client_get_bool(gconf, ICD_GCONF_SETTINGS "/ui/pap_enabled",
                              FALSE);
    g_object_unref(gconf);

    if (pap_enabled)
      eap_peap = "EAP PAP";
    else
      eap_peap = NULL;

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

  if (wlancond_capability & WLAN_CAP_ADHOCWLAN)
    iap_security = 0x40;
  else
    iap_security = 0x80;

  if (wlancond_capability & WLAN_CAP_SECURITY_WPA_EAP)
    return ~((unsigned int)~(iap_security << 12) >> 12);

  if (wlancond_capability & WLAN_CAP_SECURITY_WPA_PSK)
    return iap_security | 0x80000;

  if (wlancond_capability & WLAN_CAP_SECURITY_WEP)
    return iap_security | 0x20000;

  return iap_security | 0x10000;
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

static const gint EAP_default_type[] = {25, 13, 21, -1};

static struct stage_widget iap_easy_wlan_wpa_eap_type_widgets[] =
{
  {
    NULL,
    NULL,
    "EAP_TYPE",
    "EAP_default_type",
    NULL,
    &mapper_combo2int,
    &EAP_default_type
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
    GINT_TO_POINTER(1)
  },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

static const gint PEAP_tunneled_eap_type[] = {6, 26, 99, 98, -1};

static struct stage_widget iap_easy_wlan_wpa_eap_ttls_auth_widgets[] =
{
  {
    NULL,
    NULL,
    "EAP_CERTIFICATE",
    "EAP_TLS_PEAP_client_certificate_file",
    NULL,
    &mapper_combo2string,
    GINT_TO_POINTER(1)
  },
  {
    NULL,
    NULL,
    "EAP_TUNNELED_TYPE",
    "PEAP_tunneled_eap_type",
    NULL,
    &mapper_combo2int,
    &PEAP_tunneled_eap_type
  }
};

static gboolean
EAP_MSCHAPV2_validate(const struct stage *s, const gchar *name,
                      const gchar *key)
{
  int eap_type = stage_get_int(s, "PEAP_tunneled_eap_type");

  return eap_type == 99 || eap_type == 26 || eap_type == 98;
}

static gboolean
EAP_GTC_validate(const struct stage *s,const gchar *name, const gchar *key)
{
  return stage_get_int(s, "PEAP_tunneled_eap_type") == 6;
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

  if (wlancond_capability & 0x1E00)
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
  else if (ewlan->security == 0x10000)
    wlan_security = "NONE";
  else if (ewlan->security == 0xFFF00000)
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
  else if (ewlan->security == 0x20000)
  {
    resp_id =
        iap_easy_wlan_dialog_run(ewlan,
                                 _("conn_set_iap_ti_wlan_ent_wepkey"),
                                 iap_easy_wlan_wepkey_verify_response_cb,
                                 iap_easy_wlan_wepkey_add_widgets_cb, NULL,
                                 iap_easy_wlan_wepkey_widgets);
    wlan_security = "WEP";
  }
  else if (ewlan->security == 0x80000)
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

  if (ewlan->security == 0x20000000 || ewlan->security == 0x10000000)
  {
    const char *msgeap =
        ewlan->security == 0x20000000 ?
          "conn_set_iap_ti_wlan_wpa_eap_ttls_auth" :
          "conn_set_iap_ti_wlan_wpa_eap_peap_auth";

    resp_id = iap_easy_wlan_dialog_run(
          ewlan, _(msgeap), 0, iap_easy_wlan_wpa_eap_ttls_auth_add_widgets_cb,
          iap_easy_wlan_wpa_eap_ttls_auth_done_cb,
          iap_easy_wlan_wpa_eap_ttls_auth_widgets);
  }
  else if (ewlan->security == 0x400000)
  {
    resp_id = iap_easy_wlan_dialog_run(
          ewlan, _("conn_set_iap_ti_wlan_wpa_eap_tls_auth"), NULL,
          iap_easy_wlan_wpa_eap_tls_auth_add_widgets_cb, NULL,
          iap_easy_wlan_wpa_eap_tls_auth_widgets);
  }
  else if (ewlan->security & 0xB800000)
  {
    resp_id = iap_easy_wlan_dialog_run(
          ewlan, ewlan->security & 0x800000 ?
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

  iap_security = iap_security_from_wlan_security(*wlancond_capability);
  memset(&ewlan, 0, sizeof(ewlan));

  if (!network_id && !*network_id)
  {
    if (iap_hidden_ssid_dialog(parent, &hidden_ssid, wlancond_capability) ||
        !*wlancond_capability)
    {
      is_hidden = TRUE;
      ewlan.network_id = g_strdup(hidden_ssid);
      iap_security = iap_security_from_wlan_security(*wlancond_capability);
    }
    else
      return NULL;
  }
  else
  {
    is_hidden = FALSE;
    ewlan.network_id = g_strdup(network_id);
  }

  ewlan.parent = parent;
  ewlan.security = iap_security >> 16 << 16;
  ewlan.widgets = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
  wlan_common_mangle_ssid(ewlan.network_id, strlen(ewlan.network_id));
  stage_create_cache(&ewlan.stage, NULL);

  if (iap_security & 0x40)
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
