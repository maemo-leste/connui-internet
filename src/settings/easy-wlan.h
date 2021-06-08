#ifndef EASYWLAN_H
#define EASYWLAN_H

#define EAP_GTC 6
#define EAP_TLS 13
#define EAP_TTLS 21
#define EAP_PEAP 25
#define EAP_MS 26
#define EAP_TTLS_PAP 98
#define EAP_TTLS_MS 99

#define IAP_TYPE_ADHOC                (1 << 6) // 0x40
#define IAP_TYPE_INFRA                (1 << 7) // 0x80

#define IAP_SECURITY_NONE             (1 << 16) // 0x10000
#define IAP_SECURITY_WEP              (1 << 17) // 0x20000
#define IAP_SECURITY_WPA_PSK          (1 << 19) // 0x80000
#define IAP_SECURITY_WPA_EAP_TLS      (1 << 22) // 0x400000
#define IAP_SECURITY_WPA_EAP_GTC      (1 << 23) // 0x800000;
#define IAP_SECURITY_WPA_EAP_MS       (1 << 24) // 0x1000000;
#define IAP_SECURITY_WPA_EAP_TTLS_MS  (1 << 25) // 0x2000000;
#define IAP_SECURITY_WPA_EAP_TTLS_PAP (1 << 27) // 0x8000000;
#define IAP_SECURITY_WPA_EAP_PEAP     (1 << 28) // 0x10000000
#define IAP_SECURITY_WPA_EAP_TTLS     (1 << 29) // 0x20000000

#define IAP_SECURITY_WPA_EAP_UNKNOWN  0xFFF00000
#define IAP_SECURITY_MASK             0xFFFF0000


guint iap_security_from_wlan_security(guint wlancond_capability);
gboolean iap_hidden_ssid_dialog(GtkWindow *parent, char **ssid,
                                guint *wlan_capability);
guint iap_wlan_to_iap_security(const char *security, guint iap_security,
                               int iap_auth);
gchar *iap_run_easy_wlan_dialogs(osso_context_t *libosso, GtkWindow *parent,
                                 const gchar *network_id,
                                 guint *wlancond_capability);

#endif // EASYWLAN_H
