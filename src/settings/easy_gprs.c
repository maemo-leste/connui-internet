#include <libosso.h>
#include <hildon/hildon.h>
#include <connui/connui.h>

#include <libintl.h>
#include <string.h>

#include "mapper.h"
#include "widgets.h"


MAPPER(hildoncheck, bool);

struct easy_gprs
{
  GtkWindow *dialog;
  gchar *iap;
  GHashTable *widgets;
  struct stage stage;
};

static struct stage_widget easy_gprs_widgets[] =
{
  {
    NULL,
    NULL,
    "NAME",
    "name",
    NULL,
    &mapper_entry2string,
    NULL
  },
  {
    NULL,
    NULL,
    "GPRS_AP_NAME",
    "gprs_accesspointname",
    NULL,
    &mapper_entry2string,
    NULL
  },
  {
    NULL,
    NULL,
    "GPRS_USERNAME",
    "gprs_username",
    NULL,
    &mapper_entry2string,
    NULL
  },
  {
    NULL,
    NULL,
    "GPRS_PASSWORD",
    "gprs_password",
    NULL,
    &mapper_entry2string,
    NULL
  },
  {
    NULL,
    NULL,
    "GPRS_ASK_PASSWORD",
    "ask_password",
    NULL,
    &mapper_hildoncheck2bool,
    NULL
  },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

static struct
{
  const char *id;
  const char *label;
  const char *msgid;
}
easy_wlan_fields[] =
{
  {"NAME", "NAME_CAPTION", "conn_set_iap_fi_conn_name"},
  {"GPRS_AP_NAME", "GPRS_AP_NAME_CAPTION", "conn_set_iap_fi_accessp_name"},
  {"GPRS_USERNAME", NULL, "conn_set_iap_fi_username"},
  {"GPRS_PASSWORD", "GPRS_PASSWORD_CAPTION", "conn_set_iap_fi_password"}
};

static void
bool2hildoncheck(const struct stage *s, GtkWidget *entry,
                 const struct stage_widget *sw)
{
  hildon_check_button_set_active(HILDON_CHECK_BUTTON(entry),
                                 stage_get_bool(s, sw->key));
}

static void
hildoncheck2bool(struct stage *s, const GtkWidget *entry,
                 const struct stage_widget *sw)
{
  stage_set_bool(s, sw->key,
                 hildon_check_button_get_active(HILDON_CHECK_BUTTON(entry)));
}

static void
iap_easy_gprs_ask_password_toggled_cb(GtkToggleButton *button,
                                      gpointer user_data)
{
  gtk_widget_set_sensitive(
        GTK_WIDGET(user_data),
        hildon_check_button_get_active(HILDON_CHECK_BUTTON(button)) == FALSE);
}

static void
iap_easy_gprs_name_chaged_cb(GtkEditable *editable, struct easy_gprs *egprs)
{
  const gchar *iap_name = iap_widgets_h22_entry_get_text(
        GTK_WIDGET(g_hash_table_lookup(egprs->widgets, "NAME")));
  gboolean sensitive =
      iap_name && *iap_name && !iap_settings_iap_exists(iap_name, egprs->iap);

  gtk_dialog_set_response_sensitive(GTK_DIALOG(egprs->dialog), GTK_RESPONSE_OK,
                                    sensitive);
}

static GtkWidget *
iap_easy_gprs_get_widget(gpointer user_data, const gchar *id)
{
  struct easy_gprs *egprs = (struct easy_gprs *)user_data;

  return GTK_WIDGET(g_hash_table_lookup(egprs->widgets, id));
}

gchar *
iap_run_easy_gprs_dialogs(osso_context_t *osso, GtkWindow *parent,
                          const gchar *iap)
{
  GtkDialogFlags dialog_flags = GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR;
  GtkWidget *hbox;
  GtkWidget *h22_entry;
  GtkWidget *label;
  GtkWidget *gprs_ask_password;
  GtkSizeGroup *size_group;
  GtkWidget *dialog;
  GtkWidget *vbox;
  struct easy_gprs egprs;
  struct stage s;
  int i;

  memset(&egprs, 0, sizeof(egprs));

  egprs.widgets = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
  egprs.iap = g_strdup(iap);

  stage_create_for_iap(&s, egprs.iap);
  stage_create_cache(&egprs.stage, NULL);
  stage_copy(&s, &egprs.stage);
  stage_set_string(&egprs.stage, "name",
                   dgettext("osso-connectivity-ui",
                            "conn_va_placeholder_iap_name"));

  if (parent)
    dialog_flags |= GTK_DIALOG_DESTROY_WITH_PARENT;

  dialog = hildon_dialog_new_with_buttons(
        dgettext("osso-connectivity-ui", "conn_fi_placeholder_iap_settings"),
        parent, dialog_flags,
        dgettext("hildon-libs", "wdgt_bd_save"), GTK_RESPONSE_OK,
        NULL);

  egprs.dialog = GTK_WINDOW(dialog);
  iap_common_set_close_response(dialog, GTK_RESPONSE_CANCEL);
  vbox = GTK_DIALOG(dialog)->vbox;
  size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

  for (i = 0; i < G_N_ELEMENTS(easy_wlan_fields); i++)
  {
    HildonGtkInputMode im;

    hbox = gtk_hbox_new(0, 8);
    h22_entry = iap_widgets_create_h22_entry();
    im = hildon_gtk_entry_get_input_mode(GTK_ENTRY(h22_entry));
    hildon_gtk_entry_set_input_mode(
          GTK_ENTRY(h22_entry), im & ~HILDON_GTK_INPUT_MODE_AUTOCAP);
    g_hash_table_insert(egprs.widgets, g_strdup(easy_wlan_fields[i].id),
                        h22_entry);

    label = gtk_label_new(dgettext("osso-connectivity-ui",
                                   easy_wlan_fields[i].msgid));

    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_size_group_add_widget(size_group, label);

    if (easy_wlan_fields[i].label)
    {
      g_hash_table_insert(egprs.widgets, g_strdup(easy_wlan_fields[i].label),
                          label);
    }

    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), h22_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  }

