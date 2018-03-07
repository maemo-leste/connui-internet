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
