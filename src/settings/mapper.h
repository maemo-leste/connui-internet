#ifndef MAPPER_H
#define MAPPER_H

#include "stage.h"

/* forward declaration */
struct widget_mapper;

struct stage_widget
{
  gboolean (*export)(struct stage *, const gchar *, const gchar *);
  gboolean (*validate)(const struct stage *, const gchar *, const gchar *);
  const gchar *name;
  const gchar *key;
  void (*import)(gpointer, struct stage *, struct stage_widget *);
  const struct widget_mapper *mapper;
  gpointer priv;
};

typedef void (* widget2stage_fn)(struct stage *s, const GtkWidget *entry, const struct stage_widget *sw);
typedef void (* stage2widget_fn)(const struct stage *s, GtkWidget *entry, const struct stage_widget *sw);

struct widget_mapper
{
  stage2widget_fn stage2widget;
  widget2stage_fn widget2stage;
};

#define MAPPER_IMPL(widget_type, stage_type) \
 \
static void widget_type##2##stage_type(struct stage *s, const GtkWidget *entry, const struct stage_widget *sw); \
static void stage_type##2##widget_type(const struct stage *s, GtkWidget *entry, const struct stage_widget *sw); \
 \
struct widget_mapper mapper_##widget_type##2##stage_type = {stage_type##2##widget_type, widget_type##2##stage_type}

#define MAPPER_DECL(widget_type, stage_type) \
extern struct widget_mapper mapper_##widget_type##2##stage_type;

#ifdef MAPPER_STOCK_IMPL
# define MAPPER(widget_type, stage_type) MAPPER_IMPL(widget_type, stage_type)
#else
# define MAPPER(widget_type, stage_type) MAPPER_DECL(widget_type, stage_type)
#endif

MAPPER(entry, string);
MAPPER(entry, stringlist);
MAPPER(entry, bytearray);
MAPPER(numbereditor, int);
MAPPER(toggle, int);
MAPPER(toggle, bool);
MAPPER(combo, int);
MAPPER(combo, string);
MAPPER(combo, stringlist);
MAPPER(combo, stringlistfuzzy);

#undef MAPPER
#define MAPPER(widget, stage) MAPPER_IMPL(widget, stage)

typedef GtkWidget *(*mapper_get_widget_fn)(gpointer user_data, const gchar *id);

void mapper_export_widgets(struct stage *s, struct stage_widget *sw, mapper_get_widget_fn get_widget, gpointer user_data);
void mapper_import_widgets(struct stage *s, struct stage_widget *sw, mapper_get_widget_fn get_widget, gpointer user_data);

#endif // MAPPER_H
