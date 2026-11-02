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

typedef union {
    int lit_int;
    double lit_double;
    char *lit_string;
    char *variable;
} expr_data;

typedef struct expr_struct {
    expr_node_type n_type;
    value_type v_type;
    expr_data data;
} expr_struct;

#endif