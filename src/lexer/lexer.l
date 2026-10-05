%{
/**
 * ============================================================================
 * Lexical Analyzer (lexer.l)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * Simple Flex scanner to convert character stream into tokens.
 * ============================================================================
 */

#include "common.h"
#include "ast.h"
#include "parser.tab.h"
#include <string.h>

extern int yylineno;
%}

%option noyywrap
%option yylineno
%option noinput nounput

%%

"int"           { return KW_INT; }
"float"         { return KW_FLOAT; }
"string"        { return KW_STRING; }
"bool"          { return KW_BOOL; }
"true"          { yylval.bval = true; return KW_TRUE; }
"false"         { yylval.bval = false; return KW_FALSE; }

[0-9]+\.[0-9]+  { yylval.fval = atof(yytext); return FLOAT_LIT; }
[0-9]+          { yylval.ival = atoi(yytext); return INT_LIT; }

\"([^"\\]|\\.)*\" { 
                    /* Strip surrounding quotes from string */
                    size_t len = strlen(yytext);
                    char *s = (char *)malloc(len - 1);
                    strncpy(s, yytext + 1, len - 2);
                    s[len - 2] = '\0';
                    yylval.sval = s;
                    return STRING_LIT;
                }

[a-zA-Z_][a-zA-Z0-9_]*(\.[a-zA-Z_][a-zA-Z0-9_]*)* {
                    yylval.sval = strdup(yytext);
                    return IDENT;
                }

"@"             { return '@'; }
"{"             { return '{'; }
"}"             { return '}'; }
"("             { return '('; }
")"             { return ')'; }
":"             { return ':'; }
"="             { return '='; }
";"             { return ';'; }
"+"             { return '+'; }
"-"             { return '-'; }
"*"             { return '*'; }
"/"             { return '/'; }
".."            { return DOTDOT; }

"//".*          { /* ignore single-line comment */ }
"/*"([^*]|\*+[^*/])*\*+"/" { /* ignore block comment */ }
[ \t\r]+        { /* ignore whitespace */ }
\n              { /* line counter handled by yylineno */ }

.               { fprintf(stderr, "Lexical Error at line %d: unexpected character '%s'\n", yylineno, yytext); }

%%
