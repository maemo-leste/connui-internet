#ifndef ADVANCED_H
#define ADVANCED_H

#include "mapper.h"

struct iap_advanced_widget
{
  gboolean (*init)(struct stage *);
  gchar *id;
  gchar *proxy_id;
  gchar *auto_proxy_id;
  gchar *msgid;
  GtkWidget *(*create)();
  gboolean is_toggle;
};

struct iap_advanced_page
{
  gboolean has_content;
  const gchar *msgid;
  struct iap_advanced_widget *widgets;
  void (*activate)(gpointer priv);
  const gchar *tag;
  gpointer priv;
};

struct iap_wizard_advanced
{
  gpointer user_data;
  GtkWidget *dialog;
  gint current_page;
  GtkNotebook *notebook;
  GHashTable *widgets;
  struct stage_widget *sw;
  struct iap_advanced_page *pages;
  int import_mode;
  struct stage *stage;
};

struct iap_wizard_advanced *iap_advanced_create(gpointer user_data, GtkWindow *parent, struct iap_advanced_page *pages, struct stage_widget *sw, struct stage *s);
void iap_advanced_show(struct iap_wizard_advanced *adv);
void iap_advanced_destroy(struct iap_wizard_advanced *adv);

void iap_advanced_import(struct iap_wizard_advanced *adv, struct stage *s);
void iap_advanced_export(struct iap_wizard_advanced *adv, struct stage *s);
void iap_advanced_save_state(struct iap_wizard_advanced *adv, GByteArray *array);
gboolean iap_advanced_restore_state(struct iap_wizard_advanced *adv, struct stage_cache *data);

#endif // ADVANCED_H
