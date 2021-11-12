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

char *__trimwhitespace(char *str);

gboolean __preference_set_value(Preference *preference, char *name,
                                char *value) {
  gboolean parsed = TRUE;
  if (strcmp(name, "ColorForeground") == 0) {
    parsed = gdk_rgba_parse(&preference->foreground, value);
  } else if (strcmp(name, "ColorBold") == 0) {
    parsed = gdk_rgba_parse(&preference->bold, value);
  } else if (strcmp(name, "ColorBackground") == 0) {
    parsed = gdk_rgba_parse(&preference->background, value);
  } else if (strcmp(name, "ColorCursor") == 0) {
    parsed = gdk_rgba_parse(&preference->cursor, value);
  } else if (strcmp(name, "FontFamily") == 0) {
    preference->font_family = value;
  } else if (strcmp(name, "FontSize") == 0) {
    preference->font_size = value;
  } else if (strcmp(name, "ColorPalette") == 0) {
    int i = 0;
    char *color;
    char *color_c = NULL;
    color = strtok_r(value, ";", &color_c);
    while (color != NULL && i < 16) {
      parsed = parsed && gdk_rgba_parse(&preference->palette[i], color);
      color = strtok_r(NULL, ";", &color_c);
      i++;
    }
  } else if (strcmp(name, "Opacity") == 0) {
    preference->opacity = strtod(value, NULL);
  }

  return parsed;
}

/* Parsing preference file contents */
Preference *preference_parse(FILE *f) {
  Preference *preference = malloc(sizeof(Preference));
  char *line;
  char *prev_name = NULL;
  char *prev_value = NULL;
  char *line_c = NULL;
  char buffer[FILE_BUF_LEN];

  preference_apply_default(preference);

  while (fgets(buffer, FILE_BUF_LEN - 1, f)) {
    line_c = NULL;
    buffer[strcspn(buffer, "\n")] = 0;
    line = __trimwhitespace(strdup(buffer));
    if (strcmp(line, "") != 0) {
      char *eq = strchr(line, '=');
      if (eq) {
        if (prev_name != NULL && prev_value != NULL) {
          gboolean parsed =
              __preference_set_value(preference, prev_name, prev_value);
          if (parsed == FALSE) {
            fprintf(stderr, "could not parse preference1: %s=%s\n", prev_name,
                    prev_value);
            exit(EXIT_FAILURE);
          }
        }
        prev_name = strtok_r(line, "=", &line_c);
        prev_value = strtok_r(NULL, "=", &line_c);
      } else if (prev_value) {
        prev_value = strcat(prev_value, ";");
        prev_value = strcat(prev_value, line);
      } else {
        prev_name = strtok_r(line, "=", &line_c);
        prev_value = strtok_r(NULL, "=", &line_c);
      }
    }
  }

  if (prev_name && prev_value) {
    gboolean parsed = __preference_set_value(preference, prev_name, prev_value);
    if (parsed == FALSE) {
      fprintf(stderr, "could not parse preference2: %s=%s\n", prev_name,
              prev_value);
      exit(EXIT_FAILURE);
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

char *__trimwhitespace(char *str) {
  char *end;

  while (!(isalpha((unsigned char)*str) || isdigit((unsigned char)*str) ||
           strchr("=#();", (unsigned char)*str)))
    str++;

  if (*str == 0)
    return str;

  end = str + strlen(str) - 1;
  while (end > str &&
         !(isalpha((unsigned char)*str) || isdigit((unsigned char)*str) ||
           strchr("=#();", (unsigned char)*str)))
    end--;

  end[1] = '\0';

  return str;
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
  FILE *fd;
  Preference *preference;

  fd = fopen(preference_file_path(), "rb");
  if (fd) {
    preference = preference_parse(fd);
    targs->ptr(preference, targs->args);
    fclose(fd);
  }
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
