/**
 * Project name: Implementation of an IFJ26 Imperative Language Compiler
 * @author Vojtěch Ziaťko (xziatkv00)
 * @file expression.h
 * @brief Declares the precedence analysis using a modified stack
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
