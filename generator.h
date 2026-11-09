/* 
IFJ/IAL project - Compiler for language IFJ26 to language IFJ26code
*
@file generator.h
*
@date 7-11-2026
@author xdongop00 Phuong Uyen Dongova
*/

#ifndef GENERATOR_H
#define GENERATOR_H


#include <stdio.h>
#include "ast.h"

int gen_code(program_struct *program_code, FILE *output);
void gen_function(function_struct* function);
void gen_statement(statement_struct* statement);
void gen_expression(expr_struct* expression);

#endif
