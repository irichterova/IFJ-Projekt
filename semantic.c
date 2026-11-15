/**
 * Project name: Compiler for IFJ26
 * @author Filip Bůžek, xbuzekf00
 * @file semantic.c
 */

#include "semantic.h"
#include "error.h"
#include "symtable.h"
#include <stdlib.h>

#define MAX_PARAMS 3

static size_t codegen_next;
static hashtable *func_hash;
static hashtable *scope_hash;

typedef struct builtin_func {
    char *func_name;
    size_t param_argc;
    param_type p_type[MAX_PARAMS];
    param_type return_type;
} builtin_func;

static const builtin_func funcs[] = {{"print", 1, {any_dt}, unit_dt},
                                     {"readLine", 0, {0}, string_dt},
                                     {"readInt", 0, {0}, int_dt},
                                     {"readDouble", 0, {0}, double_dt},
                                     {"toStr", 1, {any_dt}, string_dt},
                                     {"toInt", 1, {double_dt}, int_dt},
                                     {"strLength", 1, {string_dt}, int_dt},
                                     {"subStr", 3, {string_dt, int_dt, int_dt}, string_dt},
                                     {"ord", 2, {string_dt, int_dt}, int_dt},
                                     {"chr", 1, {int_dt}, string_dt}};

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

/**
 * @brief inserts all builtin functions into function table
 */
static void insert_builtin_funcs(void) {
    size_t builtin_amount = sizeof(funcs) / sizeof(funcs[0]);
    for (size_t idx = 0; idx < builtin_amount; idx++) {
        attributes attr = {0};
        attr.return_type = funcs[idx].return_type;
        attr.num_of_param = funcs[idx].param_argc;
        attr.param_type_array = (param_type *)funcs[idx].p_type;
        attr.builtin_flag = true;
        attr.symbol = function;
        insert_status status = insert_hash(func_hash, funcs[idx].func_name, &attr);
        if (status != insert_ok) {
            error_exit(ERR_INTERNAL, 0, "Insert hashtable failed!");
        }
    }
}