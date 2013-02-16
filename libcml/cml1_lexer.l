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
 

#include <ctype.h>
#include "debug.h"
#define YY_NO_UNPUT 1	/* prevent compiler warning from unused fn */

CVSID2("$Id: cml1_lexer.l,v 1.5 2002/09/01 08:15:11 gnb Exp $");

/*============================================================*/

#if DEBUG
#define lineno_debug() \
    if (debug & DEBUG_LINENO) \
    	fprintf(stderr, "#line %s %d\n", yylocation.filename, yylocation.lineno)
#else
#define lineno_debug()
#endif

/* for handling multi-line prompts -- e.g. "choice" statements */
static char *qstring;
static int qstring_len;
/*============================================================*/

typedef struct
{
    cml_location location;  /* filename, lineno */
    FILE *filep;
    YY_BUFFER_STATE buffer;
} source_stack_rec;
/* stack of source_stack_rec, top first */
static GList *source_stack = 0;

static gboolean
yylex_push_file(const char *relfilename)
{
    source_stack_rec *rec;
    char *filename;
    FILE *fp;

    DDPRINTF1(DEBUG_INCLUDE, "pushing file \"%s\"\n", relfilename);
    
    /* record line number of `source' in old file, if any */
    if (source_stack != 0)
    {
	rec = (source_stack_rec *)source_stack->data;
	rec->location.lineno = yylocation.lineno;
    }
    
    filename = g_strdup(relfilename);
    if ((fp = fopen(filename, "r")) == 0)
    {
    	yyerror("can't open sourced file `%s': %s",
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
    lineno_debug();
    
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

    DDPRINTF1(DEBUG_INCLUDE, "popping file \"%s\"\n", rec->location.filename);
    
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
	lineno_debug();
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
/* used to handle multi-line prompts */
%x CON_MLPROMPT

%%

-a 	    	return K_AND;
bool		statement_start(); return K_BOOL;
choice		statement_start(); return K_CHOICE;
comment		statement_start(); return K_COMMENT;
define_bool	statement_start(); return K_DEFINE_BOOL;
define_hex	statement_start(); return K_DEFINE_HEX;
define_int	statement_start(); return K_DEFINE_INT;
define_string	statement_start(); return K_DEFINE_STRING;
define_tristate	statement_start(); return K_DEFINE_TRISTATE;
dep_bool	statement_start(); return K_DEP_BOOL;
dep_hex		statement_start(); return K_DEP_HEX;
dep_int		statement_start(); return K_DEP_INT;
dep_mbool	statement_start(); return K_DEP_MBOOL;
dep_string	statement_start(); return K_DEP_STRING;
dep_tristate	statement_start(); return K_DEP_TRISTATE;
else		return K_ELSE;
endmenu		return K_ENDMENU;
fi		return K_FI;
hex		statement_start(); return K_HEX;
if		return K_IF;
int		statement_start(); return K_INT;
mainmenu_name	statement_start(); return K_MAINMENU_NAME;
mainmenu_option	statement_start(); return K_MAINMENU_OPTION;
next_comment	return K_NEXT_COMMENT;
null	    	return K_NULL;
-o  	    	return K_OR;
=  	    	return K_EQUALS;
!  	    	return K_NOT;
!=  	    	return K_NOT_EQUALS;
source	    	BEGIN(CON_SOURCE);
string		statement_start(); return K_STRING;
then		return K_THEN;
tristate	statement_start(); return K_TRISTATE;
unset		statement_start(); return K_UNSET;

@assert    	statement_start(); return K_TEST_ASSERT;
@clear     	statement_start(); return K_TEST_CLEAR;
@commit    	statement_start(); return K_TEST_COMMIT;
@freeze    	statement_start(); return K_TEST_FREEZE;
@error     	statement_start(); return K_TEST_ERROR;
@noerror   	statement_start(); return K_TEST_NOERROR;
@parsetest 	return K_TEST_PARSETEST;
@set	    	statement_start(); return K_TEST_SET;
@succeeded 	statement_start(); return K_TEST_SUCCEEDED;
@visible   	statement_start(); return K_TEST_VISIBLE;
@saveable  	statement_start(); return K_TEST_SAVEABLE;

[ymn]	    	{
		    yylval.string = g_strdup(yytext);
		    return TRISTATE;
		}

\"[ymn]\"   	{
    	    	    yytext[2] = '\0';
		    yylval.string = g_strdup(yytext+1);
		    return TRISTATE;
		}

CONFIG_[A-Za-z0-9_]+ {
		    yylval.string = g_strdup(yytext);
		    return SYMBOL;
    	    	}

$CONFIG_[A-Za-z0-9_]+ {
		    yylval.string = g_strdup(yytext+1);
		    return SYMBOLREF;
    	    	}

\"$CONFIG_[A-Za-z0-9_]+\" {
    	    	    yytext[yyleng-1] = '\0'; /* lose trailing quote */
		    yylval.string = g_strdup(yytext+2);
		    return SYMBOLREF;
    	    	}

\"$ARCH\"   	{
    	    	    /* The undocumented $ARCH symbol is used in some rules */
    	    	    yytext[yyleng-1] = '\0'; /* lose trailing quote */
		    yylval.string = g_strdup(yytext+2);
		    if (rb_warning_enabled(rb, CW_SYMBOL_ARCH))
			cml_warningl(&yylocation, "undocumented symbol $ARCH used");
		    return SYMBOLREF;
    	    	}

