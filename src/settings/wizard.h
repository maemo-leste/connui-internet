#ifndef WIZARD_H
#define WIZARD_H

struct iap_wizard;
struct iap_wizard_page;

GtkWidget *iap_wizard_get_dialog(struct iap_wizard *iw);
struct stage *iap_wizard_get_active_stage(struct iap_wizard *iw);
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

struct iap_wizard *iap_wizard_create(gpointer user_data, GtkWindow *parent);
void iap_wizard_show(struct iap_wizard *iw);
void iap_wizard_destroy(struct iap_wizard *iw);

#endif // WIZARD_H
