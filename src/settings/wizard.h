#ifndef WIZARD_H
#define WIZARD_H

struct iap_wizard;

struct iap_wizard_page
{
  gchar *id;
  gchar *msgid;
  GtkWidget * (*create)(gpointer private);
  const char * (*get_page)(gpointer private, gboolean show_note);
  void (*finish)(gpointer private);
  void (*prev)(gpointer private);
  gchar *next_page;
  gchar *unk2;
  gpointer priv;
};
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
  void (*restore)(gpointer priv, struct stage_cache *);
  void (*advanced_show)(gpointer priv, struct stage *s);
  void (*advanced_done)(gpointer priv);
};

struct iap_wizard
{
  osso_context_t *osso;
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

enum wizzard_button
{
  WIZARD_BUTTON_FINISH = 0,
  WIZARD_BUTTON_PREVIOUS = 1,
  WIZARD_BUTTON_NEXT = 2,
  WIZARD_BUTTON_CLOSE = 3,
  WIZARD_BUTTON_ADVANCED = 4
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
gboolean iap_wizard_restore_state(struct iap_wizard *iw, struct stage_cache *data);

struct iap_wizard *iap_wizard_create(osso_context_t *osso, GtkWindow *parent);
void iap_wizard_show(struct iap_wizard *iw);
void iap_wizard_destroy(struct iap_wizard *iw);
void iap_wizard_import(struct iap_wizard *iw, struct stage *s);

#endif // WIZARD_H
