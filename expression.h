/**
 * Project name: Compiler for IFJ26
 * @author Vojtěch Ziaťko, xziatkv00
 * @file expression.h
 */

#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "ast.h" // For expr_struct
#include "scanner.h" // For token_t

// For expr and expr_noid - first token in, tree out
expr_struct *parse_expr(token_t first, token_t *next);

// For expr_rest - first two tokens in, tree out
expr_struct *parse_expr_rest(token_t first, token_t second, token_t *next);

#endif
