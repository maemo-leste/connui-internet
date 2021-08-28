#include <hildon/hildon-button.h>
#include <hildon/hildon-banner.h>
#include <libhildondesktop/libhildondesktop.h>
#include <connui/connui.h>
#include <libintl.h>
#include <icd/dbus_api.h>
#include <osso-log.h>

#include <string.h>

#include "config.h"

#define _(x) dgettext(GETTEXT_PACKAGE, x)

#define CONNUI_INTERNET_STATUS_MENU_ITEM_TYPE \
  (connui_internet_status_menu_item_get_type())
#define CONNUI_INTERNET_STATUS_MENU_ITEM(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  CONNUI_INTERNET_STATUS_MENU_ITEM_TYPE, \
  ConnuiInternetStatusMenuItem))

#define GET_PRIVATE(item) \
  ((ConnuiInternetStatusMenuItemPrivate *) \
  connui_internet_status_menu_item_get_instance_private( \
                                          (ConnuiInternetStatusMenuItem *)item))

typedef struct _ConnuiInternetStatusMenuItem ConnuiInternetStatusMenuItem;
typedef struct _ConnuiInternetStatusMenuItemClass ConnuiInternetStatusMenuItemClass;
typedef struct _ConnuiInternetStatusMenuItemPrivate ConnuiInternetStatusMenuItemPrivate;

struct _ConnuiInternetStatusMenuItem
{
  HDStatusMenuItem parent;
};

struct _ConnuiInternetStatusMenuItemClass
{
  HDStatusMenuItemClass parent;
};

struct _ConnuiInternetStatusMenuItemPrivate
{
  GtkWidget *conn_icon;
  GtkWidget *button;
  ConnuiPixbufCache *pixbuf_cache;
  ConnuiPixbufAnim *pixbuf_anim;
  network_entry *network;
  enum inetstate_status connection_state;
  gboolean is_active;
  gboolean is_displayed;
  gboolean signals_set;
  gboolean suspended;
  guint32 suspendcode;
  gint signal_strength;
  osso_context_t *osso_context;
  osso_display_state_t display_state;
  struct timespec tp;
};

HD_DEFINE_PLUGIN_MODULE_WITH_PRIVATE(ConnuiInternetStatusMenuItem,
                                     connui_internet_status_menu_item,
                                     HD_TYPE_STATUS_MENU_ITEM)

static void
connui_internet_status_menu_item_class_finalize(
    ConnuiInternetStatusMenuItemClass *klass)
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

static gchar *
connui_internet_status_menu_item_get_icon(network_entry *entry, gboolean dimmed)
{
  gchar *rv = NULL;
  GConfClient *gconf = gconf_client_get_default();

  if (entry &&
      entry->service_type && *entry->service_type &&
      entry->service_id && *entry->service_id)
  {

    if (dimmed)
    {
      iap_common_get_service_properties(entry->service_type,
                                        entry->service_id,
                                        "type_statusbar_dimmed_icon_name", &rv,
                                        NULL);
    }
    else
    {
      iap_common_get_service_properties(entry->service_type,
                                        entry->service_id,
                                        "type_statusbar_icon_name", &rv,
                                        NULL);
    }
  }

  if (entry->network_type && !rv)
  {
    gchar *s = dimmed ? "%s/%s/statusbar_dimmed_icon_name" :
                        "%s/%s/statusbar_icon_name";

    s = g_strdup_printf(s, "/system/osso/connectivity/network_type",
                        entry->network_type);
    rv = gconf_client_get_string(gconf, s, NULL);
    g_free(s);
  }

  if (dimmed && !rv)
    rv = g_strdup("statusarea_internetconnection_no");

  g_object_unref(G_OBJECT(gconf));

  return rv;
}

static void
connui_internet_status_menu_item_anim_set_pixbuf(gpointer user_data,
                                                 GdkPixbuf *pixbuf)
{
  ConnuiInternetStatusMenuItem *item =
      CONNUI_INTERNET_STATUS_MENU_ITEM(user_data);

  g_return_if_fail(item != NULL && pixbuf != NULL);

  hd_status_plugin_item_set_status_area_icon(HD_STATUS_PLUGIN_ITEM(item),
                                             pixbuf);
}

static void
connui_internet_status_menu_item_start_anim(ConnuiInternetStatusMenuItem *item)
{
  ConnuiInternetStatusMenuItemPrivate *priv = GET_PRIVATE(item);

  g_return_if_fail(item != NULL && priv != NULL);

  if (priv->pixbuf_anim && priv->display_state != OSSO_DISPLAY_OFF)
  {
    connui_pixbuf_anim_start(priv->pixbuf_anim, item,
                             connui_internet_status_menu_item_anim_set_pixbuf);
  }
}

