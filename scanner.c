/**
 * Project name: Implementace překladače imperativního jazyka IFJ26
 * @author Izabela Richterová (xrichti00)
 * @file scanner.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "scanner.h"
#include "stack.h"
#include "buffer.h"

enum states {
    S_START,
    S_GT,
    S_LT,
    S_ASSIGN,
    S_NOT,
    S_AND,
    S_OR,
    S_DIV,
    S_IDENTIF,
    S_ID_UNDERSCORE,
    S_INT,
    S_INT_UNDERSCORE,
    S_ZERO,
    S_ZERO_UNDERSCORE,
    S_PERIOD,
    S_INT_PERIOD,
    S_DBL_DECIMAL,
    S_DBL_DECIMAL_UNDERSCORE,
    S_EXPONENT,
    S_DBL_EXP,
    S_DBL_EXP_SIGN,
    S_DBL_EXP_UNDERSCORE,
    S_STR,
    S_ESC_SEQ,
    S_STR_END,
    S_MULTI_LN_STR,
    S_MULTI_LN_STR_END1,
    S_MULTI_LN_STR_END2,
    S_LINE_COM,
    S_BLOCK_COM,
    S_BLOCK_COM_END,
    S_NESTED_COM,
    S_AFTER_BLOCK_COM,
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
    "while",
    "Boolean",
};

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
 * @brief Converts the string to integer 
 * @param string String ending with '\0' to be converted to integer
 * @return Converted integer value if the conversion was successful
 */
int convert_str2int (char* string) {
    errno = 0;
    char* endptr;
    long long number = strtoll(string, &endptr, 10);
    if (*endptr == '\0' && errno == 0 && number <= INT_MAX) return number;
    fprintf(stderr, "Error: Invalid number\n");
    exit(1);   
}

/**
 * @brief Converts the string to double
 * @param string String ending with '\0'to be converted to double
 * @return Converted double value if the conversion was successful
 */
double convert_str2dbl (char* string) {
    errno = 0;
    char* endptr;
    double number = strtod(string, &endptr);
    if (*endptr == '\0' && errno == 0) return number;
    fprintf(stderr, "Error: Invalid number\n");
    exit(1);
}

/**
 * @brief Checks whether the EOL rule is being followed
 * @param eol_before_token Indicates whether an EOL is present before the token
 * @param rule EOL rule that must be followed
 */
void check_eol_rule (bool eol_before_token, enum eol_rules rule) {
    if ((rule == EOL_REQUIRED && !eol_before_token) || (rule == EOL_FORBIDDEN && eol_before_token)) {
        fprintf(stderr, "Error: EOL rule not followed\n");
        exit(2);
    }
}

/**
 * @brief Updates the indentation stack
 * @param indent_stack Indentation stack to be updated
 * @param space_count Number of spaces at the beginning of the physical line
 * @param symbol Character most recently read from stdin
 * @param token Token whose type may be set to TOKEN_INDENT or TOKEN_DEDENT
 */
void update_indent_stack (stack_t* indent_stack, int space_count, int symbol, token_t* token) {
    if (space_count > stack_top(indent_stack)) {
        ungetc(symbol, stdin);
        stack_push(indent_stack, space_count);
        token->type = TOKEN_INDENT;
    }
    else if (space_count < stack_top(indent_stack)) {
        ungetc(symbol, stdin);
        stack_pop(indent_stack);
        if (space_count <= stack_top(indent_stack)) token->type = TOKEN_DEDENT;
        else {
            fprintf(stderr, "Error: Wrong indentation\n");
            exit(2);
        }
    }
}

/**
 * @brief Handles indentation and checks indentation rules
 * @param tab_at_beginning Indicates whether a tab is at the beginning of a non-empty line
 * @param logical_line_cont Indicates whether the current line is a logical line continuation
 * @param symbol Character most recently read from stdin
 * @param eol_before_token Indicates whether an EOL is present before the token
 * @param space_count Number of spaces at the beginning of the physical line
 * @param indent_stack Stack used to track levels of indentation
 * @param file_start Indicates whether the beginning of the file is being processed
 * @param pending_indentation Indicates whether indentation is waiting to be processed
 * @param token Token whose type may be set to TOKEN_INDENT or TOKEN_DEDENT
 * @return true if indentation was processed; false otherwise
 */
