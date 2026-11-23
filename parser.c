/**
 * Project name: Compiler for IFJ26
 * @author Filip Bůžek, xbuzekf00
 * @file parser.c
 */

#include "parser.h"
#include "error.h"
#include "expression.h"
#include "scanner.h"
#include <stdlib.h>
#include <string.h>

static token_t token;
static void parse_params_next(function_struct *func);
static statement_struct *parse_while(void);
static statement_struct *parse_if(void);
static expr_struct *parse_expr_or_if(void);
static statement_struct *parse_for(void);

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

/**
 * @brief checks syntax of statement end, it needs to be semicolon or eol
 * @param allow_else if there can be else statement
 */
static void parse_statement_end(bool allow_else) {
    if (token.type == TOKEN_SEMICOLON) {
        next_token(EOL_OPTIONAL);
    } else if (!token.after_eol && token.type != TOKEN_DEDENT &&
               !(allow_else && token.type == TOKEN_KEYWORD && token.data.keyword == KEYWORD_ELSE)) {
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
        case KEYWORD_BOOLEAN:
            next_token(eol);
            return TYPE_BOOL;
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
    stmt->data.var_stmt.init_expr = parse_expr_or_if();
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
        stmt->data.ret_stmt = parse_expr_or_if();
    }
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
        stmt->data.id_stmt.assign_expr = parse_expr_or_if();
    } else {
        stmt = create_stmt(line, STMT_EXPR);
        stmt->data.expr_stmt = parse_expr_rest(id_token, token, &token);
    }
    return stmt;
}

/**
 * @brief checks syntax of statement in body
 * @return statement node
 */
static statement_struct *parse_statement(void) {
    if (token.type == TOKEN_KEYWORD) {
        switch (token.data.keyword) {
        case KEYWORD_VAL:
        case KEYWORD_VAR:
            return parse_def_var();
        case KEYWORD_RETURN:
            return parse_return();
        case KEYWORD_WHILE:
            return parse_while();
        case KEYWORD_IF:
            return parse_if();
        case KEYWORD_FOR:
            return parse_for();
        default:
            error_exit(ERR_SYN, get_line_number(), "Unexpected keyword!");
        }
    } else {
        if (token.type == TOKEN_IDENTIFIER) {
            token_t id_token = token;
            int line = get_line_number();
            next_token(EOL_OPTIONAL);
            statement_struct *stmt = parse_statement_id(id_token, line);
            return stmt;
        } else {
            statement_struct *stmt = create_stmt(get_line_number(), STMT_EXPR);
            stmt->data.expr_stmt = parse_expr(token, &token);
            return stmt;
        }
    }
    return NULL;
}

/**
 * @brief checks whereas if ends with expression
 * @param stmt if statement which we check
 * @return if ends with expression returns true, if not returns false
 */
static bool ends_with_if_expr(statement_struct *stmt) {
    expr_struct *expr = NULL;
    switch (stmt->stmt_type) {
    case STMT_VAL:
    case STMT_VAR:
        expr = stmt->data.var_stmt.init_expr;
        break;
    case STMT_ASSIGN:
        expr = stmt->data.id_stmt.assign_expr;
        break;
    case STMT_RETURN:
        expr = stmt->data.ret_stmt;
        break;
    case STMT_EXPR:
        expr = stmt->data.expr_stmt;
        break;
    default:
        return false;
    }
    if (expr != NULL && expr->node_type == EXPR_IF) {
        return true;
    }
    return false;
}

/**
 * @brief checks syntax of remaining statements until dedent
 * @return rest of statement node, NULL if none
 */
static statement_struct *parse_statement_next(void) {
    if (token.type != TOKEN_DEDENT) {
        statement_struct *stmt = parse_statement();
        if (stmt->stmt_type != STMT_IF && stmt->stmt_type != STMT_WHILE &&
            !ends_with_if_expr(stmt) && stmt->stmt_type != STMT_FOR) {
            parse_statement_end(false);
        }
        stmt->next = parse_statement_next();
        return stmt;
    }
    return NULL;
}

