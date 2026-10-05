%{
/**
 * ============================================================================
 * Bison LALR(1) Syntax Analyzer (parser.y)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * SYLLABUS MAPPING:
 *   - Experiment 4: Syntax Analyzer using YACC / Bison
 *   - Experiment 5: Syntax Error Handling and Recovery in YACC
 *
 * GRAMMAR PRODUCTIONS (BNF):
 *   program     -> item_list
 *   item_list   -> empty | item_list item
 *   item        -> block | declaration | error ';'  <-- (Exp 5 Error Recovery)
 *   block       -> IDENT '{' item_list '}'
 *   declaration -> IDENT ':' type [range] '=' expr ';'
 *                | IDENT '=' expr ';'
 *   expr        -> expr ('+' | '-' | '*' | '/') expr
 *                | '-' expr
 *                | '(' expr ')'
 *                | INT | FLOAT | STRING | BOOL | '@' path
 * ============================================================================
 */

#include "arena.h"
#include "ast.h"
#include "diag.h"
#include "lexer.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

%}

%define api.pure full
%define api.prefix {configlang_yy}
%locations
%define parse.error verbose

%lex-param { void *scanner }
%parse-param { void *scanner }
%parse-param { lexer_extra_t *extra }
%parse-param { ast_node_t **out_ast }

%union {
    char *str;
    int64_t ival;
    double fval;
    data_type_t type;
    ast_range_t range;
    struct {
        double val;
        bool is_float;
    } bound;
    ast_node_t *node;
}

%token <str> IDENT STRING_LIT
%token <ival> INT_LIT
%token <fval> FLOAT_LIT
%token KW_INT KW_FLOAT KW_STRING KW_BOOL KW_TRUE KW_FALSE
%token LBRACE RBRACE LPAREN RPAREN ASSIGN COLON SEMI DOT DOTDOT AT
%token PLUS MINUS STAR SLASH

%type <node> program item_list item block declaration expr
%type <str> path
%type <type> type
%type <range> opt_range bound_spec
%type <bound> bound

%left PLUS MINUS
%left STAR SLASH
%precedence UMINUS

%expect 0

%code {
void configlang_yyerror(CONFIGLANG_YYLTYPE *loc, void *scanner, lexer_extra_t *extra,
                        ast_node_t **out_ast, const char *msg);
int configlang_yylex(CONFIGLANG_YYSTYPE *yylval_param, CONFIGLANG_YYLTYPE *yylloc_param,
                     void *yyscanner);
int configlang_yylex_init(void **ptr_yy_globals);
int configlang_yylex_destroy(void *yyscanner);
void configlang_yyset_extra(lexer_extra_t *user_defined, void *yyscanner);
typedef struct yy_buffer_state *YY_BUFFER_STATE;
YY_BUFFER_STATE configlang_yy_scan_string(const char *yy_str, void *yyscanner);
void configlang_yy_delete_buffer(YY_BUFFER_STATE b, void *yyscanner);
}

%%

program
    : item_list
      {
          *out_ast = $1;
      }
    ;

item_list
    : %empty
      {
          diag_loc_t loc = { extra->filename, 1, 1, 1 };
          $$ = ast_create_program(extra->arena, loc);
      }
    | item_list item
      {
          if ($2 != NULL) {
              ast_append_item(extra->arena, $1, $2);
          }
          $$ = $1;
      }
    ;

item
    : block        { $$ = $1; }
    | declaration  { $$ = $1; }
    | error SEMI   { $$ = NULL; yyerrok; }
    ;

block
    : IDENT LBRACE item_list RBRACE
      {
          diag_loc_t loc = { extra->filename, (size_t)@1.first_line, (size_t)@1.first_column, (size_t)(@1.last_column - @1.first_column) };
          $$ = ast_create_block(extra->arena, $1, loc);
          for (ast_item_t *cur = $3->data.program.items_head; cur != NULL; cur = cur->next) {
              ast_append_item(extra->arena, $$, cur->node);
          }
      }
    ;

declaration
    : IDENT COLON type opt_range ASSIGN expr SEMI
      {
          diag_loc_t loc = { extra->filename, (size_t)@1.first_line, (size_t)@1.first_column, (size_t)(@1.last_column - @1.first_column) };
          $$ = ast_create_decl(extra->arena, $1, $3, $4, $6, loc);
      }
    | IDENT ASSIGN expr SEMI
      {
          diag_loc_t loc = { extra->filename, (size_t)@1.first_line, (size_t)@1.first_column, (size_t)(@1.last_column - @1.first_column) };
          ast_range_t no_range;
          memset(&no_range, 0, sizeof(no_range));
          $$ = ast_create_decl(extra->arena, $1, TYPE_NONE, no_range, $3, loc);
      }
    ;

type
    : KW_INT    { $$ = TYPE_INT; }
    | KW_FLOAT  { $$ = TYPE_FLOAT; }
    | KW_STRING { $$ = TYPE_STRING; }
    | KW_BOOL   { $$ = TYPE_BOOL; }
    ;

opt_range
    : %empty
      {
          memset(&$$, 0, sizeof(ast_range_t));
      }
    | LPAREN bound_spec RPAREN
      {
          $$ = $2;
          $$.loc.filename = extra->filename;
          $$.loc.line = (size_t)@1.first_line;
          $$.loc.col = (size_t)@1.first_column;
          $$.loc.length = (size_t)(@3.last_column >= @1.first_column ? @3.last_column - @1.first_column : 1);
      }
    ;

