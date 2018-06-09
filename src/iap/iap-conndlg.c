#include <libintl.h>
#include <connui/iap-network.h>
#include <connui/connui.h>
#include <connui/connui-box-view.h>
#include <connui/connui-conndlgs.h>
#include <connui/connui-flightmode.h>
#include <connui/connui-log.h>
#include <connui/connui-scan-box-view.h>
#include <connui/libicd-network-wlan-dev.h>
#include <icd/dbus_api.h>
#include <icd/network_api_defines.h>
#include <mce/mode-names.h>

#include <string.h>

#include "easy-wlan.h"
#include "easy-gprs.h"

static void iap_conndlg_close();

IAP_DIALOGS_PLUGIN_DEFINE_EXTENDED(iap_conndlg, ICD_UI_SHOW_CONNDLG_REQ,
                                   iap_common_activate_iap(NULL);)

struct iap_conndlg_private
{
  GtkWidget *dialog;
  GtkWidget *scan_box_view;
  GtkWidget *no_conn;
  osso_context_t *libosso;
  gboolean selected;
  network_entry entry;
  gboolean ui_disabled;
  int iap_id;
  iap_dialogs_done_fn done;
  dbus_bool_t offline;
};

static struct iap_conndlg_private *private = NULL;

static struct iap_conndlg_private **
get_private()
{
  if (!private)
    private = g_new0(struct iap_conndlg_private, 1);

  return &private;
}

static void
iap_conndlg_flightmode_cb(dbus_bool_t offline, gpointer user_data)
{
  struct iap_conndlg_private **iap_conndlg =
      (struct iap_conndlg_private **)user_data;

  g_return_if_fail(iap_conndlg != NULL);

  (*iap_conndlg)->offline = offline;

  if (offline)
  {
    iap_common_activate_iap(NULL);
    iap_conndlg_close();
  }
}

static void
iap_conndlg_inetstate_cb(enum inetstate_status status, network_entry *entry,
                         gpointer user_data)
{
  struct iap_conndlg_private **iap_conndlg =
      (struct iap_conndlg_private **)user_data;

  g_return_if_fail(iap_conndlg != NULL && (*iap_conndlg) != NULL &&
      (*iap_conndlg)->dialog != NULL);

  if (status == INETSTATE_STATUS_CONNECTING ||
      status == INETSTATE_STATUS_CONNECTED)
  {
    connui_scan_entry *scan_entry = g_new0(connui_scan_entry, 1);

    entry = iap_network_entry_dup(entry);

    scan_entry->network.service_type = entry->service_type;
    scan_entry->network.service_attributes = entry->service_attributes;
    scan_entry->network.service_id = entry->service_id;
    scan_entry->network.network_type = entry->network_type;
    scan_entry->network.network_attributes = entry->network_attributes;
    scan_entry->network.network_id = entry->network_id;
    scan_entry->network_priority = 200;

    g_free(entry);

    gtk_window_set_title(GTK_WINDOW((*iap_conndlg)->dialog),
                         dgettext("osso-connectivity-ui",
                                  "conn_ti_connect_disconnect_dialog_title"));

    if (!iap_scan_add_scan_entry(scan_entry, TRUE))
      iap_scan_free_scan_entry(scan_entry);
  }
}

static void
selection_changed(GtkTreeSelection *selection,
                  gpointer user_data)
{
  struct iap_conndlg_private **iap_conndlg =
      (struct iap_conndlg_private **)user_data;
  gboolean selected;

  selected = gtk_tree_selection_get_selected(selection, NULL, NULL);

  if (selected != (*iap_conndlg)->selected)
  {
    (*iap_conndlg)->selected = selected;
    gtk_dialog_set_response_sensitive(GTK_DIALOG((*iap_conndlg)->dialog),
                                      GTK_RESPONSE_OK, selected);
  }
}

