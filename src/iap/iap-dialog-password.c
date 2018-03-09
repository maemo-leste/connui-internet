#include <connui/connui.h>
#include <connui/connui-conndlgs.h>
#include <connui/connui-log.h>
#include <connui/connui-dbus.h>

#include <string.h>
#include <libintl.h>

IAP_DIALOGS_PLUGIN_DEFINE(password, ICD_UI_SHOW_PASSWD_REQ)

struct dialog_data
{
  DBusMessage *message;
  GtkEntry *username_entry;
  GtkEntry *password_entry;
  GtkCheckButton *check_button;
  gchar *iap_name;
  iap_dialogs_done_fn done;
  int iap_id;
};

static gboolean
iap_dialog_password_send_reply(dbus_bool_t ok_pressed, const char *dest,
                               const gchar *username, const gchar *password,
                               const gchar *iap_name)
{
  gboolean rv ;
  DBusMessage *reply = dbus_message_new_signal(ICD_UI_DBUS_PATH,
                                               ICD_UI_DBUS_INTERFACE,
                                               ICD_UI_PASSWD_SIG);

  if (!reply)
  {
    CONNUI_ERR("could create password reply");
    return FALSE;
  }

  if (dbus_message_append_args(reply,
                               DBUS_TYPE_STRING, &username,
                               DBUS_TYPE_STRING, &password,
                               DBUS_TYPE_STRING, &iap_name,
                               DBUS_TYPE_BOOLEAN, &ok_pressed,
                               DBUS_TYPE_INVALID))
  {
    dbus_message_set_destination(reply, dest);
    connui_dbus_send_system_msg(reply);
    rv = TRUE;
  }
  else
  {
    CONNUI_ERR("could not append args to password reply");
    rv = FALSE;
  }

  dbus_message_unref(reply);

  return rv;
}

static gchar *
iap_settings_get_gconf_string(struct dialog_data *data, const gchar *key)
{
  GConfValue *val = iap_settings_get_gconf_value(data->iap_name, key);
  gchar *rv;

  if (!val)
    return NULL;

  rv = g_strdup(gconf_value_get_string(val));
  gconf_value_free(val);

  return rv;
}

static void
iap_settings_set_gconf_string(struct dialog_data *data, const gchar *key,
                              const gchar *s)
{
  GConfValue *val = gconf_value_new(GCONF_VALUE_STRING);

  gconf_value_set_string(val, s);
  iap_settings_set_gconf_value(data->iap_name, key, val);
  gconf_value_free(val);
}

static gboolean
iap_settings_iap_is_gprs(struct dialog_data *data)
{
  GConfValue *val = iap_settings_get_gconf_value(data->iap_name, "type");
  gboolean rv;
  const gchar *s;

  if (!val)
    return FALSE;

  s = gconf_value_get_string(val);
  rv = (s && !strncmp(s, "GPRS", 4));
  gconf_value_free(val);

  return rv;
}

static void
iap_settings_get_iap_type_credentials(struct dialog_data *data,
                                      const gchar *type, gchar **username,
                                      gchar **password)
{
  gchar *type_username = g_strdup_printf("%s_username", type);
  gchar *type_password = g_strdup_printf("%s_password", type);

  *username = iap_settings_get_gconf_string(data, type_username);
  *password = iap_settings_get_gconf_string(data, type_password);

  g_free(type_username);
  g_free(type_password);
}

static void
iap_settings_set_iap_type_credentials(struct dialog_data *data,
                                      const gchar *type, const gchar *username,
                                      const gchar *password)
{
  gchar *type_username = g_strdup_printf("%s_username", type);
  gchar *type_password = g_strdup_printf("%s_password", type);

  iap_settings_set_gconf_string(data, type_username, username);
  iap_settings_set_gconf_string(data, type_password, password);

  g_free(type_username);
  g_free(type_password);
}

static gboolean
iap_dialog_password_response_ok_cb(gpointer user_data)
{
  gtk_dialog_response(GTK_DIALOG(user_data), GTK_RESPONSE_OK);

  return FALSE;
}

