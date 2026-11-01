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

#endif