static gboolean
connui_internet_status_menu_item_is_suspended(
    ConnuiInternetStatusMenuItemPrivate *priv)
{
  g_return_val_if_fail(priv != NULL && priv->network != NULL &&
      priv->network->network_type != NULL, FALSE);

  return priv->suspended ? !strcmp(priv->network->network_type, "GPRS") : FALSE;
}

static void
connui_internet_status_menu_item_set_active_conn_info(
    ConnuiInternetStatusMenuItem *self)
{
  ConnuiInternetStatusMenuItemPrivate *priv = GET_PRIVATE(self);
  gchar *button_icon_name;
  gchar *network_name;
  GdkPixbuf *button_icon;
  GdkPixbuf *status_area_icon = NULL;
  ConnuiPixbufAnim *pixbuf_anim = NULL;

  g_return_if_fail(priv != NULL && priv->pixbuf_cache != NULL);

  if (priv->connection_state == INETSTATE_STATUS_CONNECTING ||
      priv->connection_state == INETSTATE_STATUS_CONNECTED)
  {
    if (connui_internet_status_menu_item_is_suspended(priv))
    {
      struct timespec tp;

      clock_gettime(CLOCK_MONOTONIC, &tp);
      if (priv->suspendcode == 5 &&
          tp.tv_sec - priv->tp.tv_sec > 10 &&
          !strcmp(priv->network->network_type, "GPRS"))
      {
        hildon_banner_show_information(0, 0, _("conn_ib_suspended"));
        priv->tp.tv_sec = tp.tv_sec;
        priv->tp.tv_nsec = tp.tv_nsec;
      }

      network_name = g_strdup(_("stab_me_internet_suspended"));
      button_icon_name = g_strdup("general_packetdata_suspended");
    }
    else
    {
      gchar *s = iap_settings_get_name_by_network(priv->network, 0, 0);
      gchar *tmp = _("stab_me_internet_connected_to");

      button_icon_name = iap_settings_get_iap_icon_name_by_network_and_signal(
            priv->network,
            priv->signal_strength);

      network_name = g_strdup_printf(tmp, s);
      g_free(s);
    }
  }
  else
  {
    button_icon_name = g_strdup("statusarea_internetconnection_no");
    network_name = g_strdup(_("stab_me_internet_disconnected"));
  }

  button_icon =
      connui_pixbuf_cache_get(priv->pixbuf_cache, button_icon_name, 48);

  if (button_icon && priv->conn_icon)
    gtk_image_set_from_pixbuf(GTK_IMAGE(priv->conn_icon), button_icon);

  if (priv->button)
    hildon_button_set_value(HILDON_BUTTON(priv->button), network_name);

  g_free(network_name);
  g_free(button_icon_name);

  if (priv->network && priv->network->network_type)
  {
    gchar *s = NULL;

    if (priv->connection_state == INETSTATE_STATUS_CONNECTED)
    {
      if (connui_internet_status_menu_item_is_suspended(priv))
        s = g_strdup("general_packetdata_suspended");
      else
        s = connui_internet_status_menu_item_get_icon(priv->network, FALSE);
      if (s)
        status_area_icon = connui_pixbuf_cache_get(priv->pixbuf_cache, s, 16);
    }
    else if (priv->connection_state == INETSTATE_STATUS_CONNECTING)
    {
      gchar *dimmed;

      s = connui_internet_status_menu_item_get_icon(priv->network, FALSE);
      dimmed = connui_internet_status_menu_item_get_icon(priv->network, TRUE);
      pixbuf_anim = connui_pixbuf_anim_new_from_icons(16, 1.5, s, dimmed, NULL);
      g_free(dimmed);
    }

    g_free(s);
#if 0 /* Noone sets err */
    if (err)
    {
      ULOG_ERR("Unable to load status area icons or animations: %s",
               err->message);
      g_clear_error(&err);

      return;
    }
#endif
  }

  if (priv->pixbuf_anim)
  {
    connui_pixbuf_anim_destroy(priv->pixbuf_anim);
    priv->pixbuf_anim = 0;
  }

  if (pixbuf_anim)
  {
    priv->pixbuf_anim = pixbuf_anim;
    connui_internet_status_menu_item_start_anim(self);
  }
  else
    hd_status_plugin_item_set_status_area_icon(HD_STATUS_PLUGIN_ITEM(self),
                                               status_area_icon);

}

