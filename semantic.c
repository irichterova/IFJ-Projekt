/**
 * Project name: Compiler for IFJ26
 * @author Filip Bůžek, xbuzekf00
 * @file semantic.c
 */

#include "semantic.h"
#include "error.h"
#include "symtable.h"
#include <stdlib.h>

static size_t codegen_next;
static hashtable *func_hash;
static hashtable *scope_hash;

/**
 * @brief converts value_type to param_type
 * @param value_t type of value that is converted to param_type
 * @return returns type of param from value type
 */
static param_type convert_type(value_type value_t) {
    switch (value_t) {
    case TYPE_INT:
        return int_dt;
    case TYPE_DOUBLE:
        return double_dt;
    case TYPE_STRING:
        return string_dt;
    case TYPE_UNIT:
        return unit_dt;
    case TYPE_BOOL:
        return bool_dt;
    default:
        error_exit(ERR_INTERNAL, 0, "Cannot convert unknown type to param type!");
        return 0;
    }
}

/**
 * @brief creates new scope and makes it current
 */
static void push_scope(void) {
    hashtable *hash = create_hashtable();
    if (hash == NULL) {
        error_exit(ERR_INTERNAL, 0, "Memory allocation failed!");
    }
    hash->parent = scope_hash;
    scope_hash = hash;
}

/**
 * @brief deletes current scope
 */
static void pop_scope(void) {
    if (scope_hash == NULL) {
        error_exit(ERR_INTERNAL, 0, "Cannot pop scope, no scope!");
    }
    hashtable *temp = scope_hash->parent;
    delete_hashtable(scope_hash);
    scope_hash = temp;
}

/**
 * @brief searches variable that matches key
 * @param key variable that we are looking for
 * @return returns attributes of found variable, NULL if not found
 */
static attributes *search_variable(char *key) {
    hashtable *cursor = scope_hash;
    while (cursor != NULL) {
        attributes *var = search_symbol(cursor, key);
        if (var != NULL) {
            return var;
        }
        cursor = cursor->parent;
    }
    return NULL;
}