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
#include "lexer.h"
#include "util.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Forward declaration for helper function */
static int handle_int_literal(const char *text, diag_loc_t loc,
                             lexer_extra_t *extra, token_val_t *val);

/* Dynamic buffer used during string literal accumulation */
static strbuf_t s_strbuf;
static diag_loc_t s_str_start_loc;
static diag_loc_t s_last_token_loc;

#define YY_USER_ACTION do {                                      \
    lexer_extra_t *extra = yyextra;                              \
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
} while (0);

%}

%option noyywrap nounput noinput
%option reentrant
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
                    return TOK_STRING_LIT;
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

"int"           { return TOK_KW_INT; }
"float"         { return TOK_KW_FLOAT; }
"string"        { return TOK_KW_STRING; }
"bool"          { return TOK_KW_BOOL; }
"true"          { return TOK_KW_TRUE; }
"false"         { return TOK_KW_FALSE; }

".."            { return TOK_DOTDOT; }
"."             { return TOK_DOT; }
"{"             { return TOK_LBRACE; }
"}"             { return TOK_RBRACE; }
"("             { return TOK_LPAREN; }
")"             { return TOK_RPAREN; }
"="             { return TOK_ASSIGN; }
":"             { return TOK_COLON; }
";"             { return TOK_SEMI; }
"@"             { return TOK_AT; }
"+"             { return TOK_PLUS; }
"-"             { return TOK_MINUS; }
"*"             { return TOK_STAR; }
"/"             { return TOK_SLASH; }

{FLOAT_LIT}     { return TOK_FLOAT_LIT; }
{INT_LIT}       { return TOK_INT_LIT; }
{IDENT}         { return TOK_IDENT; }

.               {
                    diag_error(yyextra->diags, DIAG_E101, s_last_token_loc,
                               "illegal character '%c'", yytext[0]);
                }

<<EOF>>         { return TOK_EOF; }

%%

static int handle_int_literal(const char *text, diag_loc_t loc,
                             lexer_extra_t *extra, token_val_t *val) {
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
        val->ival = 0;
    } else {
        val->ival = (int64_t)acc;
    }
    return TOK_INT_LIT;
}

const char *lexer_token_name(token_kind_t kind) {
    switch (kind) {
        case TOK_IDENT:      return "IDENT";
        case TOK_STRING_LIT: return "STRING_LIT";
        case TOK_INT_LIT:    return "INT_LIT";
        case TOK_FLOAT_LIT:  return "FLOAT_LIT";
        case TOK_KW_INT:     return "KW_INT";
        case TOK_KW_FLOAT:   return "KW_FLOAT";
        case TOK_KW_STRING:  return "KW_STRING";
        case TOK_KW_BOOL:    return "KW_BOOL";
        case TOK_KW_TRUE:    return "KW_TRUE";
        case TOK_KW_FALSE:   return "KW_FALSE";
        case TOK_LBRACE:     return "LBRACE";
        case TOK_RBRACE:     return "RBRACE";
        case TOK_LPAREN:     return "LPAREN";
        case TOK_RPAREN:     return "RPAREN";
        case TOK_ASSIGN:     return "ASSIGN";
        case TOK_COLON:      return "COLON";
        case TOK_SEMI:       return "SEMI";
        case TOK_DOT:        return "DOT";
        case TOK_DOTDOT:     return "DOTDOT";
        case TOK_AT:         return "AT";
        case TOK_PLUS:       return "PLUS";
        case TOK_MINUS:      return "MINUS";
        case TOK_STAR:       return "STAR";
        case TOK_SLASH:      return "SLASH";
        case TOK_EOF:        return "EOF";
        default:             return "UNKNOWN";
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
    int tok_code;
    while ((tok_code = configlang_yylex(scanner)) != TOK_EOF) {
        token_kind_t kind = (token_kind_t)tok_code;
        diag_loc_t loc = s_last_token_loc;

        const char *lexeme = configlang_yyget_text(scanner);
        if (kind == TOK_STRING_LIT) {
            lexeme = s_strbuf.data;
        } else if (kind == TOK_INT_LIT) {
            token_val_t val;
            handle_int_literal(lexeme, loc, &extra, &val);
        }

        if (dump_tokens) {
            char pos_str[32];
            snprintf(pos_str, sizeof(pos_str), "%zu:%zu", loc.line, loc.col);
            fprintf(stderr, "%-10s %-13s %s\n", pos_str, lexer_token_name(kind),
                    lexeme ? lexeme : "");
        }

        if (kind == TOK_STRING_LIT) {
            util_strbuf_destroy(&s_strbuf);
        }
    }

    configlang_yy_delete_buffer(bp, scanner);
    configlang_yylex_destroy(scanner);
    return (diags->error_count == initial_errs);
}
