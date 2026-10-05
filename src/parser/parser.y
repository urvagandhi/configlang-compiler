%{
/**
 * ============================================================================
 * Syntax Analyzer (parser.y)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * Simple LALR(1) Grammar for Configuration Language:
 *   - Declarations: IDENT = expr; or IDENT : type = expr;
 *   - Nested blocks: IDENT { ... }
 *   - Arithmetic expressions: +, -, *, /, literals, @references
 * ============================================================================
 */

#include "common.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>

extern int yylineno;
extern int yylex(void);
void yyerror(const char *s);

ASTNode *root_ast = NULL;
%}

%union {
    int ival;
    double fval;
    char *sval;
    bool bval;
    DataType type;
    ASTNode *node;
}

%token <ival> INT_LIT
%token <fval> FLOAT_LIT
%token <sval> STRING_LIT IDENT
%token <bval> KW_TRUE KW_FALSE
%token KW_INT KW_FLOAT KW_STRING KW_BOOL DOTDOT

%type <node> program stmt_list stmt block decl expr
%type <type> type_spec

%left '+' '-'
%left '*' '/'
%nonassoc UMINUS

%%

program
    : stmt_list { root_ast = $1; }
    ;

stmt_list
    : stmt stmt_list { $1->next = $2; $$ = $1; }
    | /* empty */    { $$ = NULL; }
    ;

stmt
    : decl  { $$ = $1; }
    | block { $$ = $1; }
    ;

block
    : IDENT '{' stmt_list '}' {
        $$ = ast_new_block($1, $3, yylineno);
    }
    ;

decl
    : IDENT ':' type_spec opt_range '=' expr ';' {
        $$ = ast_new_decl($1, $3, $6, yylineno);
    }
    | IDENT '=' expr ';' {
        $$ = ast_new_decl($1, TYPE_UNKNOWN, $3, yylineno);
    }
    ;

type_spec
    : KW_INT    { $$ = TYPE_INT; }
    | KW_FLOAT  { $$ = TYPE_FLOAT; }
    | KW_STRING { $$ = TYPE_STRING; }
    | KW_BOOL   { $$ = TYPE_BOOL; }
    ;

opt_range
    : '(' expr DOTDOT expr ')' { /* optional range constraint */ }
    | /* empty */
    ;

expr
    : expr '+' expr { $$ = ast_new_binary('+', $1, $3, yylineno); }
    | expr '-' expr { $$ = ast_new_binary('-', $1, $3, yylineno); }
    | expr '*' expr { $$ = ast_new_binary('*', $1, $3, yylineno); }
    | expr '/' expr { $$ = ast_new_binary('/', $1, $3, yylineno); }
    | '-' expr %prec UMINUS { $$ = ast_new_binary('-', ast_new_int(0, yylineno), $2, yylineno); }
    | '(' expr ')'  { $$ = $2; }
    | '@' IDENT     { $$ = ast_new_ref($2, yylineno); }
    | INT_LIT       { $$ = ast_new_int($1, yylineno); }
    | FLOAT_LIT     { $$ = ast_new_float($1, yylineno); }
    | STRING_LIT    { $$ = ast_new_string($1, yylineno); }
    | KW_TRUE       { $$ = ast_new_bool(true, yylineno); }
    | KW_FALSE      { $$ = ast_new_bool(false, yylineno); }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Syntax Error at line %d: %s\n", yylineno, s);
}
