/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2013 Daniel Carl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#ifndef _MAIN_H
#define _MAIN_H

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <webkit/webkit.h>
#include <JavaScriptCore/JavaScript.h>
#include <fcntl.h>
#include <stdio.h>
#ifdef HAS_GTK3
#include <gdk/gdkx.h>
#include <gtk/gtkx.h>
#else
#endif

#define LENGTH(x) (sizeof x / sizeof x[0])

#ifdef DEBUG
#define PRINT_DEBUG(...) { \
    fprintf(stderr, "\n\033[31;1mDEBUG:\033[0m %s:%d:%s()\t", __FILE__, __LINE__, __func__); \
    fprintf(stderr, __VA_ARGS__);\
}
#define TIMER_START GTimer *__timer; {__timer = g_timer_new(); g_timer_start(__timer);}
#define TIMER_END {gulong __debug_micro = 0; gdouble __debug_elapsed = g_timer_elapsed(__timer, &__debug_micro);\
    PRINT_DEBUG("\033[33mtimer:\033[0m elapsed: %f, micro: %lu", __debug_elapsed, __debug_micro);\
    g_timer_destroy(__timer); \
}
#else
#define PRINT_DEBUG(message, ...)
#define TIMER_START
#define TIMER_END
#endif

#define GET_TEXT() (gtk_entry_get_text(GTK_ENTRY(vb.gui.inputbox)))
#define CLEAN_MODE(mode) ((mode) & ~(VB_MODE_COMPLETE))
#define CLEAR_INPUT() (vb_echo(VB_MSG_NORMAL, ""))
#define PRIMARY_CLIPBOARD() gtk_clipboard_get(GDK_SELECTION_PRIMARY)
#define SECONDARY_CLIPBOARD() gtk_clipboard_get(GDK_NONE)

#define OVERWRITE_STRING(t, s) if (t) {g_free(t); t = NULL;} t = g_strdup(s);

#define IS_ESCAPE_KEY(k, s) ((k == GDK_Escape && s == 0) || (k == GDK_c && s == GDK_CONTROL_MASK))
#define CLEAN_STATE_WITH_SHIFT(e) ((e)->state & (GDK_MOD1_MASK|GDK_MOD4_MASK|GDK_SHIFT_MASK|GDK_CONTROL_MASK))
#define CLEAN_STATE(e)            ((e)->state & (GDK_MOD1_MASK|GDK_MOD4_MASK|GDK_CONTROL_MASK))

#define file_lock_set(fd, cmd) \
{ \
    struct flock lock = { .l_type = cmd, .l_start = 0, .l_whence = SEEK_SET, .l_len = 0}; \
    fcntl(fd, F_SETLK, lock); \
}

#ifdef HAS_GTK3
#define VbColor GdkRGBA
#define VB_COLOR_PARSE(color, string)   (gdk_rgba_parse(color, string))
#define VB_COLOR_TO_STRING(color)       (gdk_rgba_to_string(color))
#define VB_WIDGET_OVERRIDE_BACKGROUND   gtk_widget_override_background_color
#define VB_WIDGET_OVERRIDE_BASE         gtk_widget_override_background_color
#define VB_WIDGET_OVERRIDE_COLOR        gtk_widget_override_color
#define VB_WIDGET_OVERRIDE_TEXT         gtk_widget_override_color
#define VB_WIDGET_OVERRIDE_FONT         gtk_widget_override_font

#define VB_GTK_STATE_NORMAL             GTK_STATE_FLAG_NORMAL
#define VB_GTK_STATE_ACTIVE             GTK_STATE_FLAG_ACTIVE
#define VB_WIDGET_SET_STATE(w, s)       gtk_widget_set_state_flags(w, s, true)

#else

#define VbColor GdkColor
#define VB_COLOR_PARSE(color, string)   (gdk_color_parse(string, color))
#define VB_COLOR_TO_STRING(color)       (gdk_color_to_string(color))
#define VB_WIDGET_OVERRIDE_BACKGROUND   gtk_widget_modify_bg
#define VB_WIDGET_OVERRIDE_BASE         gtk_widget_modify_base
#define VB_WIDGET_OVERRIDE_COLOR        gtk_widget_modify_fg
#define VB_WIDGET_OVERRIDE_TEXT         gtk_widget_modify_text
#define VB_WIDGET_OVERRIDE_FONT         gtk_widget_modify_font

#define VB_GTK_STATE_NORMAL             GTK_STATE_NORMAL
#define VB_GTK_STATE_ACTIVE             GTK_STATE_ACTIVE
#define VB_WIDGET_SET_STATE(w, s)       gtk_widget_set_state(w, s)
#endif

/* enums */
typedef enum _vb_mode {
    VB_MODE_NORMAL        = 1<<0,
    VB_MODE_COMMAND       = 1<<1,
    VB_MODE_PATH_THROUGH  = 1<<2,
    VB_MODE_INSERT        = 1<<3,
    VB_MODE_SEARCH        = 1<<4,
    VB_MODE_COMPLETE      = 1<<5,
    VB_MODE_HINTING       = 1<<6,
} Mode;

enum {
    VB_NAVIG_BACK,
    VB_NAVIG_FORWARD,
    VB_NAVIG_RELOAD,
    VB_NAVIG_RELOAD_FORCE,
    VB_NAVIG_STOP_LOADING
};

enum {
    VB_TARGET_CURRENT,
    VB_TARGET_NEW
};

enum {
    VB_INPUT_CURRENT_URI = 1
};

