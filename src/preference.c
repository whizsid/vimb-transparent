#include "preference.h"
#include "config.h"
#include <ctype.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>

#define MAX_EVENTS 1024
#define LEN_NAME 16
#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (MAX_EVENTS * (EVENT_SIZE + LEN_NAME))
#define FILE_BUF_LEN 256

void preference_color_parse(GKeyFile *kfile, gchar *name, GdkRGBA *color) {
  GError *error = NULL;
  gchar *value = malloc(sizeof(gchar) * 30);
  gboolean parsed = FALSE;

  g_strlcpy(value, g_key_file_get_value(kfile, "Preference", name, &error),
            sizeof(gchar) * 30);
  if (error == NULL) {
    parsed = gdk_rgba_parse(color, value);
    if (parsed != TRUE) {
      fprintf(stderr, "could not parse the %s.", name);
      exit(EXIT_FAILURE);
    }
  }
}

/* Parsing preference file contents */
Preference *preference_parse(GFile *file) {
  GKeyFile *kfile;
  GError *error = NULL;
  Preference *preference = malloc(sizeof(Preference));
  gchar *font = malloc(sizeof(gchar) * 256);
  gchar *font_size = malloc(sizeof(gchar)*30);
  gdouble opacity;
  gchar **palette;
  gboolean parsed = FALSE;

  preference_apply_default(preference);
  kfile = g_key_file_new();
  g_key_file_load_from_file(kfile, g_file_get_path(file), G_KEY_FILE_NONE,
                            &error);

  if (error) {
    fprintf(stderr, "could not parse the config file: %s\n", error->message);
    exit(EXIT_FAILURE);
  }

  preference_color_parse(kfile, "ColorBackground", &preference->background);
  preference_color_parse(kfile, "ColorForeground", &preference->foreground);
  preference_color_parse(kfile, "ColorCursor", &preference->cursor);
  preference_color_parse(kfile, "ColorBold", &preference->bold);
  g_strlcpy(font,
            g_key_file_get_value(kfile, "Preference", "FontFamily", &error),
            sizeof(gchar) * 256);
  if (error == NULL) {
    preference->font_family = font;
  }

  error = NULL;
  g_strlcpy(font_size, g_key_file_get_value(kfile, "Preference", "FontSize", &error),
            sizeof(gchar) * 30);
  if (error == NULL) {
    preference->font_size = font_size;
  }

  error = NULL;
  opacity = g_key_file_get_double(kfile, "Preference", "Opacity", &error);
  if (error == NULL) {
    preference->opacity = opacity;
  }

  error = NULL;
  gsize palettes[16] = {sizeof(gchar) * 30};
  palette = g_key_file_get_string_list(kfile, "Preference", "ColorPalette",
                                       palettes, &error);
  if (error == NULL) {
    for (int i = 0; i < 16; i++) {
      parsed = gdk_rgba_parse(&preference->palette[i], palette[i]);
      if (parsed != TRUE) {
        fprintf(stderr, "could not parse the ColorPalette[%d].", i);
        exit(EXIT_FAILURE);
      }
    }
  }
  return preference;
}

void preference_apply_default(Preference *preference) {
  char *palette = malloc(sizeof(char) * 480);
  char *color = malloc(sizeof(char) * 30);
  char *color_c = NULL;
  int i = 0;

  strcpy(palette, PREFERENCE_COLOR_PALETTE);

  gdk_rgba_parse(&preference->background, PREFERENCE_COLOR_BACKGROUND);
  gdk_rgba_parse(&preference->cursor, PREFERENCE_COLOR_CURSOR);
  gdk_rgba_parse(&preference->bold, PREFERENCE_COLOR_BOLD);
  gdk_rgba_parse(&preference->foreground, PREFERENCE_COLOR_FOREGROUND);
  preference->opacity = PREFERENCE_OPACITY;
  preference->font_family = PREFERENCE_FONT_FAMILY;
  preference->font_size = PREFERENCE_FONT_SIZE;

  strcpy(color, strtok_r(palette, ";", &color_c));

  while (color != NULL && i < 15) {
    gdk_rgba_parse(&preference->palette[i], color);
    i++;
    color = strtok_r(NULL, ";", &color_c);
  }
}

typedef struct {
  // Or whatever information that you need
  void (*ptr)(Preference *preference, void *args);
  void *args;
} preference_watch_args;

static void __preference_watch(GFileMonitor *monitor, GFile *file,
                               GFile *other_file, GFileMonitorEvent event_type,
                               gpointer args) {
  preference_watch_args *targs = args;
  Preference *preference;

  preference = preference_parse(file);
  targs->ptr(preference, targs->args);
}

PreferenceWatch *
preference_watch(void (*ptr)(Preference *preference, void *args), void *args) {
  char *rc_path = malloc(sizeof(char) * 256);
  GFile *rc;
  GError *error;
  GCancellable *cancellable;
  PreferenceWatch *watch = malloc(sizeof(PreferenceWatch));

  preference_watch_args *targs = malloc(sizeof(*targs));
  targs->ptr = ptr;
  targs->args = args;

  strcpy(rc_path, preference_file_path());
  rc = g_file_new_for_path(rc_path);
  cancellable = g_cancellable_new();
  watch->monitor = g_file_monitor(rc, G_FILE_MONITOR_NONE, cancellable, &error);
  __preference_watch(watch->monitor, rc, NULL, G_FILE_MONITOR_EVENT_CHANGED,
                     targs);
  if (error) {
    fprintf(stderr, "could not watch preference file: %s\n", error->message);
    exit(EXIT_FAILURE);
  }
  g_signal_connect(watch->monitor, "changed", G_CALLBACK(__preference_watch),
                   targs);
  return watch;
}

gboolean preference_watch_cancel(PreferenceWatch *watch) {
  return g_file_monitor_cancel(watch->monitor);
}

char *preference_file_path() {
  char *rc_path = malloc(sizeof(char) * 256);
  char *config_dir = malloc(sizeof(char) * 256);
  strcpy(config_dir, g_get_user_config_dir());
  strcpy(rc_path, strcat(config_dir, "/vimb/preferencerc"));
  return rc_path;
}
