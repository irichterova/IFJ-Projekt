/**
 * Project name: Compiler for IFJ26
 * @author Vojtěch Ziaťko, xziatkv00
 * @file expression.h
 */

#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "ast.h"     // For expr_struct
#include "scanner.h" // For token_t

/**
 * @brief Parses an expression (expr OR expr_noid), creates its tree, stores following token
 * @param first First token
 * @param next Where to store the following token
 * @return Pointer to the root of the expression tree
 */
expr_struct *parse_expr(token_t first, token_t *next);

/**
 * @brief Parses an expression (expr_rest), creates its tree, stores following token
 * @param first First token
 * @param second Second token
 * @param next Where to store the following token
 * @return Pointer to the root of the expression tree
 */
expr_struct *parse_expr_rest(token_t first, token_t second, token_t *next);

#endif
