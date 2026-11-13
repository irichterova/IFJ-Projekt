/**
 * Project name: Compiler for IFJ26
 * @author Filip Bůžek, xbuzekf00
 * @file ast.h
 */

#ifndef AST_H
#define AST_H
#include <stddef.h>

typedef enum { TYPE_INT, TYPE_DOUBLE, TYPE_STRING, TYPE_UNIT, TYPE_BOOL, TYPE_UNKNOWN } value_type;

typedef enum {
    EXPR_LIT_INT,
    EXPR_LIT_DOUBLE,
    EXPR_LIT_STRING,
    EXPR_LIT_UNIT,
    EXPR_VARIABLE,
    EXPR_UNA_OP,
    EXPR_BIN_OP,
    EXPR_CALL_FUNC,
    EXPR_LIT_TRUE,
    EXPR_LIT_FALSE
} expr_node_type;

typedef enum {
    OP_PLUS,
    OP_BIN_MINUS,
    OP_UNA_MINUS,
    OP_MULTIPLICATION,
    OP_DIVISION,
    OP_LESS,
    OP_GREATER,
    OP_EQUAL_LESS,
    OP_EQUAL_GREATER,
    OP_EQUAL,
    OP_NOTEQUAL,
    OP_NOT,
    OP_AND,
    OP_OR
} operator_type;

typedef enum {
    STMT_VAR,
    STMT_VAL,
    STMT_ASSIGN,
    STMT_RETURN,
    STMT_WHILE,
    STMT_IF,
    STMT_EXPR
} statement_type;

typedef struct unary_operator {
    operator_type op_type;
    struct expr_struct *operand;
} unary_operator;

typedef struct binary_operator {
    operator_type op_type;
    struct expr_struct *left_operand;
    struct expr_struct *right_operand;
} binary_operator;

typedef struct call_function {
    char *name;
    struct expr_struct **args;
    int argc;
} call_function;

typedef struct def_var {
    char *id_name;
    size_t codegen_id;
    value_type declared_type;
    struct expr_struct *init_expr;
} def_var;

typedef struct id_assign {
    char *id_name;
    size_t codegen_id;
    value_type id_type;
    struct expr_struct *assign_expr;
} id_assign;

typedef struct while_loop {
    struct expr_struct *while_expr;
    struct statement_struct *while_body;
} while_loop;

typedef struct if_statement {
    struct expr_struct *if_expr;
    struct statement_struct *then_stmt;
    struct statement_struct *else_stmt;
} if_statement;

typedef union {
    def_var var_stmt;
    id_assign id_stmt;
    while_loop while_lp;
    if_statement if_stmt;
    struct expr_struct *ret_stmt;
    struct expr_struct *expr_stmt;
} statement_data;

typedef struct statement_struct {
    int line;
    statement_type stmt_type;
    struct statement_struct *next;
    statement_data data;
} statement_struct;

typedef struct params_struct {
    char *param_name;
    size_t codegen_id;
    value_type param_type;
} params_struct;

typedef struct function_struct {
    char *func_name;
    int line;
    struct params_struct *param_args;
    int param_argc;
    value_type result_ret_type;
    value_type declared_ret_type;
    int is_main;
    struct statement_struct *stmt_body;
    struct function_struct *next;
} function_struct;

typedef struct program_struct {
    struct function_struct *first_func;
} program_struct;

typedef struct var_struct {
    char *id_name;
    size_t codegen_id;
} var_struct;

typedef union {
    int lit_int;
    double lit_double;
    char *lit_string;
    var_struct variable;
    unary_operator unary_op;
    binary_operator binary_op;
    call_function call_fn;
} expr_data;

typedef struct expr_struct {
    int line;
    expr_node_type node_type;
    value_type result_type;
    expr_data data;
} expr_struct;

/**
 * @brief frees entire AST with all strings
 * @param program struct with all stored data in AST
 */
void free_program(program_struct *program);

#endif