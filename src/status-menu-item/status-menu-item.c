#include <hildon/hildon-button.h>
#include <libhildondesktop/libhildondesktop.h>
#include <libconnui.h>
#include <libintl.h>
#include <icd/dbus_api.h>

#include "config.h"

#define _(x) dgettext(GETTEXT_PACKAGE, x)

#define CONNUI_INTERNET_STATUS_MENU_ITEM_TYPE (connui_internet_status_menu_item_get_type())
#define CONNUI_INTERNET_STATUS_MENU_ITEM(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CONNUI_INTERNET_STATUS_MENU_ITEM_TYPE, ConnuiInternetStatusMenuItem))

typedef struct _ConnuiInternetStatusMenuItem ConnuiInternetStatusMenuItem;
typedef struct _ConnuiInternetStatusMenuItemClass ConnuiInternetStatusMenuItemClass;
typedef struct _ConnuiInternetStatusMenuItemPrivate ConnuiInternetStatusMenuItemPrivate;

struct _ConnuiInternetStatusMenuItem
{
  HDStatusMenuItem parent;
  ConnuiInternetStatusMenuItemPrivate *priv;
};

struct _ConnuiInternetStatusMenuItemClass
{
  HDStatusMenuItemClass parent;
};

struct _ConnuiInternetStatusMenuItemPrivate
{
  GtkWidget *conn_icon;
  GtkWidget *button;
  struct pixbuf_cache *pixbuf_cache;
  struct pixbuf_anim *pixbuf_anim;
  struct network_entry *network;
  int connection_state;
  gboolean is_active;
  gboolean is_displayed;
  int signals_set;
  gboolean suspended;
  guint32 suspendcode;
  gint signal_strength;
  osso_context_t *osso_context;
  osso_display_state_t display_state;
  struct timespec tp;
};

HD_DEFINE_PLUGIN_MODULE(ConnuiInternetStatusMenuItem,
                        connui_internet_status_menu_item,
                        HD_TYPE_STATUS_MENU_ITEM)

static void
connui_internet_status_menu_item_class_finalize(ConnuiInternetStatusMenuItemClass *klass)
{
}

static void
connui_internet_status_menu_item_request_select_connection(GtkButton *button,
                                                           gpointer user_data)
{
  DBusGConnection *bus;
  GError *error = NULL;

  if (!(bus = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error)))
  {
    if (error)
      ULOG_ERR("Error: %s", error->message);

    g_clear_error(&error);

    return;
  }

  DBusGProxy *proxy = dbus_g_proxy_new_for_name(bus,
                                                ICD_DBUS_API_INTERFACE,
                                                ICD_DBUS_API_PATH,
                                                ICD_DBUS_API_INTERFACE);
  dbus_g_connection_unref(bus);

  if (!proxy)
  {
    ULOG_ERR("Unable to get DBUS proxy for ICd2");

    return;
  }

  if (!dbus_g_proxy_call(proxy,
                         ICD_DBUS_API_SELECT_REQ,
                         &error,
                         G_TYPE_UINT, ICD_CONNECTION_FLAG_UI_EVENT,
                         G_TYPE_INVALID, G_TYPE_INVALID))
  {
    if (error)
      ULOG_ERR("Error: %s", error->message);

    g_clear_error(&error);
  }

  g_object_unref(G_OBJECT(proxy));
}

static void
_connui_internet_status_menu_item_inet_status_cb(int state,
                                                 struct network_entry *entry,
                                                 gpointer user_data)
{
  // FIXME
}

static void
connui_internet_status_menu_item_set_active_conn_info(ConnuiInternetStatusMenuItem *self)
{
  // FIXME
}

static void
connui_internet_status_menu_item_conn_strength_cb(struct network_entry *entry,
                                                  struct network_stats *statistics,
                                                  gpointer user_data)
{
  ConnuiInternetStatusMenuItem *self =
      CONNUI_INTERNET_STATUS_MENU_ITEM(user_data);
  ConnuiInternetStatusMenuItemPrivate *priv = self->priv;

  g_return_if_fail(priv != NULL && priv->conn_icon != NULL && statistics != NULL);

  if ( priv->signal_strength != statistics->signal_strength )
  {
    priv->signal_strength = statistics->signal_strength;
    connui_internet_status_menu_item_set_active_conn_info(self);
  }
}

