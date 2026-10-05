# ConfigLang Compiler Makefile
# Course: PCD (Compiler Design), 4CS501CC25
# Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)

CC = gcc
CFLAGS = -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror -g
FLEX_CFLAGS = -std=c11 -D_POSIX_C_SOURCE=200809L -g -Wall -Wextra -Wno-sign-compare -Wno-unused-function -Wno-unused-parameter -Wno-conversion
BISON_CFLAGS = -std=c11 -D_POSIX_C_SOURCE=200809L -g -Wall -Wextra -Wno-conversion -Wno-sign-compare
INCLUDES = -Isrc -I$(BUILD_DIR) -I.

BUILD_DIR = build
BIN = $(BUILD_DIR)/configlang

SRCS = src/util.c src/arena.c src/diag.c src/ast.c src/symtab.c src/semantic.c src/tac.c src/optimizer.c src/validator.c src/codegen.c src/main.c
OBJS = $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRCS)) $(BUILD_DIR)/parser.o $(BUILD_DIR)/lexer.o

.PHONY: all clean test memcheck demo directories

all: directories $(BIN)

directories:
	@mkdir -p $(BUILD_DIR) tests/valid tests/invalid examples

$(BUILD_DIR)/%.o: src/%.c | directories
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/parser.c $(BUILD_DIR)/parser.h: src/parser.y | directories
	bison -Wall -d -o $(BUILD_DIR)/parser.c src/parser.y

$(BUILD_DIR)/parser.o: $(BUILD_DIR)/parser.c | directories
	$(CC) $(BISON_CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/lexer.c: src/lexer.l $(BUILD_DIR)/parser.h | directories
	flex -o $@ $<

$(BUILD_DIR)/lexer.o: $(BUILD_DIR)/lexer.c | directories
	$(CC) $(FLEX_CFLAGS) $(INCLUDES) -c $< -o $@

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

test: $(BIN)
	@echo "=== Running ConfigLang Test Suite ==="
	@bash tests/run_tests.sh

memcheck: $(BIN)
	@echo "=== Running Memory Leak Checks (Valgrind) ==="
	valgrind --leak-check=full --error-exitcode=42 ./$(BIN) --version > /dev/null
	valgrind --leak-check=full --error-exitcode=42 ./$(BIN) --help > /dev/null
	valgrind --leak-check=full --error-exitcode=42 ./$(BIN) examples/server.cfg > /dev/null
	valgrind --leak-check=full --error-exitcode=42 ./$(BIN) -f yaml examples/server.cfg > /dev/null
	valgrind --leak-check=full --error-exitcode=42 ./$(BIN) --phases examples/server.cfg > /dev/null 2>&1
	valgrind --leak-check=full --error-exitcode=42 ./$(BIN) tests/invalid/lex_illegal_char.cfg > /dev/null 2>&1 || [ $$? -ne 42 ]
	valgrind --leak-check=full --error-exitcode=42 ./$(BIN) tests/invalid/syn_missing_semi.cfg > /dev/null 2>&1 || [ $$? -ne 42 ]
	valgrind --leak-check=full --error-exitcode=42 ./$(BIN) tests/invalid/sem_duplicate.cfg > /dev/null 2>&1 || [ $$? -ne 42 ]
	valgrind --leak-check=full --error-exitcode=42 ./$(BIN) tests/invalid/sem_cycle.cfg > /dev/null 2>&1 || [ $$? -ne 42 ]
	valgrind --leak-check=full --error-exitcode=42 ./$(BIN) tests/invalid/val_range_exceeded.cfg > /dev/null 2>&1 || [ $$? -ne 42 ]
	valgrind --leak-check=full --error-exitcode=42 ./$(BIN) tests/invalid/opt_div_zero.cfg > /dev/null 2>&1 || [ $$? -ne 42 ]
	@echo "All memchecks passed with 0 memory errors or leaks!"

demo: $(BIN)
	@echo "=== Running ConfigLang Demo ==="
	./$(BIN) --phases examples/server.cfg

clean:
	rm -rf $(BUILD_DIR)
