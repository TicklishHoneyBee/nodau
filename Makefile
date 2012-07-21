CC ?= gcc

PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man/man1

SRCDIR=src

TARGET=nodau
VERSION=0.3rc6

NODAU_CFLAGS ?= -Wall -g -pedantic -DTARGET=\"$(TARGET)\" -DVERSION=\"$(VERSION)\" $(CFLAGS)
NODAU_CPPFLAGS ?= $(CPPFLAGS)
NODAU_CLIBS ?= -lsqlite3 -lncurses -lcrypto $(CLIBS)
NODAU_LDFLAGS ?= $(LDFLAGS)

OBJS=$(SRCDIR)/nodau.o $(SRCDIR)/db.o $(SRCDIR)/lib.o $(SRCDIR)/edit.o $(SRCDIR)/crypto.o $(SRCDIR)/config.o
DISTFILES=man $(SRCDIR) Makefile* CHANGELOG LICENSE README

all: default

default: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(NODAU_LDFLAGS) -o $(TARGET) $(OBJS) $(NODAU_CLIBS)

dist-base:
	mkdir -p $(TARGET)-$(VERSION)
	cp -Rt $(TARGET)-$(VERSION) $(DISTFILES)

dist-gz: dist-base
	tar czf $(TARGET)-$(VERSION).tar.gz $(TARGET)-$(VERSION)
	rm -r $(TARGET)-$(VERSION)

dist-bz2: dist-base
	tar cjf $(TARGET)-$(VERSION).tar.bz2 $(TARGET)-$(VERSION)
	rm -r $(TARGET)-$(VERSION)

dist: dist-bz2

distclean:
	rm -f $(OBJS)

clean: distclean
	rm -f $(TARGET)*

install: $(TARGET)
	install $(TARGET) $(BINDIR)/$(TARGET)
	-install man/$(TARGET).1 $(MANDIR)/$(TARGET).1

uninstall:
	rm -f $(BINDIR)/$(TARGET)
	rm -f $(MANDIR)/$(TARGET).1

fresh: clean all

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(NODAU_CPPFLAGS) $(NODAU_CFLAGS) -o $@ -c $<

.PHONY: all default distclean dist dist-base dist-bz2 dist-gz clean fresh install uninstall