static void
connui_internet_status_menu_item_conn_strength_start(ConnuiInternetStatusMenuItem *self)
{
  ConnuiInternetStatusMenuItemPrivate *priv = self->priv;

  if (priv->is_active && priv->is_displayed &&
      priv->display_state != OSSO_DISPLAY_OFF)
  {
    connui_inetstate_statistics_start(1000,
                                      connui_internet_status_menu_item_conn_strength_cb,
                                      self);
  }
}

static void
connui_internet_status_menu_item_conn_strength_stop(ConnuiInternetStatusMenuItem *self)
{
  connui_inetstate_statistics_stop(connui_internet_status_menu_item_conn_strength_cb);
  self->priv->signal_strength = 0;
}

static void
connui_internet_status_menu_item_is_displayed(GtkWidget *widget,
                                              gpointer user_data)
{
  ConnuiInternetStatusMenuItem *self =
      CONNUI_INTERNET_STATUS_MENU_ITEM(user_data);
  ConnuiInternetStatusMenuItemPrivate *priv = self->priv;

  priv->is_displayed = TRUE;

  if (priv->is_active)
    connui_internet_status_menu_item_conn_strength_start(self);
}

static void
connui_internet_status_menu_item_is_not_displayed(GtkWidget *widget,
                                                  gpointer user_data)
{

  ConnuiInternetStatusMenuItem *self =
      CONNUI_INTERNET_STATUS_MENU_ITEM(user_data);
  ConnuiInternetStatusMenuItemPrivate *priv = self->priv;

  priv->is_displayed = FALSE;

  if (priv->is_active)
    connui_internet_status_menu_item_conn_strength_stop(self);
}

static void
connui_internet_status_menu_item_parent_set_signal(GtkWidget *widget,
                                                   GtkObject *old_parent,
                                                   gpointer user_data)
{
  ConnuiInternetStatusMenuItem *self;
  ConnuiInternetStatusMenuItemPrivate *priv;

  self = CONNUI_INTERNET_STATUS_MENU_ITEM(user_data);
  priv = self->priv;

  if (priv->signals_set)
  {
    GObject *obj = G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(old_parent),
                                                    GTK_TYPE_WINDOW));

    g_signal_handlers_disconnect_matched(
          obj, G_SIGNAL_MATCH_DATA|G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
          connui_internet_status_menu_item_is_displayed, self);
    g_signal_handlers_disconnect_matched(
          obj, G_SIGNAL_MATCH_DATA|G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
          connui_internet_status_menu_item_is_not_displayed, self);

    priv->signals_set = 0;
  }

  widget = gtk_widget_get_ancestor(GTK_WIDGET(self), GTK_TYPE_WINDOW);
  if (widget)
  {
    g_signal_connect_data(
          G_OBJECT(widget), "map",
          G_CALLBACK(connui_internet_status_menu_item_is_displayed), self, NULL,
          0);
    g_signal_connect_data(
          G_OBJECT(widget), "unmap",
          G_CALLBACK(connui_internet_status_menu_item_is_not_displayed), self,
          NULL, 0);

    priv->signals_set = 1;
  }
}

static void
connui_internet_status_menu_item_cellular_data_suspended_status_cb(gboolean suspended,
                                                                   guint32 suspendcode,
                                                                   gpointer user_data)
{
  ConnuiInternetStatusMenuItem *self =
      CONNUI_INTERNET_STATUS_MENU_ITEM(user_data);
  ConnuiInternetStatusMenuItemPrivate *priv = self->priv;

  g_return_if_fail(priv != NULL);

  priv->suspendcode = suspendcode;
  priv->suspended = suspended;

  connui_internet_status_menu_item_set_active_conn_info(self);
}

static void
connui_internet_status_menu_item_display_cb(osso_display_state_t state,
                                            gpointer *user_data)
{

  ConnuiInternetStatusMenuItem *item =
      CONNUI_INTERNET_STATUS_MENU_ITEM(user_data);

  g_return_if_fail(item != NULL && item->priv != NULL);

  item->priv->display_state = state;

  if (state == OSSO_DISPLAY_OFF)
  {
    connui_internet_status_menu_item_conn_strength_stop(item);
    if (item->priv->pixbuf_anim)
      connui_pixbuf_anim_stop(item->priv->pixbuf_anim);
  }
  else if (state == OSSO_DISPLAY_ON)
  {
    connui_internet_status_menu_item_conn_strength_start(item);
    connui_internet_status_menu_item_start_anim(item);
  }
}

