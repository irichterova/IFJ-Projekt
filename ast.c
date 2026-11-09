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