/**
 * @brief checks syntax of indented block in statements body, while, if and else
 * @return first statement of block
 */
static statement_struct *parse_block(void) {
    type_t_check(TOKEN_INDENT, EOL_OPTIONAL);
    statement_struct *stmt = parse_statement();
    if (stmt->stmt_type != STMT_IF && stmt->stmt_type != STMT_WHILE && !ends_with_if_expr(stmt) &&
        stmt->stmt_type != STMT_FOR) {
        parse_statement_end(false);
    }
    stmt->next = parse_statement_next();
    type_t_check(TOKEN_DEDENT, EOL_OPTIONAL);
    return stmt;
}

/**
 * @brief checks syntax of branch in if body after else/then or in while loop
 * @param allow_else if there can be else statement
 * @return returns block of statements or oneline statement
 */
static statement_struct *parse_branch_body(bool allow_else) {
    statement_struct *stmt;
    if (token.after_eol) {
        stmt = parse_block();
        return stmt;
    } else {
        stmt = parse_statement();
        if (stmt->stmt_type != STMT_IF && stmt->stmt_type != STMT_WHILE &&
            !ends_with_if_expr(stmt) && stmt->stmt_type != STMT_FOR) {
            parse_statement_end(allow_else);
        }
        return stmt;
    }
}

/**
 * @brief checks syntax of else branch of if statement
 * @return block of else statement
 */
static statement_struct *parse_else(void) {
    if (token.type != TOKEN_KEYWORD || token.data.keyword != KEYWORD_ELSE) {
        return NULL;
    } else {
        next_token(EOL_OPTIONAL);
        if (token.type == TOKEN_KEYWORD && token.data.keyword == KEYWORD_IF && !token.after_eol) {
            return parse_if();
        } else {
            return parse_branch_body(false);
        }
    }
}

/**
 * @brief checks syntax of if body
 * @param if_data data of if statement
 */
static void parse_if_body(if_statement *if_data) {
    next_token(EOL_FORBIDDEN);
    if_data->if_expr = parse_expr(token, &token);
    keyword_t_check(KEYWORD_THEN, EOL_OPTIONAL);
    if_data->then_stmt = parse_branch_body(true);
    if_data->else_stmt = parse_else();
}

/**
 * @brief checks syntax of expression in if statement
 * @returns struct of expression
 */
static expr_struct *parse_if_expr(void) {
    expr_struct *expr = calloc(1, sizeof(expr_struct));
    if (expr == NULL) {
        error_exit(ERR_INTERNAL, get_line_number(), "Memory allocation failed!");
    }
    expr->line = get_line_number();
    expr->node_type = EXPR_IF;
    expr->result_type = TYPE_UNKNOWN;
    parse_if_body(&expr->data.if_stmt);
    if (expr->data.if_stmt.else_stmt == NULL) {
        error_exit(ERR_SYN, expr->line, "Else statement is mandatory!");
    }
    return expr;
}

/**
 * @brief checks syntax of expression or expression in if statement
 * @returns struct of expression
 */
static expr_struct *parse_expr_or_if(void) {
    if (token.type == TOKEN_KEYWORD && token.data.keyword == KEYWORD_IF) {
        return parse_if_expr();
    } else {
        return parse_expr(token, &token);
    }
}

/**
 * @brief checks syntax of statement if
 * @return statement node of if statement
 */
static statement_struct *parse_if(void) {
    statement_struct *stmt = create_stmt(get_line_number(), STMT_IF);
    parse_if_body(&stmt->data.if_stmt);
    return stmt;
}

/**
 * @brief checks syntax of statement while loop
 * @return statement node of while statement
 */
static statement_struct *parse_while(void) {
    statement_struct *stmt = create_stmt(get_line_number(), STMT_WHILE);
    next_token(EOL_FORBIDDEN);
    stmt->data.while_lp.while_expr = parse_expr(token, &token);
    keyword_t_check(KEYWORD_DO, EOL_OPTIONAL);
    stmt->data.while_lp.while_body = parse_branch_body(false);
    return stmt;
}

