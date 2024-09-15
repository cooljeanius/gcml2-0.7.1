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

############################################################
# Glass-tty frontend

PROGRAM=	cml-glass
SOURCE.c=	main.c
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
	
install:: installdirs
	$(INSTALL) -m 755 $(PROGRAM) $(bindir)

installdirs:
	test -d $(bindir) || $(INSTALL) -d $(bindir)

############################################################
# Clean  up test run products

clean::
	$(RM) config.h config.sh config.mk
	$(RM) -r include

distclean::
	$(RM) $(PROGRAM)
.PHONY: distclean

clean-local:
.PHONY: clean-local

############################################################

DISTFILES=	Makefile \
		$(SOURCE.c)


dist:
	for file in $(DISTFILES); do \
	  ln $$file $(distdir) 2>/dev/null || cp -p $$file $(distdir) 2>/dev/null; \
	done
	
############################################################

depend:
	makedepend -Y $(CPPFLAGS) $(SOURCE.c)

# DO NOT DELETE

main.o: ../libcml/libcml.h ../libcml/debug.h ../libcml/common.h
