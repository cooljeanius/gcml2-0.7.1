#include <stdio.h>
#include <errno.h>
#include <ctype.h>

static void yyerror(const char *fmt, ...);

#include "cml1to2.h"

static cml_location yylocation;

static void
statement_start(void)
{
}


#include "parser.tab.h"

static YYSTYPE yylval;

#include "lexer.c"
#include "lextokens.c"

static void
yyerror(const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    fprintf(stderr, "%s:%d:error:", yylocation.filename, yylocation.lineno);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

const char *
describe_yylval(int tok)
{
    switch (tok)
    {
    case PROMPT:
    case WORD:
    case SYMBOL:
    case SYMBOLREF:
    case DECIMAL:
    case HEXADECIMAL:
    case TRISTATE:
    	return yylval.string;
    }
    return "";
}

int
main(int argc, char **argv)
{
    char *filename = argv[1];
    int tok;
    
    if (!yylex_push_file(filename))
    	return 1;
	
    while ((tok = yylex()) != 0)
    {
    	fprintf(stderr, "%s:%d:%s:%s\n",
	    yylocation.filename,
	    yylocation.lineno,
	    describe_token(tok),
	    describe_yylval(tok));
    }
    
    return 0;
}
