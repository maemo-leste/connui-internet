#include <icd/osso-ic-dbus.h>
#include <icd/osso-ic-gconf.h>
#include <connui/connui.h>
#include <connui/connui-conndlgs.h>
#include <connui/connui-log.h>
#include <connui/connui-dbus.h>
#include <conbtui/gateway/settings.h>

#include <libintl.h>
#include <string.h>

#include "../settings/widgets.h"

IAP_DIALOGS_PLUGIN_DEFINE_EXTENDED(others, ICD_UI_SHOW_RETRY_REQ, {})

struct dialog_data
{
  DBusMessage *message;
  iap_dialogs_showing_fn showing;
  iap_dialogs_done_fn done;
  int iap_id;
  GtkWidget *entry;
  gchar *iap_name;
  osso_context_t *libosso;
};

static gboolean
iap_dialog_save_wpapsk_passphrase_and_key(struct dialog_data *data)
{
  GConfValue *val;
  gboolean rv;

  iap_settings_set_gconf_value(data->iap_name, "EAP_wpa_preshared_key", NULL);
  val = gconf_value_new(GCONF_VALUE_STRING);
  gconf_value_set_string(val, gtk_entry_get_text(GTK_ENTRY(data->entry)));

  if (iap_settings_set_gconf_value(data->iap_name,
                                   "EAP_wpa_preshared_passphrase", val))
  {
    rv = TRUE;
  }
  else
    rv = FALSE;

  gconf_value_free(val);

  return rv;
}

static DBusMessage *
iap_dialog_others_create_reply(DBusMessage *message, dbus_bool_t retry_pressed,
                               dbus_bool_t retry_iap)
{
  DBusMessage *reply = NULL;
  DBusError error;
  const gchar *iap_name = NULL;

  dbus_error_init(&error);

  if (dbus_message_get_args(message, &error,
                            DBUS_TYPE_STRING, &iap_name,
                            DBUS_TYPE_INVALID))
  {
    reply = dbus_message_new_signal(ICD_UI_DBUS_PATH,
                                    ICD_UI_DBUS_INTERFACE,
                                    ICD_UI_RETRY_SIG);
    if (reply)
    {
      if (!dbus_message_append_args(reply,
                                    DBUS_TYPE_STRING, &iap_name,
                                    DBUS_TYPE_BOOLEAN, &retry_pressed,
                                    DBUS_TYPE_BOOLEAN, &retry_iap,
                                    DBUS_TYPE_INVALID))
      {
        CONNUI_ERR("appending of arguments failed");
        dbus_message_unref(reply);
        reply = NULL;
      }
    }
    else
      CONNUI_ERR("reply message creation failed");
  }
  else
    dbus_error_free(&error);

  return reply;
}

