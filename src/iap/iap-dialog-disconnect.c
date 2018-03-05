#include <connui/connui.h>
#include <connui/connui-conndlgs.h>
#include <connui/connui-log.h>
#include <connui/connui-dbus.h>

IAP_DIALOGS_PLUGIN_DEFINE_EXTENDED(disconnect, ICD_UI_SHOW_DISCONNDLG_REQ, {})

gboolean
iap_dialog_disconnect_show(int iap_id, DBusMessage *message,
                           iap_dialogs_showing_fn showing,
                           iap_dialogs_done_fn done, osso_context_t *libosso)
{
  DBusMessage *reply;
  const char *sender;
  dbus_bool_t disconnect_pressed = TRUE;

  showing();

  sender = dbus_message_get_sender(message);
  reply = dbus_message_new_signal(ICD_UI_DBUS_PATH, ICD_UI_DBUS_INTERFACE,
                                  ICD_UI_DISCONNECT_SIG);

  if (reply)
  {
    if (dbus_message_append_args(reply,
                                 DBUS_TYPE_BOOLEAN, &disconnect_pressed,
                                 DBUS_TYPE_INVALID))
    {
      dbus_message_set_destination(reply, sender);
      connui_dbus_send_system_msg(reply);
    }
    else
      CONNUI_ERR("appending of arguments failed");

    dbus_message_unref(reply);
  }
  else
    CONNUI_ERR("iap_dialog_disconnect(): could not create reply signal");

  done(iap_id, TRUE);

  return TRUE;
}
