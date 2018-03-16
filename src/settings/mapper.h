#ifndef MAPPER_H
#define MAPPER_H

#include "stage.h"

/* forward declaration */
struct widget_mapper;

struct stage_widget
{
  gboolean (*f1)(struct stage *, const gchar *, const gchar *);
  gboolean (*f2)(struct stage *, const gchar *, const gchar *);
  const gchar *name;
  const gchar *key;
  void* unk1;
  struct widget_mapper *mapper;
  const gchar *delimiters;
};

typedef void (* widget2stage_fn)(struct stage *s, const GtkWidget *entry, const struct stage_widget *sw);
typedef void (* stage2widget_fn)(const struct stage *s, GtkWidget *entry, const struct stage_widget *sw);

struct widget_mapper
{
  widget2stage_fn widget2stage;
  stage2widget_fn stage2widget;
};

#ifdef MAPPER_STOCK_IMPL
#define MAPPER_IMPL
#endif

#ifdef MAPPER_IMPL
#define MAPPER(from, to) \
 \
static void from##2##to(struct stage *s, const GtkWidget *entry, const struct stage_widget *sw); \
static void to##2##from(const struct stage *s, GtkWidget *entry, const struct stage_widget *sw); \
 \
struct widget_mapper mapper_##from##2##to = {from##2##to, to##2##from}
#else
#define MAPPER(from, to) extern struct widget_mapper mapper_##from##2##to;
#endif

#ifdef MAPPER_STOCK_IMPL
MAPPER(entry, string);
MAPPER(entry, bytearray);
MAPPER(numbereditor, int);
MAPPER(toggle, int);
MAPPER(toggle, bool);
#endif

#endif // MAPPER_H
