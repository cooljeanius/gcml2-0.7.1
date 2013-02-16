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
# Build cml2lib library

LIBRARY=	libcml.a
SOURCE.c=	node.c atom.c expr.c rule.c rulebase.c save.c load.c \
		base64.c blob.c range.c util.c message.c \
		transactions.c postparse.c cml1pass2.c dnf.c debug.c
SOURCE.y=	cml2_parser.y cml1_parser.y
SOURCE.l=	cml2_lexer.l cml1_lexer.l
PUBHEADERS=	libcml.h  
PRIHEADERS=	private.h util.h base64.h common.h cml1.h debug.h dnf.h
OBJECTS=	$(SOURCE.c:.c=.o) $(SOURCE.y:.y=.o)

all:: $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	pwd
	ar rv $@ $(OBJECTS)

# Force dependancy on included lexer code
cml1_parser.o: cml1_lexer.c
cml2_parser.o: cml2_lexer.c

clean::
	$(RM) $(LIBRARY) $(OBJECTS) $(SOURCE.y:.y=.c) $(SOURCE.l:.l=.c)

installdirs install:
	# Nothing to install here

ifdef OVERLAP_DNF
CPPFLAGS += -DOVERLAP_DNF=$(OVERLAP_DNF)
endif

############################################################

test:: rangetest
	
rangetest: range.c
	$(LINK.c) -DRANGE_TEST=1 -o $@ $< $(LDLIBS)

test::
	./rangetest < rangetest.inp | diff -u rangetest.exp -

clean::
	$(RM) rangetest
	
############################################################
# Bison & Flex support

YYPREFIX_cml1=	cml1_yy
YYPREFIX_cml2=	cml2_yy
_empty=
_space=$(_empty) $(_empty)
YYPREFIX=	$(YYPREFIX_$(firstword $(subst _,$(_space),$@)))

BISON=	bison $(BISONFLAGS) $(addprefix -p ,$(YYPREFIX))
ifneq ($(DEBUG),)
    BISONFLAGS += -t
endif

%.c: %.y
	$(BISON)  -o $@ $<
	

FLEX=		flex $(FLEXFLAGS) $(addprefix -P,$(YYPREFIX))
ifneq ($(DEBUG),)
    FLEXFLAGS += -d
endif

%.c: %.l
	$(FLEX) -o$@ $<


############################################################

cml1_lextest: cml1_lextest.o $(LIBRARY)
	$(LINK.c) -o $@ cml1_lextest.o $(LIBRARY) $(LDLIBS)
	
cml1_lextest.o: cml1_lexer.c cml1_lextokens.c

cml2_lextest: cml2_lextest.o $(LIBRARY)
	$(LINK.c) -o $@ cml2_lextest.o $(LIBRARY) $(LDLIBS)
	
cml2_lextest.o: cml2_lexer.c cml2_lextokens.c

%.tab.h: %.y
	$(BISON) -d $<
	
%lextokens.c: %parser.tab.h tab2tokens.sh
	./tab2tokens.sh <$< >$@
	
clean::
	$(RM) $(SOURCE.y:.y=.tab.c) $(SOURCE.y:.y=.tab.h)
	$(RM) cml1_lextest.o cml1_lextest cml1_lextokens.c
	$(RM) cml2_lextest.o cml2_lextest cml2_lextokens.c

############################################################

DISTFILES=	Makefile \
		$(SOURCE.c) $(SOURCE.y) $(SOURCE.l) $(PUBHEADERS) $(PRIHEADERS) \
		cml1_lextest.c cml2_lextest.c \
		rangetest.exp rangetest.inp

dist:
	for file in $(DISTFILES); do \
	  ln $$file $(distdir) 2>/dev/null || cp -p $$file $(distdir) 2>/dev/null; \
	done
	
############################################################

depend:
	makedepend -Y $(CPPFLAGS) $(SOURCE.c) $(SOURCE.y)

############################################################
# DO NOT DELETE

node.o: private.h libcml.h common.h debug.h
atom.o: private.h libcml.h common.h
expr.o: private.h libcml.h common.h
rule.o: private.h libcml.h common.h debug.h
rulebase.o: private.h libcml.h common.h util.h debug.h
save.o: private.h libcml.h common.h util.h
load.o: private.h libcml.h common.h util.h
base64.o: common.h base64.h private.h libcml.h
blob.o: common.h private.h libcml.h
range.o: private.h libcml.h common.h
util.o: util.h common.h
message.o: common.h private.h libcml.h
transactions.o: private.h libcml.h common.h util.h debug.h
postparse.o: private.h libcml.h common.h
cml1pass2.o: cml1.h private.h libcml.h common.h debug.h
debug.o: debug.h common.h
cml2_parser.o: private.h libcml.h common.h debug.h cml2_lexer.c base64.h
cml1_parser.o: cml1.h private.h libcml.h common.h debug.h cml1_lexer.c
