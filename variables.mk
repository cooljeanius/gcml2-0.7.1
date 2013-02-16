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
 

PACKAGE=	gcml2
VERSION=	0.7.1

prefix=		/usr/local
exec_prefix=	$(prefix)
bindir=		$(exec_prefix)/bin
datadir=	$(prefix)/share
pkgdatadir=	$(datadir)/$(PACKAGE)
mandir= 	$(prefix)/man
INSTALL=	install

BASH:=		$(firstword $(wildcard $(BASH) /bin/bash /opt/fw/bin/bash /usr/freeware/bin/bash ))
ifeq ($(GLIB_CONFIG),)
    GLIB_CONFIG=glib-config
endif

CC=		gcc -Wall
CFLAGS=		
CPPFLAGS=	$(shell $(GLIB_CONFIG) --cflags glib)
LDLIBS=		$(shell $(GLIB_CONFIG) --libs glib)
# CPPFLAGS=	$(shell libglade-config --cflags)
# LDLIBS=		$(shell libglade-config --libs)

CPPFLAGS+=	-DVERSION=\"$(VERSION)\" \
		-DPACKAGE=\"$(PACKAGE)\" \
		-DPKGDATADIR=\"$(pkgdatadir)\"
ifneq ($(DEBUG),)
    CFLAGS += -g
    CPPFLAGS += -DDEBUG=$(DEBUG) -DTESTSCRIPT=1
endif

ifneq ($(SYLVESTER),)
    CPPFLAGS += -DSYLVESTER=$(SYLVESTER)
endif

ifneq ($(PROFILE),)
    CFLAGS += -pg
    CPPFLAGS += -DPROFILE=1
endif

ifneq ($(COVERAGE),)
    CFLAGS += -fprofile-arcs -ftest-coverage
    CPPFLAGS += -DCOVERAGE=1 -DTESTSCRIPT=1
endif

# Default target
all::

clean:: covclean

covclean:
	$(RM) *.da *.bb *.bbg
	
