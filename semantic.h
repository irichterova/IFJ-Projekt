/**
 * Project name: Compiler for IFJ26
 * @author Filip Bůžek, xbuzekf00
 * @file semantic.h
 */

#ifndef SEMANTIC_H
#define SEMANTIC_H
#include "ast.h"

/**
 * @brief checks semantic of program, fills codegen_id and types to AST
 * @param program struct with all stored data in AST
 */
void check_program(program_struct *program);

#endif