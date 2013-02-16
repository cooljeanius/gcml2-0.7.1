
TOPDIR=	../..
include $(TOPDIR)/variables.mk


all install clean depend::

############################################################

DELIVERABLES= \
		gcml2.glade \
		stock_first.xpm stock_last.xpm help.xpm \
		stock_left_arrow.xpm stock_revert.xpm stock_right_arrow.xpm \
		stock_save.xpm stock_search.xpm stock_top.xpm \
		stock_up_arrow.xpm right_arrow_clip.xpm gcml2-small.xpm
DISTFILES=	Makefile $(DELIVERABLES)

dist:
	for file in $(DISTFILES); do \
	  ln $$file $(distdir) 2>/dev/null || cp -p $$file $(distdir) 2>/dev/null; \
	done
	
install:: installdirs
	@for file in $(DELIVERABLES) ; do \
	  echo $(INSTALL) -m 644 $$file $(pkgdatadir); \
	  $(INSTALL) -m 644 $$file $(pkgdatadir); \
	done


installdirs:
	test -d $(pkgdatadir) || $(INSTALL) -d $(pkgdatadir)

############################################################
