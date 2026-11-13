/**
 * Project name: Implementace překladače imperativního jazyka IFJ26
 * @author Izabela Richterová (xrichti00)
 * @file scanner.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "scanner.h"
#include "stack.h"
#include "buffer.h"
#include "error.h"

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

static int line_count = 1;

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
    error_exit(1, line_count, "Invalid number");
    return -1;
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
    error_exit(1, line_count, "Invalid number");
    return -1;
}

/**
 * @brief Checks whether the EOL rule is being followed
 * @param eol_before_token Indicates whether an EOL is present before the token
 * @param rule EOL rule that must be followed
 */
void check_eol_rule (bool eol_before_token, enum eol_rules rule) {
    if ((rule == EOL_REQUIRED && !eol_before_token) || (rule == EOL_FORBIDDEN && eol_before_token)) {
        error_exit(2, line_count, "EOL rule not followed");
    }
}

/**
 * @brief Measures the indentation at the beginning of a physical line
 * @param line_indent Pointer to the variable storing the current indentation level
 * @param symbol Character read from stdin 
 * @param tab_at_beginning Pointer to a flag indicating the presence of a tab at the beginning of the physical line
 */
void measure_indentation (int* line_indent, int symbol, bool* tab_at_beginning) {
    *line_indent = 0; // indentation measurement starts at 0

    while (symbol == ' ' || symbol == '\r' || symbol == '\t') {
        if (symbol == ' ') (*line_indent)++;
        else if (symbol == '\t') *tab_at_beginning = true;
        symbol = getchar();
    }

    if (symbol != EOF) ungetc(symbol, stdin);
}

/**
 * @brief Updates the indentation stack
 * @param line_indent Pointer to the variable storing the current indentation level
 * @param indent_stack Pointer to the stack storing indentation levels
 * @param token Pointer to the token whose type is to be set to TOKEN_INDENT or TOKEN_DEDENT
 * @param symbol Last read character to be returned to stdin
 * @note A line_indent value of -1 indicates that the line indentation has already been processed 
 */
void update_indent_stack (int* line_indent, stack_t* indent_stack, token_t* token, int symbol) {
    ungetc(symbol, stdin);
    if (*line_indent > stack_top(indent_stack)) {
        stack_push(indent_stack, *line_indent);
        token->type = TOKEN_INDENT;
        *line_indent = -1;
    }
    else if (*line_indent < stack_top(indent_stack)) {
        stack_pop(indent_stack);
        if (*line_indent <= stack_top(indent_stack)) {
            token->type = TOKEN_DEDENT;
            if (*line_indent == stack_top(indent_stack)) *line_indent = -1;
        }
        else error_exit(2, line_count, "Wrong indentation");
    }
}

/**
 * @brief Handles indentation of a physical line
 * @param line_indent Pointer to the variable storing the current indentation level
 * @param indent_stack Pointer to the stack storing indentation levels
 * @param token Pointer to the token whose type is to be set to TOKEN_INDENT or TOKEN_DEDENT
 * @param symbol Last read character to be returned to stdin
 * @param tab_at_beginning Flag indicating the presence of a tab at the beginning of the physical line
 * @param logical_line_cont Indicates whether the current line is a continuation of a logical line
 * @return true if the indentation stack was updated and the token needs to be returned; false otherwise
 * @note A line_indent value of -1 indicates that the line indentation has already been processed 
 */
bool handle_indentation (int* line_indent, stack_t* indent_stack, token_t* token, int symbol, 
    bool tab_at_beginning, bool logical_line_cont) {
        
    if (tab_at_beginning) error_exit(1, line_count, "Tab can not be at the beginning of a non-empty line");
    if (logical_line_cont) {
        if (*line_indent <= stack_top(indent_stack)) {
            error_exit(2, line_count, "Invalid indentation in logical line continuation");
        }
        else *line_indent = -1;
    }
    if (!logical_line_cont) {
        if (*line_indent != stack_top(indent_stack)) {
            update_indent_stack(line_indent, indent_stack, token, symbol);
            return true;
        }
        else *line_indent = -1;
    }
    return false;
}

