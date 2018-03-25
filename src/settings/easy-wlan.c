#include <hildon/hildon.h>
#include <wlancond.h>
#include <osso-ic-gconf.h>
#include <connui/connui.h>

#include <ctype.h>
#include <string.h>
#include <libintl.h>

#include "stage.h"
#include "widgets.h"
#include "mapper.h"

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
  guint security[] = {WLANCOND_WPA_TKIP, 0x400000, WLANCOND_WPA_AES};

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

    if (len == 5 || len == 13)
      return;

    {
      if (len == 10 || len == 26)
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
