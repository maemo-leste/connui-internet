#ifndef WIDGETS_H
#define WIDGETS_H

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

#endif // WIDGETS_H
