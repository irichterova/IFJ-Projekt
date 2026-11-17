/**
 * Project name: Implementation of an IFJ26 Imperative Language Compiler
 * @author Izabela Richterová (xrichti00)
 * @file scanner.h
 * @brief Declares token structures and functions
 */

#ifndef SCANNER_H
#define SCANNER_H

#include <stdbool.h>

enum token_types {
    TOKEN_IDENTIFIER,
    TOKEN_KEYWORD,
    TOKEN_INT_LITERAL,
    TOKEN_DBL_LITERAL,
    TOKEN_STR_LITERAL,
    TOKEN_TRUE_LITERAL,
    TOKEN_FALSE_LITERAL,
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
    TOKEN_OP_NOT,
    TOKEN_OP_AND,
    TOKEN_OP_OR,
    TOKEN_GENERATOR_ARROW,
    TOKEN_FUNCTION_ARROW,
    TOKEN_ASSIGN,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_PERIOD,
    TOKEN_AT,
    TOKEN_EOF,
    TOKEN_INDENT,
    TOKEN_DEDENT,
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
    KEYWORD_BOOLEAN,
    KEYWORD_FOR,
};

enum eol_rules {
    EOL_REQUIRED,
    EOL_OPTIONAL,
    EOL_FORBIDDEN,
};

union token_data {
    int int_value;
    double dbl_value;
    char* str_value;    // needs to be freed by the caller
    char* id;           // needs to be freed by the caller
    enum keywords_enum keyword;
};

typedef struct token {
    enum token_types type;
    union token_data data;
    bool after_eol;     // indicates whether there was an EOL before the token
} token_t;

/**
 * @brief Returns the current line number
 * @return Number of the current line
 */
int get_line_number ();

/**
 * @brief Recognizes the next lexeme and represents it using the data structure "token"
 * @param rule  EOL rule specifying whether an EOL is required, optional, or forbidden
 * @return Data structure "token" representing the recognized lexeme
 */
token_t get_token (enum eol_rules rule);

#endif