static void
iap_dialog_others_dialog_hide_and_destroy(GtkDialog *dialog)
{
  gtk_widget_hide_all(GTK_WIDGET(dialog));
  gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void
iap_dialog_others_note_response_cb(GtkDialog *dialog, gint response_id,
                                   gpointer user_data)
{
  struct dialog_data *data = (struct dialog_data *)user_data;
  DBusMessage * reply =
      iap_dialog_others_create_reply(data->message,
                                     response_id == GTK_RESPONSE_OK, FALSE);

  if (reply)
  {
    dbus_message_set_destination(reply,
                                 dbus_message_get_sender(data->message));
    connui_dbus_send_system_msg(reply);
    dbus_message_unref(reply);
  }

  dbus_message_unref(data->message);

  if (GTK_IS_DIALOG(dialog))
    iap_dialog_others_dialog_hide_and_destroy(dialog);

  data->done(data->iap_id, FALSE);
}

static gboolean
iap_dialog_others_show_confirmation_note(const gchar *description,
                                         struct dialog_data *data)
{
  GtkWidget *note;

  data->showing();

  dbus_message_ref(data->message);
  note = hildon_note_new_confirmation(NULL, description);
  g_signal_connect(G_OBJECT(note), "response",
                   (GCallback)iap_dialog_others_note_response_cb, data);
  gtk_widget_show_all(note);

  return TRUE;
}

static void
iap_dialog_others_entry_activate_cb(GtkEntry *entry, gpointer user_data)
{
  GtkDialog *dialog = GTK_DIALOG(user_data);

  if (dialog)
    gtk_dialog_response(dialog, GTK_RESPONSE_OK);
}

static void
iap_dialog_others_dialog_response_cb(GtkDialog *dialog, gint response_id,
                                     gpointer user_data)
{
  struct dialog_data *data = (struct dialog_data *)user_data;
  gboolean is_ok = (response_id == GTK_RESPONSE_OK);
  DBusMessage *reply = NULL;

  if (is_ok)
  {
    if (strlen(gtk_entry_get_text(GTK_ENTRY(data->entry))) < 8)
    {
      hildon_banner_show_information(GTK_WIDGET(dialog), NULL,
                                     dgettext("osso-connectivity-ui",
                                              "conn_ib_min8val_req"));
      g_signal_stop_emission_by_name(G_OBJECT(dialog), "response");
      return;
    }

    if (iap_dialog_save_wpapsk_passphrase_and_key(data))
      reply = iap_dialog_others_create_reply(data->message, TRUE, TRUE);
  }
  else
    reply = iap_dialog_others_create_reply(data->message, FALSE, FALSE);

  if (reply)
  {
    dbus_message_set_destination(reply,
                                 dbus_message_get_sender(data->message));
    connui_dbus_send_system_msg(reply);
    dbus_message_unref(reply);
  }

  dbus_message_unref(data->message);
  iap_dialog_others_dialog_hide_and_destroy(dialog);
  data->done(data->iap_id, FALSE);
}

static gboolean
show_note(const char *msgid, struct dialog_data *data)
{
  iap_dialog_others_show_confirmation_note(
        dgettext("osso-connectivity-ui", msgid), data);

  return TRUE;
}

static void
iap_dialog_others_flightmode_cb(dbus_bool_t offline, gpointer user_data)
{
  struct dialog_data *data = (struct dialog_data *)user_data;
  static gboolean flightmode_off_requested;
  gboolean exit_offline;

  g_return_if_fail(data != NULL);

  if (!flightmode_off_requested)
    exit_offline = offline;
  else
    exit_offline = FALSE;

  flightmode_off_requested = TRUE;

  if (exit_offline)
  {
    if (!connui_flightmode_off_confirm())
    {
      CONNUI_ERR("Unable to send flightmode off request!");
      iap_dialog_others_note_response_cb(NULL, GTK_RESPONSE_CANCEL, data);
      connui_flightmode_close(iap_dialog_others_flightmode_cb);
      flightmode_off_requested = FALSE;
    }
  }
  else
  {
    iap_dialog_others_note_response_cb(NULL,  offline ? GTK_RESPONSE_CANCEL :
                                                        GTK_RESPONSE_OK,
                                       data);
    connui_flightmode_close(iap_dialog_others_flightmode_cb);
    flightmode_off_requested = FALSE;
  }
}

gboolean
iap_dialog_others_show(int iap_id, DBusMessage *message,
                       iap_dialogs_showing_fn showing,
                       iap_dialogs_done_fn done, osso_context_t *libosso)
{
  GConfClient *gconf = gconf_client_get_default();;
  const gchar *error_name = NULL;
  const gchar *iap_name = NULL;
  static struct dialog_data data;
  DBusError error;

  data.message = message;
  data.iap_id = iap_id;
  data.showing = showing;
  data.done = done;
  data.libosso = libosso;

  if (gconf)
  {
    if (gconf_client_get_bool(
          gconf, ICD_GCONF_SETTINGS "/ui/auto_cancel_retry_dialogs", FALSE))
    {
      DBusMessage *reply;

      g_object_unref(G_OBJECT(gconf));
      reply = iap_dialog_others_create_reply(data.message, FALSE, FALSE);

      if (reply)
      {
        dbus_message_set_destination(reply,
                                     dbus_message_get_sender(data.message));
        connui_dbus_send_system_msg(reply);
        dbus_message_unref(reply);
      }

      data.done(data.iap_id, FALSE);
      return TRUE;
    }

    g_object_unref(G_OBJECT(gconf));
  }

  if (!iap_dialogs_plugin_match(data.message))
    return FALSE;

  dbus_error_init(&error);

  if (!dbus_message_get_args(data.message, &error,
                             DBUS_TYPE_STRING, &iap_name,
                             DBUS_TYPE_STRING, &error_name,
                             DBUS_TYPE_INVALID))
  {
    CONNUI_ERR("could not get arguments from request: %s", error.message);
    dbus_error_free(&error);
    return FALSE;
  }

  if (!strcmp(ICD_DBUS_ERROR_GATEWAY_ERROR, error_name))
  {
    gchar *bt_device_name = NULL;
    gchar *bt_device = gateway_settings_get_preferred();
    gchar *description;

    if (bt_device)
    {
      const char *s = dgettext("osso-connectivity-ui",
                               "conn_nc_retry_connection_gw_failed");

      gateway_settings_get_device_data(bt_device, &bt_device_name, NULL, NULL,
                                       NULL, NULL, NULL, NULL, NULL, NULL,
                                       NULL);
      description = g_strdup_printf(s, bt_device_name);
      iap_dialog_others_show_confirmation_note(description, &data);
      g_free(bt_device_name);
      g_free(description);

      return TRUE;
    }

    return FALSE;
  }

  if (!strcmp(ICD_DBUS_ERROR_WLAN_AUTH_FAILED, error_name) ||
      !strcmp(ICD_DBUS_ERROR_PPP_AUTH_FAILED, error_name))
  {
    return show_note("conn_nc_retry_connection_auth_failed", &data);
  }

  if (!strcmp(ICD_DBUS_ERROR_WLAN_WPA_PSK_AUTH_FAILED, error_name))
  {
    gchar *settings_name;
    GtkWidget *dialog;
    GtkSizeGroup *size_group;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *entry;
    HildonGtkInputMode im;
    GtkWidget *caption;
    GdkGeometry geometry;

    hildon_banner_show_information(NULL, NULL,
                                   dgettext("osso-connectivity-ui",
                                            "conn_ib_weppsk_auth_failed"));
    dbus_message_ref(data.message);
    dbus_error_init(&error);

    if (!dbus_message_get_args(data.message, &error,
                               DBUS_TYPE_STRING, &data.iap_name,
                               DBUS_TYPE_INVALID))
    {
      dbus_error_free(&error);
    }

    settings_name = iap_settings_get_name(data.iap_name);
    dialog = gtk_dialog_new_with_buttons(
          dgettext("osso-connectivity-ui", "conn_set_iap_ti_wlan_ent_wpa_psk"),
          NULL, GTK_DIALOG_NO_SEPARATOR,
          dgettext("hildon-libs", "wdgt_bd_done"), GTK_RESPONSE_OK,
          NULL);

    iap_common_set_close_response(dialog, GTK_RESPONSE_CANCEL);
    size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
    hbox = gtk_hbox_new(0, 16);

    label = gtk_label_new(dgettext("osso-connectivity-ui",
                                   "conn_set_iap_fi_wlan_network"));

    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.0);
    gtk_size_group_add_widget(size_group, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    label = gtk_label_new(settings_name);
    g_free(settings_name);

    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(
          GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox,  FALSE, FALSE, 0);

    entry = gtk_entry_new();
    data.entry = entry;
    im = hildon_gtk_entry_get_input_mode(GTK_ENTRY(data.entry));
    im &= ~(HILDON_GTK_INPUT_MODE_AUTOCAP | HILDON_GTK_INPUT_MODE_DICTIONARY);
    hildon_gtk_entry_set_input_mode(GTK_ENTRY(data.entry), im);
    g_object_set(G_OBJECT(data.entry), "max-length", 63, NULL);
    g_signal_connect(G_OBJECT(data.entry), "insert_text",
                     G_CALLBACK(iap_widgets_insert_only_ascii_text),
                                data.entry);
    g_signal_connect(G_OBJECT(data.entry), "insert-text",
                     G_CALLBACK(iap_widgets_insert_text_no_8bit_maxval_reach),
                     data.entry);
    g_signal_connect(G_OBJECT(data.entry), "activate",
                     G_CALLBACK(iap_dialog_others_entry_activate_cb), dialog);

    caption = hildon_caption_new(size_group,
                                 dgettext("osso-connectivity-ui",
                                          "conn_set_iap_fi_wlan_wpa_psk_txt"),
                                 data.entry, 0, 0);
    gtk_box_pack_start(
          GTK_BOX(GTK_DIALOG(dialog)->vbox), caption, FALSE, FALSE, 0);
    g_object_unref(G_OBJECT(size_group));

    geometry.min_width = 540;
    geometry.min_height = -1;
    gtk_window_set_geometry_hints(GTK_WINDOW(dialog), dialog,
                                  &geometry, GDK_HINT_MIN_SIZE);
    g_signal_connect(G_OBJECT(dialog), "response",
                     G_CALLBACK(iap_dialog_others_dialog_response_cb), &data);
    gtk_widget_show_all(dialog);

    return TRUE;
  }

  if (!strcmp(ICD_DBUS_ERROR_DHCP_FAILED, error_name))
    return show_note("conn_ni_link_local_ip", &data);

  if (!strcmp(ICD_DBUS_ERROR_DHCP_WEP_FAILED, error_name))
    return show_note("conn_ni_link_local_ip_wep", &data);

  if (!strcmp(ICD_DBUS_ERROR_SERVER_ERROR, error_name))
    return show_note("conn_nc_retry_connection_remote_error", &data);

  if (!strcmp(ICD_DBUS_ERROR_NETWORK_ERROR, error_name))
    return show_note("conn_nc_retry_connection_network_error", &data);

  if (!strcmp("com.nokia.icd.error.ap_settings_not_supported", error_name))
    return show_note("conn_nc_incompatible_ap", &data);

  if (!strcmp(ICD_DBUS_ERROR_SAP_NOT_SUPPORTED, error_name) ||
      !strcmp(ICD_DBUS_ERROR_SAP_CONNECT_FAILED, error_name) ||
      !strcmp(ICD_DBUS_ERROR_SAP_NO_PHONE, error_name))
  {
    return FALSE;
  }

  if (!strcmp(ICD_DBUS_ERROR_FLIGHT_MODE, error_name))
  {
    if (connui_flightmode_status(iap_dialog_others_flightmode_cb, &data))
    {
      dbus_message_ref(data.message);
      data.showing();
      return TRUE;
    }
    else
    {
      CONNUI_ERR("Unable to register flight mode status callback!");
      return FALSE;
    }
  }

  return show_note("conn_nc_retry_connection", &data);
}