  gtk_entry_set_visibility(
        GTK_ENTRY(GTK_WIDGET(g_hash_table_lookup(egprs.widgets,
                                                 "GPRS_PASSWORD"))),
        FALSE);
  gprs_ask_password = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
  gtk_button_set_label(GTK_BUTTON(gprs_ask_password),
                       dgettext("osso-connectivity-ui",
                                "conn_set_iap_fi_ask_pw_every"));
  g_hash_table_insert(egprs.widgets, g_strdup("GPRS_ASK_PASSWORD"),
                                              gprs_ask_password);
  g_signal_connect(G_OBJECT(gprs_ask_password), "toggled",
                   G_CALLBACK(iap_easy_gprs_ask_password_toggled_cb),
                   GTK_WIDGET(g_hash_table_lookup(egprs.widgets,
                                                  "GPRS_PASSWORD")));

  g_signal_connect(G_OBJECT(g_hash_table_lookup(egprs.widgets, "NAME")),
                   "changed", G_CALLBACK(iap_easy_gprs_name_chaged_cb),
                   &egprs);

  gtk_box_pack_start(GTK_BOX(vbox), gprs_ask_password, FALSE, FALSE, 0);

  g_object_unref(G_OBJECT(size_group));
  gtk_widget_show_all(dialog);

  mapper_import_widgets(&egprs.stage, easy_gprs_widgets,
                        iap_easy_gprs_get_widget, &egprs);

  if (gtk_dialog_run(GTK_DIALOG(egprs.dialog)) == GTK_RESPONSE_OK)
  {
    mapper_export_widgets(&egprs.stage, easy_gprs_widgets,
                          iap_easy_gprs_get_widget, &egprs);

    stage_copy(&egprs.stage, &s);
  }
  else
  {
    g_free(egprs.iap);
    egprs.iap = NULL;
  }

  gtk_widget_destroy(GTK_WIDGET(egprs.dialog));
  stage_free(&egprs.stage);
  stage_free(&s);
  g_hash_table_destroy(egprs.widgets);

  return egprs.iap;
}
