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
static void parse_while(void);
static void parse_if(void);

/**
 * @brief selects next token from scanner
 * @param eol applies one of the three rules that tells if eol is required, optional or forbidden
 */
static void next_token(enum eol_rules eol) {
    token = get_token(eol);
}

/**
 * @brief checks token type, if type doesn't match exits with syntax error
 * @param t_type enum of types that is compared with current token type
 * @param eol applies one of the three rules that tells if eol is required, optional or forbidden
 */
static void type_t_check(enum token_types t_type, enum eol_rules eol) {
    if (token.type == t_type) {
        next_token(eol);
    } else {
        error_exit(ERR_SYN, get_line_number(), "Type of token doesn't match!");
    }
}

/**
 * @brief checks token keyword, if keyword doesn't match exits with syntax error
 * @param t_keyword enum of keywords that is compared with current token keyword
 * @param eol applies one of the three rules that tells if eol is required, optional or forbidden
 */
static void keyword_t_check(enum keywords_enum t_keyword, enum eol_rules eol) {
    if (token.type == TOKEN_KEYWORD && token.data.keyword == t_keyword) {
        next_token(eol);
    } else {
        error_exit(ERR_SYN, get_line_number(), "Keyword of token doesn't match!");
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
        error_exit(ERR_INTERNAL, get_line_number(), "Param allocation goes wrong!");
    }
    func->param_args = array_param;
    func->param_args[func->param_argc].param_name = p_name;
    func->param_args[func->param_argc].param_type = p_type;
    func->param_args[func->param_argc].codegen_id = 0; // temporary, semantic will assign
    func->param_argc++;
}

/**
 * @brief allocates statement node by statement type
 * @param line line number where statement starts
 * @param type type of statement from enum
 * @return allocated statement_struct
 */
static statement_struct *create_stmt(int line, statement_type type) {
    statement_struct *stmt = calloc(1, sizeof(statement_struct));
    if (stmt == NULL) {
        error_exit(ERR_INTERNAL, line, "Memory allocation failed!");
    }
    stmt->line = line;
    stmt->stmt_type = type;
    return stmt;
}

// temporary, PSA will replace
static expr_struct *parse_expr_temp(void) {
    if (token.type == TOKEN_IDENTIFIER) {
        free(token.data.id);
    }
    next_token(EOL_OPTIONAL);
    return NULL;
}

/**
 * @brief checks syntax of statement end, it needs to be semicolon or eol
 */
static void parse_statement_end(void) {
    if (token.type == TOKEN_SEMICOLON) {
        next_token(EOL_OPTIONAL);
    } else if (!token.after_eol && token.type != TOKEN_DEDENT) {
        error_exit(ERR_SYN, get_line_number(), "Expected eol or semicolon after statement!");
    }
}

/**
 * @brief checks syntax of token type keyword (Int, Double, String, Unit)
 * @param eol applies one of the three rules that tells if eol is required, optional or forbidden
 * @return returns value_type that matches keyword
 */
static value_type parse_type(enum eol_rules eol) {
    if (token.type == TOKEN_KEYWORD) {
        switch (token.data.keyword) {
        case KEYWORD_INT:
            next_token(eol);
            return TYPE_INT;
        case KEYWORD_DOUBLE:
            next_token(eol);
            return TYPE_DOUBLE;
        case KEYWORD_STRING:
            next_token(eol);
            return TYPE_STRING;
        case KEYWORD_UNIT:
            next_token(eol);
            return TYPE_UNIT;
        default:
            error_exit(ERR_SYN, get_line_number(), "Token type doesn't match any type!");
        }
    } else {
        error_exit(ERR_SYN, get_line_number(), "Expected keyword type!");
    }
    return TYPE_UNKNOWN;
}

/**
 * @brief checks syntax of optional type after colon
 * @return returns type of token from parse_type, returns unknown if there isn't colon
 */
static value_type parse_optional_type(void) {
    if (token.type == TOKEN_COLON) {
        next_token(EOL_FORBIDDEN);
        return parse_type(EOL_FORBIDDEN);
    } else {
        return TYPE_UNKNOWN;
    }
}

/**
 * @brief checks syntax of next parameter after comma and trailing comma
 * @param func struct in which params are stored
 */
