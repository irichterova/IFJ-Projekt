#include <stdio.h>
#include <stdlib.h>

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
    TOKEN_OP_LT,
    TOKEN_OP_LE,
    TOKEN_OP_GT,
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

enum states {
    S_START,
    S_GT,
    S_LT,
    S_ASSIGN,
    S_NOT,
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

token_t get_token() {
    int symbol;
    token_t token = {0};
    enum states state;
    state = S_START;
    while (1) {
        symbol = getchar();
        switch (state) {
            case S_START:
                if (symbol == EOF) {
                    token.type = TOKEN_EOF;
                    return token;
                }
                else if (symbol == '>') state = S_GT;
                else if (symbol == '<') state = S_LT;
                else if (symbol == '=') state = S_ASSIGN;
                else if (symbol == '!') state = S_NOT;
                break;
            case S_GT:
                if (symbol == '=') {
                    token.type = TOKEN_OP_GE;
                    return token;
                }
                else if (symbol == EOF) {
                    token.type = TOKEN_OP_GT;
                    return token;
                }
                else {
                    ungetc(symbol, stdin);
                    token.type = TOKEN_OP_GT;
                    return token;
                }
            case S_LT:
                if (symbol == '=') {
                    token.type = TOKEN_OP_LE;
                    return token;
                }
                else if (symbol == EOF) {
                    token.type = TOKEN_OP_LT;
                    return token;
                }
                else {
                    ungetc(symbol, stdin);
                    token.type = TOKEN_OP_LT;
                    return token;
                }
            case S_ASSIGN:
                if (symbol == '=') {
                    token.type = TOKEN_OP_EQ;
                    return token;
                }
                else if (symbol == EOF) {
                    token.type = TOKEN_ASSIGN;
                    return token;
                }
                else {
                    ungetc(symbol, stdin);
                    token.type = TOKEN_ASSIGN;
                    return token;
                }
            case S_NOT:
                if (symbol == '=') {
                    token.type = TOKEN_OP_NEQ;
                    return token;
                }
                else {
                    fprintf(stderr, "Error: expected '=' after '!'\n");
                    exit(1);
                }
        }
    }
}