static void
show_no_conn_available(struct iap_conndlg_private **iap_conndlg)
{
  GList *l =
      gtk_container_get_children(GTK_CONTAINER((*iap_conndlg)->scan_box_view));

  if (!g_list_length(l) && !(*iap_conndlg)->no_conn)
  {
    (*iap_conndlg)->no_conn =
        gtk_label_new(dgettext("osso-connectivity-ui",
                               "conn_fi_no_conn_available"));
    gtk_misc_set_alignment(GTK_MISC((*iap_conndlg)->no_conn), 0.5, 0.0);
    gtk_container_add(GTK_CONTAINER((*iap_conndlg)->scan_box_view),
                      (*iap_conndlg)->no_conn);
    gtk_widget_show_all((*iap_conndlg)->no_conn);
    gtk_widget_set_sensitive(GTK_WIDGET((*iap_conndlg)->scan_box_view), FALSE);
    (*iap_conndlg)->ui_disabled = TRUE;
  }

  g_list_free(l);
}

static void
scan_started(gpointer user_data)
{
  struct iap_conndlg_private **iap_conndlg =
      (struct iap_conndlg_private **)user_data;

  if ((*iap_conndlg)->ui_disabled)
  {
    if ((*iap_conndlg)->no_conn)
    {
      gtk_widget_destroy((*iap_conndlg)->no_conn);
      (*iap_conndlg)->no_conn = NULL;
    }

    gtk_widget_set_sensitive(GTK_WIDGET((*iap_conndlg)->scan_box_view), TRUE);
    (*iap_conndlg)->ui_disabled = FALSE;
  }

  if ((*iap_conndlg)->dialog)
  {
    hildon_gtk_window_set_progress_indicator(GTK_WINDOW((*iap_conndlg)->dialog),
                                             TRUE);
  }
}

static void
scan_stopped(gpointer user_data)
{
  struct iap_conndlg_private **iap_conndlg =
      (struct iap_conndlg_private **)user_data;

  if ((*iap_conndlg)->dialog)
  {
    hildon_gtk_window_set_progress_indicator(GTK_WINDOW((*iap_conndlg)->dialog),
                                             FALSE);
  }

  show_no_conn_available(iap_conndlg);
}

static void stop_scan()
{
  iap_scan_stop();
}

static void
iap_conndlg_display_event_cb(const char *status,
                             struct iap_conndlg_private **iap_conndlg)
{
  if (iap_conndlg && *iap_conndlg)
  {
    if (!g_strcmp0(status, MCE_DISPLAY_OFF_STRING))
      stop_scan();
    else if (!g_strcmp0(status, MCE_DISPLAY_ON_STRING))
    {
      connui_inetstate_close(iap_conndlg_inetstate_cb);

      if (!connui_inetstate_status(iap_conndlg_inetstate_cb, iap_conndlg))
        CONNUI_ERR("Unable to register inetstate callback");

      if (GTK_WIDGET_VISIBLE((*iap_conndlg)->dialog))
      {
        if (!iap_scan_start(1, scan_started, scan_stopped, NULL,
                            (*iap_conndlg)->scan_box_view,
                            &(*iap_conndlg)->entry, selection_changed,
                            iap_conndlg))
        {
          show_no_conn_available(iap_conndlg);
        }
      }
    }
  }
}

static void
iap_conndlg_close()
{
  struct iap_conndlg_private **  priv = get_private();

  if (priv && *priv)
  {
    connui_inetstate_close(iap_conndlg_inetstate_cb);
    connui_flightmode_close(iap_conndlg_flightmode_cb);
    connui_display_event_close(iap_conndlg_display_event_cb);
    iap_scan_close();

    if (GTK_IS_WIDGET((*priv)->dialog))
      gtk_widget_destroy((*priv)->dialog);

    if (GTK_IS_WIDGET((*priv)->scan_box_view))
      g_object_unref((*priv)->scan_box_view);

    iap_network_entry_clear(&(*priv)->entry);

    if ((*priv)->done)
      (*priv)->done((*priv)->iap_id, FALSE);

    g_free((*priv));
    *priv = NULL;
  }
  else
    CONNUI_ERR("not initialized");
}

