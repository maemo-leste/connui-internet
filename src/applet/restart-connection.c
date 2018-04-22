#include <connui/connui.h>
#include <icd/dbus_api.h>

static GMainLoop *mainloop = NULL;
static network_entry *net_entry = NULL;

static void
inetstate_status_cb(enum inetstate_status status, network_entry *entry,
                    gpointer user_data)
{
  if (status == INETSTATE_STATUS_CONNECTED)
  {
    if (!net_entry)
    {
      net_entry = iap_network_entry_dup(entry);
      iap_network_entry_disconnect(ICD_CONNECTION_FLAG_UI_EVENT, entry);
    }
  }
  else if (status == INETSTATE_STATUS_ONLINE)
  {
    if (net_entry)
    {
      network_entry *entries[] = {net_entry, NULL};
      iap_network_entry_connect(ICD_CONNECTION_FLAG_UI_EVENT, entries);
    }

    connui_inetstate_close(inetstate_status_cb);
    iap_network_entry_clear(net_entry);
    g_free(net_entry);
    g_main_loop_quit(mainloop);
  }
}

int
main()
{
#if !GLIB_CHECK_VERSION(2,35,0)
  g_type_init();
#endif

  mainloop = g_main_loop_new(NULL, TRUE);

  if (connui_inetstate_status(inetstate_status_cb, NULL))
  {
    g_main_loop_run(mainloop);
    return 0;
  }
  else
  {
    g_warning("Unable to query inetstate");
    return -1;
  }
}
