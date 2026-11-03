/**
 * Project name: Compiler for IFJ26
 * @author Filip Bůžek, xbuzekf00
 * @file parser.c
 */

#include "parser.h"
#include "error.h"
#include "scanner.h"

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
