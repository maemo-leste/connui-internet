#include <connui/connui.h>
#include <connui/connui-conndlgs.h>
#include <connui/connui-log.h>
#include <connui/connui-dbus.h>

IAP_DIALOGS_PLUGIN_DEFINE(save, ICD_UI_SHOW_DISCONNDLG_REQ)

struct dialog_data
{
  gchar *iap_name;
  DBusMessage *message;
  iap_dialogs_done_fn done;
  int iap_id;
};

static struct dialog_data *
get_dialog_data()
{
  static struct dialog_data dialog_data;

  return &dialog_data;
}

static gboolean
iap_dialog_save_send_reply(const gchar *saved_name)
{
  struct dialog_data *data = get_dialog_data();
  DBusMessage *reply;

  g_return_val_if_fail(data != NULL && data->iap_name != NULL, FALSE);

  if (data->message)
  {
    reply = dbus_message_new_signal(ICD_UI_DBUS_PATH, ICD_UI_DBUS_INTERFACE,
                                    ICD_UI_SAVE_SIG);

    if (!reply)
    {
      CONNUI_ERR("could not create save reply");
      return FALSE;
    }

    if (!dbus_message_append_args(reply,
                                  DBUS_TYPE_STRING, data->iap_name,
                                  DBUS_TYPE_STRING, &saved_name,
                                  DBUS_TYPE_INVALID))
    {
      CONNUI_ERR("could not append args to save reply");
      dbus_message_unref(reply);
      return FALSE;
    }

    dbus_message_set_destination(reply, dbus_message_get_sender(data->message));
    connui_dbus_send_system_msg(reply);
    dbus_message_unref(reply);
    dbus_message_unref(data->message);
    data->message = NULL;
  }

  g_free(data->iap_name);
  data->iap_name = NULL;

  return TRUE;
}

gboolean
iap_dialog_save_show(int iap_id, DBusMessage *message,
                     iap_dialogs_showing_fn showing,
                     iap_dialogs_done_fn done, osso_context_t *libosso)
{
  gchar *candidate = NULL;
  struct dialog_data *data = get_dialog_data();
  int i = 0;
  gchar *saved_name;
  DBusError error;
  gchar *current_iap = NULL;

  dbus_error_init(&error);

  if (!dbus_message_get_args(message, &error,
                             DBUS_TYPE_STRING, &current_iap,
                             DBUS_TYPE_INVALID))
  {
    CONNUI_ERR("could not get arguments: %s", error.message);
    dbus_error_free(&error);
    return FALSE;
  }

  if (data->iap_name)
    g_free(data->iap_name);

  dbus_message_ref(message);

  data->iap_name = g_strdup(current_iap);
  data->message = message;
  data->done = done;
  data->iap_id = iap_id;

  showing();

  for (saved_name = iap_settings_get_name(data->iap_name); ;
       saved_name = g_strdup_printf("%s(%d)", candidate, i))
  {
    i++;

    if (!iap_settings_iap_exists(saved_name, data->iap_name))
      break;

    if (!candidate)
    {
      candidate = saved_name;
      saved_name = NULL;
    }

    g_free(saved_name);
  }

  g_free(candidate);
  iap_dialog_save_send_reply(saved_name);
  g_free(saved_name);

  data->done(data->iap_id, FALSE);

  return TRUE;
}

static gboolean
iap_dialog_save_cancel(DBusMessage *message)
{
  return iap_dialog_save_send_reply("");
}
