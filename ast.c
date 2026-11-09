/**
 * Project name: Compiler for IFJ26
 * @author Filip Bůžek, xbuzekf00
 * @file ast.c
 */

#include "ast.h"
#include <stdlib.h>

/**
 * @brief frees parameters in function
 * @param param array of all parameters in function
 * @param argc how many parameters are in function
 */
static void free_params(params_struct *param, int argc) {
    for (int idx = 0; idx < argc; idx++) {
        free(param[idx].param_name);
    }
    free(param);
}

/**
 * @brief frees function in AST
 * @param func struct of freed function
 */
static void free_func(function_struct *func) {
    free(func->func_name);
    free_params(func->param_args, func->param_argc);
    // TODO: free body of the function
    free(func);
}

void free_program(program_struct *program) {
    if (program == NULL) {
        return;
    }
    function_struct *cursor = program->first_func;
    while (cursor != NULL) {
        function_struct *next = cursor->next;
        free_func(cursor);
        cursor = next;
    }
    free(program);
}