#include <hildon/hildon.h>
#include <libosso.h>
#include <osso-ic-gconf.h>
#include <connui/connui.h>
#include <connui/connui-log.h>
#include <connui/connui-cell-renderer-operator.h>
#include <icd/network_api_defines.h>
#include <icd/dbus_api.h>

#include <string.h>
#include <stdlib.h>

#include "stage.h"
#include "wizard.h"

#include "connections.h"

enum
{
  IAP_CONNECTIONS_COLUMN_PIXBUF = 0,
  IAP_CONNECTIONS_COLUMN_NAME = 1,
  IAP_CONNECTIONS_COLUMN_ID = 2,
  IAP_CONNECTIONS_COLUMN_TYPE = 3,
  IAP_CONNECTIONS_COLUMN_SERVICE_TYPE = 4,
  IAP_CONNECTIONS_COLUMN_SERVICE_ID = 5
};

struct column_data
{
  GtkTreeSelection *selection;
  gint col_id;
  gchar *text;
  gboolean found;
};

static int
cmp_network_id(network_entry *a, network_entry *b)
{
  return strcmp(a->network_id, b->network_id);
}

static network_entry *
get_network_entry(struct iap_connections *conns, gchar *iap)
{
  GSList *l;
  network_entry data;

  g_return_val_if_fail(conns != NULL, NULL);

  data.network_id = iap;
  l = g_slist_find_custom(conns->networks, &data, (GCompareFunc)cmp_network_id);

  if (l)
    return (network_entry *)l->data;

  return NULL;
}

void
iap_connections_save_state(struct iap_connections *connections,
                           GByteArray *array)
{
  GtkTreeIter iter;
  gchar *iap = NULL;
  GtkTreeModel *model;
  guint8 len;

  if (gtk_tree_selection_get_selected(connections->selection, &model, &iter))
    gtk_tree_model_get(model, &iter, IAP_CONNECTIONS_COLUMN_ID, &iap, -1);

  if (iap)
    len = strlen(iap) + 1;
  else
    len = 0;

  g_byte_array_append(array, &len, sizeof(len));

  if (len)
    g_byte_array_append(array, (const guint8 *)iap, len);

  g_free(iap);

  if (connections->wizard)
    len = 1;
  else
    len = 0;

  g_byte_array_append(array, &len, sizeof(len));

  if (len)
    iap_wizard_save_state(connections->wizard, array);
}

static gboolean
tree_model_is_empty_cb(GtkTreeModel *model, GtkTreePath *path,
                       GtkTreeIter *iter, gpointer data)
{
  *(gboolean *)data = FALSE;

  return TRUE;
}

static gboolean
tree_model_is_empty(GtkTreeModel *tree_model)
{
  gboolean empty = TRUE;

  gtk_tree_model_foreach(tree_model, tree_model_is_empty_cb, &empty);

  return empty;
}

static void
iap_network_entry_free(network_entry *entry)
{
  iap_network_entry_clear(entry);
  g_free(entry);
}

static void
iap_connections_inetstate_cb(enum inetstate_status status, network_entry *entry,
                             gpointer user_data)
{
  struct iap_connections *conns = user_data;

  g_return_if_fail(conns != NULL);

  if (status == INETSTATE_STATUS_OFFLINE)
  {
    g_slist_foreach(conns->networks, (GFunc)iap_network_entry_free, NULL);
    g_slist_free(conns->networks);
    conns->networks = NULL;
  }
  else
  {
    if (entry && entry->network_id &&
        entry->network_attributes & ICD_NW_ATTR_IAPNAME)
    {
      GSList *found = g_slist_find_custom(
            conns->networks, entry, (GCompareFunc)iap_network_entry_compare);

      if (status == INETSTATE_STATUS_ONLINE ||
          status == INETSTATE_STATUS_DISCONNECTED ||
          status == INETSTATE_STATUS_DISCONNECTING)
      {
        if (found)
        {
          g_free(found->data);
          conns->networks = g_slist_delete_link(conns->networks, found);
        }
      }
      else if (!found)
      {
        conns->networks = g_slist_prepend(conns->networks,
                                          iap_network_entry_dup(entry));
      }
    }
  }
}