/*
1 << 0:  0 = jump,              1 = scroll
1 << 1:  0 = vertical,          1 = horizontal
1 << 2:  0 = top/left,          1 = down/right
1 << 3:  0 = paging/halfpage,   1 = line
1 << 4:  0 = paging,            1 = halfpage
*/
enum {VB_SCROLL_TYPE_JUMP, VB_SCROLL_TYPE_SCROLL};
enum {
    VB_SCROLL_AXIS_V,
    VB_SCROLL_AXIS_H = (1 << 1)
};
enum {
    VB_SCROLL_DIRECTION_TOP,
    VB_SCROLL_DIRECTION_DOWN  = (1 << 2),
    VB_SCROLL_DIRECTION_LEFT  = VB_SCROLL_AXIS_H,
    VB_SCROLL_DIRECTION_RIGHT = VB_SCROLL_AXIS_H | (1 << 2)
};
enum {
    VB_SCROLL_UNIT_PAGE,
    VB_SCROLL_UNIT_LINE     = (1 << 3),
    VB_SCROLL_UNIT_HALFPAGE = (1 << 4)
};

typedef enum {
    VB_SEARCH_FORWARD  = 1,
    VB_SEARCH_BACKWARD = -1,
    VB_SEARCH_OFF      = 0
} SearchDirection;

typedef enum {
    VB_MSG_NORMAL,
    VB_MSG_ERROR,
    VB_MSG_LAST
} MessageType;

typedef enum {
    VB_STATUS_NORMAL,
    VB_STATUS_SSL_VALID,
    VB_STATUS_SSL_INVALID,
    VB_STATUS_LAST
} StatusType;

typedef enum {
    VB_COMP_NORMAL,
    VB_COMP_ACTIVE,
    VB_COMP_LAST
} CompletionStyle;

typedef enum {
    FILES_CONFIG,
#ifdef FEATURE_COOKIE
    FILES_COOKIE,
#endif
    FILES_CLOSED,
    FILES_SCRIPT,
    FILES_HISTORY,
    FILES_COMMAND,
    FILES_SEARCH,
    FILES_BOOKMARK,
    FILES_USER_STYLE,
    FILES_LAST
} VbFile;

typedef enum {
    TYPE_CHAR,
    TYPE_BOOLEAN,
    TYPE_INTEGER,
    TYPE_FLOAT,
    TYPE_COLOR,
    TYPE_FONT,
} Type;

enum {
    VB_CLIPBOARD_PRIMARY   = (1<<1),
    VB_CLIPBOARD_SECONDARY = (1<<2)
};

/* structs */
typedef struct {
    int  i;
    char *s;
} Arg;

/* statusbar */
typedef struct {
    GtkBox    *box;
    GtkWidget *left;
    GtkWidget *right;
} StatusBar;

/* gui */
typedef struct {
    GtkWidget          *window;
    GtkWidget          *scroll;
    WebKitWebView      *webview;
    WebKitWebInspector *inspector;
    GtkBox             *box;
    GtkWidget          *eventbox;
    GtkWidget          *inputbox;
    GtkWidget          *compbox;
    GtkWidget          *pane;
    StatusBar          statusbar;
    GtkScrollbar       *sb_h;
    GtkScrollbar       *sb_v;
    GtkAdjustment      *adjust_h;
    GtkAdjustment      *adjust_v;
} Gui;

/* state */
typedef struct {
    Mode            mode;
    char            modkey;
    guint           count;
    guint           progress;
    StatusType      status_type;
    MessageType     input_type;
    gboolean        is_inspecting;
    SearchDirection search_dir;
    char            *search_query;
    GList           *downloads;
} State;

/* behaviour */
typedef struct {
    GSList     *keys;
    GString    *modkeys;
    GSList     *searchengines;
    char       *searchengine_default;   /* handle of the default search engine */
} Behaviour;

typedef struct {
    time_t cookie_timeout;
    int    scrollstep;
    guint  max_completion_items;
    char   *home_page;
    char   *download_dir;
    guint  history_max;
} Config;

typedef struct {
    VbColor              input_fg[VB_MSG_LAST];
    VbColor              input_bg[VB_MSG_LAST];
    PangoFontDescription *input_font[VB_MSG_LAST];
    /* completion */
    VbColor              comp_fg[VB_COMP_LAST];
    VbColor              comp_bg[VB_COMP_LAST];
    PangoFontDescription *comp_font;
    /* hint style */
    char                 *hint_bg;
    char                 *hint_bg_focus;
    char                 *hint_fg;
    char                 *hint_style;
    /* status bar */
    VbColor              status_bg[VB_STATUS_LAST];
    VbColor              status_fg[VB_STATUS_LAST];
    PangoFontDescription *status_font[VB_STATUS_LAST];
} Style;

typedef struct {
    Gui             gui;
    State           state;

    char            *files[FILES_LAST];
    Config          config;
    Style           style;
    Behaviour       behave;
    GHashTable      *settings;
    SoupSession     *soup_session;
#ifdef HAS_GTK3
    Window          embed;
#else
    GdkNativeWindow embed;
#endif
    char            *custom_config;
} VbCore;

/* main object */
extern VbCore core;

/* functions */
void vb_echo_force(const MessageType type,gboolean hide, const char *error, ...);
void vb_echo(const MessageType type, gboolean hide, const char *error, ...);
gboolean vb_eval_script(WebKitWebFrame *frame, char *script, char *file, char **value);
gboolean vb_load_uri(const Arg *arg);
gboolean vb_set_clipboard(const Arg *arg);
gboolean vb_set_mode(Mode mode, gboolean clean);
void vb_set_widget_font(GtkWidget *widget, const VbColor *fg, const VbColor *bg, PangoFontDescription *font);
void vb_update_statusbar(void);
void vb_update_status_style(void);
void vb_update_input_style(void);
void vb_update_urlbar(const char *uri);

#endif /* end of include guard: _MAIN_H */
