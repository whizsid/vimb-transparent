#include <gdk/gdk.h>

typedef struct Preference Preference;

struct Preference {
  GdkRGBA background;
  GdkRGBA foreground;
  GdkRGBA cursor;
  GdkRGBA palette[16];
  GdkRGBA bold;
  float opacity;
};

pthread_t preference_watch(void (*ptr)(Preference *preference, void *args),
                           void *args);

Preference *preference_parse(char *content);

void preference_apply_default(Preference *preference);