static void parse_params_rest(function_struct *func) {
    if (token.type == TOKEN_IDENTIFIER) {
        char *id_name = token.data.id;
        next_token(EOL_OPTIONAL);
        type_t_check(TOKEN_COLON, EOL_OPTIONAL);
        value_type param_type = parse_type(EOL_OPTIONAL);
        add_param(func, id_name, param_type);
        parse_params_next(func);
    } else {
        // Checks trailing comma if there is no identifier after comma
        if (!token.after_eol && token.type == TOKEN_R_PARENTH) {
            error_exit(ERR_SYN, get_line_number(), "After trailing comma has to be end of line!");
        }
    }
}

/**
 * @brief checks whether there is comma after parameter in function
 * @param func struct in which params are stored
 */
static void parse_params_next(function_struct *func) {
    if (token.type == TOKEN_COMMA) {
        next_token(EOL_OPTIONAL);
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
        next_token(EOL_OPTIONAL);
        type_t_check(TOKEN_COLON, EOL_OPTIONAL);
        value_type param_type = parse_type(EOL_OPTIONAL);
        add_param(func, id_name, param_type);
        parse_params_next(func);
    }
}

/**
 * @brief checks syntax of optional main
 * @return if there is main (1) or not (0)
 */
static int parse_optional_main(void) {
    if (token.type == TOKEN_AT) {
        next_token(EOL_FORBIDDEN);
        if (token.type == TOKEN_IDENTIFIER && strcmp(token.data.id, "main") == 0) {
            free(token.data.id);
            next_token(EOL_OPTIONAL); // checks syntax of opt_eol
            return 1;
        } else {
            error_exit(ERR_SYN, get_line_number(), "Identifier doesn't match with string main!");
        }
    }
    return 0;
}

/**
 * @brief checks syntax of statement val or var
 * @return statement node var/val
 */
static statement_struct *parse_def_var(void) {
    statement_type va_type;
    if (token.data.keyword == KEYWORD_VAL) {
        va_type = STMT_VAL;
    } else {
        va_type = STMT_VAR;
    }
    statement_struct *stmt = create_stmt(get_line_number(), va_type);
    next_token(EOL_FORBIDDEN);
    if (token.type == TOKEN_IDENTIFIER) {
        stmt->data.var_stmt.id_name = token.data.id;
        next_token(EOL_FORBIDDEN);
    } else {
        error_exit(ERR_SYN, get_line_number(), "Expected identifier after val/var!");
    }
    stmt->data.var_stmt.declared_type = parse_optional_type();
    type_t_check(TOKEN_ASSIGN, EOL_FORBIDDEN);
    stmt->data.var_stmt.init_expr = parse_expr_temp();
    parse_statement_end();
    return stmt;
}

/**
 * @brief checks syntax of statement return with optional expression
 * @return statement node return
 */
static statement_struct *parse_return(void) {
    statement_struct *stmt = create_stmt(get_line_number(), STMT_RETURN);
    next_token(EOL_OPTIONAL);
    if (!token.after_eol && token.type != TOKEN_SEMICOLON && token.type != TOKEN_DEDENT) {
        stmt->data.ret_stmt = parse_expr_temp();
    }
    parse_statement_end();
    return stmt;
}

/**
 * @brief checks syntax of statement beginning with id
 * @param id_token first id that will be given to PSA
 * @param line line number where statement starts
 * @return statement node id
 */
static statement_struct *parse_statement_id(token_t id_token, int line) {
    statement_struct *stmt;
    if (token.type == TOKEN_ASSIGN && !token.after_eol) {
        stmt = create_stmt(line, STMT_ASSIGN);
        stmt->data.id_stmt.id_name = id_token.data.id;
        next_token(EOL_FORBIDDEN);
        stmt->data.id_stmt.assign_expr = parse_expr_temp();
    } else {
        stmt = create_stmt(line, STMT_EXPR);
        free(id_token.data.id);
        while (token.type != TOKEN_SEMICOLON && token.type != TOKEN_DEDENT &&
               token.type != TOKEN_EOF && !token.after_eol) {
            if (token.type == TOKEN_IDENTIFIER) {
                free(token.data.id);
            }
            next_token(EOL_OPTIONAL);
        }
    }
    return stmt;
}

/**
 * @brief checks syntax of statement in body
 */
static void parse_statement(void) {
    if (token.type == TOKEN_KEYWORD) {
        switch (token.data.keyword) {
        case KEYWORD_VAL:
        case KEYWORD_VAR:
            parse_def_var();
            break;
        case KEYWORD_RETURN:
            parse_return();
            break;
        case KEYWORD_WHILE:
            parse_while();
            break;
        case KEYWORD_IF:
            parse_if();
            break;
        default:
            error_exit(ERR_SYN, get_line_number(), "Unexpected keyword!");
        }
    } else {
        if (token.type == TOKEN_IDENTIFIER) {
            token_t id_token = token;
            int line = get_line_number();
            next_token(EOL_OPTIONAL);
            parse_statement_id(id_token, line);
            parse_statement_end();
        } else {
            parse_expr_temp();
            parse_statement_end();
        }
    }
}

