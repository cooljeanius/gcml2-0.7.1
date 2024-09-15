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
# Curses-based frontend

SCRIPTSDIR=	linux-scripts
LXDIALOGDIR=	$(SCRIPTSDIR)/lxdialog
LXDIALOGSRC=	menubox.c checklist.c textbox.c inputbox.c msgbox.c yesno.c util.c
LXDIALOGHDRS=	dialog.h colors.h
vpath %.c $(LXDIALOGDIR)

PROGRAM=	cml-curses
SOURCE.c=	main.c strvec.c $(addprefix $(LXDIALOGDIR)/,$(LXDIALOGSRC))
SOURCE.h=	common.h strvec.h $(addprefix $(LXDIALOGDIR)/,$(LXDIALOGHDRS))
		
OBJECTS=	$(notdir $(SOURCE.c:.c=.o))
CPPFLAGS+=	-I../libcml -I$(LXDIALOGDIR) -DCURSES_LOC="<ncurses.h>" -I/opt/local/include
LDFLAGS+=	-L../libcml
LDLIBS+=	-lcml -lncurses

all:: $(PROGRAM)

$(PROGRAM): $(OBJECTS) ../libcml/libcml.a
	pwd
	$(LINK.c) -o $@ $(OBJECTS) $(LDLIBS)

clean::
	$(RM) $(PROGRAM) $(OBJECTS)
	
# convert README to C for online help
main.o: README.Menuconfig.h

README.Menuconfig.h: $(SCRIPTSDIR)/README.Menuconfig
	sed -e 's|\\|\\\\|g' -e 's|"|\\"|g' -e 's|^\(.*\)$$|"\1\\n"|' <$< >$@
	
clean::
	$(RM) README.Menuconfig.h

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
		$(SOURCE.c) $(SOURCE.h) $(SCRIPTSDIR)/README.Menuconfig


dist:
	for file in $(DISTFILES); do \
	  dir=`dirname $$file`; \
	  test -d $(distdir)/$$dir || mkdir -p $(distdir)/$$dir; \
	  ln $$file $(distdir)/$$dir 2>/dev/null || cp -p $$file $(distdir)/$$dir 2>/dev/null; \
	done
	
############################################################

depend:
	makedepend -Y $(CPPFLAGS) $(SOURCE.c) 

# DO NOT DELETE

main.o: common.h strvec.h linux-scripts/lxdialog/dialog.h ../libcml/libcml.h
main.o: ../libcml/debug.h README.Menuconfig.h
strvec.o: common.h strvec.h
linux-scripts/lxdialog/menubox.o: linux-scripts/lxdialog/dialog.h
linux-scripts/lxdialog/checklist.o: linux-scripts/lxdialog/dialog.h
linux-scripts/lxdialog/textbox.o: linux-scripts/lxdialog/dialog.h
linux-scripts/lxdialog/inputbox.o: linux-scripts/lxdialog/dialog.h
linux-scripts/lxdialog/msgbox.o: linux-scripts/lxdialog/dialog.h
linux-scripts/lxdialog/yesno.o: linux-scripts/lxdialog/dialog.h
linux-scripts/lxdialog/util.o: linux-scripts/lxdialog/dialog.h
linux-scripts/lxdialog/util.o: linux-scripts/lxdialog/colors.h
