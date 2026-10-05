# ConfigLang: Architectural Design Document

**Course**: Principles of Compiler Design (4CS501CC25)  
**Institution**: Institute of Technology, Nirma University  
**Authors**: Urva Gandhi (`23BCE078`) & Rakshit Gajnotar (`23BCE077`)  

---

## 1. Executive Summary

**ConfigLang** is an educational, typed configuration language compiler written in standard C11 using **Flex** and **Bison**. It translates structured, typed configuration files (`.cfg`) into standard **JSON**. The compiler is specifically designed for undergraduate academic study, emphasizing readability, simplicity, and a clear mapping to classical compiler design principles.

---

## 2. Compiler Pipeline Architecture

ConfigLang executes as an 8-phase pipeline orchestrated sequentially by the driver program in `src/main.c`:

```
 +-----------------------------------------------------------------------+
 |                            Source File (.cfg)                         |
 +-----------------------------------------------------------------------+
                                     |
                                     v
 [Phase 1: Lexical Analysis] (Flex Scanner: src/lexer/lexer.l)
   - Converts character stream into tokens: INT_LIT, FLOAT_LIT, STRING_LIT, IDENT, keywords
   - Ignores whitespace, line comments (//), and block comments (/* ... */)
   - Tracks line numbers automatically using %option yylineno
                                     |
                                     v
 [Phase 2: Syntax Analysis] (Bison Parser: src/parser/parser.y)
   - LALR(1) parsing using Context-Free Grammar
   - Resolves operator precedence with %left '+''-', %left '*''/', %nonassoc UMINUS
                                     |
                                     v
 [Phase 3: Abstract Syntax Tree] (src/parser/ast.c)
   - Constructs a lightweight hierarchical AST (NODE_DECL, NODE_BLOCK, NODE_BINARY, literals)
   - Provides indented tree visualization via ast_print()
                                     |
                                     v
 [Phase 4: Symbol Table] (src/symtab/symtab.c)
   - Linked-list table storing identifier names, data types, and evaluated values
   - Supports canonical dotted names for nested blocks (e.g. server.port)
                                     |
                                     v
 [Phase 5: Semantic Analysis] (src/semantic/semantic.c)
   - Pass 1: Traverses AST to register symbols and detect duplicate declarations
   - Pass 2: Type checking, undefined reference detection (@x), and circular check (a = @a)
                                     |
                                     v
 [Phase 6: Three-Address Code] (src/tac/tac.c)
   - Linearizes expressions into quadruples: LOAD, STORE, ADD, SUB, MUL, DIV
   - Allocates sequential temporary registers (t1, t2, t3, ...)
                                     |
                                     v
 [Phase 7: Code Optimization] (src/optimizer/optimizer.c)
   - Constant Propagation: Replaces variable and temporary uses with known constant values
   - Constant Folding: Computes constant arithmetic at compile time (e.g. 30 * 1000 -> 30000)
   - Removes redundant temporary instructions from the TAC stream
                                     |
                                     v
 [Phase 8: Target Code Generation] (src/codegen/codegen.c)
   - Traverses the AST and queries evaluated Symbol Table values
   - Emits clean, 2-space indented standard JSON
```

---

## 3. Data Structures & Representations

### 3.1 Abstract Syntax Tree (`ASTNode`)
Located in `src/parser/ast.h`:
```c
typedef enum {
    NODE_BLOCK,     /* Nested scope: server { ... } */
    NODE_DECL,      /* Declaration: port : int = 8080; */
    NODE_BINARY,    /* Binary expression: left OP right */
    NODE_INT,       /* Integer literal */
    NODE_FLOAT,     /* Floating-point literal */
    NODE_STRING,    /* String literal */
    NODE_BOOL,      /* Boolean literal */
    NODE_REF        /* Variable reference: @base_port */
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    int line;
    char *name;              /* Variable or block name */
    DataType data_type;      /* Declared or inferred type */
    int ival;
    double fval;
    char *sval;
    bool bval;
    char op;                 /* '+', '-', '*', '/' */
    struct ASTNode *left;
    struct ASTNode *right;
    struct ASTNode *next;    /* Sibling in statement list */
} ASTNode;
```

### 3.2 Symbol Table (`Symbol`)
Located in `src/symtab/symtab.h`:
```c
typedef struct Symbol {
    char name[64];           /* Canonical name, e.g. "server.port" */
    DataType type;           /* int, float, string, bool */
    int ival;                /* Computed integer value */
    double fval;             /* Computed float value */
    char sval[128];          /* Computed string value */
    bool bval;               /* Computed boolean value */
    bool is_assigned;        /* Assignment status flag */
    struct Symbol *next;     /* Linked list pointer */
} Symbol;
```

### 3.3 Three-Address Code Quadruple (`TAC`)
Located in `src/tac/tac.h`:
```c
typedef struct TAC {
    char op[16];             /* "ADD", "SUB", "MUL", "DIV", "LOAD", "STORE" */
    char arg1[64];           /* First operand or referenced variable */
    char arg2[64];           /* Second operand (or empty for unary/LOAD/STORE) */
    char result[64];         /* Temporary variable (t1) or target variable */
    struct TAC *next;        /* Linked list pointer */
} TAC;
```

---

## 4. Key Algorithms

### 4.1 Two-Pass Semantic Analysis
1. **Pass 1 (Symbol Registration)**: Recursively traverses the AST. When visiting a block, prefixes the block name (e.g. `server.`). When encountering a declaration, verifies that the identifier is not already present in the symbol table; if already present, reports a duplicate error; otherwise, inserts the symbol.
2. **Pass 2 (Expression Checking)**: Evaluates expression node types recursively:
   - For references `@ident`, verifies that `ident` exists in the symbol table.
   - For binary operations, checks that both operands are compatible (numbers with numbers, strings with `+`).
   - Verifies that the expression type matches the declared type of the variable.

### 4.2 Constant Propagation & Folding
1. An associative map stores known constant bindings for variables and temporaries.
2. The optimizer scans TAC instructions in order:
   - For `LOAD @var`: Retrieves the constant value of `var` and binds the temporary to it.
   - For arithmetic operations: If both operands are known constants, precomputes the result immediately, binds the result temporary, and unlinks the TAC instruction.
   - For `STORE var = val`: Updates the final evaluated value in both the symbol table and the bindings map.
