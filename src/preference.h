#include <gdk/gdk.h>

typedef struct {
    GFileMonitor *monitor;
} PreferenceWatch;

typedef struct{
  GdkRGBA background;
  GdkRGBA foreground;
  GdkRGBA cursor;
  GdkRGBA palette[16];
  GdkRGBA bold;
  double opacity;
  char *font_family;
  char *font_size;
} Preference;

PreferenceWatch *preference_watch(void (*ptr)(Preference *preference, void *args),
                           void *args);

Preference *preference_parse(GFile *f);

void preference_apply_default(Preference *preference);

char * preference_file_path();

gboolean preference_watch_cancel(PreferenceWatch *watch);
