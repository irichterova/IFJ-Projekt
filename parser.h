/**
 * Project name: Compiler for IFJ26
 * @author Filip Bůžek, xbuzekf00
 * @file parser.h
 */

#ifndef PARSER_H
#define PARSER_H
#include "ast.h"

/**
 * @brief creates AST from the whole program
 * @return returns root of AST, if error then exits with error_exit
 */
program_struct *parse_program(void);

#endif