static gboolean
selection_set_cursor_to_column_data_match_cb(GtkTreeModel *model,
                                             GtkTreePath *path,
                                             GtkTreeIter *iter,
                                             gpointer user_data)
{
  struct column_data *data = user_data;
  gchar *s;

  gtk_tree_model_get(model, iter, data->col_id, &s, -1);

  if (!data->text || !s || !strcmp(data->text, s))
  {
    GtkTreeView *tree_view;

    gtk_tree_selection_select_iter(data->selection, iter);
    tree_view = gtk_tree_selection_get_tree_view(data->selection);
    gtk_tree_view_set_cursor(tree_view, path, NULL, FALSE);
    data->found = TRUE;
    g_free(s);
    return TRUE;
  }

  g_free(s);

  return FALSE;
}

static void
iap_connections_selection_changed_cb(GtkTreeSelection *selection,
                                     struct iap_connections *conns)
{
  gboolean edit_sensitive;
  gboolean delete_sensitive;
  GtkTreeIter iter;
  gchar *type;

  if (gtk_tree_selection_get_selected(selection, NULL, &iter))
  {
    gtk_tree_model_get(GTK_TREE_MODEL(conns->list_store), &iter,
                       IAP_CONNECTIONS_COLUMN_TYPE, &type,
                       -1);

    if (type && !strncmp(type, "GPRS", 4))
    {
      hildon_helper_set_insensitive_message(conns->delete_button, NULL);
      delete_sensitive = FALSE;
      edit_sensitive = TRUE;
    }
    else
    {
      edit_sensitive = !tree_model_is_empty(GTK_TREE_MODEL(conns->list_store));
      delete_sensitive = edit_sensitive;
      hildon_helper_set_insensitive_message(conns->delete_button,
                                            _("conn_ib_no_conn_del"));
      hildon_helper_set_insensitive_message(conns->edit_button,
                                            _("conn_ib_no_conn_edit"));
    }

    g_free(type);
    gtk_widget_set_sensitive(conns->edit_button, edit_sensitive);
    gtk_widget_set_sensitive(conns->delete_button, delete_sensitive);
  }
}

static void
iap_connections_fill_iaps(struct iap_connections *conns)
{
  GtkTreeModel *model = gtk_tree_view_get_model(conns->tree_view);
  struct column_data data;
  gboolean sensitive;

  data.found = FALSE;

  gtk_list_store_clear(conns->list_store);

  iap_settings_get_iap_list(conns->list_store, NULL,
                            IAP_CONNECTIONS_COLUMN_TYPE,
                            IAP_CONNECTIONS_COLUMN_ID,
                            IAP_CONNECTIONS_COLUMN_NAME,
                            IAP_CONNECTIONS_COLUMN_PIXBUF,
                            IAP_CONNECTIONS_COLUMN_SERVICE_TYPE,
                            IAP_CONNECTIONS_COLUMN_SERVICE_ID);

  if (conns->iap_id)
  {
    data.selection = conns->selection;
    data.text = conns->iap_id;
    data.col_id = IAP_CONNECTIONS_COLUMN_ID;
    gtk_tree_model_foreach(model, selection_set_cursor_to_column_data_match_cb,
                           &data);
  }

  if (!data.found)
  {
    GtkTreeIter iter;

    if (!tree_model_is_empty(GTK_TREE_MODEL(conns->list_store)))
    {
      gtk_tree_model_get_iter_first(GTK_TREE_MODEL(conns->list_store), &iter);
      gtk_tree_selection_select_iter(conns->selection, &iter);
      g_free(conns->iap_id);
      conns->iap_id = NULL;
    }
  }

  sensitive = !tree_model_is_empty(GTK_TREE_MODEL(conns->list_store));
  gtk_widget_set_sensitive(conns->edit_button, sensitive);
  gtk_widget_set_sensitive(conns->delete_button, sensitive);
  iap_connections_selection_changed_cb(conns->selection, conns);
}

static void
gconf_changed_cb(GConfClient *client, const gchar *key, GConfValue *value,
                 void *data)
{
  iap_connections_fill_iaps((struct iap_connections *)data);
}

void
iap_connections_show(struct iap_connections *conns)
{
  gtk_widget_show_all(conns->dialog);
  iap_connections_selection_changed_cb(conns->selection, conns);

  if (conns->wizard)
    iap_wizard_show(conns->wizard);
  else
  {
    if (tree_model_is_empty(GTK_TREE_MODEL(conns->list_store)))
      gtk_dialog_response(GTK_DIALOG(conns->dialog), IAP_CONNECTIONS_NEW);
  }
}