static void
iap_dialog_password_response_cb(GtkDialog *dialog, gint response_id,
                                gpointer user_data)
{
  struct dialog_data *data = (struct dialog_data *)user_data;
  dbus_bool_t ok_pressed = (response_id == GTK_RESPONSE_OK);
  const gchar *username = gtk_entry_get_text(data->username_entry);
  const gchar *password;

  if (ok_pressed)
  {
    password = gtk_entry_get_text(data->password_entry);

    if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->check_button)))
    {
      GConfValue *val =
          iap_settings_get_gconf_value(data->iap_name, "ask_password");

      if (val)
      {
        gconf_value_set_bool(val, FALSE);
        iap_settings_set_gconf_value(data->iap_name, "ask_password", val);
        gconf_value_free(val);

        if (iap_settings_iap_is_gprs(data))
        {
          iap_settings_set_iap_type_credentials(data, "gprs",
                                                username, password);
        }
        else
        {
          iap_settings_set_iap_type_credentials(data, "dun",
                                                username, password);
        }
      }
      else
      {
        val = iap_settings_get_gconf_value(data->iap_name,
                                           "EAP_MSCHAPV2_password_prompt");

        if (val)
        {
          gconf_value_set_int(val, 0);
          iap_settings_set_gconf_value(data->iap_name,
                                       "EAP_MSCHAPV2_password_prompt", val);
          gconf_value_free(val);
          iap_settings_set_iap_type_credentials(data, "EAP_MSCHAPV2",
                                                username, password);
        }
        else
          CONNUI_ERR("Unable to save credentials for IAP %s", data->iap_name);
      }
    }
  }
  else
    password = "";

  if (iap_dialog_password_send_reply(ok_pressed,
                                     dbus_message_get_sender(data->message),
                                     username, password, data->iap_name))
  {
    g_free(data->iap_name);
    dbus_message_unref(data->message);
    gtk_widget_hide_all(GTK_WIDGET(dialog));
    gtk_widget_destroy(GTK_WIDGET(dialog));
    data->done(data->iap_id, FALSE);
  }
}

