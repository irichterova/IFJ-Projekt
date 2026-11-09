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
static void parse_params_next(function_struct *func);

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
 * @brief adds parameter at end of array of parameters in function
 * @param func function in which parameter is added
 * @param p_name name of parameter
 * @param p_type type of parameter
 */
static void add_param(function_struct *func, char *p_name, value_type p_type) {
    params_struct *array_param =
        realloc(func->param_args, sizeof(params_struct) * (func->param_argc + 1));
    if (array_param == NULL) {
        error_exit(ERR_INTERNAL, 0, "Param allocation goes wrong!");
    }
    func->param_args = array_param;
    func->param_args[func->param_argc].param_name = p_name;
    func->param_args[func->param_argc].param_type = p_type;
    func->param_args[func->param_argc].codegen_id = 0; // temporary, semantic will assign
    func->param_argc++;
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
 * @brief checks syntax of next parameter after comma
 * @param func struct in which params are stored
 */
static void parse_params_rest(function_struct *func) {
    if (token.type == TOKEN_IDENTIFIER) {
        char *id_name = token.data.id;
        next_token();
        type_t_check(TOKEN_COLON);
        value_type param_type = parse_type();
        add_param(func, id_name, param_type);
        parse_params_next(func);
    }
}

/**
 * @brief checks whether there is comma after parameter in function
 * @param func struct in which params are stored
 */
static void parse_params_next(function_struct *func) {
    if (token.type == TOKEN_COMMA) {
        next_token();
        parse_params_rest(func);
    }
}

/**
 * @brief checks syntax of parameters in function
 * @param func struct in which params are stored
 */
static void parse_params(function_struct *func) {
    if (token.type == TOKEN_IDENTIFIER) {
        char *id_name = token.data.id;
        next_token();
        type_t_check(TOKEN_COLON);
        value_type param_type = parse_type();
        add_param(func, id_name, param_type);
        parse_params_next(func);
    }
}

/**
 * @brief checks syntax of optional eol after main
 */
static void parse_optional_eol(void) {
    if (token.type == TOKEN_NL) {
        next_token();
    }
}

/**
 * @brief checks syntax of optional main
 * @return if there is main (1) or not (0)
 */
static int parse_optional_main(void) {
    if (token.type == TOKEN_AT) {
        next_token();
        if (token.type == TOKEN_IDENTIFIER && strcmp(token.data.id, "main") == 0) {
            free(token.data.id);
            next_token();
            parse_optional_eol();
            return 1;
        } else {
            error_exit(ERR_SYN, 0, "Identifier doesn't match with string main!");
        }
    }
    return 0;
}

/**
 * @brief checks syntax of definition of function
 * @return function_struct func with is_main, func_name and declared_ret_type
 */
static function_struct *parse_func(void) {
    int main_found = parse_optional_main();
    function_struct *func = calloc(1, sizeof(function_struct));
    if (func == NULL) {
        error_exit(ERR_INTERNAL, 0, "Function allocation goes wrong!");
    }
    func->is_main = main_found;
    keyword_t_check(KEYWORD_DEF);
    if (token.type == TOKEN_IDENTIFIER) {
        func->func_name = token.data.id;
        next_token();
        type_t_check(TOKEN_L_PARENTH);
        parse_params(func);
        type_t_check(TOKEN_R_PARENTH);
        func->declared_ret_type = parse_optional_type();
        type_t_check(TOKEN_ASSIGN);
        // TODO: implementation of parse_body
        // temporary body:
        if (token.type == TOKEN_IDENTIFIER) {
            free(token.data.id);
        }
        next_token();
        type_t_check(TOKEN_NL);
        return func;
    } else {
        error_exit(ERR_SYN, 0, "Missing identifier after 'def'!");
        return NULL;
    }
}

/**
 * @brief checks syntax of list of function or functions
 */
static void parse_func_list(void) {
    if (token.type == TOKEN_AT ||
        (token.type == TOKEN_KEYWORD && token.data.keyword == KEYWORD_DEF)) {
        parse_func();
        parse_func_list();
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
    parse_func_list();
    if (token.type != TOKEN_EOF) {
        error_exit(ERR_SYN, 0, "Expected end of file!");
    }
    return NULL;
}