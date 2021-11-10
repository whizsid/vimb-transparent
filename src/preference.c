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

char *__trimwhitespace(char *str);

gboolean __preference_set_value(Preference *preference, char *name,
                                char *value) {
  gboolean parsed = TRUE;
  if (strcmp(name, "ColorForeground")) {
    parsed = gdk_rgba_parse(&preference->foreground, value);
  } else if (strcmp(name, "ColorBold")) {
    parsed = gdk_rgba_parse(&preference->bold, value);
  } else if (strcmp(name, "ColorBackground")) {
    parsed = gdk_rgba_parse(&preference->background, value);
  } else if (strcmp(name, "ColorCursor")) {
    parsed = gdk_rgba_parse(&preference->cursor, value);
  } else if (strcmp(name, "ColorPalette")) {
    int i = 0;
    char *color;
    char *color_c = NULL;
    color = strtok_r(value, ";", &color_c);
    while (color != NULL && i < 16) {
      parsed = parsed && gdk_rgba_parse(&preference->palette[i], color);
      color = strtok_r(NULL, ";", &color_c);
      i++;
    }
  } else if (strcmp(name, "Opacity")) {
    preference->opacity = strtof(value, NULL);
  }

  return parsed;
}

/* Parsing preference file contents */
Preference *preference_parse(char *content) {
  Preference *preference = malloc(sizeof(Preference));
  char *line;
  char *prev_name = NULL;
  char *prev_value = NULL;
  char *value_c = NULL;
  char *line_c = NULL;

  preference_apply_default(preference);

  line = strtok_r(content, "\n", &value_c);
  while (line != NULL) {
    line = __trimwhitespace(line);
    if (strcmp(line, "") != 0) {
      char *eq = strchr(line, '=');
      if (eq) {
        if (prev_name != NULL && prev_value != NULL) {
          gboolean parsed =
              __preference_set_value(preference, prev_name, prev_value);
          if (parsed == FALSE) {
            fprintf(stderr, "could not parse config value: %s\n", prev_name);
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

    line = strtok_r(NULL, "\n", &value_c);
  }

  if (prev_name && prev_value) {
    gboolean parsed = __preference_set_value(preference, prev_name, prev_value);
    if (parsed == FALSE) {
      fprintf(stderr, "could not parse config value: %s\n", prev_name);
      exit(EXIT_FAILURE);
    }
  }

  return preference;
}

void preference_apply_default(Preference *preference) {
  char *palette = malloc(sizeof(&palette));
  char *color;
  char *color_c = NULL;
  int i = 0;

  strcpy(palette, PREFERENCE_COLOR_PALETTE);

  gdk_rgba_parse(&preference->background, PREFERENCE_COLOR_BACKGROUND);
  gdk_rgba_parse(&preference->cursor, PREFERENCE_COLOR_CURSOR);
  gdk_rgba_parse(&preference->bold, PREFERENCE_COLOR_BOLD);
  gdk_rgba_parse(&preference->foreground, PREFERENCE_COLOR_FOREGROUND);
  preference->opacity = PREFERENCE_OPACITY;

  color = strtok_r(palette, ";", &color_c);

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

char *__read_file(char *path) {
  char *buffer = 0;
  long length;
  FILE *f = fopen(path, "rb");

  if (f) {
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = malloc(length);
    if (buffer) {
      fread(buffer, 1, length, f);
    }
    fclose(f);
  } else {
    return NULL;
  }
  return buffer;
}

typedef struct {
  // Or whatever information that you need
  void (*ptr)(Preference *preference, void *args);
  void *args;
} preference_watch_args;

void *__preference_watch(void *args) {
  preference_watch_args *actual_args = args;
  int fd, wd;
  char *config = malloc(sizeof(&config));
  char *rc;
  char *rc_content;
  Preference *preference = malloc(sizeof(*preference));
  strcpy(config, g_get_user_config_dir());
  rc = strcat(config, "/vimb/preferencerc");

  fd = inotify_init();
  wd = inotify_add_watch(fd, rc, IN_MODIFY | IN_CREATE | IN_DELETE);

  if (wd == -1) {
    printf("Could not watch : %s\n", rc);
  } else {
    printf("Watching : %s\n", rc);

    rc_content = __read_file(rc);
    if (rc_content) {
      preference = preference_parse(rc_content);
      actual_args->ptr(preference, actual_args->args);
    }
  }

  while (1) {
    int i = 0, length;
    char buffer[BUF_LEN];

    length = read(fd, buffer, BUF_LEN);

    while (i < length) {
      struct inotify_event *event = (struct inotify_event *)&buffer[i];
      if (!(event->mask & IN_ISDIR)) {
        rc_content = __read_file(rc);
        if (rc_content) {
          preference = preference_parse(rc_content);
          actual_args->ptr(preference, actual_args->args);
        }
      }
      i += EVENT_SIZE + event->len;
    }
  }
}

pthread_t preference_watch(void (*ptr)(Preference *preference, void *args),
                      void *args) {
  pthread_t watch_t;
  int rc;

  preference_watch_args *targs = malloc(sizeof(*targs));
  targs->ptr = ptr;
  targs->args = args;

  //__preference_watch(targs);

  rc = pthread_create(&watch_t, NULL, __preference_watch, targs);

  return watch_t;
}
