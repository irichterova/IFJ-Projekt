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

static void check_command(statement_struct *command);

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

/**
 * @brief inserts all program functions into hashtable
 * @param program struct with all functions in program
 */
static void insert_program_funcs(program_struct *program) {
    int main_cnt = 0;
    function_struct *cursor = program->first_func;
    while (cursor != NULL) {
        attributes attr = {0};
        attr.symbol = function;
        attr.main_flag = cursor->is_main;
        attr.num_of_param = cursor->param_argc;
        if (cursor->declared_ret_type != TYPE_UNKNOWN) {
            attr.return_type = convert_type(cursor->declared_ret_type);
            attr.ret_state = ret_inferred;
        } else {
            attr.ret_state = ret_unknown;
        }
        param_type *param = NULL;
        if (cursor->param_argc > 0) {
            param = malloc(sizeof(param_type) * cursor->param_argc);
            if (param == NULL) {
                error_exit(ERR_INTERNAL, cursor->line, "Memory allocation failed!");
            }
            for (int kdx = 0; kdx < cursor->param_argc; kdx++) {
                param[kdx] = convert_type(cursor->param_args[kdx].param_type);
            }
            attr.param_type_array = param;
        }
        insert_status status = insert_hash(func_hash, cursor->func_name, &attr);
        free(param);
        if (status == insert_fail) {
            error_exit(ERR_INTERNAL, cursor->line, "Insert hashtable failed!");
        } else if (status == insert_duplicate) {
            error_exit(ERR_SEM_RDEF, cursor->line, "Function '%s' is already defined!",
                       cursor->func_name);
        }
        if (attr.main_flag) {
            main_cnt++;
            if (attr.num_of_param > 0) {
                error_exit(ERR_SEM_ARG, cursor->line, "Main function cannot have parameters!");
            } else if (attr.ret_state == ret_inferred && attr.return_type != unit_dt) {
                error_exit(ERR_SEM_TYPE, cursor->line, "Return type is not Unit!");
            }
        }
        cursor = cursor->next;
    }
    if (main_cnt == 0) {
        error_exit(ERR_SEM_NDEF, 0, "Main is not defined in program!");
    } else if (main_cnt > 1) {
        error_exit(ERR_SEM_RDEF, 0, "Main is defined multiple times!");
    }
}

/**
 * @brief checks semantic analysis of block
 * @param statement struct with first statement of the block
 */
static void check_block(statement_struct *statement) {
    push_scope();
    statement_struct *cursor = statement;
    while (cursor != NULL) {
        check_command(cursor);
        cursor = cursor->next;
    }
    pop_scope();
}

static void check_command(statement_struct *command) {
}

/**
 * @brief checks semantic analysis of function
 * @param func struct with function attributes
 */
static void check_function(function_struct *func) {
    push_scope();
    for (int jdx = 0; jdx < func->param_argc; jdx++) {
        attributes attr = {0};
        attr.symbol = variable;
        attr.value_modif = nonmodif_val;
        attr.data_type = convert_type(func->param_args[jdx].param_type);
        attr.codegen_id = codegen_next;
        func->param_args[jdx].codegen_id = codegen_next;
        codegen_next++;
        insert_status status = insert_hash(scope_hash, func->param_args[jdx].param_name, &attr);
        if (status == insert_fail) {
            error_exit(ERR_INTERNAL, func->line, "Insert hashtable failed!");
        } else if (status == insert_duplicate) {
            error_exit(ERR_SEM_RDEF, func->line, "Parameter '%s' is already defined!",
                       func->param_args[jdx].param_name);
        }
    }
    check_block(func->stmt_body);
    pop_scope();
}

void check_program(program_struct *program) {
    func_hash = create_hashtable();
    if (func_hash == NULL) {
        error_exit(ERR_INTERNAL, 0, "Memory allocation failed!");
    }
    insert_builtin_funcs();
    insert_program_funcs(program);
    function_struct *cursor = program->first_func;
    while (cursor != NULL) {
        check_function(cursor);
        cursor = cursor->next;
    }
    delete_hashtable(func_hash);
}