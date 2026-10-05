/**
 * @file main.c
 * @brief Command-line driver and options parser for ConfigLang compiler.
 * @module main
 * @phase Driver
 *
 * Course: PCD (Compiler Design), 4CS501CC25
 * Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)
 */

#include "arena.h"
#include "ast.h"
#include "codegen.h"
#include "diag.h"
#include "lexer.h"
#include "optimizer.h"
#include "parser.h"
#include "semantic.h"
#include "symtab.h"
#include "tac.h"
#include "util.h"
#include "validator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CONFIGLANG_VERSION "configlang 1.0.0"

/** Target output formatting mode. */
typedef enum output_format {
    FORMAT_JSON = 0,
    FORMAT_YAML = 1
} output_format_t;

/** Parsed command line options. */
typedef struct compiler_options {
    const char *input_path;
    const char *output_path;
    output_format_t format;
    bool dump_tokens;
    bool dump_ast;
    bool dump_symtab;
    bool dump_deps;
    bool dump_tac;
    bool dump_tac_opt;
    bool dump_phases;
    bool no_color;
} compiler_options_t;

static void print_usage(const char *prog_name) {
    printf("Usage: %s [options] <input-file>\n\n", prog_name);
    printf("Options:\n");
    printf("  -o, --output <file>       write result to file (default: stdout)\n");
    printf("  -f, --format <json|yaml>  output format (default: json)\n");
    printf("      --tokens              dump token stream\n");
    printf("      --ast                 dump abstract syntax tree\n");
    printf("      --symtab              dump symbol table\n");
    printf("      --deps                dump dependency edges\n");
    printf("      --tac                 dump unoptimized TAC\n");
    printf("      --tac-opt             dump optimized TAC and statistics\n");
    printf("      --phases              enable all dump flags above, with banners\n");
    printf("      --no-color            disable ANSI colour\n");
    printf("  -h, --help                usage\n");
    printf("  -v, --version             print \"%s\"\n", CONFIGLANG_VERSION);
}

static bool parse_dump_flags(compiler_options_t *opts, const char *arg) {
    if (strcmp(arg, "--tokens") == 0) {
        opts->dump_tokens = true;
    } else if (strcmp(arg, "--ast") == 0) {
        opts->dump_ast = true;
    } else if (strcmp(arg, "--symtab") == 0) {
        opts->dump_symtab = true;
    } else if (strcmp(arg, "--deps") == 0) {
        opts->dump_deps = true;
    } else if (strcmp(arg, "--tac") == 0) {
        opts->dump_tac = true;
    } else if (strcmp(arg, "--tac-opt") == 0) {
        opts->dump_tac_opt = true;
    } else if (strcmp(arg, "--phases") == 0) {
        opts->dump_phases = true;
        opts->dump_tokens = opts->dump_ast = opts->dump_symtab = true;
        opts->dump_deps = opts->dump_tac = opts->dump_tac_opt = true;
    } else if (strcmp(arg, "--no-color") == 0) {
        opts->no_color = true;
    } else {
        return false;
    }
    return true;
}

static bool parse_arg_options(int argc, char **argv, int *idx,
                             compiler_options_t *opts) {
    const char *arg = argv[*idx];
    if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
        print_usage(argv[0]);
        exit(EXIT_SUCCESS);
    }
    if (strcmp(arg, "-v") == 0 || strcmp(arg, "--version") == 0) {
        printf("%s\n", CONFIGLANG_VERSION);
        exit(EXIT_SUCCESS);
    }
    if ((strcmp(arg, "-o") == 0 || strcmp(arg, "--output") == 0) && *idx + 1 < argc) {
        opts->output_path = argv[++(*idx)];
        return true;
    }
    if ((strcmp(arg, "-f") == 0 || strcmp(arg, "--format") == 0) && *idx + 1 < argc) {
        const char *fmt = argv[++(*idx)];
        if (strcmp(fmt, "yaml") == 0) {
            opts->format = FORMAT_YAML;
        } else if (strcmp(fmt, "json") != 0) {
            fprintf(stderr, "error: unsupported format '%s' (choose json or yaml)\n", fmt);
            exit(2);
        }
        return true;
    }
    return parse_dump_flags(opts, arg);
}

static compiler_options_t parse_arguments(int argc, char **argv) {
    compiler_options_t opts = {0};
    opts.format = FORMAT_JSON;
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            if (!parse_arg_options(argc, argv, &i, &opts)) {
                fprintf(stderr, "error: unrecognized option '%s'\n", argv[i]);
                print_usage(argv[0]);
                exit(2);
            }
        } else {
            if (opts.input_path != NULL) {
                fprintf(stderr, "error: only one input file allowed\n");
                exit(2);
            }
            opts.input_path = argv[i];
        }
    }
    if (opts.input_path == NULL) {
        fprintf(stderr, "error: no input file specified\n");
        print_usage(argv[0]);
        exit(2);
    }
    return opts;
}