bound_spec
    : bound DOTDOT bound
      {
          $$.present = true;
          $$.lo = $1.val;
          $$.lo_is_float = $1.is_float;
          $$.hi = $3.val;
          $$.hi_is_float = $3.is_float;
      }
    ;

bound
    : INT_LIT             { $$.val = (double)$1; $$.is_float = false; }
    | FLOAT_LIT           { $$.val = $1; $$.is_float = true; }
    | MINUS INT_LIT       { $$.val = -(double)$2; $$.is_float = false; }
    | MINUS FLOAT_LIT     { $$.val = -$2; $$.is_float = true; }
    ;

expr
    : expr PLUS expr
      {
          diag_loc_t loc = { extra->filename, (size_t)@2.first_line, (size_t)@2.first_column, 1 };
          $$ = ast_create_binary(extra->arena, BIN_ADD, $1, $3, loc);
      }
    | expr MINUS expr
      {
          diag_loc_t loc = { extra->filename, (size_t)@2.first_line, (size_t)@2.first_column, 1 };
          $$ = ast_create_binary(extra->arena, BIN_SUB, $1, $3, loc);
      }
    | expr STAR expr
      {
          diag_loc_t loc = { extra->filename, (size_t)@2.first_line, (size_t)@2.first_column, 1 };
          $$ = ast_create_binary(extra->arena, BIN_MUL, $1, $3, loc);
      }
    | expr SLASH expr
      {
          diag_loc_t loc = { extra->filename, (size_t)@2.first_line, (size_t)@2.first_column, 1 };
          $$ = ast_create_binary(extra->arena, BIN_DIV, $1, $3, loc);
      }
    | MINUS expr %prec UMINUS
      {
          diag_loc_t loc = { extra->filename, (size_t)@1.first_line, (size_t)@1.first_column, 1 };
          $$ = ast_create_unary(extra->arena, UN_NEG, $2, loc);
      }
    | LPAREN expr RPAREN
      {
          $$ = $2;
      }
    | INT_LIT
      {
          diag_loc_t loc = { extra->filename, (size_t)@1.first_line, (size_t)@1.first_column, (size_t)(@1.last_column >= @1.first_column ? @1.last_column - @1.first_column : 1) };
          $$ = ast_create_int(extra->arena, $1, loc);
      }
    | FLOAT_LIT
      {
          diag_loc_t loc = { extra->filename, (size_t)@1.first_line, (size_t)@1.first_column, (size_t)(@1.last_column >= @1.first_column ? @1.last_column - @1.first_column : 1) };
          $$ = ast_create_float(extra->arena, $1, loc);
      }
    | STRING_LIT
      {
          diag_loc_t loc = { extra->filename, (size_t)@1.first_line, (size_t)@1.first_column, (size_t)(@1.last_column >= @1.first_column ? @1.last_column - @1.first_column : 1) };
          $$ = ast_create_string(extra->arena, $1, loc);
      }
    | KW_TRUE
      {
          diag_loc_t loc = { extra->filename, (size_t)@1.first_line, (size_t)@1.first_column, 4 };
          $$ = ast_create_bool(extra->arena, true, loc);
      }
    | KW_FALSE
      {
          diag_loc_t loc = { extra->filename, (size_t)@1.first_line, (size_t)@1.first_column, 5 };
          $$ = ast_create_bool(extra->arena, false, loc);
      }
    | AT path
      {
          diag_loc_t loc = { extra->filename, (size_t)@1.first_line, (size_t)@1.first_column, (size_t)(@2.last_column >= @1.first_column ? @2.last_column - @1.first_column : 1) };
          $$ = ast_create_ref(extra->arena, $2, loc);
      }
    ;

path
    : IDENT
      {
          $$ = $1;
      }
    | path DOT IDENT
      {
          size_t len1 = strlen($1);
          size_t len2 = strlen($3);
          char *p = (char *)arena_alloc(extra->arena, len1 + len2 + 2);
          memcpy(p, $1, len1);
          p[len1] = '.';
          memcpy(p + len1 + 1, $3, len2 + 1);
          $$ = p;
      }
    ;

%%

void configlang_yyerror(CONFIGLANG_YYLTYPE *loc, void *scanner, lexer_extra_t *extra,
                        ast_node_t **out_ast, const char *msg) {
    (void)scanner;
    (void)out_ast;
    diag_loc_t dloc;
    dloc.filename = extra->filename;
    dloc.line = (size_t)loc->first_line;
    dloc.col = (size_t)loc->first_column;
    dloc.length = (loc->last_column >= loc->first_column)
                      ? (size_t)(loc->last_column - loc->first_column)
                      : 1U;
    if (dloc.length == 0) {
        dloc.length = 1U;
    }
    diag_error(extra->diags, DIAG_E201, dloc, "%s", msg);
}

ast_node_t *parser_parse(const char *source, const char *filename,
                         diag_collector_t *diags, arena_t *arena) {
    void *scanner;
    configlang_yylex_init(&scanner);

    lexer_extra_t extra;
    extra.diags = diags;
    extra.arena = arena;
    extra.filename = filename;
    extra.cur_line = 1;
    extra.cur_col = 1;
    configlang_yyset_extra(&extra, scanner);

    YY_BUFFER_STATE bp = configlang_yy_scan_string(source, scanner);

    ast_node_t *root = NULL;
    int res = configlang_yyparse(scanner, &extra, &root);

    configlang_yy_delete_buffer(bp, scanner);
    configlang_yylex_destroy(scanner);

    if (res != 0 || diags->error_count > 0) {
        return NULL;
    }
    return root;
}