static void
connui_internet_status_menu_item_conn_strength_cb(
    network_entry *entry, inetstate_network_stats *stats, gpointer user_data)
{
  ConnuiInternetStatusMenuItem *self =
      CONNUI_INTERNET_STATUS_MENU_ITEM(user_data);
  ConnuiInternetStatusMenuItemPrivate *priv = GET_PRIVATE(self);

  g_return_if_fail(priv != NULL && priv->conn_icon != NULL && stats != NULL);

  if ( priv->signal_strength != stats->signal_strength )
  {
    priv->signal_strength = stats->signal_strength;
    connui_internet_status_menu_item_set_active_conn_info(self);
  }
}

static void
connui_internet_status_menu_item_conn_strength_start(
    ConnuiInternetStatusMenuItem *self)
{
  ConnuiInternetStatusMenuItemPrivate *priv = GET_PRIVATE(self);

  if (priv->is_active && priv->is_displayed &&
      priv->display_state != OSSO_DISPLAY_OFF)
  {
    connui_inetstate_statistics_start(
          1000, connui_internet_status_menu_item_conn_strength_cb, self);
  }
}

static void
connui_internet_status_menu_item_conn_strength_stop(
    ConnuiInternetStatusMenuItem *self)
{
  ConnuiInternetStatusMenuItemPrivate *priv = GET_PRIVATE(self);

  connui_inetstate_statistics_stop(
        connui_internet_status_menu_item_conn_strength_cb);
  priv->signal_strength = 0;
}

static void
connui_internet_status_menu_item_set_network(ConnuiInternetStatusMenuItem *self,
                                             network_entry *network)
{
  ConnuiInternetStatusMenuItemPrivate *priv = GET_PRIVATE(self);

  iap_network_entry_clear(priv->network);
  g_free(priv->network);
  priv->network = network;
  connui_internet_status_menu_item_set_active_conn_info(self);
}

static void
connui_internet_status_menu_item_inet_status_cb(enum inetstate_status state,
                                                network_entry *entry,
                                                gpointer user_data)
{
  ConnuiInternetStatusMenuItem *self =
      CONNUI_INTERNET_STATUS_MENU_ITEM(user_data);
  ConnuiInternetStatusMenuItemPrivate *priv = GET_PRIVATE(self);
  static enum inetstate_status old_state = INETSTATE_STATUS_ONLINE;

  g_return_if_fail(priv != NULL);

  priv->connection_state = state;

  switch (state)
  {
    case INETSTATE_STATUS_OFFLINE:
    {
      if (old_state)
      {
        hildon_banner_show_information(0, 0,
                                       _("conn_ib_flight_mode_activated"));
      }
    }
    case INETSTATE_STATUS_ONLINE:
    case INETSTATE_STATUS_DISCONNECTED:
    {
      connui_internet_status_menu_item_set_network(self, NULL);

      break;
    }
    case INETSTATE_STATUS_CONNECTING:
    case INETSTATE_STATUS_CONNECTED:
    {
      connui_internet_status_menu_item_set_network(
            self, iap_network_entry_dup(entry));

      break;
    }
    case INETSTATE_STATUS_DISCONNECTING:
    {
      if (!entry || !priv->network ||
          iap_network_entry_equal(entry, priv->network))
      {
        connui_internet_status_menu_item_set_network(
              self, iap_network_entry_dup(entry));
      }

      break;
    }
    default:
      break;
  }

  if (old_state == INETSTATE_STATUS_OFFLINE && state != old_state)
  {
    hildon_banner_show_information(NULL, NULL,
                                   _("conn_ib_normal_mode_activated"));
  }

  if (entry && entry->network_type &&
      (!strncmp(entry->network_type, "WLAN_", 5) ||
       !strncmp(entry->network_type, "WIMAX", 5)))
  {

    priv->is_active =
        state == INETSTATE_STATUS_CONNECTING ||
        state == INETSTATE_STATUS_CONNECTED ||
        state == INETSTATE_STATUS_DISCONNECTING;

    if (priv->is_active)
      connui_internet_status_menu_item_conn_strength_start(self);
    else
      connui_internet_status_menu_item_conn_strength_stop(self);
  }
  else
    priv->is_active = FALSE;

  old_state = state;
}

static void
connui_internet_status_menu_item_is_displayed(GtkWidget *widget,
                                              gpointer user_data)
{
  ConnuiInternetStatusMenuItem *self =
      CONNUI_INTERNET_STATUS_MENU_ITEM(user_data);
  ConnuiInternetStatusMenuItemPrivate *priv = GET_PRIVATE(self);

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
  ConnuiInternetStatusMenuItemPrivate *priv = GET_PRIVATE(self);

  priv->is_displayed = FALSE;

  if (priv->is_active)
    connui_internet_status_menu_item_conn_strength_stop(self);
}

