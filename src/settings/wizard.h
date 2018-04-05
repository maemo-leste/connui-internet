#ifndef WIZARD_H
#define WIZARD_H

struct iap_wizard;
struct iap_wizard_page;

struct iap_wizard_plugin
{
  const gchar *name;
  guint prio;
  struct iap_wizard_page *pages;
  GHashTable *widgets;
  struct stage_widget *stage_widgets;
  gpointer priv;
  const gchar **(*get_widgets)(gpointer priv);
  const char *(*get_page)(gpointer priv, int index, gboolean show_note);
  struct iap_advanced_page *(*get_advanced)(gpointer priv);
  void (*save_state)(gpointer priv, GByteArray *state);
  int restore;
  void (*advanced_show)(gpointer priv, struct stage *s);
  void (*advanced_done)(gpointer priv);
};

struct iap_wizard
{
  gpointer user_data;
  GtkWidget *dialog;
  GtkWidget *button_next;
  GtkWidget *button_finish;
  GtkWindow *parent;
  guint response_id;
  GtkNotebook *notebook;
  gchar **page_ids;
  GHashTable *widgets;
  GHashTable *pages;
  GSList *plugins;
  GSList *plugin_modules;
  struct stage_widget *stage_widgets;
  struct stage *stage;
  struct iap_wizard_plugin *plugin;
  struct iap_wizard_advanced *advanced;
  int import_mode;
  int unk2;
  gchar *iap_id;
  struct {
    int current;
    int index[8];
    gboolean in_progress;
  } page;
};

GtkWidget *iap_wizard_get_dialog(struct iap_wizard *iw);
struct stage *iap_wizard_get_active_stage(struct iap_wizard *iw);
void iap_wizard_set_active_stage(struct iap_wizard *iw, struct stage *new_stage);
gchar *iap_wizard_get_current_page(struct iap_wizard *iw);
gchar *iap_wizard_get_iap_id(struct iap_wizard *iw);
void iap_wizard_select_plugin_label(struct iap_wizard *iw, gchar *name, guint idx);
void iap_wizard_set_completed(struct iap_wizard *iw, gboolean completed);
void iap_wizard_validate_finish_button(struct iap_wizard *iw);
GtkWidget *iap_wizard_get_widget(struct iap_wizard *iw, const gchar *id);
void iap_wizzard_create_page(const gchar *id, struct iap_wizard_page *wp, struct iap_wizard *iw);
void iap_wizard_set_empty_values(struct iap_wizard *iw);
void iap_wizard_set_start_page(struct iap_wizard *iw, const gchar *page_id);
gboolean iap_wizard_set_current_page(struct iap_wizard *iw, const gchar *id);
int iap_wizard_get_import_mode(struct iap_wizard *iw);
void iap_wizard_set_import_mode(struct iap_wizard *iw, int mode);
GtkWidget *iap_wizard_export(struct iap_wizard *iw, struct stage *s, gboolean reconnect);
void iap_wizard_save_state(struct iap_wizard *iw, GByteArray *state);

struct iap_wizard *iap_wizard_create(gpointer user_data, GtkWindow *parent);
void iap_wizard_show(struct iap_wizard *iw);
void iap_wizard_destroy(struct iap_wizard *iw);
void iap_wizard_import(struct iap_wizard *iw, struct stage *s);

#endif // WIZARD_H
