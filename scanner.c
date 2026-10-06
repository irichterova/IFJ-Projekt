#include <stdio.h>

enum token_types {
    TOKEN_IDENTIFIER,
    TOKEN_KEYWORD,
    TOKEN_INT_LITERAL,
    TOKEN_DBL_LITERAL,
    TOKEN_STR_LITERAL,
    TOKEN_L_PARENTH,
    TOKEN_R_PARENTH,
    TOKEN_OP_ADD,
    TOKEN_OP_SUB,
    TOKEN_OP_MUL,
    TOKEN_OP_DIV,
    TOKEN_OP_L,
    TOKEN_OP_LE,
    TOKEN_OP_G,
    TOKEN_OP_GE,
    TOKEN_OP_EQ,
    TOKEN_OP_NEQ,
    TOKEN_ASSIGN,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_PERIOD,
    TOKEN_AT,
    TOKEN_EOF,
};

enum keywords_enum {
    KEYWORD_DEF,
    KEYWORD_DO,
    KEYWORD_DOUBLE,
    KEYWORD_ELSE,
    KEYWORD_IF,
    KEYWORD_IMPORT,
    KEYWORD_INT,
    KEYWORD_RETURN,
    KEYWORD_STRING,
    KEYWORD_THEN,
    KEYWORD_UNIT,
    KEYWORD_VAL,
    KEYWORD_VAR,
    KEYWORD_WHILE,
};

union token_data {
    int int_value;
    double dbl_value;
    char* str_value;
    enum keywords_enum keyword;
    // + symtable pointer
};

typedef struct token {
    enum token_types type;
    union token_data data;
} token_t;

char* keywords_arr[] = {
    "def", 
    "do", 
    "Double", 
    "else",
    "if",
    "import",
    "Int",
    "return",
    "String",
    "then",
    "Unit",
    "val",
    "var",
    "while"
};
