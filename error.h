/**
 * Project name: Implementation of an IFJ26 Imperative Language Compiler
 * @author Filip Bůžek (xbuzekf00)
 * @file error.h
 * @brief Declares error codes and the error handling function
 */

#ifndef ERROR_H
#define ERROR_H

typedef enum {
    ERR_LEX = 1,
    ERR_SYN = 2,
    ERR_SEM_NDEF = 3,
    ERR_SEM_RDEF = 4,
    ERR_SEM_ARG = 5,
    ERR_SEM_TYPE = 6,
    ERR_SEM_NVAL = 7,
    ERR_SEM_OTHER = 10,
    ERR_INTERNAL = 99
} error_code;

/**
 * @brief exits program and writes to stderr with error code type when error occurs
 * @param code what type of error happened
 * @param line at what line error happened, if line number is lesser or equal to 0 then line is unknown
 * @param fmt format of error, after this parameter can be any arguments
 */
void error_exit(error_code code, int line, const char *fmt, ...);

#endif
