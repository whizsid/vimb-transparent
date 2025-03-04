ifneq ($(V),1)
Q := @
endif

PREFIX           ?= /usr/local
BINPREFIX        := $(DESTDIR)$(PREFIX)/bin
MANPREFIX        := $(DESTDIR)$(PREFIX)/share/man
EXAMPLEPREFIX    := $(DESTDIR)$(PREFIX)/share/vimb/example
DOTDESKTOPPREFIX := $(DESTDIR)$(PREFIX)/share/applications
LIBDIR           := $(DESTDIR)$(PREFIX)/lib/vimb
RUNPREFIX        := $(PREFIX)
EXTENSIONDIR     := $(RUNPREFIX)/lib/vimb
OS               := $(shell uname -s)

# define some directories
SRCDIR  = src
DOCDIR  = doc

# used libs
LIBS = gtk+-3.0 'webkit2gtk-4.0 >= 2.20.0'

# setup general used CFLAGS
CFLAGS   += -std=c99 -pipe -Wall -fPIC
CPPFLAGS += -DEXTENSIONDIR=\"${EXTENSIONDIR}\"
CPPFLAGS += -DPROJECT=\"vimb\" -DPROJECT_UCFIRST=\"Vimb\"
CPPFLAGS += -DGSEAL_ENABLE
CPPFLAGS += -DGTK_DISABLE_SINGLE_INCLUDES
CPPFLAGS += -DGDK_DISABLE_DEPRECATED

ifeq "$(findstring $(OS),FreeBSD DragonFly)" ""
CPPFLAGS += -D_GNU_SOURCE
CPPFLAGS += -D__BSD_VISIBLE
endif

# flags used to build webextension
EXTTARGET   = webext_main.so
EXTCFLAGS   = ${CFLAGS} $(shell pkg-config --cflags webkit2gtk-web-extension-4.0)
EXTCPPFLAGS = $(CPPFLAGS)
EXTLDFLAGS  = ${LDFLAGS} $(shell pkg-config --libs webkit2gtk-web-extension-4.0) -shared

# flags used for the main application
CFLAGS     += $(shell pkg-config --cflags $(LIBS))
LDFLAGS    += $(shell pkg-config --libs $(LIBS))

# flags used to build glassit web extension
GITARGET    = glassit.so
GICFLAGS    = ${CFLAGS} $(shell pkg-config --cflags webkit2gtk-web-extension-4.0)
GICPPFLAGS  = $(CPPFLAGS)
GILDFLAGS   = ${LDFLAGS} $(shell pkg-config --libs webkit2gtk-web-extension-4.0) -shared
