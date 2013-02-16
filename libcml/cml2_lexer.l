%{

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
 

#include "base64.h"
#include <ctype.h>
#define YY_NO_UNPUT 1	/* prevent compiler warning from unused fn */

CVSID2("$Id: cml2_lexer.l,v 1.2 2002/09/01 08:40:06 gnb Exp $");

/*============================================================*/

static char *textdata;

/*============================================================*/

typedef struct
{
    cml_location location;  /* filename, lineno */
    FILE *filep;
    YY_BUFFER_STATE buffer;
} source_stack_rec;
/* stack of source_stack_rec, top first */
static GList *source_stack = 0;

/*
 * Implements the includer-relative file search semantics.
 * Returns a new filename (which has to be g_free()d) relative
 * to the original includer (i.e which can be directly fopen()ed).
 */
static char *
find_file(const char *relfilename)
{
    char *incdir = 0;
    char *filename;
    
    if (g_path_is_absolute(relfilename))
    	return g_strdup(relfilename);
	
    if (source_stack != 0)
    {
	const char *includer = 0;
	includer = ((source_stack_rec *)source_stack->data)->location.filename;
	incdir = g_dirname(includer);
	if (!strcmp(incdir, "."))
	{
	    g_free(incdir);
	    incdir = 0;
	}
    }
    filename = (incdir == 0 ?
    	    	    g_strdup(relfilename) :
		    g_strdup_printf("%s/%s", incdir, relfilename));

    DDPRINTF2(DEBUG_LEXER, "\"%s\" -> \"%s\"\n",
	    relfilename,
	    filename);

    if (incdir != 0)
	g_free(incdir);  
    return filename;
}


static gboolean
yylex_push_file(const char *relfilename)
{
    source_stack_rec *rec;
    char *filename;
    FILE *fp;
    
    DDPRINTF1(DEBUG_LEXER, "pushing file `%s'\n", relfilename);
    
    /* record line number of `source' in old file, if any */
    if (source_stack != 0)
    {
	rec = (source_stack_rec *)source_stack->data;
	rec->location.lineno = yylocation.lineno;
    }
    
    filename = find_file(relfilename);
    if ((fp = fopen(filename, "r")) == 0)
    {
    	yyerror("Can't open sourced file `%s': %s",
	    filename,
	    strerror(errno));
	g_free(filename);
    	return FALSE;
    }
    
    /* create new record */
    rec = g_new(source_stack_rec, 1);
    rec->location.filename = filename;
    rec->location.lineno = 1;
    rec->filep = fp;
    rec->buffer = yy_create_buffer(rec->filep, YY_BUF_SIZE);

    /* push new record */    
    source_stack = g_list_prepend(source_stack, rec);
    yylocation = rec->location;
    rb->filenames = g_list_prepend(rb->filenames, filename);
    yy_switch_to_buffer(rec->buffer);
    
    return TRUE;
}

static int
yywrap(void)
{
    source_stack_rec *rec;

    /* pop off old top of stack */    
    assert(source_stack != 0);
    rec = (source_stack_rec *)source_stack->data;
    source_stack = g_list_remove_link(source_stack, source_stack);

    DDPRINTF1(DEBUG_LEXER, "popping file `%s'\n", rec->location.filename);
    
    /* delete old record and all forms of buffering */
    yy_delete_buffer(rec->buffer);
    fclose(rec->filep);
    /* don't free rec->filename yet, it's saved for rule & node locations */
    g_free(rec);
    
    /* revert to previous top if it exists */
    if (source_stack != 0)
    {
	rec = (source_stack_rec *)source_stack->data;
	yy_switch_to_buffer(rec->buffer);
	yylocation = rec->location;
    	return FALSE;	    /* outer file to continue reading */
    }
    return TRUE;
}

/*============================================================*/

%}

/*
 * Start conditions
 */
/* used to handle `source' for lexical inclusion */
%x CON_SOURCE
/* used to handle base64 data for `icon' statement */
%x CON_BASE64
/* used to handle text data for `text' statement */
%x CON_TEXT

%%

and 	    return K_AND;
banner	    return K_BANNER;
choices     return K_CHOICES;
condition   statement_start(); return K_CONDITION;
debug	    return K_DEBUG;
default     return K_DEFAULT;
dependent   return K_DEPENDENT;
derive	    statement_start(); return K_DERIVE;
enum	    return K_ENUM;
explanation return K_EXPLANATION;
explanations return K_EXPLANATIONS;
==  	    return K_EQUALS;
from	    return K_FROM;
>=  	    return K_GREATER_EQUALS;
icon	    BEGIN(CON_BASE64); return K_ICON;
implies     return K_IMPLIES;
"<="  	    return K_LESS_EQUALS;
menu	    return K_MENU;
menus	    return K_MENUS;
nohelp	    return K_NOHELP;
not 	    return K_NOT;
!=  	    return K_NOT_EQUALS;
on  	    return K_ON;
or  	    return K_OR;
prefix	    return K_PREFIX;
prohibit    statement_start(); return K_PROHIBIT;
range	    return K_RANGE;
require     statement_start(); return K_REQUIRE;
save	    return K_SAVE;
show	    return K_SHOW;
source	    BEGIN(CON_SOURCE);
start	    statement_start(); return K_START;
suppress    return K_SUPPRESS;
symbols     return K_SYMBOLS;
text	    BEGIN(CON_TEXT); return K_TEXT;
trits	    return K_TRITS;
unless	    return K_UNLESS;
warndepend  return K_WARNDEPEND;
when	    return K_WHEN;

