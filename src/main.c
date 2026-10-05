/**
 * ============================================================================
 * ConfigLang - Student Compiler Project (main.c)
 * ============================================================================
 * Course: Principles of Compiler Design (4CS501CC25) - Nirma University
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 *
 * THE 8 COMPILER PHASES:
 *   Phase 1: Lexical Analysis   (Flex scanner in src/lexer/lexer.l)
 *   Phase 2: Syntax Analysis    (Bison parser in src/parser/parser.y)
 *   Phase 3: Abstract Syntax Tree (src/parser/ast.c)
 *   Phase 4: Symbol Table       (src/symtab/symtab.c)
 *   Phase 5: Semantic Analysis  (src/semantic/semantic.c)
 *   Phase 6: Three-Address Code (src/tac/tac.c)
 *   Phase 7: Code Optimization  (src/optimizer/optimizer.c)
 *   Phase 8: Code Generation    (src/codegen/codegen.c)
 * ============================================================================
 */

#include "common.h"
#include "ast.h"
#include "symtab.h"
#include "semantic.h"
#include "tac.h"
#include "optimizer.h"
#include "codegen.h"

extern FILE *yyin;
extern int yyparse(void);
extern ASTNode *root_ast;

static void print_usage(const char *prog_name) {
    printf("ConfigLang: Typed Configuration Language Compiler\n");
    printf("Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)\n\n");
    printf("Usage:\n");
    printf("  %s [options] <input.cfg>\n\n", prog_name);
    printf("Options:\n");
    printf("  --phases          Display all intermediate compiler phases\n");
    printf("  -o <file>         Write output to file (default: stdout)\n");
    printf("  -h, --help        Show this help message\n");
    printf("  -v, --version     Show version\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    bool show_phases = false;
    const char *input_path = NULL;
    const char *output_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("ConfigLang 1.0 (PCD Innovative Assignment)\n");
            return 0;
        } else if (strcmp(argv[i], "--phases") == 0) {
            show_phases = true;
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_path = argv[++i];
        } else if (argv[i][0] != '-') {
            input_path = argv[i];
        }
    }

    if (!input_path) {
        fprintf(stderr, "Error: No input configuration file specified.\n");
        return 1;
    }

    yyin = fopen(input_path, "r");
    if (!yyin) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", input_path);
        return 1;
    }

    // ------------------------------------------------------------------------
    // Step 1 & 2: Lexical Analysis & Syntax Analysis (Flex & Bison)
    // ------------------------------------------------------------------------
    if (show_phases) {
        printf("==========================================================\n");
        printf("=== [Phase 1 & 2: Lexical Analysis & Parsing (AST)] ===\n");
        printf("==========================================================\n");
    }

    if (yyparse() != 0) {
        fprintf(stderr, "Parsing failed due to syntax errors.\n");
        fclose(yyin);
        return 1;
    }
    fclose(yyin);

    if (!root_ast) {
        if (output_path) {
            FILE *f = fopen(output_path, "w");
            if (f) { fprintf(f, "{\n}\n"); fclose(f); }
        } else {
            printf("{\n}\n");
        }
        return 0;
    }

    if (show_phases) {
        printf("Constructed Abstract Syntax Tree (AST):\n");
        ast_print(root_ast, 0);
        printf("\n");
    }

    // ------------------------------------------------------------------------
    // Step 3 & 4: Symbol Table & Semantic Analysis
    // ------------------------------------------------------------------------
    if (show_phases) {
        printf("==========================================================\n");
        printf("=== [Phase 3 & 4: Symbol Table & Semantic Analysis] ===\n");
        printf("==========================================================\n");
    }

    symtab_init();
    int errors = semantic_check(root_ast);
    if (errors > 0) {
        fprintf(stderr, "Compilation aborted with %d semantic error(s).\n", errors);
        return 1;
    }

    if (show_phases) {
        printf("Populated Symbol Table:\n");
        symtab_print();
        printf("\n");
    }

    // ------------------------------------------------------------------------
    // Step 5: Three-Address Code (TAC) Generation
    // ------------------------------------------------------------------------
    if (show_phases) {
        printf("==========================================================\n");
        printf("=== [Phase 5: Three-Address Code (TAC) Generation] ===\n");
        printf("==========================================================\n");
    }

    tac_init();
    tac_generate(root_ast, "");

    if (show_phases) {
        printf("Generated Raw TAC Instructions:\n");
        tac_print();
        printf("\n");
    }

    // ------------------------------------------------------------------------
    // Step 6: Code Optimization (Constant Folding & Propagation)
    // ------------------------------------------------------------------------
    if (show_phases) {
        printf("==========================================================\n");
        printf("=== [Phase 6: Code Optimization (Constant Folding)] ===\n");
        printf("==========================================================\n");
    }

    int folds = optimize_tac();

    if (show_phases) {
        printf("Optimized TAC (%d constants folded at compile time):\n", folds);
        optimizer_print();
        printf("\n");
    }

    // ------------------------------------------------------------------------
    // Step 7: Target Code Generation (JSON)
    // ------------------------------------------------------------------------
    if (show_phases) {
        printf("==========================================================\n");
        printf("=== [Phase 7: Target Code Generation (JSON)] ===\n");
        printf("==========================================================\n");
    }

    FILE *out = stdout;
    if (output_path) {
        out = fopen(output_path, "w");
        if (!out) {
            fprintf(stderr, "Error: Could not open output file '%s'\n", output_path);
            return 1;
        }
    }

    codegen_json(root_ast, out);

    if (output_path) {
        fclose(out);
        if (show_phases) {
            printf("Output successfully written to '%s'\n", output_path);
        }
    }

    return 0;
}
