/**
 * Project name: Compiler for IFJ26
 * @author Vojtěch Ziaťko, xziatkv00
 * @file expression.c
 */

#include "expression.h"

/**
 * @brief Parses an expression (expr OR expr_noid), creates its tree, stores following token
 * @param first First token
 * @param next Where to store the following token
 * @return Pointer to the root of the expression tree
 */
expr_struct *parse_expr(token_t first, token_t *next) {
    (void)first; // TODO: Remove when implemented
    (void)next;

    return NULL;
}

/**
 * @brief Parses an expression (expr_rest), creates its tree, stores following token
 * @param first First token
 * @param second Second token
 * @param next Where to store the following token
 * @return Pointer to the root of the expression tree
 */
expr_struct *parse_expr_rest(token_t first, token_t second, token_t *next) {
    (void)first; // TODO: Remove when implemented
    (void)second;
    (void)next;

    return NULL;
}
