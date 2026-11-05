/**
 * Project name: Compiler for IFJ26
 * @author Filip Bůžek, xbuzekf00
 * @file parser.c
 */

#include "parser.h"
#include "error.h"
#include "scanner.h"
#include <stdlib.h>
#include <string.h>

static token_t token;

/**
 * @brief selects next token from scanner
 */
static void next_token(void) {
    token = get_token();
}

/**
 * @brief checks token type, if type doesn't match exits with syntax error
 * @param t_type enum of types that is compared with current token type
 */
static void type_t_check(enum token_types t_type) {
    if (token.type == t_type) {
        next_token();
    } else {
        error_exit(ERR_SYN, 0, "Type of token doesn't match!");
    }
}

/**
 * @brief checks token keyword, if keyword doesn't match exits with syntax error
 * @param t_keyword enum of keywords that is compared with current token keyword
 */
static void keyword_t_check(enum keywords_enum t_keyword) {
    if (token.type == TOKEN_KEYWORD && token.data.keyword == t_keyword) {
        next_token();
    } else {
        error_exit(ERR_SYN, 0, "Keyword of token doesn't match!");
    }
}

/**
 * @brief checks syntax of token type keyword (Int, Double, String, Unit)
 * @return returns value_type that matches keyword
 */
static value_type parse_type(void) {
    if (token.type == TOKEN_KEYWORD) {
        switch (token.data.keyword) {
        case KEYWORD_INT:
            next_token();
            return TYPE_INT;
        case KEYWORD_DOUBLE:
            next_token();
            return TYPE_DOUBLE;
        case KEYWORD_STRING:
            next_token();
            return TYPE_STRING;
        case KEYWORD_UNIT:
            next_token();
            return TYPE_UNIT;
        default:
            error_exit(ERR_SYN, 0, "Token type doesn't match any type!");
        }
    } else {
        error_exit(ERR_SYN, 0, "Expected keyword type!");
    }
    return TYPE_UNKNOWN;
}

/**
 * @brief checks syntax of optional type after colon
 * @return returns type of token from parse_type, returns unknown if there isn't colon
 */
static value_type parse_optional_type(void) {
    if (token.type == TOKEN_COLON) {
        next_token();
        return parse_type();
    } else {
        return TYPE_UNKNOWN;
    }
}

/**
 * @brief checks syntax of prolog (import ifj26.*)
 */
static void parse_prolog(void) {
    keyword_t_check(KEYWORD_IMPORT);
    if (token.type == TOKEN_IDENTIFIER && strcmp(token.data.id, "ifj26") == 0) {
        free(token.data.id);
        next_token();
    } else {
        error_exit(ERR_SYN, 0, "Identifier doesn't match with string ifj26!");
    }
    type_t_check(TOKEN_PERIOD);
    type_t_check(TOKEN_OP_MUL);
    type_t_check(TOKEN_NL);
}

program_struct *parse_program(void) {
    next_token();
    parse_prolog();
    return NULL;
}