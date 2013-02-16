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
 
#include "common.h"
#include "base64.h"
#include <ctype.h>

CVSID("$Id: base64.c,v 1.4 2001/04/17 12:57:55 gnb Exp $");

/*============================================================*/

static unsigned char *data = 0;
static unsigned long length = 0;
static unsigned long allocated = 0;
#define INCREMENT 256
static unsigned long decode_buf;
static unsigned int decode_nsyms, decode_padsyms;

static void
decode_init(void)
{
    data = 0;
    length = 0;
    allocated = 0;
    decode_nsyms = 0;
    decode_padsyms = 0;
    decode_buf = 0UL;
}

void
b64_decode_begin(void)
{
    decode_init();
}

static void
add_output_byte(unsigned char b)
{
    if (length+1 > allocated)
    {
    	unsigned char *old = data;
    	allocated += INCREMENT;
	data = g_malloc(allocated);
	if (old != 0)
	{
	    memcpy(data, old, length);
	    g_free(old);
	}
    }
    data[length++] = b;
}

static gboolean
decode_char(char c)
{
    unsigned char d6 = 0;   	/* decoded 6 bit quantity */
    static unsigned int shifts[4] = { 18, 12, 6, 0 };
    
    if (c >= 'A' && c <= 'Z')
    	d6 = c - 'A';
    else if (c >= 'a' && c <= 'z')
    	d6 = c - 'a' + 26;
    else if (c >= '0' && c <= '9')
    	d6 = c - '0' + 52;
    else if (c == '+')
    	d6 = 62;
    else if (c == '/')
    	d6 = 63;
    else if (c == '=')
    	decode_padsyms++;
    else if (isspace(c))
    	return TRUE;	    /* ignore whitespace */
    else
    	return FALSE;	    /* illegal character */
	
    decode_buf |= (d6 << shifts[decode_nsyms]);
    if (++decode_nsyms == 4)
    {
    	add_output_byte((decode_buf >> 16) & 0xff);
	if (decode_padsyms < 2)
    	    add_output_byte((decode_buf >> 8) & 0xff);
	if (decode_padsyms < 1)
    	    add_output_byte(decode_buf & 0xff);
    	decode_buf = 0;
    	decode_nsyms = 0;
	decode_padsyms = 0;
    }
	
    return TRUE;
}

/*============================================================*/

gboolean
b64_decode_input(const char *s)
{
    for ( ; *s ; s++)
    	if (!decode_char(*s))
	    return FALSE;
    return TRUE;
}

cml_blob *
b64_decode_take_data_as_blob(void)
{
    cml_blob *blob = blob_new(data, length);
    decode_init();
    return blob;
}

void
b64_decode_end(void)
{
    if (data != 0)
    	g_free(data);
    decode_init();
}

/*============================================================*/
/*END*/
