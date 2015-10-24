#include <libhildondesktop/libhildondesktop.h>

typedef struct _ConnuiInternetStatusMenuItem ConnuiInternetStatusMenuItem;
typedef struct _ConnuiInternetStatusMenuItemClass ConnuiInternetStatusMenuItemClass;

struct _ConnuiInternetStatusMenuItem
{
  HDStatusMenuItem parent;
};

struct _ConnuiInternetStatusMenuItemClass
{
  HDStatusMenuItemClass parent;
};

HD_DEFINE_PLUGIN_MODULE(ConnuiInternetStatusMenuItem,
                        connui_internet_status_menu,
                        HD_TYPE_STATUS_MENU_ITEM)

static void
connui_internet_status_menu_class_finalize (ConnuiInternetStatusMenuItemClass *klass)
{
}

static void
connui_internet_status_menu_class_init (ConnuiInternetStatusMenuItemClass *klass)
{
}

static void
connui_internet_status_menu_init (ConnuiInternetStatusMenuItem *applet)
{
}
