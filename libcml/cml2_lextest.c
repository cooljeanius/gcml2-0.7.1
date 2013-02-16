#include <stdio.h>
#include <errno.h>
#include <ctype.h>

static void yyerror(const char *fmt, ...);
static int yywrap(void);

#include "private.h"

static cml_location yylocation;
static cml_rulebase *rb;

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
    static char buf[32];
    static const char *tritstrs[3] = {"n", "y", "m"};
    
    switch (tok)
    {
    case SYMBOL:
    	return yylval.node->name;
    case DECIMAL:
    	sprintf(buf, "%ld", yylval.integer);
    	return buf;
    case HEXADECIMAL:
    	sprintf(buf, "0x%lX", yylval.integer);
    	return buf;
    case STRING:
    	return yylval.string;
    case TRITVAL:
    	return tritstrs[yylval.tritval];
    }
    return "";
}

int
main(int argc, char **argv)
{
    char *filename = argv[1];
    int tok;
    
    rb = rb_new();
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