static gint
iap_connections_sort_func(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b,
                          gpointer user_data)
{
  gint rv = 0;
  gchar *svc_type_b;
  gchar *svc_type_a;
  gchar *name_b;
  gchar *name_a;

  gtk_tree_model_get(model, a,
                     IAP_CONNECTIONS_COLUMN_SERVICE_TYPE, &svc_type_a, -1);
  gtk_tree_model_get(model, b,
                     IAP_CONNECTIONS_COLUMN_SERVICE_TYPE, &svc_type_b, -1);

  if (!svc_type_a && svc_type_b)
    rv = 1;
  else if (svc_type_a && !svc_type_b)
    rv = -1;

  g_free(svc_type_a);
  g_free(svc_type_b);

  if (rv)
    return rv;

  gtk_tree_model_get(model, a, IAP_CONNECTIONS_COLUMN_NAME, &name_a, -1);
  gtk_tree_model_get(model, b, IAP_CONNECTIONS_COLUMN_NAME, &name_b, -1);

  if (!name_a && name_b)
    rv = 1;
  else if (name_a && !name_b)
    rv = -1;
  else
    rv = strcoll(name_a, name_b);

  g_free(name_a);
  g_free(name_b);

  return rv;
}

static void
restart_connection(GtkDialog *dialog, gint response_id, gpointer user_data)
{
  struct iap_connections *conns = user_data;
  gchar *iap_id;
  network_entry *entry = NULL;
  gchar *type = NULL;
  gboolean reconnect = FALSE;
  struct stage s;
  GError *err = NULL;

  if (response_id == WIZARD_BUTTON_FINISH)
  {
    struct stage *active_stage = iap_wizard_get_active_stage(conns->wizard);

    if (active_stage)
      type = stage_get_string(active_stage, "type");

    iap_id = iap_wizard_get_iap_id(conns->wizard);

    if (iap_id)
    {
      entry = get_network_entry(conns, iap_id);

      if (entry)
      {
        gchar *argv[] = {"iapsettings-restart-connection", NULL};
        GPid pid = 0;

        entry = iap_network_entry_dup(entry);
        gtk_widget_set_sensitive(dialog->action_area, FALSE);

        if (!g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, 0, NULL, &pid,
                           &err))
        {
          ULOG_DEBUG(err->message);
          g_clear_error(&err);
        }

        g_spawn_close_pid(pid);
        reconnect = TRUE;
      }
    }
    else
      iap_id = iap_settings_create_iap_id();

    if ((!type || strcmp(type, "WIMAX")) && iap_id)
    {
      stage_create_for_iap(&s, iap_id);
      gconf_client_remove_dir(conns->gconf, ICD_GCONF_PATH, &err);

      if (err)
      {
        CONNUI_ERR("GConf error: %s", err->message);
        g_clear_error(&err);
        iap_wizard_export(conns->wizard, &s, reconnect);
        stage_free(&s);
      }
      else
      {
        iap_wizard_export(conns->wizard, &s, reconnect);
        stage_free(&s);
        gconf_client_add_dir(conns->gconf, ICD_GCONF_PATH,
                             GCONF_CLIENT_PRELOAD_ONELEVEL, &err);
      }

      if (err)
      {
        CONNUI_ERR("GConf error: %s", err->message);
        g_clear_error(&err);
      }

      g_free(conns->iap_id);
      conns->iap_id = iap_id;
      iap_connections_fill_iaps(conns);
    }

    g_free(type);

    if (entry)
    {
      network_entry *entries[] = {entry, NULL};

      if (!iap_network_entry_connect(ICD_CONNECTION_FLAG_UI_EVENT, entries))
        CONNUI_ERR("Unable to reconnect");

      iap_network_entry_free(entry);
    }
  }

  if (response_id == WIZARD_BUTTON_FINISH || response_id == WIZARD_BUTTON_CLOSE)
  {
    iap_wizard_destroy(conns->wizard);
    conns->wizard = NULL;
  }
}

