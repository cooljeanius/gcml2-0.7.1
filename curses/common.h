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
 
#ifndef _common_h_
#define _common_h_

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <libintl.h>
#include <assert.h>
#include <stdarg.h>
#include "strvec.h"

#ifndef DEBUG
#define DEBUG 0
#endif

#define CVSID(s) \
    static const char *__cvsid[2] = { (s), (const char *)__cvsid }

#ifndef _
#define _(x)	gettext(x)
#endif


#endif /* _common_h_ */
