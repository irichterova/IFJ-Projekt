/**
 * Project name: Compiler for IFJ26
 * @author Filip Bůžek, xbuzekf00
 * @file ast.c
 */

#include "ast.h"
#include <stdlib.h>

/**
 * @brief frees expression in AST
 * @param expr struct of freed expression
 */
static void free_expr(expr_struct *expr) {
    if (expr == NULL) {
        return;
    }
    switch (expr->node_type) {
    case EXPR_LIT_STRING:
        free(expr->data.lit_string);
        break;
    case EXPR_VARIABLE:
        free(expr->data.variable.id_name);
        break;
    case EXPR_CALL_FUNC:
        for (int idx = 0; idx < expr->data.call_fn.argc; idx++) {
            free_expr(expr->data.call_fn.args[idx]);
        }
        free(expr->data.call_fn.args);
        free(expr->data.call_fn.name);
        break;
    case EXPR_BIN_OP:
        free_expr(expr->data.binary_op.left_operand);
        free_expr(expr->data.binary_op.right_operand);
        break;
    case EXPR_UNA_OP:
        free_expr(expr->data.unary_op.operand);
        break;
    default:
        break;
    }
    free(expr);
}

/**
 * @brief frees list of statements in AST
 * @param stmt struct of freed statement
 */
static void free_statements(statement_struct *stmt) {
    statement_struct *cursor = stmt;
    while (cursor != NULL) {
        statement_struct *next = cursor->next;
        switch (cursor->stmt_type) {
        case STMT_VAL:
        case STMT_VAR:
            free(cursor->data.var_stmt.id_name);
            free_expr(cursor->data.var_stmt.init_expr);
            break;
        case STMT_ASSIGN:
            free(cursor->data.id_stmt.id_name);
            free_expr(cursor->data.id_stmt.assign_expr);
            break;
        case STMT_RETURN:
            free_expr(cursor->data.ret_stmt);
            break;
        case STMT_EXPR:
            free_expr(cursor->data.expr_stmt);
            break;
        case STMT_WHILE:
            free_expr(cursor->data.while_lp.while_expr);
            free_statements(cursor->data.while_lp.while_body);
            break;
        case STMT_IF:
            free_expr(cursor->data.if_stmt.if_expr);
            free_statements(cursor->data.if_stmt.then_stmt);
            if (cursor->data.if_stmt.else_stmt != NULL) {
                free_statements(cursor->data.if_stmt.else_stmt);
            }
            break;
        }
        free(cursor);
        cursor = next;
    }
}

/**
 * @brief frees parameters in function
 * @param param array of all parameters in function
 * @param argc how many parameters are in function
 */
static void free_params(params_struct *param, int argc) {
    for (int idx = 0; idx < argc; idx++) {
        free(param[idx].param_name);
    }
    free(param);
}

/**
 * @brief frees function in AST
 * @param func struct of freed function
 */
static void free_func(function_struct *func) {
    free(func->func_name);
    free_params(func->param_args, func->param_argc);
    free_statements(func->stmt_body);
    free(func);
}

void free_program(program_struct *program) {
    if (program == NULL) {
        return;
    }
    function_struct *cursor = program->first_func;
    while (cursor != NULL) {
        function_struct *next = cursor->next;
        free_func(cursor);
        cursor = next;
    }
    free(program);
}