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

ACLOCAL_AMFLAGS = -I m4 --install

TOPDIR=	.
include variables.mk.am

SUBDIRS=	libcml glass gtk curses check

all-local depend install-local clean-local::
	for d in $(SUBDIRS); do \
	  if test -d $${d} && test -r $${d}/Makefile; then \
	    (pushd $${d}; $(MAKE) $@ || stat Makefile; popd); \
	  else \
	    echo "cannot make $@ in $${d}"; \
	  fi; \
	done

#
# Build an executable suitable for profiling
#
profile:
	$(MAKE) clean clean-local
	$(MAKE) SUBDIRS="libcml gtk" PROFILE=1 all
.PHONY: profile
	
#
# Build an executable suitable for coverage testing
#
coverage:
	$(MAKE) clean clean-local
	$(MAKE) SUBDIRS="libcml gtk" COVERAGE=1 all
.PHONY: coverage

############################################################

my_distdir=	$(PACKAGE)-$(VERSION)
DISTTAR=	$(PACKAGE)-$(VERSION).tar.gz
MY_DISTFILES=	README Makefile variables.mk COPYING ChangeLog TODO \
		gcml2.spec.in
DISTCLEANFILES= *~ autoscan.log build-aux/*~

dist-hook:
	$(RM) -r $(distdir)
	mkdir $(distdir)
	chmod 777 $(distdir)
	for file in $(DISTFILES); do \
	  ln $${file} $(distdir) 2>/dev/null || cp -p $${file} $(distdir) 2>/dev/null; \
	done
	for subdir in $(SUBDIRS); do \
	  $(MKDIR_P) $(distdir)/$${subdir} || exit 1; \
	  chmod 777 $(distdir)/$${subdir}; \
	  $(MAKE) -C $${subdir} distdir=../$(distdir)/$${subdir} $@ || exit 1; \
	done
	tar chozf $(DISTTAR) $(distdir)
	$(RM) -r $(distdir)
.PHONY: dist

############################################################

HERE:= 		$(shell /bin/pwd)
RPMDIR:= 	$(HERE)/rpm.d
RPMSUBDIRS=	BUILD SRPMS RPMS/i386

rpm: dist-hook
	@for d in $(RPMSUBDIRS); do \
	    [ -d $(RPMDIR)/$$d ] || (echo $(MKDIR_P) $(RPMDIR)/$${d}; $(MKDIR_P) $(RPMDIR)/$${d}); \
	done
	$(SED) \
		-e 's|@PACKAGE@|$(PACKAGE)|g' \
		-e 's|@VERSION@|$(VERSION)|g' \
		< gcml2.spec.in > gcml2.spec
	$(RPM) -bb -v \
		--define '_topdir $(RPMDIR)' \
		--define '_sourcedir $(HERE)' \
		gcml2.spec
	mv -f $(RPMDIR)/RPMS/*/*.rpm .
.PHONY: rpm

clean-local::
	$(RM) -r $(RPMDIR) || rmdir $(RPMDIR)
.PHONY: clean-local

distclean-local::
	$(RM) -r autom4te.cache || rmdir autom4te.cache
.PHONY: distclean-local
