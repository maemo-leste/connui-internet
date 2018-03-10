#include <hildon/hildon.h>

#include <libintl.h>
#include <string.h>

void
iap_widgets_insert_only_ascii_text(GtkEditable *editable, gchar *new_text,
                                   gint new_text_length, gpointer position,
                                   gpointer user_data)
{
  int i;

  if (new_text_length <= 0)
    return;

  for (i = 0; i < new_text_length; i++)
  {
    if (new_text[i] < 0)
    {
      hildon_banner_show_information(GTK_WIDGET(user_data), NULL,
                                     dgettext("osso-connectivity-ui",
                                              "conn_ib_only_ascii_characters"));
      g_signal_stop_emission_by_name(G_OBJECT(editable), "insert-text");
      return;
    }
  }
}

void
iap_widgets_insert_text_no_8bit_maxval_reach(GtkEditable *editable,
                                             gchar *new_text,
                                             gint new_text_length,
                                             gpointer position,
                                             gpointer user_data)
{
  int i;

  if (new_text_length < 0)
    new_text_length = strlen(new_text);

  for (i = 0; i < new_text_length; i++)
  {
    if (new_text[i] < 0)
    {
      g_signal_stop_emission_by_name((gpointer)editable, "insert-text");
      return;
    }
  }

  if (new_text_length + GTK_ENTRY(editable)->text_length >
      GTK_ENTRY(editable)->text_max_length)
  {
    hildon_banner_show_information(GTK_WIDGET(user_data), NULL,
                                   dgettext("osso-connectivity-ui",
                                            "conn_ib_maxval_reach"));
  }
}

static void
iap_widgets_picker_button_set_nonactive(GtkWidget *button)
{
  hildon_picker_button_set_active(HILDON_PICKER_BUTTON(button), FALSE);
  hildon_button_set_alignment(HILDON_BUTTON(button), 0.0, 0.5, 1.0, 1.0);
}

GtkWidget *
iap_widgets_create_static_picker_button(const gchar *title,
                                        const gchar *text1, ...)
{
  GtkWidget *selector;
  GtkWidget *button;
  va_list ap;

  va_start(ap, text1);

  selector = hildon_touch_selector_new_text();
  button = hildon_picker_button_new(HILDON_SIZE_FINGER_HEIGHT,
                                    HILDON_BUTTON_ARRANGEMENT_HORIZONTAL);
  hildon_picker_button_set_selector(HILDON_PICKER_BUTTON(button),
                                    HILDON_TOUCH_SELECTOR(selector));

  while (text1)
  {
    hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR(selector), text1);
    text1 = va_arg(ap, const gchar *);
  }

  hildon_button_set_title(HILDON_BUTTON(button), title);
  iap_widgets_picker_button_set_nonactive(button);

  va_end(ap);

  return button;
}

GtkWidget *
iap_widgets_create_h22_entry(void)
{
  return hildon_entry_new(HILDON_SIZE_FINGER_HEIGHT);
}

const gchar *
iap_widgets_h22_entry_get_text(GtkWidget *entry)
{
  if (HILDON_IS_ENTRY(entry))
    return hildon_entry_get_text(HILDON_ENTRY(entry));
  else
    return gtk_entry_get_text(GTK_ENTRY(entry));
}

void
iap_widgets_h22_entry_set_text(GtkWidget *entry, const gchar *text)
{
  if (HILDON_IS_ENTRY(entry))
    hildon_entry_set_text(HILDON_ENTRY(entry), text);
  else
    gtk_entry_set_text(GTK_ENTRY(entry), text);
}

GtkWidget *
iap_widgets_create_static_combo_box(const gchar *text1, ...)
{
  GtkWidget *combo_box = gtk_combo_box_new_text();
  va_list ap;

  va_start(ap, text1);

  while (text1)
  {
    gtk_combo_box_append_text(GTK_COMBO_BOX(combo_box), text1);
    text1 = va_arg(ap, const gchar *);
  }

  va_end(ap);

  gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), FALSE);

  return combo_box;
}
