#include <hildon/hildon.h>
#include <maemosec_certman.h>
#include <connui/connui-log.h>

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
  GtkEntry *entry = GTK_ENTRY(editable);

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

  if (new_text_length + entry->text_length > entry->text_max_length)
  {
    hildon_banner_show_information(GTK_WIDGET(user_data), NULL,
                                   dgettext("osso-connectivity-ui",
                                            "conn_ib_maxval_reach"));
  }
}

void
iap_widgets_insert_text_maxval_reach(GtkEditable *editable, gchar *new_text,
                                     gint new_text_length, gpointer position,
                                     gpointer user_data)
{
  GtkEntry *entry = GTK_ENTRY(editable);

  if (new_text_length < 0)
    new_text_length = strlen(new_text);

  if (new_text_length + entry->text_length > entry->text_max_length)
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
iap_widgets_h22_entry_get_text(const GtkWidget *entry)
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

static void
get_cert_data(X509_NAME *subject_name, gchar **buf, int nid)
{
  int idx = X509_NAME_get_index_by_NID(subject_name, nid, -1);

  if (idx != -1)
  {
    X509_NAME_ENTRY *entry = X509_NAME_get_entry(subject_name, idx);

    ASN1_STRING_to_UTF8((unsigned char **)buf, X509_NAME_ENTRY_get_data(entry));
  }
}

static int
iap_widgets_iterate_certs_cb(int order, X509 *cert, void *user_data)
{
  GtkListStore **list_store = (GtkListStore **)user_data;
  ASN1_STRING *entry_data;
  char key_id_buf[MAEMOSEC_KEY_ID_STR_LEN];
  maemosec_key_id key_id;
  GtkTreeIter iter;
  gchar cert_name_buf[512];

  maemosec_certman_get_key_id(cert, key_id);

  if (maemosec_certman_key_id_to_str(key_id, key_id_buf,
                                     MAEMOSEC_KEY_ID_STR_LEN))
  {
    CONNUI_ERR("Unable to convert key ID to string!");
  }
  else
  {
    X509_NAME *subject_name = X509_get_subject_name(cert);
    int idx = -1;
    gchar *cert_name = NULL;
    gchar *common_name = NULL;

    gtk_list_store_append(*list_store, &iter);

    while (1)
    {
      X509_NAME_ENTRY *entry;

      idx = X509_NAME_get_index_by_NID(subject_name, NID_commonName, idx);

      if (idx == -1)
        break;

      entry = X509_NAME_get_entry(subject_name, idx);
      entry_data = X509_NAME_ENTRY_get_data(entry);

      ASN1_STRING_to_UTF8((unsigned char **)&common_name, entry_data);

      if (common_name)
      {
        if (cert_name)
        {
          gchar *s = g_strconcat(cert_name, ", ", common_name, NULL);

          g_free(cert_name);
          cert_name = s;
        }
        else
          cert_name = g_strdup(common_name);

        OPENSSL_free(common_name);
      }
    }

    if (cert_name)
    {
      g_snprintf(cert_name_buf, sizeof(cert_name_buf), "%s", cert_name);
      g_free(cert_name);
    }
    else
    {
      gchar *org_unit_name = NULL;
      gchar *org_name = NULL;
      gchar *country_name = NULL;

      get_cert_data(subject_name, &org_unit_name, NID_organizationalUnitName);
      get_cert_data(subject_name, &org_name, NID_organizationName);
      get_cert_data(subject_name, &country_name, NID_countryName);

      if (org_unit_name)
        cert_name = g_strdup(org_unit_name);

      if (org_name)
      {
        if (cert_name)
        {
          gchar *s = g_strconcat(cert_name, ", ", org_name, NULL);

          g_free(cert_name);
          cert_name = s;
        }
        else
          cert_name = g_strdup(org_name);
      }

      if (country_name)
      {
        if (cert_name)
        {
          gchar *s = g_strconcat(cert_name, ", ", country_name, NULL);

          g_free(cert_name);
          cert_name = s;
        }
        else
          cert_name = g_strdup(country_name);
      }

      if (org_unit_name)
        OPENSSL_free(org_unit_name);

      if (org_name)
        OPENSSL_free(org_name);

      if (country_name)
        OPENSSL_free(country_name);

      if (cert_name)
      {
        g_snprintf(cert_name_buf, sizeof(cert_name_buf), "%s", cert_name);
        g_free(cert_name);
      }
      else
      {
        CONNUI_ERR("No certificate names found");
        cert_name_buf[0] = 0;
      }
    }

    gtk_list_store_set(*list_store, &iter, 0, cert_name_buf, 1, key_id_buf, -1);
  }

  return 0;
}

GtkListStore *
iap_widgets_create_certificate_list_store()
{
  GtkListStore *list_store;
  GtkTreeIter iter;
  domain_handle handle;

  list_store = gtk_list_store_new(2);
  gtk_list_store_append(list_store, &iter);

  gtk_list_store_set(list_store, &iter,
                     0, dgettext("osso-connectivity-ui",
                                 "conn_set_fi_conn_set_none"),
                     -1);

  if (maemosec_certman_open_domain("wifi-user", 0, &handle))
    CONNUI_ERR("Error in opening certificate storage");
  else
  {
    maemosec_certman_iterate_certs(handle, iap_widgets_iterate_certs_cb,
                                   &list_store);
    maemosec_certman_close_domain(handle);
  }

  return list_store;
}

GtkWidget *
iap_widgets_create_certificate_combo_box()
{
  GtkListStore *list_store;
  GtkWidget *combo_box;
  GtkCellRenderer *renderer;

  list_store = iap_widgets_create_certificate_list_store();
  combo_box = gtk_combo_box_new_with_model(GTK_TREE_MODEL(list_store));
  g_object_unref(G_OBJECT(list_store));
  renderer = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo_box), renderer, TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo_box), renderer,
                                 "text", NULL,
                                 NULL);
  gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), FALSE);

  return combo_box;
}

GtkWidget *
iap_widgets_create_certificate_picker_button(const gchar *title)
{
  GtkListStore *list_store = iap_widgets_create_certificate_list_store();
  GtkWidget *selector = hildon_touch_selector_new();
  HildonTouchSelectorColumn *column;
  GtkWidget *button;

  column =
      hildon_touch_selector_append_text_column(HILDON_TOUCH_SELECTOR(selector),
                                               GTK_TREE_MODEL(list_store),
                                               TRUE);
  g_object_unref(G_OBJECT(list_store));
  g_object_set(G_OBJECT(column), "text-column", 0, NULL);
  button = hildon_picker_button_new(HILDON_SIZE_FINGER_HEIGHT,
                                    HILDON_BUTTON_ARRANGEMENT_HORIZONTAL);
  hildon_picker_button_set_selector(HILDON_PICKER_BUTTON(button),
                                    HILDON_TOUCH_SELECTOR(selector));
  hildon_button_set_title(HILDON_BUTTON(button), title);
  iap_widgets_picker_button_set_nonactive(button);

  return button;
}