static void
iap_connections_response_cb(GtkDialog *dialog, gint response_id,
                            gpointer user_data)
{
  struct iap_connections *conns = user_data;
  struct stage stage;
  gchar *type = NULL;
  gchar *name = NULL;
  gchar *iap = NULL;
  static struct stage *active_stage;

  if (response_id == IAP_CONNECTIONS_EDIT ||
      response_id == IAP_CONNECTIONS_DELETE)
  {
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (!gtk_tree_selection_get_selected(conns->selection, &model, &iter))
      return;

    gtk_tree_model_get(model,&iter,
                       IAP_CONNECTIONS_COLUMN_ID, &iap,
                       IAP_CONNECTIONS_COLUMN_NAME, &name,
                       IAP_CONNECTIONS_COLUMN_TYPE, &type,
                       -1);
  }

  memset(&stage, 0, sizeof(stage));
  conns->response_id = response_id;

  switch (response_id)
  {
    case IAP_CONNECTIONS_NEW:
    case IAP_CONNECTIONS_EDIT:
    {
      if (response_id == IAP_CONNECTIONS_NEW)
      {
        conns->wizard  = iap_wizard_create(conns->osso, GTK_WINDOW(dialog));
        iap_wizard_set_empty_values(conns->wizard );
      }
      else
      {
        stage_create_for_iap(&stage, iap);
        conns->wizard = iap_wizard_create(conns->osso, GTK_WINDOW(dialog));
        iap_wizard_import(conns->wizard , &stage);
        iap_wizard_set_start_page(conns->wizard, "NAME_AND_TYPE");
      }

      g_signal_connect(G_OBJECT(conns->wizard->dialog), "response",
                       G_CALLBACK(restart_connection), conns);
      iap_wizard_show(conns->wizard);
      break;
    }
    case IAP_CONNECTIONS_DELETE:
    {
      network_entry *entry = get_network_entry(conns, iap);
      const char *msgid;
      gchar *desc;
      GtkWidget *note;

      if (entry)
        msgid = "conn_del_iap_fi_disconnect_delete";
      else
        msgid = "conn_del_iap_fi_delete";

      desc = g_strdup_printf(_(msgid), name);
      note = hildon_note_new_confirmation(GTK_WINDOW(dialog), desc);
      response_id = gtk_dialog_run(GTK_DIALOG(note));
      gtk_widget_destroy(note);
      g_free(desc);

      if (response_id == GTK_RESPONSE_OK)
      {
        if (entry)
          iap_network_entry_disconnect(ICD_CONNECTION_FLAG_UI_EVENT, entry);

        iap_settings_remove_iap(iap);
      }
      break;
    }
  }

  if (active_stage)
  {
    stage_free(active_stage);
    g_free(active_stage);
    active_stage = NULL;
  }

  if (conns->wizard)
  {
    if (conns->wizard->stage == &stage)
    {
      active_stage = (struct stage *)g_malloc(sizeof(struct stage));
      stage_create_cache(active_stage, &stage);
      iap_wizard_set_active_stage(conns->wizard, active_stage);
    }
  }

  stage_free(&stage);
  g_free(name);
  g_free(iap);
  g_free(type);
}

