/**
 * ============================================================================
 * Code Generator Interface (codegen.h)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * Emits final JSON configuration from evaluated AST and Symbol Table.
 * ============================================================================
 */

#ifndef CONFIGLANG_CODEGEN_H
#define CONFIGLANG_CODEGEN_H

#include "ast.h"
#include <stdio.h>

/* Emits formatted JSON configuration to the specified output stream */
void codegen_json(ASTNode *root, FILE *out);

#endif /* CONFIGLANG_CODEGEN_H */
