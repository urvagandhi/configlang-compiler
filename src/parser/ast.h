/**
 * ============================================================================
 * Abstract Syntax Tree (ast.h)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * Simple AST node structures and constructors for syntax trees.
 * ============================================================================
 */

#ifndef CONFIGLANG_AST_H
#define CONFIGLANG_AST_H

#include "common.h"

/** Kinds of nodes in our Abstract Syntax Tree */
typedef enum {
    NODE_BLOCK,     /* Section containing child statements: server { ... } */
    NODE_DECL,      /* Variable declaration: port = 8080; */
    NODE_BINARY,    /* Binary math operation: left + right */
    NODE_INT,       /* Integer literal: 8000 */
    NODE_FLOAT,     /* Floating-point literal: 3.14 */
    NODE_STRING,    /* String literal: "localhost" */
    NODE_BOOL,      /* Boolean literal: true / false */
    NODE_REF        /* Variable reference: @base_port */
} ASTNodeType;

/** AST Node structure */
typedef struct ASTNode {
    ASTNodeType type;
    char *name;              /* Variable name or block name */
    DataType data_type;      /* Declared or inferred type */
    int ival;                /* For integers */
    double fval;             /* For floats */
    char *sval;              /* For strings */
    bool bval;               /* For booleans */
    char op;                 /* '+', '-', '*', '/' */
    struct ASTNode *left;    /* Left child */
    struct ASTNode *right;   /* Right child */
    struct ASTNode *next;    /* Next statement in list */
    int line;                /* Source line number */
} ASTNode;

/* Node constructors */
ASTNode *ast_new_int(int val, int line);
ASTNode *ast_new_float(double val, int line);
ASTNode *ast_new_string(char *val, int line);
ASTNode *ast_new_bool(bool val, int line);
ASTNode *ast_new_ref(char *name, int line);
ASTNode *ast_new_binary(char op, ASTNode *left, ASTNode *right, int line);
ASTNode *ast_new_decl(char *name, DataType type, ASTNode *expr, int line);
ASTNode *ast_new_block(char *name, ASTNode *body, int line);

/* Pretty-print the AST to stdout */
void ast_print(ASTNode *node, int indent);

#endif /* CONFIGLANG_AST_H */
