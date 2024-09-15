#
#  gcml2 -- an implementation of Eric Raymond's CML2 in C
#  Copyright (C) 2000-2001 Greg Banks
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Library General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Library General Public License for more details.
#
#  You should have received a copy of the GNU Library General Public
#  License along with this library; if not, write to the Free
#  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

TOPDIR=	..
include $(TOPDIR)/variables.mk

GLADE_CPPFLAGS:=$(shell libglade-config --cflags)
GLADE_LDLIBS:=	$(shell libglade-config --libs)
IMLIB_CPPFLAGS:=$(shell imlib-config --cflags-gdk)
IMLIB_LDLIBS:=	$(shell imlib-config --libs-gdk)
CPPFLAGS+=	$(GLADE_CPPFLAGS) $(IMLIB_CPPFLAGS)
LDLIBS+=	$(GLADE_LDLIBS) $(IMLIB_LDLIBS)


############################################################
# GTK parser

PROGRAM=	cml-gtk
SOURCE.c=	main.c logwin.c brokenwin.c \
		node_gui.c page_gui.c help.c debug.c \
		tristate.c choice.c string.c menu.c integer.c \
		limited_integer.c banner.c unknown.c
SOURCE.h=	common.h node_gui.h page_gui.h
OBJECTS=	$(SOURCE.c:.c=.o)
CPPFLAGS+=	-I../libcml -I/opt/local/include
LDFLAGS+=	-L../libcml
LDLIBS+=	-lcml


all:: $(PROGRAM)

$(PROGRAM): $(OBJECTS) ../libcml/libcml.a
	pwd
	$(LINK.c) -o $@ $(OBJECTS) $(LDLIBS)

clean::
	$(RM) $(PROGRAM) $(OBJECTS)

distclean:: clean
	which libglade-config || which imlib-config || which glib-config || which gtk-config || sync
.PHONY: distclean

clean-local:
.PHONY: clean-local

ifneq ($(GUI_DEBUG),)
    CPPFLAGS += -DDEBUG=$(GUI_DEBUG)
endif

main.o: licence_txt.c

licence_txt.c: ../COPYING
	sed -e 's///g' -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^\(.*\)$$/"\1\\n"/' <$< >$@

install:: installdirs
	$(INSTALL) -m 755 $(PROGRAM) $(bindir)

installdirs:
	test -d $(bindir) || $(INSTALL) -d $(bindir)

############################################################

SUBDIRS=	ui

install::
	@for d in $(SUBDIRS); do \
	    (cd $$d ; $(MAKE) $@ ) ;\
	done
	

DISTFILES=	Makefile \
		$(SOURCE.c) $(SOURCE.h)

dist:
	for file in $(DISTFILES); do \
	  ln $$file $(distdir) 2>/dev/null || cp -p $$file $(distdir) 2>/dev/null; \
	done
	for subdir in $(SUBDIRS); do \
	  mkdir $(distdir)/$$subdir || exit 1; \
	  chmod 777 $(distdir)/$$subdir; \
	  $(MAKE) -C $$subdir distdir=../$(distdir)/$$subdir $@ || exit 1; \
	done
	
############################################################

depend:
	makedepend -Y $(CPPFLAGS) $(SOURCE.c)

############################################################
# DO NOT DELETE


banner.o: ../libcml/libcml.h common.h node_gui.h 
brokenwin.o: ../libcml/libcml.h common.h 
choice.o: ../libcml/libcml.h common.h node_gui.h 
debug.o: ../libcml/libcml.h common.h node_gui.h 
help.o: ../libcml/libcml.h common.h node_gui.h 
integer.o: ../libcml/libcml.h common.h node_gui.h 
limited_integer.o: ../libcml/libcml.h common.h node_gui.h
logwin.o: ../libcml/libcml.h common.h 
main.o: ../libcml/libcml.h common.h licence_txt.c node_gui.h page_gui.h
menu.o: ../libcml/libcml.h common.h node_gui.h 
node_gui.o: ../libcml/libcml.h common.h node_gui.h page_gui.h ui/help.xpm
node_gui.o: ui/right_arrow_clip.xpm 
page_gui.o: ../libcml/libcml.h common.h node_gui.h page_gui.h
string.o: ../libcml/libcml.h common.h node_gui.h 
tristate.o: ../libcml/libcml.h common.h node_gui.h 
unknown.o: ../libcml/libcml.h common.h node_gui.h 