/**
 * @brief checks syntax of for loop
 * @return statement of for loop
 */
static statement_struct *parse_for(void) {
    statement_struct *stmt = create_stmt(get_line_number(), STMT_FOR);
    next_token(EOL_FORBIDDEN);
    if (token.type == TOKEN_IDENTIFIER) {
        stmt->data.for_lp.id_name = token.data.id;
    } else {
        error_exit(ERR_SYN, get_line_number(), "After for must be identifier!");
    }
    next_token(EOL_FORBIDDEN);
    type_t_check(TOKEN_GENERATOR_ARROW, EOL_FORBIDDEN);
    stmt->data.for_lp.from = parse_expr(token, &token);
    if (token.type == TOKEN_IDENTIFIER) {
        if (strcmp(token.data.id, "to") == 0) {
            stmt->data.for_lp.inclusive = true;
            free(token.data.id);
            next_token(EOL_FORBIDDEN);
        } else if (strcmp(token.data.id, "until") == 0) {
            stmt->data.for_lp.inclusive = false;
            free(token.data.id);
            next_token(EOL_FORBIDDEN);
        } else {
            error_exit(ERR_SYN, get_line_number(), "Token must be 'to' or 'until'!");
        }
    } else {
        error_exit(ERR_SYN, get_line_number(), "Token must be identifier!");
    }
    stmt->data.for_lp.to = parse_expr(token, &token);
    if (token.type == TOKEN_IDENTIFIER && (strcmp(token.data.id, "by") == 0)) {
        free(token.data.id);
        next_token(EOL_FORBIDDEN);
        stmt->data.for_lp.step = parse_expr(token, &token);
    }
    expr_struct *guard;
    while (token.type == TOKEN_KEYWORD && token.data.keyword == KEYWORD_IF && !token.after_eol) {
        next_token(EOL_FORBIDDEN);
        guard = parse_expr(token, &token);
        if (stmt->data.for_lp.guard == NULL) {
            stmt->data.for_lp.guard = guard;
        } else {
            expr_struct *new_guard = calloc(1, sizeof(expr_struct));
            if (new_guard == NULL) {
                error_exit(ERR_INTERNAL, stmt->line, "Memory allocation failed!");
            }
            new_guard->node_type = EXPR_BIN_OP;
            new_guard->data.binary_op.op_type = OP_AND;
            new_guard->data.binary_op.left_operand = stmt->data.for_lp.guard;
            new_guard->data.binary_op.right_operand = guard;
            new_guard->line = stmt->line;
            new_guard->result_type = TYPE_UNKNOWN;
            stmt->data.for_lp.guard = new_guard;
        }
    }
    keyword_t_check(KEYWORD_DO, EOL_OPTIONAL);
    stmt->data.for_lp.body = parse_branch_body(false);
    return stmt;
}

/**
 * @brief checks syntax of body in function
 * @return statement of body node
 */
static statement_struct *parse_body(void) {
    if (token.after_eol) {
        return parse_block();
    } else {
        statement_struct *stmt = create_stmt(get_line_number(), STMT_EXPR);
        stmt->data.expr_stmt = parse_expr_or_if();
        if (!token.after_eol && token.type != TOKEN_EOF &&
            stmt->data.expr_stmt->node_type != EXPR_IF) {
            error_exit(ERR_SYN, get_line_number(), "Expected eol after function body!");
        }
        return stmt;
    }
}

/**
 * @brief checks syntax of definition of function
 * @return function_struct func with is_main, func_name and declared_ret_type
 */
static function_struct *parse_func(void) {
    function_struct *func = calloc(1, sizeof(function_struct));
    if (func == NULL) {
        error_exit(ERR_INTERNAL, get_line_number(), "Function allocation goes wrong!");
    }
    func->line = get_line_number();
    int main_found = parse_optional_main();
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
        func->stmt_body = parse_body();
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