static void
connui_internet_status_menu_item_finalize(GObject *self)
{
  ConnuiInternetStatusMenuItemPrivate *priv =
      CONNUI_INTERNET_STATUS_MENU_ITEM(self)->priv;

  if (priv->osso_context)
  {
    osso_deinitialize(priv->osso_context);
    priv->osso_context = 0;
  }

  connui_internet_status_menu_item_conn_strength_stop(CONNUI_INTERNET_STATUS_MENU_ITEM(self));
  connui_inetstate_close(_connui_internet_status_menu_item_inet_status_cb);
  connui_cellular_data_suspended_close(connui_internet_status_menu_item_cellular_data_suspended_status_cb);

  if (priv->pixbuf_cache)
  {
    connui_pixbuf_cache_destroy(priv->pixbuf_cache);
    priv->pixbuf_cache = 0;
  }

  G_OBJECT_CLASS(self)->finalize(self);
}

static void
connui_internet_status_menu_item_class_init(ConnuiInternetStatusMenuItemClass *klass)
{
  G_OBJECT_CLASS(klass)->finalize = connui_internet_status_menu_item_finalize;
  g_type_class_add_private(klass, sizeof(ConnuiInternetStatusMenuItemPrivate));
}

static void
connui_internet_status_menu_item_init(ConnuiInternetStatusMenuItem *self)
{
  ConnuiInternetStatusMenuItemPrivate *priv =
      G_TYPE_INSTANCE_GET_PRIVATE(self,
                                  CONNUI_INTERNET_STATUS_MENU_ITEM_TYPE,
                                  ConnuiInternetStatusMenuItemPrivate);

  self->priv = priv;
  priv->pixbuf_cache = connui_pixbuf_cache_new();
  priv->button = hildon_button_new(HILDON_SIZE_FINGER_HEIGHT,
                                   HILDON_BUTTON_ARRANGEMENT_VERTICAL);
  priv->conn_icon = gtk_image_new_from_file(NULL);

  hildon_button_set_style(HILDON_BUTTON(priv->button),
                          HILDON_BUTTON_STYLE_PICKER);

  hildon_button_set_image(HILDON_BUTTON(priv->button), priv->conn_icon);

  hildon_button_set_title(HILDON_BUTTON(priv->button),
                          _("stab_me_internet_connection"));
  hildon_button_set_alignment(HILDON_BUTTON(priv->button), 0.0, 0.0, 1.0, 1.0);
  g_signal_connect_data(G_OBJECT(priv->button), "clicked",
                        (GCallback)connui_internet_status_menu_item_request_select_connection,
                        self, NULL, G_CONNECT_AFTER);

   gtk_container_add(GTK_CONTAINER(self), priv->button);
   connui_internet_status_menu_item_set_active_conn_info(self);
   gtk_widget_show_all(GTK_WIDGET(self));

   priv->tp.tv_sec = 0;
   /* FIXME - get application and version from autoconf or debian packagin */
   priv->osso_context = osso_initialize("connui_internet_status_menu_item", "2.71+0m5", 1, 0);;
   priv->tp.tv_nsec = 0;

   osso_hw_set_display_event_cb(
     priv->osso_context,
     (osso_display_event_cb_f *)connui_internet_status_menu_item_display_cb,
     self);

   if (!connui_inetstate_status(_connui_internet_status_menu_item_inet_status_cb, self))
     ULOG_ERR("inet status: cannot receive status updates");

   if (!connui_cellular_data_suspended_status(connui_internet_status_menu_item_cellular_data_suspended_status_cb, self))
     ULOG_ERR("inet status: cannot receive cellular data suspended updates");

   g_signal_connect_data(G_OBJECT(self), "parent-set",
                         G_CALLBACK(connui_internet_status_menu_item_parent_set_signal),
                         self, NULL, 0);
}
