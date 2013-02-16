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
# Syntax checking frontend

PROGRAM=	cml-check
SOURCE.c=	main.c
OBJECTS=	$(SOURCE.c:.c=.o)
CPPFLAGS+=	-I../libcml -I/opt/local/include
LDLIBS:=	-L../libcml -lcml $(LDLIBS)

all:: $(PROGRAM)

$(PROGRAM): $(OBJECTS) ../libcml/libcml.a
	pwd
	$(LINK.c) -o $@ $(OBJECTS) $(LDLIBS)

clean::
	$(RM) $(PROGRAM) $(OBJECTS)
	
install:: installdirs

install::
	$(INSTALL) -m 755 $(PROGRAM) $(bindir)

installdirs::
	test -d $(bindir) || $(INSTALL) -d $(bindir)

############################################################
# Manpages

MANPAGES_1=	cml-check cml-check-all cml-summarize
MANPAGES_DIST=	cml-check.1.in warnings.html errors.html \
		cml-check-all.1 cml-summarize.1 html2man.sed

all:: $(addsuffix .1,$(MANPAGES_1))

clean::
	$(RM) cml-check.1

install::
	$(INSTALL) $(addsuffix .1,$(MANPAGES_1)) $(mandir)/man1

installdirs::
	test -d $(mandir)/man1 || $(INSTALL) -d $(mandir)/man1
	
cml-check.1:	cml-check.1.in warnings.man errors.man
	sed \
		-e '/@INSERT_WARNINGS_MAN@/rwarnings.man' \
		-e '/@INSERT_ERRORS_MAN@/rerrors.man' \
		<$< >$@
	
warnings.man: warnings.html html2man.sed
	sed -f html2man.sed <$< >$@

errors.man: errors.html html2man.sed
	sed -f html2man.sed <$< >$@

############################################################
# Summarising shellscript

SHELLSCRIPTS=		cml-summarize cml-check-all
SHELLSCRIPTS_DIST=	$(addsuffix .sh,$(SHELLSCRIPTS)) \
			summarize.awk

all:: $(SHELLSCRIPTS)

%: %.sh
	sed \
		-e 's|@BASH@|$(BASH)|g' \
		-e 's|@VERSION@|$(VERSION)|g' \
		-e 's|@pkgdatadir@|$(pkgdatadir)|g' \
		-e 's|@DEBUG@|$(DEBUG)|g' \
		-e 's|@PROFILE@|$(PROFILE)|g' \
		<$< >$@
	chmod +x $@

clean::
	$(RM) $(SHELLSCRIPTS)

install::
	$(INSTALL) -m 755 $(SHELLSCRIPTS) $(bindir)
	$(INSTALL) summarize.awk $(pkgdatadir)

installdirs::
	test -d $(pkgdatadir) || $(INSTALL) -d $(pkgdatadir)

############################################################

DISTFILES=	Makefile \
		$(SOURCE.c) \
		$(MANPAGES_DIST) \
		$(SHELLSCRIPTS_DIST)


dist:
	for file in $(DISTFILES); do \
	  ln $$file $(distdir) 2>/dev/null || cp -p $$file $(distdir) 2>/dev/null; \
	done
	
############################################################

depend:
	makedepend -Y $(CPPFLAGS) $(SOURCE.c)

# DO NOT DELETE

main.o: ../libcml/libcml.h ../libcml/debug.h ../libcml/common.h