static void
iap_conndlg_activate_iap_connection(GtkTreeModel *model, GtkTreeIter *iter,
                                    struct iap_conndlg_private **iap_conndlg)
{
  g_return_if_fail(*iap_conndlg != NULL);

  stop_scan();
  gtk_container_remove(
        GTK_CONTAINER(gtk_widget_get_parent((*iap_conndlg)->scan_box_view)),
        (*iap_conndlg)->scan_box_view);
  gtk_widget_destroy((*iap_conndlg)->dialog);
  (*iap_conndlg)->dialog = NULL;

  if (model && iter)
  {
    gboolean is_saved = FALSE;
    gboolean can_disconnect = FALSE;
    connui_scan_entry *scan_entry = NULL;

    gtk_tree_model_get(model, iter,
                       IAP_SCAN_LIST_IS_SAVED,
                       &is_saved,
                       IAP_SCAN_LIST_SCAN_ENTRY,
                       &scan_entry,
                       IAP_SCAN_LIST_CAN_DISCONNECT,
                       &can_disconnect,
                       -1);

    if (!can_disconnect)
    {
      if (scan_entry && scan_entry->network.network_type)
      {
        if (!scan_entry->network.service_type ||
            !*scan_entry->network.service_type)
        {
          gchar *chosen_network_id = NULL;

          if (!is_saved &&
              !strncmp(scan_entry->network.network_type, "WLAN_", 5))
          {
            dbus_uint32_t cap = 0;

            nwattr2cap(scan_entry->network.network_attributes, &cap);

            chosen_network_id = iap_run_easy_wlan_dialogs(
                  (*iap_conndlg)->libosso, GTK_WINDOW((*iap_conndlg)->dialog),
                  scan_entry->network.network_id, &cap);

            cap2nwattr(cap, &scan_entry->network.network_attributes);
          }
          else if (!strncmp(scan_entry->network.network_type, "GPRS", 4))
          {
            if (!scan_entry->network_name || !*scan_entry->network_name)
            {
              chosen_network_id =
                  iap_run_easy_gprs_dialogs((*iap_conndlg)->libosso,
                                            GTK_WINDOW((*iap_conndlg)->dialog),
                                            scan_entry->network.network_id);
            }
          }

          if (chosen_network_id)
          {
            g_free(scan_entry->network.network_id);
            scan_entry->network.network_id = chosen_network_id;
            scan_entry->network.network_attributes |= ICD_NW_ATTR_IAPNAME;
            g_slist_free(scan_entry->list);
            scan_entry->list = g_slist_append(NULL, scan_entry);
          }
        }
      }
    }
    else
      iap_network_entry_disconnect(ICD_CONNECTION_FLAG_UI_EVENT,
                                   &scan_entry->network);

    if (scan_entry && scan_entry->network.network_type &&
        scan_entry->network.network_id && !can_disconnect)
    {
      network_entry **entries =
          g_new0(network_entry *, g_slist_length(scan_entry->list) + 1);
      int i = 0;
      GSList *l;

      for (l = scan_entry->list; l; l = l->next)
        entries[i++] = l->data;

      iap_network_entry_connect(ICD_CONNECTION_FLAG_UI_EVENT, entries);
      g_free(entries);
      iap_common_set_last_used_network(&scan_entry->network);
    }
    else
    {
      CONNUI_ERR("Unable to activate connection, canceling the dialog");
      iap_common_activate_iap(NULL);
    }
  }
  else
    iap_common_activate_iap(NULL);

  iap_conndlg_close();
}

static void
scan_box_view_row_activated_cb(ConnuiBoxView *view, GtkTreePath *path,
                               struct iap_conndlg_private **iap_conndlg)
{
  GtkTreeModel *model = connui_box_view_get_model(view);
  GtkTreeIter iter;

