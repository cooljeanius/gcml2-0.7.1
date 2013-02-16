/*
 *  gcml2 -- an implementation of Eric Raymond's CML2 in C
 *  Copyright (C) 2000-2001 Greg Banks
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _cml_debug_h_
#define _cml_debug_h_ 1

#include "common.h"

#define DEBUG_LEXER   	(1<<0)
#define DEBUG_PARSER   	(1<<1)
#define DEBUG_RULES   	(1<<2)
#define DEBUG_NODES	(1<<3)
#define DEBUG_SKIPGUI	(1<<4)
#define DEBUG_CONVERT	(1<<5)
#define DEBUG_INCLUDE	(1<<6)
#define DEBUG_GUI	(1<<7)
#define DEBUG_TXN	(1<<8)
#define DEBUG_LINENO	(1<<9)
#define DEBUG_LOAD	(1<<10)
#define DEBUG_DNF	(1<<11)
#define DEBUG_SAVE	(1<<12)

#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG

extern unsigned int debug;

#ifdef __GNUC__
#define DDFUNC_FMT(fmt)	"%s: " fmt, __FUNCTION__
#else
#define DDFUNC_FMT(fmt) fmt
#endif

#define DDPRINTF0(tok, fmt) \
    if (debug & (tok)) fprintf(stderr, DDFUNC_FMT(fmt))
#define DDPRINTF1(tok, fmt, a1) \
    if (debug & (tok)) fprintf(stderr, DDFUNC_FMT(fmt), a1)
#define DDPRINTF2(tok, fmt, a1, a2) \
    if (debug & (tok)) fprintf(stderr, DDFUNC_FMT(fmt), a1, a2)
#define DDPRINTF3(tok, fmt, a1, a2, a3) \
    if (debug & (tok)) fprintf(stderr, DDFUNC_FMT(fmt), a1, a2, a3)

#else

#define DDPRINTF0(tok, fmt)
#define DDPRINTF1(tok, fmt, a1)
#define DDPRINTF2(tok, fmt, a1, a2)
#define DDPRINTF3(tok, fmt, a1, a2, a3)

#endif

extern void debug_set(const char *);

#endif /* _cml_debug_h_ */
