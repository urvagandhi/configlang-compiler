# ConfigLang: Formal Grammar Specification

**Course**: PCD (Principles of Compiler Design - 4CS501CC25)  
**Authors**: Urva Gandhi (`23BCE078`) & Rakshit Gajnotar (`23BCE077`)  

---

## 1. Lexical Grammar (Tokens)

The lexical specification is implemented using GNU Flex (`src/lexer.l`).

### 1.1 Regular Definitions
```lex
digit           [0-9]
letter          [a-zA-Z_]
ident           {letter}({letter}|{digit})*
int_lit         {digit}+
float_lit       {digit}+\.{digit}+([eE][+-]?{digit}+)?
whitespace      [ \t\r\f]+
newline         \n
```

### 1.2 Keywords & Punctuation
| Token | Lexeme Pattern | Description |
|:---|:---|:---|
| `KW_INT` | `int` | 64-bit signed integer type |
| `KW_FLOAT` | `float` | 64-bit IEEE double precision float |
| `KW_STRING` | `string` | UTF-8 string literal type |
| `KW_BOOL` | `bool` | Boolean type |
| `KW_TRUE` | `true` | Boolean true literal |
| `KW_FALSE` | `false` | Boolean false literal |
| `DOTDOT` | `..` | Range separator |
| `AT` | `@` | Reference prefix operator |
| `ASSIGN` | `=` | Assignment operator |
| `COLON` | `:` | Type annotation delimiter |
| `SEMI` | `;` | Statement terminator |
| `DOT` | `.` | Scoped path delimiter |
| `LBRACE` | `{` | Scope block begin |
| `RBRACE` | `}` | Scope block end |
| `LPAREN` | `(` | Range/expression grouping begin |
| `RPAREN` | `)` | Range/expression grouping end |
| `PLUS` | `+` | Addition / string concatenation |
| `MINUS` | `-` | Subtraction / unary negation |
| `STAR` | `*` | Multiplication |
| `SLASH` | `/` | Division |

### 1.3 Disambiguation & Longest Match
- **Range Slice vs Float**: In `1..65535`, the sequence `1.` is followed immediately by another `.`. Flex's maximal-munch scanning and priority ordering ensures that `..` is recognized as `DOTDOT`, producing `INT_LIT(1)`, `DOTDOT`, `INT_LIT(65535)`.
- **String Literals**: Enclosed in double quotes `"...""`. Handled via a start condition `<STR>` which collects characters and translates escape sequences (`\n`, `\t`, `\r`, `\"`, `\\`). Invalid escape sequences report `E104`.
- **Comments**: Single-line comments (`//...`) discard characters until newline. Multi-line block comments (`/* ... */`) discard characters until `*/`, or report `E105` if EOF is reached.

---

## 2. Context-Free Grammar (Syntax)

The syntax specification is implemented using GNU Bison (`src/parser.y`) with `%expect 0`.

### 2.1 Grammar in EBNF / BNF Form

```bnf
program
    : item_list
    | /* empty */
    ;

item_list
    : item_list item
    | item
    ;

item
    : declaration
    | block
    | error SEMI        /* Error recovery rule */
    ;

block
    : IDENT LBRACE item_list RBRACE
    | IDENT LBRACE RBRACE
    ;

declaration
    : IDENT COLON type range_opt ASSIGN expr SEMI
    | IDENT ASSIGN expr SEMI
    ;

type
    : KW_INT
    | KW_FLOAT
    | KW_STRING
    | KW_BOOL
    ;

range_opt
    : LPAREN range_bound DOTDOT range_bound RPAREN
    | /* empty */
    ;

range_bound
    : INT_LIT
    | FLOAT_LIT
    | MINUS INT_LIT
    | MINUS FLOAT_LIT
    ;

expr
    : expr PLUS expr
    | expr MINUS expr
    | expr STAR expr
    | expr SLASH expr
    | MINUS expr %prec UMINUS
    | LPAREN expr RPAREN
    | literal
    | reference
    ;

literal
    : INT_LIT
    | FLOAT_LIT
    | STRING_LIT
    | KW_TRUE
    | KW_FALSE
    ;

reference
    : AT path
    ;

path
    : path DOT IDENT
    | IDENT
    ;
```

---

## 3. Operator Precedence & Associativity

Ambiguity in arithmetic expressions is resolved using Bison precedence declarations:

| Precedence | Associativity | Operator Tokens | Description |
|:---:|:---:|:---:|:---|
| **1 (Lowest)** | Left | `PLUS`, `MINUS` | Binary addition, subtraction, string concatenation |
| **2** | Left | `STAR`, `SLASH` | Binary multiplication, division |
| **3 (Highest)** | Right | `UMINUS` (Unary `MINUS`) | Arithmetic negation |

### Conflict Elimination
By expressing operator precedence directly via `%left` and `%right` and maintaining distinct rules for typed declarations and untyped assignments, the LALR(1) state machine has **0 shift/reduce conflicts** and **0 reduce/reduce conflicts**.
