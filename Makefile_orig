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
 
TOPDIR=	.
include variables.mk

SUBDIRS=	libcml glass gtk curses check

all depend install clean::
	for d in $(SUBDIRS); do \
	    (cd $$d ; $(MAKE) $@ ) ;\
	done

#
# Build an executable suitable for profiling
#
profile:
	$(MAKE) clean
	$(MAKE) SUBDIRS="libcml gtk" PROFILE=1 all
	
#
# Build an executable suitable for coverage testing
#
coverage:
	$(MAKE) clean
	$(MAKE) SUBDIRS="libcml gtk" COVERAGE=1 all

############################################################

distdir=	$(PACKAGE)-$(VERSION)
DISTTAR=	$(PACKAGE)-$(VERSION).tar.gz
DISTFILES=	README Makefile variables.mk COPYING ChangeLog TODO \
		gcml2.spec.in
		

dist:
	$(RM) -r $(distdir)
	mkdir $(distdir)
	chmod 777 $(distdir)
	for file in $(DISTFILES); do \
	  ln $$file $(distdir) 2>/dev/null || cp -p $$file $(distdir) 2>/dev/null; \
	done
	for subdir in $(SUBDIRS); do \
	  mkdir -p $(distdir)/$$subdir || exit 1; \
	  chmod 777 $(distdir)/$$subdir; \
	  $(MAKE) -C $$subdir distdir=../$(distdir)/$$subdir $@ || exit 1; \
	done
	tar chozf $(DISTTAR) $(distdir)
	$(RM) -r $(distdir)

############################################################

HERE:= 		$(shell /bin/pwd)
RPMDIR:= 	$(HERE)/rpm.d
RPMSUBDIRS=	BUILD SRPMS RPMS/i386

rpm: dist
	@for d in $(RPMSUBDIRS) ; do \
	    [ -d $(RPMDIR)/$$d ] || (echo mkdir -p $(RPMDIR)/$$d; mkdir -p $(RPMDIR)/$$d); \
	done
	sed \
		-e 's|@PACKAGE@|$(PACKAGE)|g' \
		-e 's|@VERSION@|$(VERSION)|g' \
		< gcml2.spec.in > gcml2.spec
	rpm -bb -v \
		--define '_topdir $(RPMDIR)' \
		--define '_sourcedir $(HERE)' \
		gcml2.spec
	mv -f $(RPMDIR)/RPMS/*/*.rpm .

clean::
	$(RM) -r $(RPMDIR)
