/**
 * Project name: Implementation of an IFJ26 Imperative Language Compiler
 * @author Filip Bůžek (xbuzekf00)
 * @file parser.h
 * @brief Declares parser functions for syntax analysis and AST construction
 */

#ifndef PARSER_H
#define PARSER_H
#include "ast.h"
#include "scanner.h"

/**
 * @brief checks syntax of if as expression
 * @param first First token
 * @param next Where to store the following token
 * @return returns struct of if expression
 */
expr_struct *parse_if_in_expr(token_t first, token_t *next);

/**
 * @brief creates AST from the whole program
 * @return returns root of AST, if error then exits with error_exit
 */
program_struct *parse_program(void);

#endif