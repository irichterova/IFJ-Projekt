/**
 * Project name: Compiler for IFJ26
 * @author Filip Bůžek, xbuzekf00
 * @file semantic.c
 */

#include "semantic.h"
#include "error.h"
#include "symtable.h"
#include <stdlib.h>
#include <string.h>

#define MAX_PARAMS 3

static void check_function(function_struct *func);
static void check_command(statement_struct *command);
static void check_condition(expr_struct *expr);

static size_t codegen_next;
static hashtable *func_hash;
static hashtable *scope_hash;
static function_struct *func_inspect;
static program_struct *prog;

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
                                     {"chr", 1, {int_dt}, string_dt},
                                     {"readBoolean", 0, {0}, bool_dt},
                                     {"$floor", 1, {double_dt}, double_dt}};

/**
 * @brief converts value_type to param_type
 * @param value_t type of value that is converted to param_type
 * @return returns type of param from value type
 */
static param_type convert_value2param(value_type value_t) {
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
        error_exit(ERR_INTERNAL, 0, "Cannot convert unknown value type to param type!");
        return 0;
    }
}

/**
 * @brief converts param_type to value_type
 * @param param_t type of param that is converted to value_type
 * @return returns type of value from param type
 */
static value_type convert_param2value(param_type param_t) {
    switch (param_t) {
    case int_dt:
        return TYPE_INT;
    case double_dt:
        return TYPE_DOUBLE;
    case string_dt:
        return TYPE_STRING;
    case unit_dt:
        return TYPE_UNIT;
    case bool_dt:
        return TYPE_BOOL;
    default:
        error_exit(ERR_INTERNAL, 0, "Cannot convert unknown param type to value type!");
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
        attr.ret_state = ret_inferred;
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
            attr.return_type = convert_value2param(cursor->declared_ret_type);
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
                param[kdx] = convert_value2param(cursor->param_args[kdx].param_type);
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

/**
 * @brief checks compatibility of conversion of types
 * @param expected type we expect
 * @param actual type we get
 * @return returns true if expected is same as actual or conversion is int2double, returns false if
 * not
 */
static bool compatible_type(value_type expected, value_type actual) {
    if (expected == actual || (expected == TYPE_DOUBLE && actual == TYPE_INT)) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief checks if branch ends with unit
 * @param branch branch of if statement
 * @return returns true if branch ends with unit, false if not
 */
static bool ends_with_unit(statement_struct *branch) {
    statement_struct *cursor = branch;
    while (cursor->next != NULL) {
        cursor = cursor->next;
    }
    if (cursor->stmt_type == STMT_EXPR) {
        if (cursor->data.expr_stmt->result_type == TYPE_UNIT) {
            return true;
        } else {
            return false;
        }
    } else if (cursor->stmt_type == STMT_VAL || cursor->stmt_type == STMT_VAR ||
               cursor->stmt_type == STMT_ASSIGN || cursor->stmt_type == STMT_WHILE ||
               cursor->stmt_type == STMT_FOR) {
        return true;
    } else if (cursor->stmt_type == STMT_IF) {
        if (cursor->data.if_stmt.else_stmt == NULL) {
            return ends_with_unit(cursor->data.if_stmt.then_stmt);
        } else {
            return ends_with_unit(cursor->data.if_stmt.else_stmt) &&
                   ends_with_unit(cursor->data.if_stmt.then_stmt);
        }
    } else {
        return false;
    }
}

/**
 * @brief compares two value types
 * @param first first type of branch
 * @param second second type of branch
 * @param line on which line is branch
 * @return if they are same returns them, if one is unknown returns the other one, if one is int and
 * second i double returns double, if none above returns error
 */
static value_type unify_types(value_type first, value_type second, int line) {
    if (first == TYPE_UNKNOWN && second != TYPE_UNKNOWN) {
        return second;
    } else if (second == TYPE_UNKNOWN && first != TYPE_UNKNOWN) {
        return first;
    } else if (first == second) {
        return first;
    } else if ((first == TYPE_INT && second == TYPE_DOUBLE) ||
               (first == TYPE_DOUBLE && second == TYPE_INT)) {
        return TYPE_DOUBLE;
    } else {
        error_exit(ERR_SEM_TYPE, line, "Types of branch does not match with each other!");
        return TYPE_UNKNOWN;
    }
}

/**
 * @brief checks result type of block based on type of statement
 * @param block block we are checking
 * @return returns type of value
 */
static value_type block_result_type(statement_struct *block) {
    statement_struct *cursor = block;
    while (cursor->next != NULL) {
        cursor = cursor->next;
    }
    if (cursor->stmt_type == STMT_EXPR) {
        return cursor->data.expr_stmt->result_type;
    } else if (cursor->stmt_type == STMT_VAL || cursor->stmt_type == STMT_VAR ||
               cursor->stmt_type == STMT_ASSIGN || cursor->stmt_type == STMT_WHILE ||
               cursor->stmt_type == STMT_FOR) {
        return TYPE_UNIT;
    } else if (cursor->stmt_type == STMT_IF) {
        if (cursor->data.if_stmt.else_stmt == NULL) {
            return TYPE_UNIT;
        } else {
            return unify_types(block_result_type(cursor->data.if_stmt.then_stmt),
                               block_result_type(cursor->data.if_stmt.else_stmt), cursor->line);
        }
    } else if (cursor->stmt_type == STMT_RETURN) {
        return TYPE_UNKNOWN;
    } else {
        return TYPE_UNKNOWN;
    }
}

/**
 * @brief finds function by its name in program
 * @param name name of wanted function
 * @return returns wanted function, if not found returns NULL
 */
static function_struct *find_func(char *name) {
    function_struct *cursor = prog->first_func;
    while (cursor != NULL) {
        if (strcmp(name, cursor->func_name) == 0) {
            return cursor;
        }
        cursor = cursor->next;
    }
    return NULL;
}

/**
 * @brief checks if operand type is double or int
 * @param operand_type type of operand
 * @return returns true if type of operand is double or int, if not returns false
 */
static bool is_int_or_double(value_type operand_type) {
    if (operand_type == TYPE_INT || operand_type == TYPE_DOUBLE) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief checks semantic analysis of binary operators
 * @param op_type operator type
 * @param left_operand_type on the left of operator
 * @param right_operand_type on the right of operator
 * @param line on which line operator is
 * @return returns type of operator
 */
static value_type check_bin_op(operator_type op_type, value_type left_operand_type,
                               value_type right_operand_type, int line) {
    switch (op_type) {
    case OP_PLUS:
        if (left_operand_type == TYPE_STRING && right_operand_type == TYPE_STRING) {
            return TYPE_STRING;
        }
        /* fall through */
    case OP_BIN_MINUS:
    case OP_MULTIPLICATION:
    case OP_DIVISION:
        if (!is_int_or_double(left_operand_type) || !is_int_or_double(right_operand_type)) {
            error_exit(ERR_SEM_TYPE, line, "One of the operands is not int or double!");
            return 0;
        } else if (left_operand_type == TYPE_DOUBLE || right_operand_type == TYPE_DOUBLE) {
            return TYPE_DOUBLE;
        } else {
            return TYPE_INT;
        }
    case OP_LESS:
    case OP_GREATER:
    case OP_EQUAL_GREATER:
    case OP_EQUAL_LESS:
        if ((is_int_or_double(left_operand_type) && is_int_or_double(right_operand_type)) ||
            (left_operand_type == TYPE_STRING && right_operand_type == TYPE_STRING)) {
            return TYPE_BOOL;
        } else {
            error_exit(ERR_SEM_TYPE, line, "Operands types are not double/int or string!");
            return 0;
        }
    case OP_EQUAL:
    case OP_NOTEQUAL:
        if ((is_int_or_double(left_operand_type) && is_int_or_double(right_operand_type)) ||
            (left_operand_type == right_operand_type)) {
            return TYPE_BOOL;
        } else {
            error_exit(ERR_SEM_TYPE, line, "Operands types are not matching or are not numeric!");
            return 0;
        }
    case OP_AND:
    case OP_OR:
        if (left_operand_type == TYPE_BOOL && right_operand_type == TYPE_BOOL) {
            return TYPE_BOOL;
        } else {
            error_exit(ERR_SEM_TYPE, line, "Operands types are not bools!");
            return 0;
        }
    default:
        error_exit(ERR_INTERNAL, line, "Operator type doesn't match with any types!");
        return 0;
    }
}

/**
 * @brief checks semantic analysis of expression
 * @param expr struct with expression
 */
static void check_expression(expr_struct *expr) {
    attributes *expr_attr;
    switch (expr->node_type) {
    case EXPR_VARIABLE:
        expr_attr = search_variable(expr->data.variable.id_name);
        if (expr_attr == NULL) {
            error_exit(ERR_SEM_NDEF, expr->line, "Variable '%s' is not defined!",
                       expr->data.variable.id_name);
        }
        expr->data.variable.codegen_id = expr_attr->codegen_id;
        expr->result_type = convert_param2value(expr_attr->data_type);
        break;
    case EXPR_UNA_OP:
        check_expression(expr->data.unary_op.operand);
        if (expr->data.unary_op.op_type == OP_UNA_MINUS) {
            if (is_int_or_double(expr->data.unary_op.operand->result_type)) {
                expr->result_type = expr->data.unary_op.operand->result_type;
            } else {
                error_exit(ERR_SEM_TYPE, expr->line, "Operand is not numeric!");
            }
        } else if (expr->data.unary_op.op_type == OP_NOT) {
            if (expr->data.unary_op.operand->result_type == TYPE_BOOL) {
                expr->result_type = TYPE_BOOL;
            } else {
                error_exit(ERR_SEM_TYPE, expr->line, "Operand is not boolean!");
            }
        } else {
            error_exit(ERR_INTERNAL, expr->line, "Type of unary operator doesn't match!");
        }
        break;
    case EXPR_BIN_OP:
        check_expression(expr->data.binary_op.left_operand);
        check_expression(expr->data.binary_op.right_operand);
        expr->result_type = check_bin_op(
            expr->data.binary_op.op_type, expr->data.binary_op.left_operand->result_type,
            expr->data.binary_op.right_operand->result_type, expr->line);
        break;
    case EXPR_CALL_FUNC:
        expr_attr = search_symbol(func_hash, expr->data.call_fn.name);
        if (expr_attr == NULL) {
            error_exit(ERR_SEM_NDEF, expr->line, "Function '%s' is not defined!",
                       expr->data.call_fn.name);
        }
        if (expr_attr->num_of_param != (size_t)expr->data.call_fn.argc) {
            error_exit(ERR_SEM_ARG, expr->line,
                       "Amount of arguments in function '%s' does not match!",
                       expr->data.call_fn.name);
        }
        for (int hdx = 0; hdx < expr->data.call_fn.argc; hdx++) {
            check_expression(expr->data.call_fn.args[hdx]);
            if (expr_attr->param_type_array[hdx] != any_dt) {
                value_type val_type = convert_param2value(expr_attr->param_type_array[hdx]);
                if (!compatible_type(val_type, expr->data.call_fn.args[hdx]->result_type)) {
                    if (expr->data.call_fn.is_valmem) {
                        error_exit(ERR_SEM_TYPE, expr->line,
                                   "Member cannot be used for this type!");
                    } else {
                        error_exit(ERR_SEM_ARG, expr->line,
                                   "Argument is not compatible with parameter type!");
                    }
                }
            }
        }
        if (expr_attr->ret_state == ret_inferred) {
            expr->result_type = convert_param2value(expr_attr->return_type);
        } else {
            function_struct *found_func = find_func(expr->data.call_fn.name);
            if (found_func == NULL) {
                error_exit(ERR_INTERNAL, expr->line, "Function was not found!");
            }
            check_function(found_func);
            expr->result_type = convert_param2value(expr_attr->return_type);
        }
        break;
    case EXPR_IF:
        check_condition(expr->data.if_stmt.if_expr);
        check_block(expr->data.if_stmt.then_stmt);
        check_block(expr->data.if_stmt.else_stmt);
        expr->result_type =
            unify_types(block_result_type(expr->data.if_stmt.then_stmt),
                        block_result_type(expr->data.if_stmt.else_stmt), expr->line);
        break;
    default:
        break;
    }
}

/**
 * @brief checks semantic analysis of condition in while or if statement
 * @param expr struct of condition
 */
static void check_condition(expr_struct *expr) {
    check_expression(expr);
    if (expr->result_type != TYPE_BOOL) {
        error_exit(ERR_SEM_TYPE, expr->line, "Result type of condition is not Boolean!");
    }
}

/**
 * @brief checks semantic analysis of commands
 * @param command struct with statement command
 */
static void check_command(statement_struct *command) {
    attributes attr = {0};
    insert_status status;
    attributes *id_attr;
    switch (command->stmt_type) {
    case STMT_ASSIGN:
        check_expression(command->data.id_stmt.assign_expr);
        id_attr = search_variable(command->data.id_stmt.id_name);
        if (id_attr == NULL) {
            error_exit(ERR_SEM_NDEF, command->line, "Variable '%s' is not defined!",
                       command->data.id_stmt.id_name);
        } else if (id_attr->value_modif == nonmodif_val) {
            error_exit(ERR_SEM_NVAL, command->line, "Variable '%s' cannot be modified!",
                       command->data.id_stmt.id_name);
        }
        value_type id_data_type = convert_param2value(id_attr->data_type);
        if (compatible_type(id_data_type, command->data.id_stmt.assign_expr->result_type)) {
            command->data.id_stmt.id_type = id_data_type;
        } else {
            error_exit(ERR_SEM_TYPE, command->line, "Id type is not compatible with result type!");
        }
        command->data.id_stmt.codegen_id = id_attr->codegen_id;
        break;
    case STMT_VAL:
    case STMT_VAR:
        check_expression(command->data.var_stmt.init_expr);
        attr.symbol = variable;
        if (command->stmt_type == STMT_VAR) {
            attr.value_modif = modif_var;
        } else {
            attr.value_modif = nonmodif_val;
        }
        if (command->data.var_stmt.declared_type != TYPE_UNKNOWN) {
            if (compatible_type(command->data.var_stmt.declared_type,
                                command->data.var_stmt.init_expr->result_type)) {
                attr.data_type = convert_value2param(command->data.var_stmt.declared_type);
            } else {
                error_exit(ERR_SEM_TYPE, command->line,
                           "Declared type is not compatible with result type!");
            }
        } else {
            attr.data_type = convert_value2param(command->data.var_stmt.init_expr->result_type);
        }
        attr.codegen_id = codegen_next;
        command->data.var_stmt.codegen_id = codegen_next;
        codegen_next++;
        status = insert_hash(scope_hash, command->data.var_stmt.id_name, &attr);
        if (status == insert_fail) {
            error_exit(ERR_INTERNAL, command->line, "Insert hashtable failed!");
        } else if (status == insert_duplicate) {
            error_exit(ERR_SEM_RDEF, command->line, "Variable '%s' is already defined!",
                       command->data.var_stmt.id_name);
        }
        break;
    case STMT_RETURN:
        if (command->data.ret_stmt != NULL) {
            check_expression(command->data.ret_stmt);
        }
        if (func_inspect->declared_ret_type == TYPE_UNKNOWN) {
            error_exit(ERR_SEM_TYPE, command->line, "Function does not have declared return type!");
        } else {
            if (command->data.ret_stmt == NULL) {
                if (func_inspect->declared_ret_type != TYPE_UNIT) {
                    error_exit(ERR_SEM_TYPE, command->line,
                               "Return with no expression has to have return type Unit!");
                }
            } else {
                if (func_inspect->declared_ret_type != TYPE_UNIT) {
                    if (!compatible_type(func_inspect->declared_ret_type,
                                         command->data.ret_stmt->result_type)) {
                        error_exit(ERR_SEM_TYPE, command->line,
                                   "Expected type and actual type are not compatible!");
                    }
                }
            }
        }
        break;
    case STMT_EXPR:
        check_expression(command->data.expr_stmt);
        break;
    case STMT_IF:
        check_condition(command->data.if_stmt.if_expr);
        check_block(command->data.if_stmt.then_stmt);
        if (command->data.if_stmt.else_stmt != NULL) {
            check_block(command->data.if_stmt.else_stmt);
        }
        break;
    case STMT_WHILE:
        check_condition(command->data.while_lp.while_expr);
        check_block(command->data.while_lp.while_body);
        break;
    case STMT_FOR:
        check_expression(command->data.for_lp.from);
        if (command->data.for_lp.from->result_type != TYPE_INT) {
            error_exit(ERR_SEM_TYPE, command->line, "'from' has to have type INT!");
        }
        check_expression(command->data.for_lp.to);
        if (command->data.for_lp.to->result_type != TYPE_INT) {
            error_exit(ERR_SEM_TYPE, command->line, "'to' has to have type INT");
        }
        if (command->data.for_lp.step != NULL) {
            check_expression(command->data.for_lp.step);
            if (command->data.for_lp.step->result_type != TYPE_INT) {
                error_exit(ERR_SEM_TYPE, command->line, "'step' has to have type INT");
            } else if (command->data.for_lp.step->node_type == EXPR_LIT_INT &&
                       command->data.for_lp.step->data.lit_int == 0) {
                error_exit(ERR_SEM_OTHER, command->line, "'step' cannot be 0!");
            }
        }
        push_scope();
        attr.symbol = variable;
        attr.value_modif = nonmodif_val;
        attr.data_type = int_dt;
        attr.codegen_id = codegen_next;
        command->data.for_lp.codegen_id = codegen_next;
        codegen_next++;
        status = insert_hash(scope_hash, command->data.for_lp.id_name, &attr);
        if (status == insert_fail) {
            error_exit(ERR_INTERNAL, command->line, "Insert hashtable failed!");
        }
        if (command->data.for_lp.guard != NULL) {
            check_condition(command->data.for_lp.guard);
        }
        check_block(command->data.for_lp.body);
        pop_scope();
        break;
    }
}

/**
 * @brief checks semantic analysis of function
 * @param func struct with function attributes
 */
static void check_function(function_struct *func) {
    if (func->state == FUNC_CHECKED) {
        return;
    } else if (func->state == FUNC_CHECKING) {
        error_exit(ERR_SEM_TYPE, func->line, "Recursion without type!");
    } else {
        func->state = FUNC_CHECKING;
    }
    function_struct *prev_func_inspect = func_inspect;
    hashtable *prev_scope = scope_hash;
    scope_hash = NULL;
    push_scope();
    for (int jdx = 0; jdx < func->param_argc; jdx++) {
        attributes attr = {0};
        attr.symbol = variable;
        attr.value_modif = nonmodif_val;
        attr.data_type = convert_value2param(func->param_args[jdx].param_type);
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
    func_inspect = func;
    statement_struct *cursor = func->stmt_body;
    statement_struct *last_stmt = NULL;
    while (cursor != NULL) {
        check_command(cursor);
        last_stmt = cursor;
        cursor = cursor->next;
    }
    if (func->declared_ret_type != TYPE_UNKNOWN) {
        func->result_ret_type = func->declared_ret_type;
        if (func->declared_ret_type != TYPE_UNIT) {
            value_type block_res = block_result_type(func->stmt_body);
            switch (last_stmt->stmt_type) {
            case STMT_EXPR:
                if (!compatible_type(func->declared_ret_type,
                                     last_stmt->data.expr_stmt->result_type)) {
                    error_exit(ERR_SEM_TYPE, last_stmt->line, "Types are not compatible!");
                }
                break;
            case STMT_VAL:
            case STMT_VAR:
            case STMT_ASSIGN:
            case STMT_FOR:
            case STMT_WHILE:
                error_exit(ERR_SEM_TYPE, last_stmt->line, "Return type is not Unit!");
                break;
            case STMT_RETURN:
                break;
            case STMT_IF:
                if (block_res != TYPE_UNKNOWN &&
                    !compatible_type(func->declared_ret_type, block_res)) {
                    error_exit(ERR_SEM_TYPE, last_stmt->line,
                               "If result does not match with declared result type!");
                }
                break;
            }
        }
    } else {
        value_type derived_type = TYPE_UNKNOWN;
        switch (last_stmt->stmt_type) {
        case STMT_EXPR:
            derived_type = last_stmt->data.expr_stmt->result_type;
            break;
        case STMT_VAL:
        case STMT_VAR:
        case STMT_WHILE:
        case STMT_ASSIGN:
        case STMT_FOR:
            derived_type = TYPE_UNIT;
            break;
        case STMT_IF:
            if (last_stmt->data.if_stmt.else_stmt != NULL) {
                derived_type = block_result_type(func->stmt_body);
            } else {
                if (ends_with_unit(func->stmt_body)) {
                    derived_type = TYPE_UNIT;
                } else {
                    error_exit(ERR_SEM_TYPE, last_stmt->line, "Function does not end with unit!");
                }
            }
            break;
        default:
            error_exit(ERR_INTERNAL, last_stmt->line, "Statement does not have return type!");
        }
        if (func->is_main && derived_type != TYPE_UNIT) {
            error_exit(ERR_SEM_TYPE, func->line, "Derived type of main function is not Unit!");
        }
        func->result_ret_type = derived_type;
        attributes *attr_func = search_symbol(func_hash, func->func_name);
        attr_func->return_type = convert_value2param(derived_type);
        attr_func->ret_state = ret_inferred;
    }
    pop_scope();
    func_inspect = prev_func_inspect;
    scope_hash = prev_scope;
    func->state = FUNC_CHECKED;
}

void check_program(program_struct *program) {
    func_hash = create_hashtable();
    if (func_hash == NULL) {
        error_exit(ERR_INTERNAL, 0, "Memory allocation failed!");
    }
    insert_builtin_funcs();
    insert_program_funcs(program);
    function_struct *cursor = program->first_func;
    prog = program;
    while (cursor != NULL) {
        check_function(cursor);
        cursor = cursor->next;
    }
    delete_hashtable(func_hash);
}