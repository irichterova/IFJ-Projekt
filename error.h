/**
 * Project name: Compiler for IFJ26
 * @author Filip Bůžek, xbuzekf00
 * @file error.h
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

#endif
