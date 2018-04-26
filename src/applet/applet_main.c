#define _XOPEN_SOURCE
#include <hildon/hildon.h>
#include <libosso.h>
#include <connui/connui-utils.h>

/* Prototypes for execute and reset */
#include <hildon-cp-plugin/hildon-cp-plugin-interface.h>

#include <libintl.h>
#include <stdlib.h>

#include "config.h"

#include "stage.h"

#include "settings.h"

static osso_context_t *g_osso_context = NULL;
static struct internet_settings *g_settings = NULL;

osso_return_t
execute(osso_context_t *osso, gpointer data, gboolean user_activated)
{
  g_osso_context =
      connui_utils_inherit_osso_context(osso, PACKAGE_NAME, PACKAGE_VERSION);

  if (!g_osso_context)
    return -1;

  bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);

  if (user_activated != TRUE)
  {
    osso_state_t state = {0, NULL};

    if (osso_state_read(g_osso_context, &state) == OSSO_OK)
    {
      struct stage_cache cache;

      cache.data = (guint8 *)state.state_data;
      cache.len = state.state_size;
      cache.processed = 0;

      g_settings = inet_settings_create(g_osso_context, (GtkWindow *)data);

      if (inet_settings_restore_state(g_settings, &cache) &&
          cache.len == cache.processed)
      {
        free(state.state_data);
        goto restored;
      }

      inet_settings_destroy(g_settings);
      free(state.state_data);
      g_settings = NULL;
    }
  }

  g_settings = inet_settings_create(g_osso_context, (GtkWindow *)data);

  inet_settings_import(g_settings);
  gtk_widget_show_all(g_settings->dialog);

restored:
  gtk_main();
  inet_settings_destroy(g_settings);
  osso_deinitialize(g_osso_context);

  g_osso_context = NULL;
  g_settings = NULL;

  return OSSO_OK;
}

osso_return_t
save_state(osso_context_t *osso, gpointer data)
{
  osso_return_t rv;
  GByteArray *array;
  osso_state_t state;

  if (!g_osso_context || !g_settings)
    return OSSO_ERROR;

  array = g_byte_array_sized_new(2048);
  inet_settings_save_state(g_settings, array);

  state.state_size = array->len;
  state.state_data = array->data;
  rv = osso_state_write(g_osso_context, &state);
  g_byte_array_free(array, TRUE);

  return rv;
}
