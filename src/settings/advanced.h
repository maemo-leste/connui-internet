#ifndef ADVANCED_H
#define ADVANCED_H

struct iap_wizard_advanced
{
  int gap0;
  GtkWidget *dialog;
  gint current_page;
  GtkNotebook *notebook;
  GHashTable *widgets;
  struct stage_widget *sw;
  gpointer field_18;
  int import_mode;
  int field_20;
};

struct iap_advanced_widget
{
  gpointer user_data;
  gchar *id;
  gchar *proxy_id;
  gchar *auto_proxy_id;
  gchar *msgid;
  GtkWidget *(*create)();
  gboolean field_18;
};

void iap_advanced_show(struct iap_wizard_advanced *adv);
void iap_advanced_destroy(struct iap_wizard_advanced *adv);

#endif // ADVANCED_H
