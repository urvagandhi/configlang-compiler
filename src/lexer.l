%{
/**
 * @file lexer.l
 * @brief Flex lexical analyzer for ConfigLang.
 * @module lexer
 * @phase Lexical Analysis
 *
 * Course: PCD (Compiler Design), 4CS501CC25
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 */

#define _POSIX_C_SOURCE 200809L
#include "arena.h"
#include "ast.h"
#include "diag.h"
#include "lexer.h"
#include "build/parser.h"
#define YYSTYPE CONFIGLANG_YYSTYPE
#define YYLTYPE CONFIGLANG_YYLTYPE
#include "util.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int handle_int_literal(const char *text, diag_loc_t loc,
                             lexer_extra_t *extra, int64_t *out_val);

static strbuf_t s_strbuf;
static diag_loc_t s_str_start_loc;
static diag_loc_t s_last_token_loc;

#define YY_USER_ACTION do {                                      \
    lexer_extra_t *extra = yyextra;                              \
    yylloc->first_line = (int)extra->cur_line;                   \
    yylloc->first_column = (int)extra->cur_col;                  \
    s_last_token_loc.filename = extra->filename;                 \
    s_last_token_loc.line = extra->cur_line;                     \
    s_last_token_loc.col = extra->cur_col;                      \
    s_last_token_loc.length = (size_t)yyleng;                    \
    for (int i = 0; i < yyleng; ++i) {                          \
        if (yytext[i] == '\n') {                                 \
            extra->cur_line++;                                   \
            extra->cur_col = 1;                                  \
        } else {                                                 \
            extra->cur_col++;                                    \
        }                                                        \
    }                                                            \
    yylloc->last_line = (int)extra->cur_line;                    \
    yylloc->last_column = (int)extra->cur_col;                   \
} while (0);

%}

%option noyywrap nounput noinput
%option reentrant bison-bridge bison-locations
%option extra-type="lexer_extra_t *"
%option prefix="configlang_yy"

%x COMMENT
%x STR

IDENT       [A-Za-z_][A-Za-z0-9_]*
INT_LIT     [0-9]+
FLOAT_LIT   [0-9]+\.[0-9]+([eE][+-]?[0-9]+)?
WHITESPACE  [ \t\r]+

%%

{WHITESPACE}    { /* Ignore whitespace */ }
\n              { /* Handled by YY_USER_ACTION */ }

"//"[^\n]*      { /* Ignore single-line comments */ }

"/*"            {
                    yyextra->comment_start_loc = s_last_token_loc;
                    BEGIN(COMMENT);
                }
<COMMENT>{
    "*/"        { BEGIN(INITIAL); }
    \n          { /* Line counting handled by YY_USER_ACTION */ }
    .           { /* Consume comment chars */ }
    <<EOF>>     {
                    diag_error(yyextra->diags, DIAG_E105,
                               yyextra->comment_start_loc,
                               "unterminated block comment");
                    BEGIN(INITIAL);
                }
}

\"              {
                    s_str_start_loc = s_last_token_loc;
                    s_strbuf = util_strbuf_create();
                    BEGIN(STR);
                }
<STR>{
    \"          {
                    BEGIN(INITIAL);
                    yylloc->first_line = (int)s_str_start_loc.line;
                    yylloc->first_column = (int)s_str_start_loc.col;
                    yylval->str = arena_strdup(yyextra->arena, s_strbuf.data);
                    util_strbuf_destroy(&s_strbuf);
                    return STRING_LIT;
                }
    \n          {
                    diag_error(yyextra->diags, DIAG_E102, s_str_start_loc,
                               "unterminated string literal");
                    util_strbuf_destroy(&s_strbuf);
                    BEGIN(INITIAL);
                }
    <<EOF>>     {
                    diag_error(yyextra->diags, DIAG_E102, s_str_start_loc,
                               "unterminated string literal");
                    util_strbuf_destroy(&s_strbuf);
                    BEGIN(INITIAL);
                }
    \\n         { util_strbuf_append_char(&s_strbuf, '\n'); }
    \\t         { util_strbuf_append_char(&s_strbuf, '\t'); }
    \\r         { util_strbuf_append_char(&s_strbuf, '\r'); }
    \\\\        { util_strbuf_append_char(&s_strbuf, '\\'); }
    \\\"        { util_strbuf_append_char(&s_strbuf, '\"'); }
    \\[^"\\ntr] {
                    diag_loc_t esc_loc = s_last_token_loc;
                    esc_loc.length = 2;
                    diag_error(yyextra->diags, DIAG_E104, esc_loc,
                               "invalid escape sequence '%s'", yytext);
                }
    [^"\\\n]+   { util_strbuf_append(&s_strbuf, yytext); }
}

