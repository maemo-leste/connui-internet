#include <connui/connui.h>
#include <connui/connui-conndlgs.h>
#include <connui/connui-log.h>
#include <connui/connui-dbus.h>

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

static struct dialog_data dialog_data;

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

gboolean
iap_dialog_others_show(int iap_id, DBusMessage *message,
                       iap_dialogs_showing_fn showing,
                       iap_dialogs_done_fn done, osso_context_t *libosso)
{
  return TRUE;
}