  gtk_tree_model_get_iter(model, &iter, path);
  iap_conndlg_activate_iap_connection(model, &iter, iap_conndlg);
}

static void
select_connection_dialog_response_cb(GtkDialog *dialog, gint response_id,
                                     gpointer user_data)
{
  iap_conndlg_activate_iap_connection(NULL, NULL,
                                      (struct iap_conndlg_private **)user_data);
}

gboolean
iap_dialog_iap_conndlg_show(int iap_id, DBusMessage *message,
                            iap_dialogs_showing_fn showing,
                            iap_dialogs_done_fn done, osso_context_t *libosso)
{
  struct iap_conndlg_private **priv;
  GtkListStore *scan_store;
  GtkWidget *pannable;
  DBusError error;
  dbus_bool_t show_con_inf047;

  dbus_error_init(&error);

  if (!dbus_message_get_args(message, &error,
                             DBUS_TYPE_BOOLEAN, &show_con_inf047,
                             DBUS_TYPE_INVALID))
  {
    CONNUI_ERR("could not get arguments: %s", error.message);
    dbus_error_free(&error);
    return FALSE;
  }

  priv = get_private();

  if (!connui_flightmode_status(iap_conndlg_flightmode_cb, priv))
  {
    CONNUI_ERR("could not get flightmode status");

    return FALSE;
  }

  (*priv)->iap_id = iap_id;
  (*priv)->done = (iap_dialogs_done_fn)done;
  (*priv)->libosso = libosso;

  if ((*priv)->offline)
    return FALSE;

  if (!GTK_IS_WIDGET((*priv)->dialog))
  {
    iap_network_entry_clear(&(*priv)->entry);
    iap_common_get_last_used_network(&(*priv)->entry);
    scan_store = iap_scan_store_create(
          (GtkTreeIterCompareFunc)iap_scan_default_sort_func, &(*priv)->entry);
    (*priv)->scan_box_view =
        connui_scan_box_view_new_with_model(GTK_TREE_MODEL(scan_store));

    /* WHY !?! */
    g_object_ref((*priv)->scan_box_view);

    pannable = hildon_pannable_area_new();

    if (GTK_IS_TREE_VIEW((*priv)->scan_box_view))
      gtk_container_add(GTK_CONTAINER(pannable), (*priv)->scan_box_view);
    else
    {
      hildon_pannable_area_add_with_viewport(HILDON_PANNABLE_AREA(pannable),
                                             (*priv)->scan_box_view);
    }

    (*priv)->dialog =

        hildon_dialog_new_with_buttons(
          dgettext("osso-connectivity-ui", "conn_ti_connect_dialog_title"),
          0,
          GTK_DIALOG_NO_SEPARATOR | GTK_DIALOG_DESTROY_WITH_PARENT |
          GTK_DIALOG_MODAL,
          0);

    gtk_widget_set_name((*priv)->dialog, "selectconnectiondialog");

    gtk_dialog_set_response_sensitive(GTK_DIALOG((*priv)->dialog),
                                      GTK_RESPONSE_OK, FALSE);

    gtk_widget_set_size_request(GTK_WIDGET(pannable), 800, 350);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG((*priv)->dialog)->vbox),
                      pannable);
    iap_common_set_close_response((*priv)->dialog, GTK_RESPONSE_CANCEL);

    g_signal_connect(G_OBJECT((*priv)->dialog), "response",
                     (GCallback)select_connection_dialog_response_cb, priv);

    g_signal_connect(G_OBJECT((*priv)->scan_box_view), "row-activated",
                     (GCallback)scan_box_view_row_activated_cb, priv);
  }

  showing();

  gtk_widget_show_all((*priv)->dialog);

  if (!connui_display_event_status(iap_conndlg_display_event_cb, priv))
    CONNUI_ERR("could not get display state");

  if (show_con_inf047)
    iap_common_show_saved_not_found_banner(GTK_WINDOW((*priv)->dialog));

  return TRUE;
}
