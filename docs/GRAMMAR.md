# ConfigLang: Formal Grammar Specification

**Course**: Principles of Compiler Design (4CS501CC25)  
**Authors**: Urva Gandhi (`23BCE078`) & Rakshit Gajnotar (`23BCE077`)  
**Institution**: Institute of Technology, Nirma University  

---

## 1. Lexical Grammar (Tokens)

The lexical specification is implemented using **GNU Flex** (`src/lexer/lexer.l`).

### 1.1 Regular Definitions
```lex
digit           [0-9]
letter          [a-zA-Z_]
ident           {letter}({letter}|{digit})*(\.{letter}({letter}|{digit})*)*
int_lit         {digit}+
float_lit       {digit}+\.{digit}+
string_lit      \"([^"\\]|\\.)*\"
whitespace      [ \t\r]+
newline         \n
```

### 1.2 Keywords & Tokens
| Token | Pattern | Meaning |
|---|---|---|
| `KW_INT` | `int` | Integer type keyword |
| `KW_FLOAT` | `float` | Float type keyword |
| `KW_STRING` | `string` | String type keyword |
| `KW_BOOL` | `bool` | Boolean type keyword |
| `KW_TRUE` | `true` | Boolean true literal |
| `KW_FALSE` | `false` | Boolean false literal |
| `INT_LIT` | `[0-9]+` | Integer literal |
| `FLOAT_LIT` | `[0-9]+\.[0-9]+` | Floating-point literal |
| `STRING_LIT` | `\"([^"\\]|\\.)*\"` | Quoted string literal |
| `IDENT` | `[a-zA-Z_][a-zA-Z0-9_]*` | Identifier / dotted path |
| `DOTDOT` | `..` | Range constraint operator |
| `'='` | `=` | Assignment operator |
| `':'` | `:` | Type annotation separator |
| `';'` | `;` | Statement terminator |
| `'@'` | `@` | Variable reference prefix |
| `'{'`, `'}'` | `{`, `}` | Block delimiters |
| `'+'`, `'-'` | `+`, `-` | Arithmetic addition & subtraction / unary minus |
| `'*'`, `'/'` | `*`, `/` | Arithmetic multiplication & division |
| `'('`, `')'` | `(`, `)` | Parenthesized grouping |

---

## 2. Context-Free Grammar (Syntax)

The syntax specification is implemented using **GNU Bison** (`src/parser/parser.y`).

### 2.1 Grammar Rules in BNF
```bnf
program
    ::= stmt_list

stmt_list
    ::= stmt stmt_list
      | ε

stmt
    ::= decl
      | block

block
    ::= IDENT '{' stmt_list '}'

decl
    ::= IDENT ':' type_spec opt_range '=' expr ';'
      | IDENT '=' expr ';'

type_spec
    ::= 'int'
      | 'float'
      | 'string'
      | 'bool'

opt_range
    ::= '(' expr '..' expr ')'
      | ε

expr
    ::= expr '+' expr
      | expr '-' expr
      | expr '*' expr
      | expr '/' expr
      | '-' expr
      | '(' expr ')'
      | '@' IDENT
      | INT_LIT
      | FLOAT_LIT
      | STRING_LIT
      | 'true'
      | 'false'
```

### 2.2 Operator Precedence and Associativity
Operator precedence is declared in Bison to eliminate ambiguity:
```yacc
%left '+' '-'
%left '*' '/'
%nonassoc UMINUS
```
1. `*` and `/` have higher precedence than `+` and `-`.
2. All binary arithmetic operators are left-associative.
3. Unary minus (`-`) has the highest precedence (`%nonassoc UMINUS`).