@assert     statement_start(); return K_TEST_ASSERT;
@clear      statement_start(); return K_TEST_CLEAR;
@commit     statement_start(); return K_TEST_COMMIT;
@freeze     statement_start(); return K_TEST_FREEZE;
@error	    statement_start(); return K_TEST_ERROR;
@noerror    statement_start(); return K_TEST_NOERROR;
@parsetest  return K_TEST_PARSETEST;
@set	    statement_start(); return K_TEST_SET;
@succeeded  statement_start(); return K_TEST_SUCCEEDED;
@visible    statement_start(); return K_TEST_VISIBLE;
@saveable   statement_start(); return K_TEST_SAVEABLE;

<CON_BASE64>[A-Za-z0-9+/][A-Za-z0-9+/=]* {
    	    	    	/* base64 data */
			b64_decode_input(yytext);
    	    	}
		
<CON_TEXT>^\.\n {
    	    	    /* single period terminates text data */
		    yylval.string = textdata;
		    textdata = 0;
		    ++yylocation.lineno;
		    BEGIN(0);
		    return STRING;
    	    	}

<CON_TEXT>^.* {
    	    	    /* text data */
		    char *old = textdata;
		    if (old == 0)
			textdata = g_strdup(yytext);
		    else
		    {
			textdata = g_strconcat(old, "\n", yytext, 0);
			g_free(old);
		    }
    	    	}
		
<CON_TEXT>\n	{   /* swallow newlines, they get put in again later */
		    ++yylocation.lineno;
    	    	}

y   	    	{
		    yylval.tritval = CML_Y;
		    return TRITVAL;
		}

m   	    	{
		    yylval.tritval = CML_M;
		    return TRITVAL;
		}

n	    	{
		    yylval.tritval = CML_N;
		    return TRITVAL;
		}

[A-Za-z][A-Za-z0-9_]* {
    	    	    cml_node *mn;
		    char *name;
		    
		    name = yytext;
		    if (rb->prefix != 0 &&
		    	!strncmp(name, rb->prefix, strlen(rb->prefix)))
			name += strlen(rb->prefix);
		    
		    mn = cml_rulebase_find_node(rb, name);

		    if (mn == 0)
		    {
			mn = rb_add_node(rb, name);
			mn->location = yylocation;
		    }
		    yylval.node = mn;
		    return SYMBOL;
    	    	}

<CON_SOURCE>\'[^\']*\'|\"[^\"]*\" {
    	    	    /*
		     * Quoted string immediately after `source' is filename
		     * to be lexically included.
		     */
    	    	    yytext[yyleng-1] = '\0';	/* lose trailing quote */
    	    	    if (!yylex_push_file(yytext+1))
		    	yyterminate();
		    BEGIN(0);
    	    	}
		
<CON_SOURCE>[^ \t\n\r]+ {
    	    	    /*
		     * Unquoted string immediately after `source' is also
		     * filename to be lexically included.  Ahh, consistency.
		     */
    	    	    if (!yylex_push_file(yytext))
		    	yyterminate();
		    BEGIN(0);
    	    	}
		
\'[^\']*\'|\"[^\"]*\"  	{
    	    	    yytext[yyleng-1] = '\0';	/* lose trailing quote */
    	    	    yylval.string = g_strdup(yytext+1);
		    return STRING;
    	    	}
		
[0-9]+	    	{
    	    	    yylval.integer = atoi(yytext);
		    return DECIMAL;
    	    	}
		
0x[A-Fa-f0-9]+	{
    	    	    yylval.integer = strtol(yytext, (char **)0, 16);
		    return HEXADECIMAL;
		}


[ \t]+	    	/* eat whitespace */


<CON_BASE64>#.*\n {
    	    	    /* comment terminates base64 data */
		    yylval.blob = b64_decode_take_data_as_blob();
		    b64_decode_end();
		    ++yylocation.lineno;
		    BEGIN(0);
		    return BINARYDATA;
    	    	}

#[^\n]*   	{
    	    	    /* eat comments (matches to end of line or file) */
    	    	}

<CON_BASE64>^\n	{
    	    	    /* empty line terminates base64 data */
		    yylval.blob = b64_decode_take_data_as_blob();
		    b64_decode_end();
    	    	    ++yylocation.lineno;
		    BEGIN(0);
		    return BINARYDATA;
		}

\n  	    	{
    	    	    ++yylocation.lineno;
		}

.   	    	{
    	    	    return yytext[0];
		}

%%
