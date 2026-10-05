/**
 * ============================================================================
 * Semantic Analyzer Interface (semantic.h)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * Performs type checking, undefined reference checks, and symbol table registration.
 * ============================================================================
 */

#ifndef CONFIGLANG_SEMANTIC_H
#define CONFIGLANG_SEMANTIC_H

#include "ast.h"
#include "symtab.h"

/* Run semantic analysis over AST. Returns number of errors found. */
int semantic_check(ASTNode *root);

#endif /* CONFIGLANG_SEMANTIC_H */
