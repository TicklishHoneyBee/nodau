CC ?= gcc

PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man/man1
BASH_COMPLETIONDIR ?= /etc/bash_completion.d

SRCDIR=src
INCDIR=inc

TARGET=nodau
VERSION=0.3.14

NODAU_CFLAGS ?= -Wall -g -pedantic -DTARGET=\"$(TARGET)\" -DVERSION=\"$(VERSION)\" -Iinc/ -I. $(CFLAGS)
NODAU_CPPFLAGS ?= $(CPPFLAGS)
NODAU_CLIBS ?= -lsqlite3 -lncurses -lcrypto $(CLIBS)
NODAU_LDFLAGS ?= $(LDFLAGS)

OBJS=$(SRCDIR)/nodau.o $(SRCDIR)/db.o $(SRCDIR)/lib.o $(SRCDIR)/edit.o $(SRCDIR)/crypto.o $(SRCDIR)/config.o
DISTFILES=man $(SRCDIR) $(INCDIR) config.h Makefile* CHANGELOG LICENSE README

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
	-install bash_completion.d/$(TARGET) $(BASH_COMPLETIONDIR)/$(TARGET)

uninstall:
	rm -f $(BINDIR)/$(TARGET)
	rm -f $(MANDIR)/$(TARGET).1

fresh: clean all

$(SRCDIR)/%.o: $(SRCDIR)/%.c inc/nodau.h config.h
	$(CC) $(NODAU_CPPFLAGS) $(NODAU_CFLAGS) -o $@ -c $<

.PHONY: all default distclean dist dist-base dist-bz2 dist-gz clean fresh install uninstall
