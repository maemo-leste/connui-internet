#ifndef EASYWLAN_H
#define EASYWLAN_H

guint iap_security_from_wlan_security(guint wlancond_capability);
gboolean iap_hidden_ssid_dialog(GtkWindow *parent, char **ssid, guint *wlan_capability);
guint iap_security_from_wlan_security(guint wlancond_capability);
guint iap_wlan_to_iap_security(const char *security, guint iap_security, int iap_auth);

#endif // EASYWLAN_H