"int"           { return KW_INT; }
"float"         { return KW_FLOAT; }
"string"        { return KW_STRING; }
"bool"          { return KW_BOOL; }
"true"          { return KW_TRUE; }
"false"         { return KW_FALSE; }

".."            { return DOTDOT; }
"."             { return DOT; }
"{"             { return LBRACE; }
"}"             { return RBRACE; }
"("             { return LPAREN; }
")"             { return RPAREN; }
"="             { return ASSIGN; }
":"             { return COLON; }
";"             { return SEMI; }
"@"             { return AT; }
"+"             { return PLUS; }
"-"             { return MINUS; }
"*"             { return STAR; }
"/"             { return SLASH; }

{FLOAT_LIT}     {
                    yylval->fval = strtod(yytext, NULL);
                    return FLOAT_LIT;
                }

{INT_LIT}       {
                    handle_int_literal(yytext, s_last_token_loc, yyextra, &yylval->ival);
                    return INT_LIT;
                }

{IDENT}         {
                    yylval->str = arena_strdup(yyextra->arena, yytext);
                    return IDENT;
                }

.               {
                    diag_error(yyextra->diags, DIAG_E101, s_last_token_loc,
                               "illegal character '%c'", yytext[0]);
                }

<<EOF>>         { return 0; }

%%

static int handle_int_literal(const char *text, diag_loc_t loc,
                             lexer_extra_t *extra, int64_t *out_val) {
    uint64_t acc = 0;
    bool overflow = false;
    for (size_t i = 0; text[i] != '\0'; ++i) {
        uint64_t d = (uint64_t)(text[i] - '0');
        if (acc > (UINT64_MAX - d) / 10ULL) {
            overflow = true;
            break;
        }
        acc = acc * 10ULL + d;
    }
    if (acc > (uint64_t)INT64_MAX) {
        overflow = true;
    }
    if (overflow) {
        diag_error(extra->diags, DIAG_E103, loc,
                   "integer literal '%s' out of range", text);
        *out_val = 0;
    } else {
        *out_val = (int64_t)acc;
    }
    return INT_LIT;
}

const char *lexer_token_name(int kind) {
    switch (kind) {
        case IDENT:      return "IDENT";
        case STRING_LIT: return "STRING_LIT";
        case INT_LIT:    return "INT_LIT";
        case FLOAT_LIT:  return "FLOAT_LIT";
        case KW_INT:     return "KW_INT";
        case KW_FLOAT:   return "KW_FLOAT";
        case KW_STRING:  return "KW_STRING";
        case KW_BOOL:    return "KW_BOOL";
        case KW_TRUE:    return "KW_TRUE";
        case KW_FALSE:   return "KW_FALSE";
        case LBRACE:     return "LBRACE";
        case RBRACE:     return "RBRACE";
        case LPAREN:     return "LPAREN";
        case RPAREN:     return "RPAREN";
        case ASSIGN:     return "ASSIGN";
        case COLON:      return "COLON";
        case SEMI:       return "SEMI";
        case DOT:        return "DOT";
        case DOTDOT:     return "DOTDOT";
        case AT:         return "AT";
        case PLUS:       return "PLUS";
        case MINUS:      return "MINUS";
        case STAR:       return "STAR";
        case SLASH:      return "SLASH";
        case 0:          return "EOF";
        default:         return "UNKNOWN";
    }
}

bool lexer_scan(const char *source, const char *filename,
                diag_collector_t *diags, arena_t *arena, bool dump_tokens) {
    yyscan_t scanner;
    configlang_yylex_init(&scanner);

    lexer_extra_t extra;
    extra.diags = diags;
    extra.arena = arena;
    extra.filename = filename;
    extra.cur_line = 1;
    extra.cur_col = 1;
    configlang_yyset_extra(&extra, scanner);

    YY_BUFFER_STATE bp = configlang_yy_scan_string(source, scanner);

    if (dump_tokens) {
        fprintf(stderr, "LINE:COL   TOKEN         LEXEME\n");
    }

    size_t initial_errs = diags->error_count;
    YYSTYPE lval;
    YYLTYPE lloc;
    int tok_code;
    while ((tok_code = configlang_yylex(&lval, &lloc, scanner)) != 0) {
        if (dump_tokens) {
            char pos_str[32];
            snprintf(pos_str, sizeof(pos_str), "%d:%d", lloc.first_line, lloc.first_column);
            const char *lexeme = configlang_yyget_text(scanner);
            fprintf(stderr, "%-10s %-13s %s\n", pos_str, lexer_token_name(tok_code),
                    lexeme ? lexeme : "");
        }
    }

    configlang_yy_delete_buffer(bp, scanner);
    configlang_yylex_destroy(scanner);
    return (diags->error_count == initial_errs);
}
