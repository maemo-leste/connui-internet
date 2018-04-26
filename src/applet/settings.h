#ifndef SETTINGS_H
#define SETTINGS_H

struct internet_settings
{
  osso_context_t *osso;
  GtkWidget *dialog;
  GtkWindow *parent;
  GtkNotebook *notebook;
  GHashTable *widgets;
  //int gap14;
  //int field_18;
  //int field_1C;
  gchar **labels;
  gchar ***network_types;
  gchar **not_found_banners;
  //int field_2C;
  struct iap_connections *conns;
  gboolean importing;
};

struct internet_settings *inet_settings_create(osso_context_t *osso, GtkWindow *parent);
void inet_settings_destroy(struct internet_settings *settings);
void inet_settings_import(struct internet_settings *settings);
void inet_settings_export(struct internet_settings *settings);
void inet_settings_save_state(struct internet_settings *settings, GByteArray *array);
gboolean inet_settings_restore_state(struct internet_settings *settings, struct stage_cache *cache);

#endif // SETTINGS_H