<CON_SOURCE>[^ \t\n\r]+ {
    	    	    /*
		     * Unquoted string immediately after `source' is filename
		     * to be lexically included.
		     */
    	    	    yylex_push_file(yytext);
		    BEGIN(0);
    	    	}
		
\"[^\"\n\r]*\\\n  	{
    	    	    /* start of a multi-line prompt */
		    /*
		     * TODO: figure out if we actually need the qstring
		     *       variable or whether we can rely on yylval
		     *       across lex rules which do not return.
		     */
		    yytext[yyleng-2] = '\0';	/* lose backslash & newline */
    	    	    qstring = g_strdup(yytext+1);
		    qstring_len = yyleng-3;
		    ++yylocation.lineno;
		    lineno_debug();
		    BEGIN(CON_MLPROMPT);
    	    	}

<CON_MLPROMPT>[^\"\n\r]*\\\n  	{
    	    	    /* continuation of a multi-line prompt */
		    /* Note: for some reason the corpus as of 2.5.20 comprised
		     *       51 double-quoted multiline prompts and no single
		     *       quoted ones;  I have taken this as indicative
		     *       of a hidden language rule and only implemented
		     *       double-quoted multiline prompts.
		     */
		    yyleng -= 2;
		    yytext[yyleng] = '\0';	/* lose backslash & newline */
		    qstring = g_realloc(qstring, qstring_len+yyleng+1);
		    strcpy(qstring+qstring_len, yytext);
		    qstring_len += yyleng;
		    ++yylocation.lineno;
		    lineno_debug();
    	    	}

<CON_MLPROMPT>[^\"\n\r]*\"  	{
    	    	    /* end of a multi-line prompt */
		    yyleng--;
    	    	    yytext[yyleng] = '\0';	/* lose trailing quote */
		    qstring = g_realloc(qstring, qstring_len+yyleng+1);
		    strcpy(qstring+qstring_len, yytext);
		    yylval.string = qstring;
		    BEGIN(0);
		    return PROMPT;
    	    	}

\'[^\'\n\r]*\'|\"[^\"\n\r]*\"  	{
    	    	    /* single-line prompt, double-quoted or single-quoted */
		    if (!strncmp(yytext, "\"CONFIG_", 8) &&
		    	rb_warning_enabled(rb, CW_SYMBOL_LIKE_LITERAL))
		    	cml_warningl(&yylocation,
			    	"suspiciously symbol-like string literal %s",
				yytext);
    	    	    yytext[yyleng-1] = '\0';	/* lose trailing quote */
    	    	    yylval.string = g_strdup(yytext+1);
		    return PROMPT;
    	    	}
		
[0-9]+	    	{
    	    	    yylval.string = g_strdup(yytext);
		    return DECIMAL;
    	    	}
		
0x[A-Fa-f0-9]+	{
    	    	    yylval.string = g_strdup(yytext);
		    return HEXADECIMAL;
		}

#[^\n]*   	{
    	    	    /* eat comments (matches to end of line or file) */
    	    	}

[^ \t\n\r\]\[;]+ {
    	    	    yylval.string = g_strdup(yytext);
		    return WORD;
    	    	}
		
[ \t]+	    	/* eat whitespace */


\\\r\n	    	{
    	    	    /* escaped newline */
    	    	    ++yylocation.lineno;
		    lineno_debug();
    	    	}

\\\n	    	{
    	    	    /* escaped newline */
    	    	    ++yylocation.lineno;
		    lineno_debug();
    	    	}

\r\n	    	{
    	    	    ++yylocation.lineno;
		    lineno_debug();
		    return EOL;
    	    	}

\n  	    	{
    	    	    ++yylocation.lineno;
		    lineno_debug();
		    return EOL;
		}

.   	    	{
    	    	    return yytext[0];
		}

%%
