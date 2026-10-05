# ConfigLang: Simple Compiler for Principles of Compiler Design (4CS501CC25)
# Authors: Urva Gandhi (23BCE078) & Rakshit Gajnotar (23BCE077)

CC = gcc
CFLAGS = -Wall -Wextra -g -Isrc -Isrc/parser -Isrc/symtab -Isrc/semantic -Isrc/tac -Isrc/optimizer -Isrc/codegen -Ibuild -I.

BUILD_DIR = build
BIN = $(BUILD_DIR)/configlang

SRCS = src/parser/ast.c src/symtab/symtab.c src/semantic/semantic.c \
       src/tac/tac.c src/optimizer/optimizer.c src/codegen/codegen.c src/main.c
OBJS = $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRCS)) $(BUILD_DIR)/parser.tab.o $(BUILD_DIR)/lex.yy.o

.PHONY: all clean test demo directories

all: directories $(BIN)

directories:
	@mkdir -p $(BUILD_DIR)/parser $(BUILD_DIR)/symtab $(BUILD_DIR)/semantic \
	          $(BUILD_DIR)/tac $(BUILD_DIR)/optimizer $(BUILD_DIR)/codegen

$(BUILD_DIR)/%.o: src/%.c | directories
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser.tab.c $(BUILD_DIR)/parser.tab.h: src/parser/parser.y | directories
	bison -d -o $(BUILD_DIR)/parser.tab.c src/parser/parser.y

$(BUILD_DIR)/parser.tab.o: $(BUILD_DIR)/parser.tab.c | directories
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lex.yy.c: src/lexer/lexer.l $(BUILD_DIR)/parser.tab.h | directories
	flex -o $@ $<

$(BUILD_DIR)/lex.yy.o: $(BUILD_DIR)/lex.yy.c | directories
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

demo: $(BIN)
	@echo "=== Running ConfigLang Full Pipeline Demo ==="
	./$(BIN) --phases examples/server.cfg

test: $(BIN)
	@echo "=== Running Student Test Suite ==="
	@bash tests/run_tests.sh

clean:
	rm -rf $(BUILD_DIR)
