/**
 * Project name: Implementace překladače imperativního jazyka IFJ26
 * @author Izabela Richterová (xrichti00)
 * @file scanner.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scanner.h"

enum states {
    S_START,
    S_GT,
    S_LT,
    S_ASSIGN,
    S_NOT,
    S_IDENTIF,
    S_UNDERSCORE,
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

typedef struct buffer {
    char* data;
    int size;
    int capacity;
} buffer_t;

/**
 * @brief Initializes the data structure "buffer"
 * @param buffer Pointer to the buffer to be initialized
 */
void buffer_init (buffer_t* buffer) {
    buffer->capacity = 10;
    buffer->size = 0;
    buffer->data = malloc (buffer->capacity * sizeof (char));
    if (!(buffer->data)) exit (99);
}

/**
 * @brief Doubles the capacity of the buffer 
 * @param buffer Pointer to the buffer whose capacity is to be increased
 */
void buffer_capacity_increase (buffer_t* buffer) {
    char* new_data = realloc (buffer->data, buffer->capacity * 2 * sizeof(char));
    if (!new_data) exit (99);
    buffer->data = new_data;
    buffer->capacity = buffer->capacity * 2;
}

/**
 * @brief Adds the symbol to the buffer
 * @param buffer Pointer to the buffer the symbol is to be added to
 * @param symbol The character to be added to the buffer
 */
void add_symbol2buffer (buffer_t* buffer, char symbol) {
    if (buffer->size == buffer->capacity) buffer_capacity_increase(buffer);
    buffer->data[buffer->size] = symbol;
    buffer->size++;
}

/**
 * @brief Destroys the data structure "buffer" 
 * @param buffer Pointer to the buffer to be destroyed
 */
void buffer_destruct (buffer_t *buffer) {
    buffer->capacity = 0;
    buffer->size = 0;
    free(buffer->data); 
    buffer->data = NULL;
}

/**
 * @brief Checks if the identifier is a keyword
 * @param identifier String ending with '\0' representing the identifier to be checked
 * @return The index of the keyword if found; -1 if not found
 */
int compare_with_keywords (char* identifier) {
    int keywords_arr_size = sizeof(keywords_arr) / sizeof(keywords_arr[0]);
    for (int i = 0; i < keywords_arr_size; i++) {
        if (strcmp(keywords_arr[i], identifier) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Recognizes a lexeme from stdin and represents it with data structure "token"
 * @return Data structure "token" 
 */
token_t get_token() {
    int symbol;
    token_t token = {0};
    enum states state;
    state = S_START;
    buffer_t identif_buffer;
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
                else if (symbol == ':') {
                    token.type = TOKEN_COLON;
                    return token;
                }
                else if (symbol == ';') {
                    token.type = TOKEN_SEMICOLON;
                    return token;
                }
                else if (symbol == ',') {
                    token.type = TOKEN_COMMA;
                    return token;
                }
                else if (symbol == '.') {
                    token.type = TOKEN_PERIOD;
                    return token;
                }
                else if (symbol == '(') {
                    token.type = TOKEN_L_PARENTH;
                    return token;
                }
                else if (symbol == ')') {
                    token.type = TOKEN_R_PARENTH;
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
                else if ((symbol >= 'A' && symbol <= 'Z') || (symbol >= 'a' && symbol <= 'z')) {
                    state = S_IDENTIF;
                    buffer_init(&identif_buffer);
                    add_symbol2buffer(&identif_buffer, symbol);
                }
                else if (symbol == '_') {
                    state = S_UNDERSCORE;
                    buffer_init(&identif_buffer);
                    add_symbol2buffer(&identif_buffer, symbol);
                }
                break;
            case S_GT:
                if (symbol == '=') {
                    token.type = TOKEN_OP_GE;
                    return token;
                }
                else if (symbol != EOF) {
                    ungetc(symbol, stdin);
                }
                token.type = TOKEN_OP_GT;
                return token;
            case S_LT:
                if (symbol == '=') {
                    token.type = TOKEN_OP_LE;
                    return token;
                }
                else if (symbol != EOF) {
                    ungetc(symbol, stdin);
                }
                token.type = TOKEN_OP_LT;
                return token;
            case S_ASSIGN:
                if (symbol == '=') {
                    token.type = TOKEN_OP_EQ;
                    return token;
                }
                else if (symbol != EOF) {
                    ungetc(symbol, stdin);
                }
                token.type = TOKEN_ASSIGN;
                return token;
            case S_NOT:
                if (symbol == '=') {
                    token.type = TOKEN_OP_NEQ;
                    return token;
                }
                else {
                    fprintf(stderr, "Error: expected '=' after '!'\n");
                    exit(1);
                }
            case S_IDENTIF:
                if ((symbol >= 'A' && symbol <= 'Z') || (symbol >= 'a' && symbol <= 'z') || 
                (symbol >= '0' && symbol <= '9') || (symbol == '_')) {
                    add_symbol2buffer(&identif_buffer, symbol);
                    break;
                }
                else if (symbol != EOF) {
                    ungetc(symbol, stdin);
                }
                add_symbol2buffer(&identif_buffer, '\0');
                int keyword = compare_with_keywords(identif_buffer.data);
                if (keyword != -1) {
                    token.type = TOKEN_KEYWORD;
                    token.data.keyword = keyword;
                }
                else {
                    token.type = TOKEN_IDENTIFIER;
                }
                buffer_destruct(&identif_buffer);
                return token;
            case S_UNDERSCORE:
                if ((symbol >= 'A' && symbol <= 'Z') || (symbol >= 'a' && symbol <= 'z') || 
                (symbol >= '0' && symbol <= '9') || (symbol == '_')) {
                    state = S_IDENTIF;
                    add_symbol2buffer(&identif_buffer, symbol);
                    break;
                }
                else {
                    fprintf(stderr, "Error: '_' is an invalid character\n");
                    exit(1);
                }
        }
    }
}