bool handle_indentation (bool tab_at_beginning, bool logical_line_cont, int symbol, bool eol_before_token, 
    int space_count, stack_t* indent_stack, bool file_start, bool* pending_indentation, token_t* token) {
    if (tab_at_beginning) {
        fprintf(stderr, "Error: Tab can not be at the beginning of a non-empty line\n");
        exit(1);
    }

    if (logical_line_cont && (symbol != ')') && eol_before_token && (space_count <= stack_top(indent_stack))) {
        fprintf(stderr, "Error: Invalid indentation in logical line continuation\n");
        exit(2);
    }

    if (!logical_line_cont) {
        if ((file_start || eol_before_token) && space_count != stack_top(indent_stack)) {
            *pending_indentation = true;
        }

        if (*pending_indentation) {
            update_indent_stack(indent_stack, space_count, symbol, token);

            if (space_count == stack_top(indent_stack)) *pending_indentation = false;

            return true;
        }
    }
    return false;
}

/**
 * @brief Recognizes a lexeme from stdin and represents it with data structure "token"
 * @param rule EOL rule determining the permission for the presence of EOL
 * @return Data structure "token" 
 */
token_t get_token(enum eol_rules rule) {
    int symbol;
    token_t token = {0};
    enum states state = S_START;
    buffer_t identif_buffer, number_buffer, string_buffer;
    bool empty_string = true;
    int unended_com_cnt = 0;
    bool eol_before_token = false;
    bool lf_in_comment = false;
    static bool file_start = true;
    static int space_count = 0;
    static stack_t indent_stack;
    static bool stack_initialized = false;
    static bool pending_indentation = false;
    static int unended_parenth_cnt = 0;
    bool logical_line_cont = false;
    bool tab_at_beginning = false;
    while (1) {
        symbol = getchar();
        switch (state) {
            case S_START:
                if (!stack_initialized) {
                    stack_init (&indent_stack);
                    stack_initialized = true;
                    stack_push(&indent_stack, 0);
                }

                if (symbol == ' ') {
                    if (file_start || eol_before_token) {
                        space_count++;
                    }
                    break;
                }
                else if (symbol == '\r') break;
                else if (symbol == '\t') {
                    if (file_start || eol_before_token) {
                        tab_at_beginning = true;
                    }
                    break;
                }
                else if (symbol == '\n') {
                    eol_before_token = true;
                    tab_at_beginning = false;
                    space_count = 0;
                    if (unended_parenth_cnt != 0) {
                        logical_line_cont = true;
                    }
                    break;
                }
                else if (symbol == EOF) {
                    eol_before_token = true;
                    check_eol_rule(eol_before_token, rule);
                    if (stack_top(&indent_stack) != 0) {
                        stack_pop(&indent_stack);
                        token.type = TOKEN_DEDENT;
                        return token;
                    }
                    token.type = TOKEN_EOF;
                    return token;
                }
                else if (symbol == '/') {
                    state = S_DIV;
                    break;
                }

                check_eol_rule(eol_before_token, rule);

                if (handle_indentation (tab_at_beginning, logical_line_cont, symbol, eol_before_token, 
                    space_count, &indent_stack, file_start, &pending_indentation, &token)) return token;      

                file_start = false;

                if (symbol == '@') {
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
                else if (symbol == '.') state = S_PERIOD;
                else if (symbol == '(') {
                    unended_parenth_cnt++;
                    token.type = TOKEN_L_PARENTH;
                    return token;
                }
                else if (symbol == ')') {
                    if (unended_parenth_cnt > 0) unended_parenth_cnt--;
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
                else if (symbol == '&') state = S_AND;
                else if (symbol == '|') state = S_OR;
                else if ((symbol >= 'A' && symbol <= 'Z') || (symbol >= 'a' && symbol <= 'z')) {
                    state = S_IDENTIF;
                    buffer_init(&identif_buffer);
                    add_symbol2buffer(&identif_buffer, symbol);
                }
                else if (symbol == '_') {
                    state = S_ID_UNDERSCORE;
                    buffer_init(&identif_buffer);
                    add_symbol2buffer(&identif_buffer, symbol);
                }
                else if (symbol >= '1' && symbol <= '9') {
                    state = S_INT;
                    buffer_init(&number_buffer);
                    add_symbol2buffer(&number_buffer, symbol);
                }
                else if (symbol == '0') state = S_ZERO;
                else if (symbol == '"') {
                    state = S_STR;
                    buffer_init(&string_buffer);
                }
                else {
                    fprintf(stderr, "Error: Invalid character\n");
                    exit(1);
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
                else if (symbol != EOF) {
                    ungetc(symbol, stdin);
                }
                token.type = TOKEN_OP_NOT;
                return token;
            case S_AND:
                if (symbol == '&') {
                    token.type = TOKEN_OP_AND;
                    return token;
                }
                else {
                    fprintf(stderr, "Error: '&' alone is an invalid character\n");
                    exit(1);
                }
            case S_OR:
                if (symbol == '|') {
                    token.type = TOKEN_OP_OR;
                    return token;
                }
                else {
                    fprintf(stderr, "Error: '|' alone is an invalid character\n");
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
                    buffer_destruct(&identif_buffer);
                }
                else {
                    if (strcmp("true", identif_buffer.data) == 0) {
                        buffer_destruct(&identif_buffer);
                        token.type = TOKEN_TRUE_LITERAL;
                    }
                    else if (strcmp("false", identif_buffer.data) == 0) {
                        buffer_destruct(&identif_buffer);
                        token.type = TOKEN_FALSE_LITERAL;
                    }
                    else {
                        token.type = TOKEN_IDENTIFIER;
                        token.data.id = identif_buffer.data;
                    }
                }
                return token;
            case S_ID_UNDERSCORE:
                if ((symbol >= 'A' && symbol <= 'Z') || (symbol >= 'a' && symbol <= 'z') || 
                (symbol >= '0' && symbol <= '9') || (symbol == '_')) {
                    state = S_IDENTIF;
                    add_symbol2buffer(&identif_buffer, symbol);
                    break;
                }
                else {
                    buffer_destruct(&identif_buffer);
                    fprintf(stderr, "Error: '_' alone is not a valid identifier\n");
                    exit(1);
                }
            case S_INT:
                if (symbol >= '0' && symbol <= '9') {
                    add_symbol2buffer(&number_buffer, symbol);
                    break;
                }
                else if (symbol == '_') {
                    state = S_INT_UNDERSCORE;
                    break;
                }
                else if (symbol == '.') {
                    add_symbol2buffer(&number_buffer, symbol);
                    state = S_INT_PERIOD;
                    break;
                }
                else if (symbol == 'e' || symbol == 'E') {
                    add_symbol2buffer(&number_buffer, symbol);
                    state = S_EXPONENT;
                    break;
                }
                else if (symbol != EOF) {
                    ungetc(symbol, stdin);
                }
                add_symbol2buffer(&number_buffer, '\0');
                token.type = TOKEN_INT_LITERAL;
                token.data.int_value = convert_str2int(number_buffer.data);
                buffer_destruct(&number_buffer);
                return token;
            case S_INT_UNDERSCORE:
                if (symbol >= '0' && symbol <= '9') {
                    state = S_INT;
                    add_symbol2buffer(&number_buffer, symbol);
                    break;
                }
                else if (symbol == '_') break;
                else {
                    buffer_destruct(&number_buffer);
                    fprintf(stderr, "Error: '_' in a number must be followed by a digit\n");
                    exit(1);
                }
            case S_ZERO:
                if (symbol == '.') {
                    buffer_init(&number_buffer);
                    add_symbol2buffer(&number_buffer, '0');
                    add_symbol2buffer(&number_buffer, symbol);
                    state = S_INT_PERIOD;
                    break;
                }
                else if (symbol == 'e' || symbol == 'E') {
                    buffer_init(&number_buffer);
                    add_symbol2buffer(&number_buffer, '0');
                    add_symbol2buffer(&number_buffer, symbol);
                    state = S_EXPONENT;
                    break;
                }
                else if (symbol >= '1' && symbol <= '9') {
                    fprintf(stderr, "Error: Nonzero literal can not begin with '0'\n");
                    exit(1);
                }
                else if (symbol == '0') break;
                else if (symbol == '_') {
                    state = S_ZERO_UNDERSCORE;
                    break;
                }
                else if (symbol != EOF) {
                    ungetc(symbol, stdin);
                }
                token.type = TOKEN_INT_LITERAL;
                token.data.int_value = 0;
                return token;
            case S_ZERO_UNDERSCORE:
                if (symbol == '0') {
                    state = S_ZERO;
                    break;
                }
                else if (symbol == '_') break;
                else {
                    fprintf(stderr, "Error: '_' in a zero literal must be followed by a '0'\n");
                    exit(1);
                }
            case S_PERIOD:
                if (symbol >= '0' && symbol <= '9') {
                    buffer_init(&number_buffer);
                    add_symbol2buffer(&number_buffer, '.');
                    add_symbol2buffer(&number_buffer, symbol);
                    state = S_DBL_DECIMAL;
                    break;
                }
                else if (symbol != EOF) {
                    ungetc(symbol, stdin);
                }
                token.type = TOKEN_PERIOD;
                return token;
            case S_INT_PERIOD:
                if (symbol >= '0' && symbol <= '9') {
                    add_symbol2buffer(&number_buffer, symbol);
                    state = S_DBL_DECIMAL;
                    break;
                }
                else {
                    buffer_destruct(&number_buffer);
                    fprintf(stderr, "Error: '.' after a digit must be followed by another digit\n");
                    exit(1);
                }
            case S_DBL_DECIMAL:
                if (symbol >= '0' && symbol <= '9') {
                    add_symbol2buffer(&number_buffer, symbol);
                    break;
                }
                else if (symbol == '_') {
                    state = S_DBL_DECIMAL_UNDERSCORE;
                    break;
                }
                else if (symbol == 'e' || symbol == 'E') {
                    add_symbol2buffer(&number_buffer, symbol);
                    state = S_EXPONENT;
                    break;
                }
                else if (symbol != EOF && symbol != 'd' && symbol != 'D') {
                    ungetc(symbol, stdin);
                }
                add_symbol2buffer(&number_buffer, '\0');
                token.type = TOKEN_DBL_LITERAL;
                token.data.dbl_value = convert_str2dbl(number_buffer.data);
                buffer_destruct(&number_buffer);
                return token;
            case S_DBL_DECIMAL_UNDERSCORE:
                if (symbol >= '0' && symbol <= '9') {
                    add_symbol2buffer(&number_buffer, symbol);
                    state = S_DBL_DECIMAL;
                    break;
                }
                else if (symbol == '_') break;
                else {
                    buffer_destruct(&number_buffer);
                    fprintf(stderr, "Error: '_' in a number must be followed by a digit\n");
                    exit(1);
                }
            case S_EXPONENT:
                if (symbol >= '0' && symbol <= '9') {
                    add_symbol2buffer(&number_buffer, symbol);
                    state = S_DBL_EXP;
                    break;
                }
                else if (symbol == '+' || symbol == '-') {
                    add_symbol2buffer(&number_buffer, symbol);
                    state = S_DBL_EXP_SIGN;
                    break;
                }
                else {
                    buffer_destruct(&number_buffer);
                    fprintf(stderr, "Error: Exponent must be followed by a digit or a sign\n");
                    exit(1);
                }
            case S_DBL_EXP_SIGN:
                if (symbol >= '0' && symbol <= '9') {
                    add_symbol2buffer(&number_buffer, symbol);
                    state = S_DBL_EXP;
                    break;
                }
                else {
                    buffer_destruct(&number_buffer);
                    fprintf(stderr, "Error: A sign in an exponent must be followed by a digit\n");
                    exit(1);
                }
            case S_DBL_EXP:
                if (symbol >= '0' && symbol <= '9') {
                    add_symbol2buffer(&number_buffer, symbol);
                    break;
                }
                else if (symbol == '_') {
                    state = S_DBL_EXP_UNDERSCORE;
                    break;
                }
                else if (symbol != EOF && symbol != 'd' && symbol != 'D'){
                    ungetc(symbol, stdin);
                }
                add_symbol2buffer(&number_buffer, '\0');
                token.type = TOKEN_DBL_LITERAL;
                token.data.dbl_value = convert_str2dbl(number_buffer.data);
                buffer_destruct(&number_buffer);
                return token;
            case S_DBL_EXP_UNDERSCORE:
                if (symbol >= '0' && symbol <= '9') {
                    add_symbol2buffer(&number_buffer, symbol);
                    state = S_DBL_EXP;
                    break;
                }
                else if (symbol == '_') break;
                else {
                    buffer_destruct(&number_buffer);
                    fprintf(stderr, "Error: '_' in a number must be followed by a digit\n");
                    exit(1);
                }
            case S_STR:
                if (symbol == '\\') {
                    empty_string = false;
                    state = S_ESC_SEQ;
                    break;
                }
                else if (symbol == '"') {
                    if (!empty_string) {
                        add_symbol2buffer(&string_buffer, '\0');
                        token.type = TOKEN_STR_LITERAL;
                        token.data.str_value = string_buffer.data;
                        return token;
                    }
                    else {
                        state = S_STR_END;
                        break;
                    }
                }
                else if (symbol == '\n' || symbol == EOF) {
                    buffer_destruct(&string_buffer);
                    fprintf(stderr, "Error: String not ended correctly\n");
                    exit(1);
                }
                else {
                    empty_string = false;
                    add_symbol2buffer(&string_buffer, symbol);
                    break;
                }
            case S_ESC_SEQ:
                if (symbol == '\\') {
                    add_symbol2buffer(&string_buffer, '\\');
                    state = S_STR;
                    break;
                }
                else if (symbol == '"') {
                    add_symbol2buffer(&string_buffer, '"');
                    state = S_STR;
                    break;
                }
                else if (symbol == 'n') {
                    add_symbol2buffer(&string_buffer, '\n');
                    state = S_STR;
                    break;
                }
                else if (symbol == 'r') {
                    add_symbol2buffer(&string_buffer, '\r');
                    state = S_STR;
                    break;
                }
                else if (symbol == 't') {
                    add_symbol2buffer(&string_buffer, '\t');
                    state = S_STR;
                    break;
                }
                else {
                    buffer_destruct(&string_buffer);
                    fprintf(stderr, "Error: Unsupported escape sequence\n");
                    exit(1);
                }
            case S_STR_END:
                if (symbol == '"') {
                    state = S_MULTI_LN_STR;
                    break;
                }
                else if (symbol != EOF) {
                    ungetc(symbol, stdin);
                }
                add_symbol2buffer(&string_buffer, '\0');
                token.type = TOKEN_STR_LITERAL;
                token.data.str_value = string_buffer.data;
                return token;
            case S_MULTI_LN_STR:
                if (symbol == EOF) {
                    buffer_destruct(&string_buffer);
                    fprintf(stderr, "Error: String not ended correctly\n");
                    exit(1);
                }
                else if (symbol == '"') {
                    state = S_MULTI_LN_STR_END1;
                    break;
                }
                else {
                    add_symbol2buffer(&string_buffer, symbol);
                    break;
                }
            case S_MULTI_LN_STR_END1:
                if (symbol == '"') {
                    state = S_MULTI_LN_STR_END2;
                    break;
                }
                else if (symbol != EOF) {
                    add_symbol2buffer(&string_buffer, '"');
                    add_symbol2buffer(&string_buffer, symbol);
                    state = S_MULTI_LN_STR;
                    break;
                }
                else {
                    buffer_destruct(&string_buffer);
                    fprintf(stderr, "Error: String not ended correctly\n");
                    exit(1);
                }
            case S_MULTI_LN_STR_END2:
                if (symbol == '"') {
                    add_symbol2buffer(&string_buffer, '\0');
                    token.type = TOKEN_STR_LITERAL;
                    token.data.str_value = string_buffer.data;
                    return token;
                }
                else if (symbol != EOF) {
                    add_symbol2buffer(&string_buffer, '"');
                    add_symbol2buffer(&string_buffer, '"');
                    add_symbol2buffer(&string_buffer, symbol);
                    state = S_MULTI_LN_STR;
                    break;
                }
                else {
                    buffer_destruct(&string_buffer);
                    fprintf(stderr, "Error: String not ended correctly\n");
                    exit(1);
                }
            case S_DIV:
                if (symbol == '/') {
                    state = S_LINE_COM;
                    break;
                }
                else if (symbol == '*') {
                    state = S_BLOCK_COM;
                    unended_com_cnt++;
                    break;
                }
                else if (symbol != EOF) {
                    ungetc (symbol, stdin);
                }
                check_eol_rule(eol_before_token, rule);
                if (handle_indentation (tab_at_beginning, logical_line_cont, symbol, eol_before_token, 
                    space_count, &indent_stack, file_start, &pending_indentation, &token)) return token;
                file_start = false;
                token.type = TOKEN_OP_DIV;
                return token;
            case S_LINE_COM:
                if (symbol == '\n' || symbol == EOF) {
                    eol_before_token = true;
                    tab_at_beginning = false;
                    space_count = 0;
                    if (unended_parenth_cnt != 0) logical_line_cont = true;
                    state = S_START;
                    break;
                }
                else break;
            case S_BLOCK_COM:
                if (symbol == '*') {
                    state = S_BLOCK_COM_END;
                    break;
                }
                else if (symbol == '/') {
                    state = S_NESTED_COM;
                    break;
                }
                else if (symbol == EOF) {
                    fprintf(stderr, "Error: Block comment ended incorrectly\n");
                    exit(1);
                }
                else if (symbol == '\n') {
                    lf_in_comment = true;
                    break;
                }
                else break;
            case S_BLOCK_COM_END:
                if (symbol == '/') {
                    unended_com_cnt--;
                    if (unended_com_cnt == 0) {
                        if (lf_in_comment) {
                            state = S_AFTER_BLOCK_COM;
                            break;
                        }
                        else {
                            file_start = false;
                            state = S_START;
                            break;
                        }
                    }
                }
                else if (symbol == '*') break;
                else if (symbol == EOF) {
                    fprintf(stderr, "Error: Block comment ended incorrectly\n");
                    exit(1);
                }
                else if (symbol == '\n') {
                    lf_in_comment = true;
                }
                state = S_BLOCK_COM;
                break;
            case S_NESTED_COM:
                if (symbol == '*') {
                    unended_com_cnt++;
                    state = S_BLOCK_COM;
                    break;
                }
                if (symbol == '/') break;
                else if (symbol == EOF) {
                    fprintf(stderr, "Error: Block comment ended incorrectly\n");
                    exit(1);
                }
                else if (symbol == '\n') {
                    lf_in_comment = true;
                }
                state = S_BLOCK_COM;
                break;
            case S_AFTER_BLOCK_COM:
                if (symbol == '\n' || symbol == EOF) {
                    eol_before_token = true;
                    tab_at_beginning = false;
                    space_count = 0;
                    if (unended_parenth_cnt != 0) logical_line_cont = true;
                    state = S_START;
                    break;
                }
                else if (symbol == ' ' || symbol == '\t' || symbol == '\r') break;
                else {
                   fprintf(stderr, "Error: Multi-line block comment can not be followed by a token\n");
                   exit(1);
                }
        }
    }
}
