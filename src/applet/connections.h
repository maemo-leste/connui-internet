#ifndef CONNECTIONS_H
#define CONNECTIONS_H

#include <libintl.h>

#include "config.h"

struct iap_connections
{
  osso_context_t *osso;
  GtkWidget *dialog;
  gint response_id;
  GtkTreeSelection *selection;
  GtkListStore *list_store;
  GtkWidget *edit_button;
  GtkWidget *delete_button;
  GtkTreeView *tree_view;
  struct iap_wizard *wizard;
  GConfClient *gconf;
  gchar *iap_id;
  GSList *networks;
};

enum
{
  IAP_CONNECTIONS_NEW = 0,
  IAP_CONNECTIONS_EDIT = 1,
  IAP_CONNECTIONS_DELETE = 2,
  IAP_CONNECTIONS_DONE = 3
};

void iap_connections_save_state(struct iap_connections *connections, GByteArray *array);
gboolean iap_connections_restore_state(struct iap_connections *conns, struct stage_cache *cache);
struct iap_connections *iap_connections_create(osso_context_t *osso, GtkWindow *parent);
void iap_connections_show(struct iap_connections *conns);
void iap_connections_destroy(struct iap_connections *conns);

#define _(a) dgettext(GETTEXT_PACKAGE, a)

#endif // CONNECTIONS_H
