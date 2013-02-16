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
#include "private.h"

CVSID("$Id: blob.c,v 1.5 2001/04/23 06:44:43 gnb Exp $");

/*============================================================*/

cml_blob *
blob_new(unsigned char *data, unsigned long length)
{
    cml_blob *blob = g_new(cml_blob, 1);

    if (blob == 0)
    	return 0;
    blob->data = data;
    blob->length = length;

    return blob;
}

cml_blob *
blob_new_copy(unsigned char *data, unsigned long length)
{
    cml_blob *blob = g_new(cml_blob, 1);

    if (blob == 0)
    	return 0;
    if (data != 0 && length > 0)
    {
    	blob->data = g_malloc(length);
	memcpy(blob->data, data, length);
    }
    else
	blob->data = 0;
    blob->length = length;

    return blob;
}

void
blob_delete(cml_blob *blob)
{
    if (blob->data != 0)
    	g_free(blob->data);
    g_free(blob);
}

/*============================================================*/
/*END*/
