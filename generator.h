/**
 * Project name: Implementation of an IFJ26 Imperative Language Compiler
 * @author Phuong Uyen Dongova (xdongop00)
 * @file generator.h
 * @brief Declares functions for generating IFJcode26 code
 */

#ifndef GENERATOR_H
#define GENERATOR_H

#include "ast.h"
/**
 * @brief Generates IFJcode26 output from the file ast.h
 * @param program_code Pointer to the root program AST structure
 * @return 0 if success
 */
int gen_code(program_struct *program_code);

#endif
