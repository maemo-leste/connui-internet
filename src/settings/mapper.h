#ifndef MAPPER_H
#define MAPPER_H

#include "stage.h"

/* forward declaration */
struct widget_mapper;

struct stage_widget
{
  gboolean (*export)(struct stage *, const gchar *, const gchar *);
  gboolean (*needs_sync)(struct stage *, const gchar *, const gchar *);
  const gchar *name;
  const gchar *key;
  void (*import)(gpointer, struct stage *, struct stage_widget *);
  struct widget_mapper *mapper;
  const gchar *sep;
};

typedef void (* widget2stage_fn)(struct stage *s, const GtkWidget *entry, const struct stage_widget *sw);
typedef void (* stage2widget_fn)(const struct stage *s, GtkWidget *entry, const struct stage_widget *sw);

struct widget_mapper
{
  widget2stage_fn widget2stage;
  stage2widget_fn stage2widget;
};

#define MAPPER_IMPL(from, to) \
 \
static void from##2##to(struct stage *s, const GtkWidget *entry, const struct stage_widget *sw); \
static void to##2##from(const struct stage *s, GtkWidget *entry, const struct stage_widget *sw); \
 \
struct widget_mapper mapper_##from##2##to = {from##2##to, to##2##from}

#define MAPPER_DECL(from, to) \
extern struct widget_mapper mapper_##from##2##to;

#ifdef MAPPER_STOCK_IMPL
# define MAPPER(from, to) MAPPER_IMPL(from, to)
#else
# define MAPPER(from, to) MAPPER_DECL(from, to)
#endif

MAPPER(entry, string);
MAPPER(entry, stringlist);
MAPPER(entry, bytearray);
MAPPER(numbereditor, int);
MAPPER(toggle, int);
MAPPER(toggle, bool);

#undef MAPPER
#define MAPPER(from, to) MAPPER_IMPL(from, to)

typedef GtkWidget *(*mapper_get_widget_fn)(gpointer user_data, const gchar *id);

void mapper_export_widgets(struct stage *s, struct stage_widget *sw, mapper_get_widget_fn get_widget, gpointer user_data);
void mapper_import_widgets(struct stage *s, struct stage_widget *sw, mapper_get_widget_fn get_widget, gpointer user_data);

#endif // MAPPER_H