gboolean
iap_dialog_password_show(int iap_id, DBusMessage *message,
                         iap_dialogs_showing_fn showing,
                         iap_dialogs_done_fn done, osso_context_t *libosso)
{
  static struct dialog_data data;
  GtkSizeGroup *size_group;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *dialog;
  GConfValue *val;
  gboolean ask_password;
  DBusError error;
  gchar *password = NULL;
  gchar *username = NULL;
  gchar *iap_name = NULL;
  gchar *pwd = NULL;
  gchar *usr = NULL;
  HildonGtkInputMode im;

  dbus_error_init(&error);

  if (!dbus_message_get_args(message, &error,
                             DBUS_TYPE_STRING, &usr,
                             DBUS_TYPE_STRING, &pwd,
                             DBUS_TYPE_STRING, &iap_name,
                             DBUS_TYPE_INVALID))
  {
    CONNUI_ERR("could not get arguments: %s", error.message);
    dbus_error_free(&error);
    return FALSE;
  }

  dbus_message_ref(message);
  data.done = done;
  data.message = message;
  data.iap_id = iap_id;
  data.iap_name = g_strdup(iap_name);

  showing();

  size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
  vbox = gtk_vbox_new(0, 0);
  hbox = gtk_hbox_new(0, 16);
  label = gtk_label_new(dgettext("osso-connectivity-ui",
                                 "conn_fi_change_eap_msc_pw_conn"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);

  gtk_size_group_add_widget(size_group, label);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox),
                     iap_common_make_connection_entry_with_type(data.iap_name,
                                                                0, 0),
                     TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  /* username */
  data.username_entry = GTK_ENTRY(gtk_entry_new());
  gtk_editable_set_position(GTK_EDITABLE(data.username_entry), -1);
  im = hildon_gtk_entry_get_input_mode(data.username_entry);
  im &= ~(HILDON_GTK_INPUT_MODE_AUTOCAP | HILDON_GTK_INPUT_MODE_DICTIONARY);
  hildon_gtk_entry_set_input_mode(data.username_entry, im);
  gtk_box_pack_start(GTK_BOX(vbox),
                     hildon_caption_new(size_group,
                                        dgettext("osso-connectivity-ui",
                                                 "conn_set_iap_fi_username"),
                                        GTK_WIDGET(data.username_entry), 0, 0),
                     FALSE, FALSE, 0);

  /* password */
  data.password_entry = GTK_ENTRY(gtk_entry_new());
  gtk_editable_set_position(GTK_EDITABLE(data.password_entry), -1);
  hildon_gtk_entry_set_input_mode(GTK_ENTRY(data.password_entry),
                                  HILDON_GTK_INPUT_MODE_FULL |
                                  HILDON_GTK_INPUT_MODE_INVISIBLE);
  gtk_box_pack_start(GTK_BOX(vbox),
                     hildon_caption_new(size_group,
                                        dgettext("osso-connectivity-ui",
                                                 "conn_set_iap_fi_password"),
                                        GTK_WIDGET(data.password_entry), 0, 0),
                     FALSE, FALSE, 0);

  /* check button */
  data.check_button = GTK_CHECK_BUTTON(gtk_check_button_new());
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data.check_button), 1);
  gtk_box_pack_start(GTK_BOX(vbox),
                     hildon_caption_new(NULL,
                                        dgettext("osso-connectivity-ui",
                                                 "conn_set_iap_fi_ask_pw_every"),
                                        GTK_WIDGET(data.check_button), 0, 0),
                     FALSE, FALSE, 0);

  g_object_unref(size_group);

  /* dialog */
  dialog = gtk_dialog_new_with_buttons(
        dgettext("osso-connectivity-ui", "conn_ti_username_pw"),
        NULL,
        GTK_DIALOG_NO_SEPARATOR | GTK_DIALOG_MODAL,
        dgettext("hildon-libs", "wdgt_bd_done"),
        GTK_RESPONSE_OK, NULL);

  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), vbox);

  iap_common_set_close_response(dialog, GTK_RESPONSE_CANCEL);

  g_signal_connect(G_OBJECT(dialog), "response",
                   G_CALLBACK(iap_dialog_password_response_cb), &data);

  /* the rest */
  val = iap_settings_get_gconf_value(data.iap_name, "ask_password");

  if (val)
  {
    ask_password = gconf_value_get_bool(val);
    gconf_value_free(val);

    if (iap_settings_iap_is_gprs(&data))
    {
      iap_settings_get_iap_type_credentials(&data, "gprs",
                                            &username, &password);
    }
    else
    {
      iap_settings_get_iap_type_credentials(&data, "dun",
                                            &username, &password);
    }
  }
  else
  {
    val = iap_settings_get_gconf_value(data.iap_name,
                                       "EAP_MSCHAPV2_password_prompt");

    if (val)
    {
      /* why int but not bool ? */
      ask_password = gconf_value_get_int(val);
      gconf_value_free(val);
      iap_settings_get_iap_type_credentials(&data, "EAP_MSCHAPV2",
                                            &username, &password);
    }
    else if (iap_settings_iap_is_gprs(&data))
    {
      iap_settings_get_iap_type_credentials(&data, "gprs",
                                            &username, &password);
      ask_password = FALSE;
    }
    else
    {
      CONNUI_ERR("Unable to load credentials for IAP %s", data.iap_name);
      ask_password = TRUE;
    }
  }

  if (!usr || !*usr)
    usr = username;

  if (!pwd || !*pwd)
    pwd = password;

  gtk_entry_set_text(data.username_entry, usr);

  if (ask_password)
  {
    gtk_widget_show_all(dialog);
    gtk_widget_grab_focus(GTK_WIDGET(data.password_entry));
    g_free(username);
    g_free(password);
  }
  else
  {
    gtk_entry_set_text(data.password_entry, pwd);
    g_free(username);
    g_free(password);
    g_idle_add(iap_dialog_password_response_ok_cb, dialog);
  }

  return TRUE;
}

static gboolean
iap_dialog_password_cancel(DBusMessage *message)
{
  DBusError error;
  const gchar *iap_name;
  const gchar *password;
  const gchar *username;

  dbus_error_init(&error);

  if (!dbus_message_get_args(message, &error,
                             DBUS_TYPE_STRING, &username,
                             DBUS_TYPE_STRING, &password,
                             DBUS_TYPE_STRING, &iap_name,
                             DBUS_TYPE_INVALID))
  {
    CONNUI_ERR("could not get arguments: %s", error.message);
    dbus_error_free(&error);
    return FALSE;
  }

  return iap_dialog_password_send_reply(FALSE, dbus_message_get_sender(message),
                                        username, "", iap_name);
}
