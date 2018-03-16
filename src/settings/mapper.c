#include <gconf/gconf-client.h>
#include <connui/iap-network.h>
#include <connui/wlan-common.h>
#include <hildon/hildon.h>

#include <string.h>

#include "widgets.h"

#define MAPPER_STOCK_IMPL

#include "mapper.h"

static void
entry2string(struct stage *s, const GtkWidget *entry,
             const struct stage_widget *sw)
{
  stage_set_string(s, sw->key, iap_widgets_h22_entry_get_text(entry));
}

static void
string2entry(const struct stage *s, GtkWidget *entry,
             const struct stage_widget *sw)
{
  gchar *str = stage_get_string(s, sw->key);

  if (str)
  {
    iap_widgets_h22_entry_set_text(entry, str);
    g_free(str);
  }
  else
    iap_widgets_h22_entry_set_text(entry, "");
}

static void
entry2bytearray(struct stage *s, const GtkWidget *entry,
                const struct stage_widget *sw)
{
  if (GTK_WIDGET_SENSITIVE(entry) && GTK_WIDGET_PARENT_SENSITIVE(entry))
    stage_set_bytearray(s, sw->key, iap_widgets_h22_entry_get_text(entry));
}

static void
bytearray2entry(const struct stage *s, GtkWidget *entry,
                const struct stage_widget *sw)
{
  gchar *str = stage_get_bytearray(s, sw->key);

  if (str)
  {
    gtk_widget_set_sensitive(entry, wlan_common_mangle_ssid(str, strlen(str)));
    iap_widgets_h22_entry_set_text(entry, str);
    g_free(str);
  }
  else
    iap_widgets_h22_entry_set_text(entry, "");
}

static void
numbereditor2int(struct stage *s, const GtkWidget *entry,
                 const struct stage_widget *sw)
{
  stage_set_int(s, sw->key,
                hildon_number_editor_get_value(HILDON_NUMBER_EDITOR(entry)));
}

static void
int2numbereditor(const struct stage *s, GtkWidget *entry,
                 const struct stage_widget *sw)
{
  hildon_number_editor_set_value(HILDON_NUMBER_EDITOR(entry),
                                 stage_get_int(s, sw->key));
}

static void
toggle2int(struct stage *s, const GtkWidget *entry,
           const struct stage_widget *sw)
{
  stage_set_int(s, sw->key,
                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(entry)));
}

static void
int2toggle(const struct stage *s, GtkWidget *entry,
           const struct stage_widget *sw)
{
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(entry),
                               stage_get_int(s, sw->key));
}

static void
toggle2bool(struct stage *s, const GtkWidget *entry,
            const struct stage_widget *sw)
{
  if (GTK_IS_TOGGLE_BUTTON(entry))
  {
    stage_set_bool(s, sw->key,
                   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(entry)));
  }
  else if (HILDON_IS_CHECK_BUTTON(entry))
  {
    stage_set_bool(s, sw->key,
                   hildon_check_button_get_active(HILDON_CHECK_BUTTON(entry)));
  }
}

static void
bool2toggle(const struct stage *s, GtkWidget *entry,
            const struct stage_widget *sw)
{
  gboolean bval = stage_get_bool(s, sw->key);

  if (GTK_IS_TOGGLE_BUTTON(entry))
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(entry), bval);
  else if (HILDON_IS_CHECK_BUTTON(entry))
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(entry), bval);
}
