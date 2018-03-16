#ifndef WIDGETS_H
#define WIDGETS_H

/* GtkEntry "insert-text" callbacks */
void
iap_widgets_insert_only_ascii_text(GtkEditable *editable, gchar *new_text,
                                   gint new_text_length, gpointer position,
                                   gpointer user_data);
void
iap_widgets_insert_text_no_8bit_maxval_reach(GtkEditable *editable,
                                             gchar *new_text,
                                             gint new_text_length,
                                             gpointer position,
                                             gpointer user_data);
void
iap_widgets_insert_text_maxval_reach(GtkEditable *editable, gchar *new_text,
                                     gint new_text_length, gpointer position,
                                     gpointer user_data);

/* static widgets */
GtkWidget *iap_widgets_create_static_picker_button(const gchar *title,
                                                   const gchar *text1, ...);
GtkWidget *iap_widgets_create_static_combo_box(const gchar *text1, ...);

/* h22 entry */
GtkWidget *iap_widgets_create_h22_entry(void);
const gchar * iap_widgets_h22_entry_get_text(const GtkWidget *entry);
void iap_widgets_h22_entry_set_text(GtkWidget *entry, const gchar *text);

/* certificate widgets */
GtkListStore *iap_widgets_create_certificate_list_store();
GtkWidget *iap_widgets_create_certificate_combo_box();
GtkWidget *iap_widgets_create_certificate_picker_button(const gchar *title);

#endif // WIDGETS_H
