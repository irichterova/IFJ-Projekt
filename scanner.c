/**
 * Project name: Implementace překladače imperativního jazyka IFJ26
 * @author Izabela Richterová (xrichti00)
 * @file scanner.c
 */

#include <stdio.h>
#include <stdlib.h>
#include "scanner.h"

enum states {
    S_START,
    S_GT,
    S_LT,
    S_ASSIGN,
    S_NOT,
};

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
                else if (symbol == '@') {
                    token.type = TOKEN_AT;
                    return token;
                }
                else if (symbol == '>') state = S_GT;
                else if (symbol == '<') state = S_LT;
                else if (symbol == '=') state = S_ASSIGN;
                else if (symbol == '!') state = S_NOT;
                else if (symbol == '+') {
                    token.type = TOKEN_OP_ADD;
                    return token;
                }
                else if (symbol == '-') {
                    token.type = TOKEN_OP_SUB;
                    return token;
                }
                else if (symbol == '*') {
                    token.type = TOKEN_OP_MUL;
                    return token;
                }
                else if (symbol == '/') {
                    token.type = TOKEN_OP_DIV;
                    return token;
                }
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