static void
connui_internet_status_menu_item_parent_set_signal(GtkWidget *widget,
                                                   GtkObject *old_parent,
                                                   gpointer user_data)
{
  ConnuiInternetStatusMenuItem *self =
      CONNUI_INTERNET_STATUS_MENU_ITEM(user_data);
  ConnuiInternetStatusMenuItemPrivate *priv = GET_PRIVATE(self);

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

    priv->signals_set = FALSE;
  }

  widget = gtk_widget_get_ancestor(GTK_WIDGET(self), GTK_TYPE_WINDOW);
  if (widget)
  {
    g_signal_connect(
          G_OBJECT(widget), "map",
          G_CALLBACK(connui_internet_status_menu_item_is_displayed), self);
    g_signal_connect(
          G_OBJECT(widget), "unmap",
          G_CALLBACK(connui_internet_status_menu_item_is_not_displayed), self);

    priv->signals_set = TRUE;
  }
}

static void
connui_internet_status_menu_item_cellular_data_suspended_status_cb(
    gboolean suspended, guint32 suspendcode, gpointer user_data)
{
  ConnuiInternetStatusMenuItem *self =
      CONNUI_INTERNET_STATUS_MENU_ITEM(user_data);
  ConnuiInternetStatusMenuItemPrivate *priv = GET_PRIVATE(self);

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
  ConnuiInternetStatusMenuItemPrivate *priv = GET_PRIVATE(item);

  g_return_if_fail(item != NULL && priv != NULL);

  priv->display_state = state;

  if (state == OSSO_DISPLAY_OFF)
  {
    connui_internet_status_menu_item_conn_strength_stop(item);

    if (priv->pixbuf_anim)
      connui_pixbuf_anim_stop(priv->pixbuf_anim);
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
  ConnuiInternetStatusMenuItemPrivate *priv = GET_PRIVATE(self);

  if (priv->osso_context)
  {
    osso_deinitialize(priv->osso_context);
    priv->osso_context = NULL;
  }

  connui_internet_status_menu_item_conn_strength_stop(
        CONNUI_INTERNET_STATUS_MENU_ITEM(self));
  connui_inetstate_close(connui_internet_status_menu_item_inet_status_cb);
  connui_cellular_data_suspended_close(
        connui_internet_status_menu_item_cellular_data_suspended_status_cb);

  if (priv->pixbuf_cache)
  {
    connui_pixbuf_cache_destroy(priv->pixbuf_cache);
    priv->pixbuf_cache = NULL;
  }

  G_OBJECT_CLASS(connui_internet_status_menu_item_parent_class)->finalize(self);
}

static void
connui_internet_status_menu_item_class_init(
    ConnuiInternetStatusMenuItemClass *klass)
{
  G_OBJECT_CLASS(klass)->finalize = connui_internet_status_menu_item_finalize;
}

static void
connui_internet_status_menu_item_init(ConnuiInternetStatusMenuItem *self)
{
  ConnuiInternetStatusMenuItemPrivate *priv = GET_PRIVATE(self);

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
  g_signal_connect_after(
        G_OBJECT(priv->button), "clicked",
        (GCallback)connui_internet_status_menu_item_request_select_connection,
        self);

   gtk_container_add(GTK_CONTAINER(self), priv->button);
   connui_internet_status_menu_item_set_active_conn_info(self);
   gtk_widget_show_all(GTK_WIDGET(self));

   priv->tp.tv_sec = 0;
   /* FIXME - get application and version from autoconf or debian packaging */
   priv->osso_context = osso_initialize("connui_internet_status_menu_item",
                                        PACKAGE_VERSION, TRUE, NULL);
   priv->tp.tv_nsec = 0;

   osso_hw_set_display_event_cb(
     priv->osso_context,
     (osso_display_event_cb_f *)connui_internet_status_menu_item_display_cb,
     self);

   if (!connui_inetstate_status(connui_internet_status_menu_item_inet_status_cb,
                                self))
     ULOG_ERR("inet status: cannot receive status updates");

   if (!connui_cellular_data_suspended_status(
         connui_internet_status_menu_item_cellular_data_suspended_status_cb,
         self))
     ULOG_ERR("inet status: cannot receive cellular data suspended updates");

   g_signal_connect(
         G_OBJECT(self), "parent-set",
         G_CALLBACK(connui_internet_status_menu_item_parent_set_signal), self);
}