/**
 * @brief checks syntax of remaining statements until dedent
 */
static void parse_statement_next(void) {
    if (token.type != TOKEN_DEDENT) {
        parse_statement();
        parse_statement_next();
    }
}

/**
 * @brief checks syntax of indented block in statements body, while, if and else
 */
static void parse_block(void) {
    type_t_check(TOKEN_INDENT, EOL_OPTIONAL);
    parse_statement();
    parse_statement_next();
    type_t_check(TOKEN_DEDENT, EOL_OPTIONAL);
}

/**
 * @brief checks syntax of else branch of if statement
 */
static void parse_else(void) {
    keyword_t_check(KEYWORD_ELSE, EOL_REQUIRED);
    parse_block();
}

/**
 * @brief checks syntax of statement if
 */
static void parse_if(void) {
    next_token(EOL_FORBIDDEN);
    parse_expr_temp();
    keyword_t_check(KEYWORD_THEN, EOL_REQUIRED);
    parse_block();
    parse_else();
}

/**
 * @brief checks syntax of statement while loop
 */
static void parse_while(void) {
    next_token(EOL_FORBIDDEN);
    parse_expr_temp();
    keyword_t_check(KEYWORD_DO, EOL_REQUIRED);
    parse_block();
}

/**
 * @brief checks syntax of body in function
 */
static void parse_body(void) {
    if (token.after_eol) {
        parse_block();
    } else {
        if (token.type == TOKEN_IDENTIFIER) {
            free(token.data.id);
        }
        next_token(EOL_REQUIRED);
    }
}

/**
 * @brief checks syntax of definition of function
 * @return function_struct func with is_main, func_name and declared_ret_type
 */
static function_struct *parse_func(void) {
    int main_found = parse_optional_main();
    function_struct *func = calloc(1, sizeof(function_struct));
    if (func == NULL) {
        error_exit(ERR_INTERNAL, get_line_number(), "Function allocation goes wrong!");
    }
    func->is_main = main_found;
    keyword_t_check(KEYWORD_DEF, EOL_FORBIDDEN);
    if (token.type == TOKEN_IDENTIFIER) {
        func->func_name = token.data.id;
        next_token(EOL_FORBIDDEN);
        type_t_check(TOKEN_L_PARENTH, EOL_OPTIONAL);
        parse_params(func);
        type_t_check(TOKEN_R_PARENTH, EOL_FORBIDDEN);
        func->declared_ret_type = parse_optional_type();
        type_t_check(TOKEN_ASSIGN, EOL_OPTIONAL);
        parse_body();
        return func;
    } else {
        error_exit(ERR_SYN, get_line_number(), "Missing identifier after 'def'!");
        return NULL;
    }
}

/**
 * @brief checks syntax of list of function or functions
 * @return first function with pointer to next function, NULL if none
 */
static function_struct *parse_func_list(void) {
    if (token.type == TOKEN_AT ||
        (token.type == TOKEN_KEYWORD && token.data.keyword == KEYWORD_DEF)) {
        function_struct *func = parse_func();
        func->next = parse_func_list();
        return func;
    } else {
        return NULL;
    }
}

/**
 * @brief checks syntax of prolog (import ifj26.*)
 */
static void parse_prolog(void) {
    keyword_t_check(KEYWORD_IMPORT, EOL_FORBIDDEN);
    if (token.type == TOKEN_IDENTIFIER && strcmp(token.data.id, "ifj26") == 0) {
        free(token.data.id);
        next_token(EOL_FORBIDDEN);
    } else {
        error_exit(ERR_SYN, get_line_number(), "Identifier doesn't match with string ifj26!");
    }
    type_t_check(TOKEN_PERIOD, EOL_FORBIDDEN);
    type_t_check(TOKEN_OP_MUL, EOL_REQUIRED);
}

program_struct *parse_program(void) {
    program_struct *program = calloc(1, sizeof(program_struct));
    if (program == NULL) {
        error_exit(ERR_INTERNAL, get_line_number(), "Program allocation goes wrong!");
    }
    next_token(EOL_OPTIONAL);
    parse_prolog();
    program->first_func = parse_func_list();
    if (token.type != TOKEN_EOF) {
        error_exit(ERR_SYN, get_line_number(), "Expected end of file!");
    }
    return program;
}