int main(int argc, char **argv) {
    compiler_options_t opts = parse_arguments(argc, argv);
    size_t file_size = 0;
    char *source = util_read_file(opts.input_path, &file_size);
    if (source == NULL) {
        fprintf(stderr, "%s:0:0: error[E901]: cannot read input file '%s'\n",
                opts.input_path, opts.input_path);
        fprintf(stderr, "1 error, 0 warnings generated.\n");
        return 2;
    }

    bool use_color = !opts.no_color && isatty(STDERR_FILENO);
    diag_collector_t diags = diag_create(source, use_color);
    arena_t *arena = arena_create(0);

    if (opts.dump_phases) {
        fprintf(stderr, "=== [Phase 1: Lexical Analysis] ===\n");
    }

    lexer_scan(source, opts.input_path, &diags, arena, opts.dump_tokens);
    if (diags.error_count > 0) {
        diag_render_all(&diags);
        diag_destroy(&diags);
        arena_destroy(arena);
        free(source);
        return 1;
    }

    if (opts.dump_phases) {
        fprintf(stderr, "=== [Phase 2: Syntax Analysis & AST Construction] ===\n");
    }

    ast_node_t *ast = parser_parse(source, opts.input_path, &diags, arena);

    if (opts.dump_ast && ast != NULL) {
        ast_dump(ast, stderr);
    }

    if (diags.error_count > 0 || ast == NULL) {
        diag_render_all(&diags);
        diag_destroy(&diags);
        arena_destroy(arena);
        free(source);
        return 1;
    }

    if (opts.dump_phases) {
        fprintf(stderr, "=== [Phase 3: Symbol Table Construction (Pass 1)] ===\n");
    }

    symtab_t *st = symtab_create(arena, opts.input_path);
    symtab_build_pass1(st, ast, &diags);

    if (opts.dump_phases) {
        fprintf(stderr, "=== [Phase 4: Semantic Analysis (Pass 2)] ===\n");
    }

    sem_context_t sem_ctx = { st, &diags, arena, NULL, NULL };
    semantic_analyze(&sem_ctx, ast);

    if (opts.dump_symtab) {
        symtab_dump(st, stderr);
    }
    if (opts.dump_deps) {
        semantic_dump_deps(&sem_ctx, stderr);
    }

    if (diags.error_count > 0) {
        diag_render_all(&diags);
        diag_destroy(&diags);
        arena_destroy(arena);
        free(source);
        return 1;
    }

    if (opts.dump_phases) {
        fprintf(stderr, "=== [Phase 5: Three-Address Code (TAC) Generation] ===\n");
    }

    tac_program_t *tac_prog = tac_generate(arena, st);
    if (opts.dump_tac && tac_prog != NULL) {
        tac_dump(tac_prog, stderr);
    }

    if (opts.dump_phases) {
        fprintf(stderr, "=== [Phase 6: Constant Propagation & Folding Optimization] ===\n");
    }

    opt_stats_t opt_stats = optimizer_run(tac_prog, &diags);
    if (opts.dump_tac_opt && tac_prog != NULL) {
        optimizer_dump(tac_prog, &opt_stats, stderr);
    }

    if (diags.error_count > 0) {
        diag_render_all(&diags);
        diag_destroy(&diags);
        arena_destroy(arena);
        free(source);
        return 1;
    }

    if (opts.dump_phases) {
        fprintf(stderr, "=== [Phase 7: Value & Range Validation] ===\n");
    }

    validator_validate(tac_prog, st, &diags);

    if (diags.error_count > 0) {
        diag_render_all(&diags);
        diag_destroy(&diags);
        arena_destroy(arena);
        free(source);
        return 1;
    }

    if (opts.dump_phases) {
        fprintf(stderr, "=== [Phase 8: Code Generation (%s)] ===\n",
                opts.format == FORMAT_YAML ? "YAML" : "JSON");
    }

    codegen_format_t fmt = (opts.format == FORMAT_YAML) ? CODEGEN_YAML : CODEGEN_JSON;
    if (!codegen_emit_file(st, fmt, opts.output_path)) {
        fprintf(stderr, "error: failed to write output to '%s'\n", opts.output_path);
        diag_destroy(&diags);
        arena_destroy(arena);
        free(source);
        return 2;
    }

    if (diags.count > 0) {
        diag_render_all(&diags);
    }
    int exit_code = (diags.error_count > 0) ? 1 : 0;

    diag_destroy(&diags);
    arena_destroy(arena);
    free(source);
    return exit_code;
}
