/**
 * Project name: Compiler for IFJ26
 * @author Vojtěch Ziaťko, xziatkv00
 * @file expression.c
 */

#include "expression.h"

// Symbols defining the table
typedef enum {
    PSA_UNARY_MINUS,
    PSA_MUL,
    PSA_DIV,
    PSA_ADD,
    PSA_SUB,
    PSA_LESS,
    PSA_LESS_EQ,
    PSA_GREATER,
    PSA_GREATER_EQ,
    PSA_EQUAL,
    PSA_NOT_EQUAL,
    PSA_ID,
    PSA_LEFT_B,
    PSA_RIGHT_B,
    PSA_START_END,
    PSA_CAT,  // LISTS
    PSA_NOT,  // BOOLTHEN
    PSA_AND,  // BOOLTHEN
    PSA_OR,   // BOOLTHEN
    // Below are stack only symbols, not in the table
    PSA_E,    // Expression E
    PSA_STOP, // Marker <
} psa_symbol;

// What can be inside the table
typedef enum {
    PSA_SHIFT,  // <
    PSA_PUSH,   // =
    PSA_REDUCE, // >
    PSA_ERROR,  // Empty
} psa_relation;

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
