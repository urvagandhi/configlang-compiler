# ConfigLang Compiler Makefile
# Course: PCD (Compiler Design), 4CS501CC25
# Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)

CC = gcc
CFLAGS = -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror -g
FLEX_CFLAGS = -std=c11 -D_POSIX_C_SOURCE=200809L -g -Wall -Wextra -Wno-sign-compare -Wno-unused-function -Wno-unused-parameter -Wno-conversion
INCLUDES = -Isrc -I$(BUILD_DIR)

BUILD_DIR = build
BIN = $(BUILD_DIR)/configlang

SRCS = src/util.c src/arena.c src/diag.c src/main.c
OBJS = $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRCS)) $(BUILD_DIR)/lexer.o

.PHONY: all clean test memcheck demo directories

all: directories $(BIN)

directories:
	@mkdir -p $(BUILD_DIR) tests/valid tests/invalid examples

$(BUILD_DIR)/%.o: src/%.c | directories
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/lexer.c: src/lexer.l | directories
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
	valgrind --leak-check=full --error-exitcode=1 ./$(BIN) --version > /dev/null
	valgrind --leak-check=full --error-exitcode=1 ./$(BIN) --help > /dev/null
	valgrind --leak-check=full --error-exitcode=1 ./$(BIN) --tokens examples/server.cfg > /dev/null 2>&1
	@echo "Memcheck passed cleanly!"

demo: $(BIN)
	@echo "=== Running ConfigLang Demo ==="
	./$(BIN) --phases examples/server.cfg

clean:
	rm -rf $(BUILD_DIR)
