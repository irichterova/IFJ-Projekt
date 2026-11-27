/**
 * Project name: Implementation of an IFJ26 Imperative Language Compiler
 * @author Filip Bůžek (xbuzekf00)
 * @file error.c
 * @brief Implements error handling and program termination
 */

#include "error.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void error_exit(error_code code, int line, const char *fmt, ...) {

    if (line > 0) {
        fprintf(stderr, "Error (line %d): ", line);
    } else {
        fprintf(stderr, "Error: ");
    }

    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
    exit(code);
}