struct iap_connections *
iap_connections_create(osso_context_t *osso, GtkWindow *parent)
{
  GtkDialogFlags flags;
  GtkDialog *dialog;
  GtkListStore *list_store;
  GtkWidget *tree_view;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkWidget *scrolled_window;
  struct iap_connections *conns = g_new0(struct iap_connections, 1);
  conns->osso = osso;
  conns->gconf = gconf_client_get_default();

  if (conns->gconf)
  {
    GError *error = NULL;

    g_signal_connect(conns->gconf, "value_changed",
                     G_CALLBACK(gconf_changed_cb), conns);
    gconf_client_add_dir(conns->gconf, ICD_GCONF_PATH,
                         GCONF_CLIENT_PRELOAD_ONELEVEL, &error);

    if (error)
    {
      ULOG_ERR("GConf error: %s", error->message);
      g_clear_error(&error);
    }
  }
  else
    DLOG_ERR("Unable to get default GConf client!");

  if ( parent )
    flags = 7;
  else
    flags = 4;

  conns->dialog = gtk_dialog_new_with_buttons(_("conn_iaps_ti_connections"),
                                              parent, flags, NULL);

  dialog = GTK_DIALOG(conns->dialog);

  gtk_dialog_add_button(dialog, _("conn_iaps_bd_new"), IAP_CONNECTIONS_NEW);
  conns->edit_button = gtk_dialog_add_button(dialog, _("conn_iaps_bd_edit"),
                                             IAP_CONNECTIONS_EDIT);
  conns->delete_button = gtk_dialog_add_button(dialog, _("conn_iaps_bd_delete"),
                                               IAP_CONNECTIONS_DELETE);
  gtk_dialog_add_button(dialog, _("conn_iaps_bd_done"), IAP_CONNECTIONS_DONE);
  hildon_helper_set_insensitive_message(conns->edit_button,
                                        _("conn_ib_no_conn_edit"));
  hildon_helper_set_insensitive_message(conns->delete_button,
                                        _("conn_ib_no_conn_del"));
  iap_common_set_close_response(conns->dialog, IAP_CONNECTIONS_DONE);

  list_store = gtk_list_store_new(6, GDK_TYPE_PIXBUF, G_TYPE_STRING,
                                  G_TYPE_STRING, G_TYPE_STRING,
                                  G_TYPE_STRING, G_TYPE_STRING);
  conns->list_store = list_store;
  gtk_tree_sortable_set_default_sort_func(
        GTK_TREE_SORTABLE(list_store), iap_connections_sort_func, NULL, NULL);
  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(list_store), -1, 0);
  tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));
  conns->tree_view = GTK_TREE_VIEW(tree_view);
  g_object_unref(G_OBJECT(conns->list_store));

  conns->selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
  gtk_tree_selection_set_mode(conns->selection, GTK_SELECTION_SINGLE);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree_view), FALSE);
  column = gtk_tree_view_column_new();
  renderer = gtk_cell_renderer_pixbuf_new();
  gtk_tree_view_column_pack_start(column, renderer, 0);
  gtk_tree_view_column_set_attributes(column, renderer, "pixbuf", NULL, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
  column = gtk_tree_view_column_new();
  renderer = connui_cell_renderer_operator_new();
  gtk_tree_view_column_pack_start(column, renderer, 1);
  gtk_tree_view_column_set_attributes(
        column, renderer,
        "service-type", IAP_CONNECTIONS_COLUMN_SERVICE_TYPE,
        "service-id", IAP_CONNECTIONS_COLUMN_SERVICE_ID,
        "service-text", IAP_CONNECTIONS_COLUMN_NAME,
        NULL);
  gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
  g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_tree_view_column_set_expand(column, TRUE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
  scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(scrolled_window), tree_view);
  gtk_widget_set_size_request(GTK_WIDGET(scrolled_window), 478, 240);

  if (!connui_inetstate_status(iap_connections_inetstate_cb, conns))
    CONNUI_ERR("Unable to register internet status change callback");

  iap_connections_fill_iaps(conns);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(conns->dialog)->vbox), scrolled_window, 1, 1, 0);

  g_signal_connect(G_OBJECT(conns->dialog), "response",
                   G_CALLBACK(iap_connections_response_cb), conns);
  g_signal_connect(G_OBJECT(conns->selection), "changed",
                   G_CALLBACK(iap_connections_selection_changed_cb), conns);
  return conns;
}

void
iap_connections_destroy(struct iap_connections *conns)
{
  if (conns->gconf)
  {
    GError *error = NULL;

    g_signal_handlers_disconnect_matched(
          conns->gconf, G_SIGNAL_MATCH_DATA | G_SIGNAL_MATCH_FUNC, 0, 0, 0,
          gconf_changed_cb, conns);

    gconf_client_remove_dir(conns->gconf, ICD_GCONF_PATH, &error);

    if (error)
    {
      CONNUI_ERR("GConf error: %s", error->message);
      g_clear_error(&error);
    }

    g_object_unref(conns->gconf);
  }

  connui_inetstate_close(iap_connections_inetstate_cb);

  if (conns->networks)
  {
    g_slist_foreach(conns->networks, (GFunc)g_free, NULL);
    g_slist_free(conns->networks);
  }

  gtk_widget_destroy(conns->dialog);
  g_free(conns);
}

gboolean
iap_connections_restore_state(struct iap_connections *conns,
                              struct stage_cache *cache)
{
  guint8 len;

  if (cache->processed + sizeof(len) > cache->len)
    return FALSE;

  len = cache->data[cache->processed];
  cache->processed += sizeof(len);

  if (len)
  {
    struct column_data data;

    if (cache->processed + len > cache->len)
      return FALSE;

    data.selection = conns->selection;
    data.text = (gchar *)&cache->data[cache->processed];
    cache->processed += len;
    data.col_id = IAP_CONNECTIONS_COLUMN_ID;
    gtk_tree_model_foreach(gtk_tree_view_get_model(conns->tree_view),
                           selection_set_cursor_to_column_data_match_cb, &data);
    iap_connections_selection_changed_cb(conns->selection, conns);
  }

  if (cache->processed + sizeof(len) > cache->len)
    return FALSE;

  len = cache->data[cache->processed];
  cache->processed += sizeof(len);

  if (len)
  {
    conns->wizard = iap_wizard_create(conns->osso, GTK_WINDOW(conns->dialog));
    iap_wizard_restore_state(conns->wizard, cache);
    g_signal_connect(G_OBJECT(conns->wizard->dialog), "response",
                     G_CALLBACK(restart_connection), conns);
  }

  return TRUE;
}
