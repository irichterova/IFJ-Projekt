/**
 * Project name: Compiler for IFJ26
 * @author Filip Bůžek, xbuzekf00
 * @file ast.h
 */

#ifndef AST_H
#define AST_H

typedef enum { TYPE_INT, TYPE_DOUBLE, TYPE_STRING, TYPE_UNIT, TYPE_UNKNOWN } value_type;

typedef enum {
    EXPR_LIT_INT,
    EXPR_LIT_DOUBLE,
    EXPR_LIT_STRING,
    EXPR_LIT_UNIT,
    EXPR_VARIABLE,
    EXPR_UNA_OP,
    EXPR_BIN_OP,
    EXPR_CALL_FUNC
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
    OP_NOTEQUAL
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
    value_type declared_type;
    struct expr_struct *init_expr;
} def_var;

typedef union {
    int lit_int;
    double lit_double;
    char *lit_string;
    char *variable;
    unary_operator unary_op;
    binary_operator binary_op;
    call_function call_fn;
} expr_data;

typedef struct expr_struct {
    expr_node_type n_type;
    value_type v_type;
    expr_data data;
} expr_struct;

#endif