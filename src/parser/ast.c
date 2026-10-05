/**
 * ============================================================================
 * Abstract Syntax Tree Implementation (ast.c)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * Simple constructor functions and tree printer for AST nodes.
 * ============================================================================
 */

#include "ast.h"

static ASTNode *ast_alloc(ASTNodeType type, int line) {
    ASTNode *n = (ASTNode *)malloc(sizeof(ASTNode));
    memset(n, 0, sizeof(ASTNode));
    n->type = type;
    n->line = line;
    return n;
}

ASTNode *ast_new_int(int val, int line) {
    ASTNode *n = ast_alloc(NODE_INT, line);
    n->ival = val;
    n->data_type = TYPE_INT;
    return n;
}

ASTNode *ast_new_float(double val, int line) {
    ASTNode *n = ast_alloc(NODE_FLOAT, line);
    n->fval = val;
    n->data_type = TYPE_FLOAT;
    return n;
}

ASTNode *ast_new_string(char *val, int line) {
    ASTNode *n = ast_alloc(NODE_STRING, line);
    n->sval = val;
    n->data_type = TYPE_STRING;
    return n;
}

ASTNode *ast_new_bool(bool val, int line) {
    ASTNode *n = ast_alloc(NODE_BOOL, line);
    n->bval = val;
    n->data_type = TYPE_BOOL;
    return n;
}

ASTNode *ast_new_ref(char *name, int line) {
    ASTNode *n = ast_alloc(NODE_REF, line);
    n->name = name;
    return n;
}

ASTNode *ast_new_binary(char op, ASTNode *left, ASTNode *right, int line) {
    ASTNode *n = ast_alloc(NODE_BINARY, line);
    n->op = op;
    n->left = left;
    n->right = right;
    return n;
}

ASTNode *ast_new_decl(char *name, DataType type, ASTNode *expr, int line) {
    ASTNode *n = ast_alloc(NODE_DECL, line);
    n->name = name;
    n->data_type = type;
    n->left = expr;
    return n;
}

ASTNode *ast_new_block(char *name, ASTNode *body, int line) {
    ASTNode *n = ast_alloc(NODE_BLOCK, line);
    n->name = name;
    n->left = body;
    return n;
}

void ast_print(ASTNode *node, int indent) {
    if (!node) return;
    for (int i = 0; i < indent; i++) printf("  ");

    switch (node->type) {
        case NODE_BLOCK:
            printf("Block '%s':\n", node->name);
            ast_print(node->left, indent + 1);
            break;
        case NODE_DECL:
            printf("Decl '%s' (%s) =\n", node->name, type_name(node->data_type));
            ast_print(node->left, indent + 1);
            break;
        case NODE_BINARY:
            printf("BinaryOp '%c':\n", node->op);
            ast_print(node->left, indent + 1);
            ast_print(node->right, indent + 1);
            break;
        case NODE_INT:
            printf("Int: %d\n", node->ival);
            break;
        case NODE_FLOAT:
            printf("Float: %g\n", node->fval);
            break;
        case NODE_STRING:
            printf("String: \"%s\"\n", node->sval);
            break;
        case NODE_BOOL:
            printf("Bool: %s\n", node->bval ? "true" : "false");
            break;
        case NODE_REF:
            printf("Ref: @%s\n", node->name);
            break;
    }

    if (node->next) {
        ast_print(node->next, indent);
    }
}