int get_line_number () {
    return line_count;
}

token_t get_token (enum eol_rules rule) {
    int symbol;
    token_t token = {0};
    enum states state = S_START;
    buffer_t identif_buffer, number_buffer, string_buffer;
    bool empty_string = true;
    int unended_com_cnt = 0;
    bool eol_before_token = false;
    bool lf_in_comment = false;
    static int unended_parenth_cnt = 0;
    bool logical_line_cont = false;

    static stack_t indent_stack;
    static bool stack_initialized = false;
    static int line_indent = -2; // -2: line indentation has not been processed yet
    static bool tab_at_beginning = false;
    
    while (1) {
        symbol = getchar();
        switch (state) {
            case S_START:
                if (!stack_initialized) {
                    stack_init(&indent_stack);
                    stack_push(&indent_stack, 0);
                    stack_initialized = true;
                }

                if (line_indent == -2) {
                    measure_indentation(&line_indent, symbol, &tab_at_beginning);
                    break;
                }

                if (symbol == ' ' || symbol == '\r' || symbol == '\t') break;

                else if (symbol == '\n') {
                    line_count++;
                    line_indent = -2;
                    eol_before_token = true;
                    tab_at_beginning = false;
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
                        token.after_eol = eol_before_token;
                        return token;
                    }
                    token.type = TOKEN_EOF;
                    token.after_eol = eol_before_token;
                    return token;
                }
                else if (symbol == '/') {
                    state = S_DIV;
                    break;
                }

                check_eol_rule(eol_before_token, rule);

                if (line_indent >= 0) {
                    if (handle_indentation (&line_indent, &indent_stack, &token, symbol, tab_at_beginning, logical_line_cont)) {
                        token.after_eol = eol_before_token;
                        return token;
                    }
                }

                if (symbol == '@') {
                    token.type = TOKEN_AT;
                    token.after_eol = eol_before_token;
                    return token;
                }
                else if (symbol == ':') {
                    token.type = TOKEN_COLON;
                    token.after_eol = eol_before_token;
                    return token;
                }
                else if (symbol == ';') {
                    token.type = TOKEN_SEMICOLON;
                    token.after_eol = eol_before_token;
                    return token;
                }
                else if (symbol == ',') {
                    token.type = TOKEN_COMMA;
                    token.after_eol = eol_before_token;
                    return token;
                }
                else if (symbol == '.') state = S_PERIOD;
                else if (symbol == '(') {
                    unended_parenth_cnt++;
                    token.type = TOKEN_L_PARENTH;
                    token.after_eol = eol_before_token;
                    return token;
                }
                else if (symbol == ')') {
                    if (unended_parenth_cnt > 0) unended_parenth_cnt--;
                    token.type = TOKEN_R_PARENTH;
                    token.after_eol = eol_before_token;
                    return token;
                }
                else if (symbol == '>') state = S_GT;
                else if (symbol == '<') state = S_LT;
                else if (symbol == '=') state = S_ASSIGN;
                else if (symbol == '!') state = S_NOT;
                else if (symbol == '+') {
                    token.type = TOKEN_OP_ADD;
                    token.after_eol = eol_before_token;
                    return token;
                }
                else if (symbol == '-') {
                    token.type = TOKEN_OP_SUB;
                    token.after_eol = eol_before_token;
                    return token;
                }
                else if (symbol == '*') {
                    token.type = TOKEN_OP_MUL;
                    token.after_eol = eol_before_token;
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
                    error_exit(1, line_count, "'%c' is an invalid character", symbol);
                }
                break;
            case S_GT:
                if (symbol == '=') {
                    token.type = TOKEN_OP_GE;
                    token.after_eol = eol_before_token;
                    return token;
                }
                else if (symbol != EOF) {
                    ungetc(symbol, stdin);
                }
                token.type = TOKEN_OP_GT;
                token.after_eol = eol_before_token;
                return token;
            case S_LT:
                if (symbol == '=') {
                    token.type = TOKEN_OP_LE;
                    token.after_eol = eol_before_token;
                    return token;
                }
                else if (symbol != EOF) {
                    ungetc(symbol, stdin);
                }
                token.type = TOKEN_OP_LT;
                token.after_eol = eol_before_token;
                return token;
            case S_ASSIGN:
                if (symbol == '=') {
                    token.type = TOKEN_OP_EQ;
                    token.after_eol = eol_before_token;
                    return token;
                }
                else if (symbol != EOF) {
                    ungetc(symbol, stdin);
                }
                token.type = TOKEN_ASSIGN;
                token.after_eol = eol_before_token;
                return token;
            case S_NOT:
                if (symbol == '=') {
                    token.type = TOKEN_OP_NEQ;
                    token.after_eol = eol_before_token;
                    return token;
                }
                else if (symbol != EOF) {
                    ungetc(symbol, stdin);
                }
                token.type = TOKEN_OP_NOT;
                token.after_eol = eol_before_token;
                return token;
            case S_AND:
                if (symbol == '&') {
                    token.type = TOKEN_OP_AND;
                    token.after_eol = eol_before_token;
                    return token;
                }
                else {
                    error_exit(1, line_count, "'&' alone is an invalid character");
                    break;
                }
            case S_OR:
                if (symbol == '|') {
                    token.type = TOKEN_OP_OR;
                    token.after_eol = eol_before_token;
                    return token;
                }
                else {
                    error_exit(1, line_count, "'|' alone is an invalid character");
                    break;
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
                token.after_eol = eol_before_token;
                return token;
            case S_ID_UNDERSCORE:
                if ((symbol >= 'A' && symbol <= 'Z') || (symbol >= 'a' && symbol <= 'z') || 
                (symbol >= '0' && symbol <= '9') || (symbol == '_')) {
                    state = S_IDENTIF;
                    add_symbol2buffer(&identif_buffer, symbol);
                }
                else {
                    buffer_destruct(&identif_buffer);
                    error_exit(1, line_count, "'_' alone is not a valid identifier");
                }
                break;
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
                token.after_eol = eol_before_token;
                buffer_destruct(&number_buffer);
                return token;
            case S_INT_UNDERSCORE:
                if (symbol >= '0' && symbol <= '9') {
                    state = S_INT;
                    add_symbol2buffer(&number_buffer, symbol);
                }
                else if (symbol != '_') {
                    buffer_destruct(&number_buffer);
                    error_exit(1, line_count, "'_' in a number must be followed by a digit");
                }
                break;
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
                    error_exit(1, line_count, "Nonzero literal can not begin with '0'");
                    break;
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
                token.after_eol = eol_before_token;
                return token;
            case S_ZERO_UNDERSCORE:
                if (symbol == '0') {
                    state = S_ZERO;
                }
                else if (symbol != '_') {
                    error_exit(1, line_count, "'_' in a zero literal must be followed by a '0'");
                }
                break;
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
                token.after_eol = eol_before_token;
                return token;
            case S_INT_PERIOD:
                if (symbol >= '0' && symbol <= '9') {
                    add_symbol2buffer(&number_buffer, symbol);
                    state = S_DBL_DECIMAL;
                }
                else {
                    buffer_destruct(&number_buffer);
                    error_exit(1, line_count, "'.' after a digit must be followed by another digit");
                }
                break;
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
                token.after_eol = eol_before_token;
                buffer_destruct(&number_buffer);
                return token;
            case S_DBL_DECIMAL_UNDERSCORE:
                if (symbol >= '0' && symbol <= '9') {
                    add_symbol2buffer(&number_buffer, symbol);
                    state = S_DBL_DECIMAL;
                }
                else if (symbol != '_') {
                    buffer_destruct(&number_buffer);
                    error_exit(1, line_count, "'_' in a number must be followed by a digit");
                }
                break;
            case S_EXPONENT:
                if (symbol >= '0' && symbol <= '9') {
                    add_symbol2buffer(&number_buffer, symbol);
                    state = S_DBL_EXP;
                }
                else if (symbol == '+' || symbol == '-') {
                    add_symbol2buffer(&number_buffer, symbol);
                    state = S_DBL_EXP_SIGN;
                }
                else {
                    buffer_destruct(&number_buffer);
                    error_exit(1, line_count, "Exponent must be followed by a digit or a sign");
                }
                break;
            case S_DBL_EXP_SIGN:
                if (symbol >= '0' && symbol <= '9') {
                    add_symbol2buffer(&number_buffer, symbol);
                    state = S_DBL_EXP;
                }
                else {
                    buffer_destruct(&number_buffer);
                    error_exit(1, line_count, "A sign in an exponent must be followed by a digit");
                }
                break;
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
                token.after_eol = eol_before_token;
                buffer_destruct(&number_buffer);
                return token;
            case S_DBL_EXP_UNDERSCORE:
                if (symbol >= '0' && symbol <= '9') {
                    add_symbol2buffer(&number_buffer, symbol);
                    state = S_DBL_EXP;
                }
                else if (symbol != '_') {
                    buffer_destruct(&number_buffer);
                    error_exit(1, line_count, "'_' in a number must be followed by a digit");
                }
                break;
            case S_STR:
                if (symbol == '\\') {
                    empty_string = false;
                    state = S_ESC_SEQ;
                }
                else if (symbol == '"') {
                    if (!empty_string) {
                        add_symbol2buffer(&string_buffer, '\0');
                        token.type = TOKEN_STR_LITERAL;
                        token.data.str_value = string_buffer.data;
                        token.after_eol = eol_before_token;
                        return token;
                    }
                    else {
                        state = S_STR_END;
                    }
                }
                else if (symbol == '\n' || symbol == EOF) {
                    buffer_destruct(&string_buffer);
                    error_exit(1, line_count, "String not ended correctly");
                }
                else {
                    empty_string = false;
                    add_symbol2buffer(&string_buffer, symbol);
                }
                break;
            case S_ESC_SEQ:
                switch (symbol) {
                    case '\\':
                        add_symbol2buffer(&string_buffer, '\\');
                        state = S_STR; 
                        break;
                    case '"':
                        add_symbol2buffer(&string_buffer, '"');
                        state = S_STR;
                        break;
                    case 'n':
                        add_symbol2buffer(&string_buffer, '\n');
                        state = S_STR;
                        break;
                    case 'r':
                        add_symbol2buffer(&string_buffer, '\r');
                        state = S_STR;
                        break;
                    case 't':
                        add_symbol2buffer(&string_buffer, '\t');
                        state = S_STR;
                        break;
                    default: 
                        buffer_destruct(&string_buffer);
                        error_exit(1, line_count, "Unsupported escape sequence");
                        break;
                }
                break;
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
                token.after_eol = eol_before_token;
                return token;
            case S_MULTI_LN_STR:
                if (symbol == EOF) {
                    buffer_destruct(&string_buffer);
                    error_exit(1, line_count, "String not ended correctly");
                }
                else if (symbol == '"') {
                    state = S_MULTI_LN_STR_END1;
                }
                else {
                    if (symbol == '\n') line_count++;
                    add_symbol2buffer(&string_buffer, symbol);
                }
                break;
            case S_MULTI_LN_STR_END1:
                if (symbol == '"') {
                    state = S_MULTI_LN_STR_END2;
                }
                else if (symbol != EOF) {
                    if (symbol == '\n') line_count++;
                    add_symbol2buffer(&string_buffer, '"');
                    add_symbol2buffer(&string_buffer, symbol);
                    state = S_MULTI_LN_STR;
                }
                else {
                    buffer_destruct(&string_buffer);
                    error_exit(1, line_count, "String not ended correctly");
                }
                break;
            case S_MULTI_LN_STR_END2:
                if (symbol == '"') {
                    add_symbol2buffer(&string_buffer, '\0');
                    token.type = TOKEN_STR_LITERAL;
                    token.data.str_value = string_buffer.data;
                    token.after_eol = eol_before_token;
                    return token;
                }
                else if (symbol != EOF) {
                    if (symbol == '\n') line_count++;
                    add_symbol2buffer(&string_buffer, '"');
                    add_symbol2buffer(&string_buffer, '"');
                    add_symbol2buffer(&string_buffer, symbol);
                    state = S_MULTI_LN_STR;
                }
                else {
                    buffer_destruct(&string_buffer);
                    error_exit(1, line_count, "String not ended correctly");
                }
                break;
            case S_DIV:
                if (symbol == '/') {
                    state = S_LINE_COM;
                    break;
                }
                else if (symbol == '*') {
                    state = S_BLOCK_COM;
                    unended_com_cnt++;
                    lf_in_comment = false;
                    break;
                }
                else if (symbol != EOF) {
                    ungetc (symbol, stdin);
                }
                check_eol_rule(eol_before_token, rule);
                if (line_indent >= 0) {
                    if (handle_indentation (&line_indent, &indent_stack, &token, '/', tab_at_beginning, logical_line_cont)) {
                        token.after_eol = eol_before_token;
                        return token;
                    }
                }
                token.type = TOKEN_OP_DIV;
                token.after_eol = eol_before_token;
                return token;
            case S_LINE_COM:
                if (symbol == '\n' || symbol == EOF) {
                    if (symbol == '\n') {
                        line_count++;
                        line_indent = -2;
                        tab_at_beginning = false;
                    }
                    eol_before_token = true;
                    if (unended_parenth_cnt != 0) logical_line_cont = true;
                    state = S_START;
                    break;
                }
                else break;
            case S_BLOCK_COM:
                if (symbol == '*') {
                    state = S_BLOCK_COM_END;
                }
                else if (symbol == '/') {
                    state = S_NESTED_COM;
                }
                else if (symbol == EOF) {
                    error_exit(1, line_count, "Block comment ended incorrectly");
                }
                else if (symbol == '\n') {
                    line_count++;
                    line_indent = -2;
                    lf_in_comment = true;
                }
                break;
            case S_BLOCK_COM_END:
                if (symbol == '/') {
                    unended_com_cnt--;
                    if (unended_com_cnt == 0) {
                        if (lf_in_comment) {
                            state = S_AFTER_BLOCK_COM;
                            break;
                        }
                        else {
                            state = S_START;
                            break;
                        }
                    }
                }
                else if (symbol == '*') break;
                else if (symbol == EOF) {
                    error_exit(1, line_count, "Block comment ended incorrectly");
                    break;
                }
                else if (symbol == '\n') {
                    line_count++;
                    line_indent = -2;
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
                    error_exit(1, line_count, "Block comment ended incorrectly");
                    break;
                }
                else if (symbol == '\n') {
                    line_count++;
                    line_indent = -2;
                    lf_in_comment = true;
                }
                state = S_BLOCK_COM;
                break;
            case S_AFTER_BLOCK_COM:
                if (symbol == '\n' || symbol == EOF) {
                    if (symbol == '\n') {
                        line_count++;
                        line_indent = -2;
                        tab_at_beginning = false;
                    }
                    eol_before_token = true;
                    if (unended_parenth_cnt != 0) logical_line_cont = true;
                    state = S_START;
                    break;
                }
                else if (symbol == ' ' || symbol == '\t' || symbol == '\r') break;
                else {
                   error_exit(1, line_count, "Multi-line block comment can be followed by whitespaces only");
                   break;
                }
        }
    }
}
