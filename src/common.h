/**
 * ============================================================================
 * ConfigLang - Common Definitions (common.h)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * Simple shared definitions for basic data types used across compiler phases.
 * ============================================================================
 */

#ifndef CONFIGLANG_COMMON_H
#define CONFIGLANG_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/** Basic data types supported in our configuration language */
typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_UNKNOWN
} DataType;

/** Convert DataType enum to human-readable string */
static inline const char *type_name(DataType t) {
    switch (t) {
        case TYPE_INT:    return "int";
        case TYPE_FLOAT:  return "float";
        case TYPE_STRING: return "string";
        case TYPE_BOOL:   return "bool";
        default:          return "unknown";
    }
}

#endif /* CONFIGLANG_COMMON_H */
