/* 
IFJ/IAL project - Compiler for language IFJ26 to language IFJ26code
*
@file generator.c
*
@date 18-11-2026
@author xdongop00 Phuong Uyen Dongova
*/
#include "generator.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/* Global counter for cycles (if, while) */
static int cycle_counter = 0;
static char *func_name = NULL;
static value_type func_ret = TYPE_UNKNOWN;
static program_struct *func_list = NULL;

/*  Generate code from ast.h */
void gen_function(function_struct* function);
void gen_statement(statement_struct* statement);
void gen_expression(expr_struct* expression);

/* INT2FLOATS conversion */
void int2float(value_type first_type, value_type second_type);

/* Check validation of pointers */
void check_ptr(void *ptr);

/*  Built in functions */
void print_func();
void read_line();
void read_int();
void read_double();
void to_str();
void to_int();
void str_len();
void sub_str();
void ord();
void chr();
void read_bool();


/**
 * @brief  Prints formatted line of IFJ26code without a newline
 * @param  *msg_format Format of a printed line
 * @param  ...         Substitution for unlimited values to print 
*/
void emit_expr(char *msg_format,...)
{
    va_list argument;
    va_start(argument, msg_format);
    vprintf(msg_format, argument);
    va_end(argument);
}

/** 
 * @brief Prints formatted line of IFJ26code with a newline
 * @param  *msg_format Format of a printed line
 * @param  ...         Substitution for unlimited values to print 
*/
void newline_emit_expr(char *msg_format,...)
{
    va_list argument;
    va_start(argument, msg_format);
    vprintf(msg_format, argument);
    va_end(argument);
    putchar('\n');
}

/** 
 * @brief  Prints edited chars of a string based on IFJ26code specifics
 * @param  *str  String which is to be printed 
*/
void emit_expr_str(char *str)
{   
    for(size_t i = 0; str[i] != '\0'; i++)
    {
       unsigned char c = str[i];
        if(c <= 32 || c == 35 || c == 92 || c >= 127)
        {
            emit_expr("\\%03i", c);
        }
        else 
        {
            emit_expr("%c", c);
        }
    }
}
/**
 * @brief   Checks if data type conversion between two variables is needed and generates the conversion 
 * @param  first_type   First variable that could be data type INT (integer)
 * @param  second_type   Second variable that could be data type FLOAT (double/float)
 */
void int2float(value_type first_type, value_type second_type)
{
    if(first_type == TYPE_INT && second_type == TYPE_DOUBLE)
    {
        newline_emit_expr("INT2FLOATS");
    }
    else 
    return;
}
/**
 * @brief   Checks if pointer is NULL, if the specific pointer is not allowed to be NULL, exit with internal error code 99
 * @param   ptr Pointer that is being checked  
 */
void check_ptr(void *ptr)
{
    if(ptr == NULL)
    {
        fprintf(stderr, "Internal error");
        exit(99);
    }
    else
    return;
}

/**
 * @brief   Generate all built in functions
 */
void gen_builtin()
{
        print_func();
        read_line();
        read_int();
        read_double();
        to_str();
        to_int();
        str_len();
        sub_str();
        ord();
        chr();
        read_bool();
}


/** 
 * @brief  Generates the final code in IFJ26code 
 * @param  program_code    A valid pointer to a structure program code
*/
int gen_code(program_struct *program_code)
{
    newline_emit_expr(".IFJcode26");
    /* Define global temporary variables used for operations with strings - concatenation */
    newline_emit_expr("DEFVAR GF@$tmp-str1");
    newline_emit_expr("DEFVAR GF@$tmp-str2");
    /*  Create temporary frame */
    newline_emit_expr("CREATEFRAME");
    function_struct *main_func = program_code->first_func;
    func_list = program_code;
    /* Generating main function first */
    while(main_func != NULL)
    {
        if(main_func->is_main == 1)
        {
            newline_emit_expr("CALL fn_%s", main_func->func_name);
            break;
        }
        else
        main_func = main_func->next;
    }
    newline_emit_expr("CLEARS");
    newline_emit_expr("EXIT int@0");

    gen_builtin();

    /*  Generate rest of the body of the code */
    for(function_struct *func = program_code->first_func; func != NULL; func = func->next)
    {
        gen_function(func);
    }
    return 0;
}

/**
 * @brief  Recursively generate all the variables in a following statement
 * @param  statement   Valid pointer to a statement from which we generate definitions of variables
*/
void emit_defvar(statement_struct *statement)
{
    /* Depending on the current statement type, generate definition of all variables in a statement */
    while(statement != NULL)
    {
        switch(statement->stmt_type)
        {
            case STMT_VAL:
            case STMT_VAR:
                newline_emit_expr("DEFVAR LF@%s$%zu", statement->data.var_stmt.id_name, statement->data.var_stmt.codegen_id);
                break;
            /*In cycles / statements with a body, generate variables from all parts of the body */
            case STMT_IF:
                emit_defvar(statement->data.if_stmt.then_stmt);
                emit_defvar(statement->data.if_stmt.else_stmt);
                break;
            case STMT_WHILE:
                emit_defvar(statement->data.while_lp.while_body);
                break; 
            default:
            break;           
        }
        /* Move the pointer of a current statement to the following statement */
        statement = statement->next;
    }
}
/**
 * @brief   Function that leaves one return value of the last expression on the stack, used for loops
 * @param   expr   Pointer to a last expression in a statement
 * @param   expect Expected return value of the expression, used for checking int2float conversion
 */
void gen_expr_return(statement_struct *expr, value_type expect_ret)
{
    for(statement_struct *stmt = expr; stmt != NULL; stmt=stmt->next)
    {
        if(stmt->next == NULL && stmt->stmt_type == STMT_EXPR)
        {
            gen_expression(stmt->data.expr_stmt);
            int2float(stmt->data.expr_stmt->result_type, expect_ret);
        }
        else
        {
            gen_statement(stmt);
            if(stmt->next == NULL)
            {
                newline_emit_expr("PUSHS nil@nil");
            }
        }
    }
}
/**
 * @brief   Function that finds a function in a program by the name 
 * @param   func_name   Name of the function we are trying to find
 * @return  func    Pointer to a function with a matched name 
 * @return  NULL    If the function is not found 
 */
function_struct *get_func(char *func_name)
{
    for(function_struct *func = func_list->first_func; func != NULL; func = func->next)
    {
        if(strcmp(func->func_name, func_name) == 0)
        {
            return func;
        }
    }
    return NULL;
}
/**
 * @brief  Generate expressions based on their node type 
 * @param  expression  Pointer to a valid expression 
*/
void gen_expression(expr_struct* expression)
{
    /*  Push expressions on a stack depending on their expression type */
    char* msg = "PUSHS";
    switch(expression->node_type)
    {
        /* Push non declared values onto a stack for usage*/
        case EXPR_LIT_INT:
            newline_emit_expr("%s int@%i", msg, expression->data.lit_int);
            break;
        case EXPR_LIT_DOUBLE:
            newline_emit_expr("%s float@%a", msg, expression->data.lit_double);
            break;
        case EXPR_LIT_UNIT:
            newline_emit_expr("%s nil@nil", msg);
            break;
        case EXPR_LIT_STRING:
            emit_expr("%s string@", msg);
            emit_expr_str(expression->data.lit_string);
            emit_expr("\n");
            break;
        case EXPR_LIT_TRUE:
            emit_expr("%s bool@true", msg);
            break;
        case EXPR_LIT_FALSE:
            emit_expr("%s bool@false", msg);
            break;
        /* Variables must be defined before and generated in emit_defvar - pushing defined local frame variable on a stack */
        case EXPR_VARIABLE:
            newline_emit_expr("%s LF@%s$%zu", msg, expression->data.variable.id_name, expression->data.variable.codegen_id);
            break;
        case EXPR_UNA_OP:
            check_ptr(expression->data.unary_op.operand);
            
            /* Generate expression for unary operand ! */
            if(expression->data.unary_op.op_type == OP_NOT)
            {
                gen_expression(expression->data.unary_op.operand);
                newline_emit_expr("NOTS");
                break;
            }
        /* If the literal is type of int, push integer zero, if the literal is type double, push zero in float format.
        Subtract the expression value */
            if(expression->result_type == TYPE_INT)
            newline_emit_expr("%s int@0", msg);
            else
            newline_emit_expr("%s float@0x0p+0", msg);

            gen_expression(expression->data.unary_op.operand);
            newline_emit_expr("SUBS");
            break;
        case EXPR_BIN_OP:
        {
        /* For binar operators, there are two expressions - operands, we have to print out both of them */
            expr_struct *left_op = expression->data.binary_op.left_operand;
            expr_struct *right_op = expression->data.binary_op.right_operand;
            check_ptr(left_op);
            check_ptr(right_op);
            
            operator_type bin_op_type = expression->data.binary_op.op_type;
            /* If binary operator is a logic operator (&&, ||), depending on the boolean value either skip the right operand (||) or generate 
            the right operator for boolean value */
            if(bin_op_type == OP_AND || bin_op_type == OP_OR)
            {
                char *bool_res;
                if(bin_op_type == OP_OR)
                {
                    bool_res = "true";
                }
                else
                {
                    bool_res = "false";
                }
                int logic_count = ++cycle_counter;
                gen_expression(left_op);
                newline_emit_expr("%s bool@%s", msg, bool_res);
                newline_emit_expr("JUMPIFEQS $skip_right_op_%i", logic_count);
                gen_expression(right_op);
                newline_emit_expr("JUMP $end_logic_op_%i", logic_count);
                newline_emit_expr("LABEL $skip_right_op_%i", logic_count);
                newline_emit_expr("%s bool@%s", msg, bool_res);
                newline_emit_expr("LABEL $end_logic_op_%i", logic_count);
                break;
            }
            value_type left_type = left_op->result_type;
            value_type right_type = right_op->result_type;

            gen_expression(left_op);
            /*  Check if conversion between operands is needed, if the data types are different (int/float) convert int to float */
            int2float(left_type, right_type);
            gen_expression(right_op);
            int2float(right_type, left_type);

            switch(expression->data.binary_op.op_type)
            {
                /* Recognize the correct PLUS operation depending on the result type, if the result is a string - do concatenation
                    for any other possible result type of a number - do addition */
                case OP_PLUS:
                    if(expression->result_type == TYPE_STRING)
                    {
                        newline_emit_expr("POPS GF@$tmp-str2");
                        newline_emit_expr("POPS GF@$tmp-str1");
                        newline_emit_expr("CONCAT GF@$tmp-str1 GF@$tmp-str1 GF@$tmp-str2");
                        newline_emit_expr("PUSHS GF@$tmp-str1");
                    }
                    else
                    {
                        newline_emit_expr("ADDS");
                    }
                    break;
                /* For any other binary operator, generate the operation */
                case OP_BIN_MINUS:
                    newline_emit_expr("SUBS");
                    break;
                case OP_MULTIPLICATION:
                    newline_emit_expr("MULS");
                    break;
                case OP_DIVISION:
                /* Differentiate dividing signed and unsigned integers */
                    if(expression->result_type == TYPE_INT)
                    {
                        newline_emit_expr("IDIVS");
                    }
                    else 
                    newline_emit_expr("DIVS");
                    break;
                case OP_LESS:
                    newline_emit_expr("LTS");
                    break;
                case OP_GREATER:
                    newline_emit_expr("GTS");
                    break;
                case OP_EQUAL_LESS:
                    newline_emit_expr("GTS");
                    newline_emit_expr("NOTS");
                    break;
                case OP_EQUAL_GREATER:
                    newline_emit_expr("LTS");
                    newline_emit_expr("NOTS");
                    break;
                case OP_EQUAL:
                    newline_emit_expr("EQS");
                    break;
                case OP_NOTEQUAL:
                    newline_emit_expr("EQS");
                    newline_emit_expr("NOTS");
                    break;
                default:
                break;
            }
            break;
        }
        case EXPR_CALL_FUNC:
            char *expr_fn_name = expression->data.call_fn.name;
            function_struct *called_func = get_func(expr_fn_name);
        /* For generating expressions in a function, we have to print out every single expression in a body of function*/
            for(int i = 0; i < expression->data.call_fn.argc; i++)
            {
            gen_expression(expression->data.call_fn.args[i]);
            /* Check if INT2FLOATS conversion is needed between arguments of a function */
            if(called_func != NULL)
            {
                int2float(expression->data.call_fn.args[i]->result_type, called_func->param_args[i].param_type);
            }
            }
            /*  Create a local frame for the function and call the function */
            newline_emit_expr("CREATEFRAME");
            /*  Check if the function is a built in type, if so call function with prefix fn_$builtin_ */
            if((strcmp(expr_fn_name, "print") == 0) ||
            (strcmp(expr_fn_name, "readLine") == 0) ||
            (strcmp(expr_fn_name, "readInt") == 0) ||
            (strcmp(expr_fn_name, "readDouble") == 0) ||
            (strcmp(expr_fn_name, "toStr") == 0) ||
            (strcmp(expr_fn_name, "toInt") == 0) ||
            (strcmp(expr_fn_name, "strLength") == 0) ||
            (strcmp(expr_fn_name, "subStr") == 0) ||
            (strcmp(expr_fn_name, "ord") == 0) ||
            (strcmp(expr_fn_name, "chr") == 0))
            {
                newline_emit_expr("CALL fn_$builtin_%s", expr_fn_name);
            }
            else
            newline_emit_expr("CALL fn_%s", expr_fn_name);
            break;
            case EXPR_IF:
            int if_expr_counter = ++cycle_counter;
            expr_struct *if_expr = expression->data.if_stmt.if_expr;
            gen_expression(if_expr);
            newline_emit_expr("PUSHS bool@false");
            newline_emit_expr("JUMPIFEQS if_expr_else_%i", if_expr_counter);
            gen_expr_return(expression->data.if_stmt.then_stmt, expression->result_type);
            newline_emit_expr("JUMP if_expr_end_%i", if_expr_counter);
            newline_emit_expr("LABEL if_expr_else_%i", if_expr_counter);
            gen_expr_return(expression->data.if_stmt.else_stmt, expression->result_type);
            newline_emit_expr("LABEL if_expr_end_%s", if_expr_counter);
            break;
        default:
        break;
    }
}


/**
 * @brief   Generate statements for all statement types
 * @param   statement  Valid pointer to a statement to generate
*/
void gen_statement(statement_struct* statement)
{
    char* msg = "POPS";
    switch(statement->stmt_type)
    {
        case STMT_VAL:
        case STMT_VAR:
        {
            check_ptr(statement->data.var_stmt.init_expr);
            check_ptr(statement->data.var_stmt.id_name);
            expr_struct *var_init = statement->data.var_stmt.init_expr;
            gen_expression(var_init);
            /*  Check if conversion is needed */
            int2float(var_init->result_type, statement->data.var_stmt.declared_type);
            newline_emit_expr("%s LF@%s$%zu", msg, statement->data.var_stmt.id_name, statement->data.var_stmt.codegen_id);
            break;   
        }
        case STMT_ASSIGN:
        {
            check_ptr(statement->data.id_stmt.id_name);
            check_ptr(statement->data.id_stmt.assign_expr);
            expr_struct *asgn_expr = statement->data.id_stmt.assign_expr;
            gen_expression(asgn_expr);
            int2float(asgn_expr->result_type, statement->data.id_stmt.id_type);
            newline_emit_expr("%s LF@%s$%zu", msg, statement->data.id_stmt.id_name, statement->data.id_stmt.codegen_id);
            break;
        }
        case STMT_EXPR:
            gen_expression(statement->data.expr_stmt);
            newline_emit_expr("%s LF@$trash", msg);
            break;
        case STMT_IF:
        {
            /*  Local counter for if-cycles, to differentiate ifs in a local frame   */
            int if_counter = 0;
            if_counter = ++cycle_counter;
            gen_expression(statement->data.if_stmt.if_expr);
            /* Push boolean value false to a stack for the use of comparing */
            newline_emit_expr("PUSHS bool@false");
            /*If the condition is false, jump to the else branch*/
            newline_emit_expr("JUMPIFEQS if_else_%i", if_counter);

            /* Do if condition is true (different from false) -> then */
            statement_struct *then_stmts = statement->data.if_stmt.then_stmt;
            while(then_stmts != NULL)
            {
                gen_statement(then_stmts); 
                then_stmts = then_stmts->next;
            }
            newline_emit_expr("JUMP if_end_%i", if_counter);

            /* Do if condition is false -> else */
            newline_emit_expr("LABEL if_else_%i", if_counter);
            statement_struct *else_stmts = statement->data.if_stmt.else_stmt;
            while(else_stmts != NULL)
            {
                gen_statement(else_stmts);
                else_stmts = else_stmts->next;
            }

            newline_emit_expr("LABEL if_end_%i", if_counter);
            break;
        }
        case STMT_RETURN:
            if(statement->data.ret_stmt != NULL)
            {
                gen_expression(statement->data.ret_stmt);
                int2float(statement->data.ret_stmt->result_type, func_ret);
            }
            /* No return value left on the stack  */
            else 
            {
                newline_emit_expr("PUSHS nil@nil");
            }
            newline_emit_expr("JUMP end_%s", func_name);
            break;
        case STMT_WHILE:
        {
            /* Local while-cycle counter to differentiate whiles in a locla frame*/
            int while_counter = 0;
            while_counter = ++cycle_counter;
            newline_emit_expr("LABEL while_%i", while_counter);
            gen_expression(statement->data.while_lp.while_expr);
            /*  Push boolean value false to a stack for the use of comparing the condition/body */
            newline_emit_expr("PUSHS bool@false");

            /*  If while the condition is not filled, jump to the end */
            newline_emit_expr("JUMPIFEQS end_while_%i", while_counter);

            /* While the condition is filled */
            for(statement_struct *stmt = statement->data.while_lp.while_body; stmt != NULL; stmt = stmt->next)
            {
                gen_statement(stmt);
            } 
            newline_emit_expr("JUMP while_%i", while_counter);

            newline_emit_expr("LABEL end_while_%i", while_counter); 
            break;
        }          
        default:
        break;   
    }
}

/** 
 * @brief   Generate a function body
 * @param   function   Valid pointer to a function  
*/
void gen_function(function_struct *function)
{
    check_ptr(function->func_name);
    /* Set global variables to a current function attributes, so the other functions can use them */ 
    func_name = function->func_name;
    func_ret = function->result_ret_type;

    /* Start generating the function, push temporary frame to a local one and define temporary variable for return value */
    newline_emit_expr("LABEL fn_%s", func_name);
    newline_emit_expr("PUSHFRAME");
    newline_emit_expr("DEFVAR LF@$trash");

    if(function->param_argc != 0)
    {
        for(int i = 0; i < function->param_argc; i++)
        {
            check_ptr(function->param_args[i].param_name);
        }
    }

    /* Define all the variables in the functions arguments */
    for(int i = 0; i< function->param_argc; i++)
    {
        newline_emit_expr("DEFVAR LF@%s$%zu", function->param_args[i].param_name, function->param_args[i].codegen_id);
    }
    /*  Define all the variables in the body of the function */
    emit_defvar(function->stmt_body);
    for(int j = function->param_argc - 1; j >= 0; j--)
    {
        newline_emit_expr("POPS LF@%s$%zu", function->param_args[j].param_name, function->param_args[j].codegen_id);
    }
    /* Generate all statements in a function*/
    statement_struct *stmt = function->stmt_body;
    /* Check the return value of every statement */
    while(stmt != NULL)
    {
        if(stmt->next == NULL)
        {
            if(stmt->stmt_type == STMT_EXPR)
            {
                gen_expression(stmt->data.expr_stmt);
                /* If a functions return type is Unit and the expected type is not Unit, throw the return value, later return nil */
                if(func_ret == TYPE_UNIT && stmt->data.expr_stmt->result_type != TYPE_UNIT)
                {
                    newline_emit_expr("POPS LF@$trash");
                }
            }
            else 
            {
                gen_statement(stmt);
            }
        }
        else
        {
            gen_statement(stmt);
        }
        stmt = stmt->next;
    }

    /* Return nil for functions with return value Unit */
    if(func_ret == TYPE_UNIT)
    {
        newline_emit_expr("PUSHS nil@nil");
    }

    /* End of function, pop local frame */
    newline_emit_expr("LABEL end_%s", func_name);
    newline_emit_expr("POPFRAME");
    newline_emit_expr("RETURN");
}
/**
 * @brief Built in function that prints the value onto standard output
 */
void print_func()
{
    char *builtin_name = "$builtin_print";
    newline_emit_expr("LABEL fn_%s", builtin_name);
    newline_emit_expr("PUSHFRAME");
    newline_emit_expr("DEFVAR LF@msg");
    newline_emit_expr("POPS LF@msg");
    newline_emit_expr("DEFVAR LF@$type");
    /*  Save the data type of the term to a temporary variable $type*/
    newline_emit_expr("TYPE LF@$type LF@msg");
    /*  If the data type is nil, jump to print_nil*/
    newline_emit_expr("JUMPIFEQ print_nil LF@$type string@nil");
    /*  For any other data type */
    newline_emit_expr("WRITE LF@msg");
    newline_emit_expr("JUMP fn_end_%s", builtin_name);
    /*  Print nil () */
    newline_emit_expr("LABEL print_nil");
    newline_emit_expr("WRITE string@()");
    newline_emit_expr("LABEL fn_end_%s", builtin_name);

    newline_emit_expr("PUSHS nil@nil");
    newline_emit_expr("POPFRAME");
    newline_emit_expr("RETURN");
}
/**
 * @brief   Built in function that returns the lenght of a string
 */
void str_len()
{
    char *builtin_name = "$builtin_strLength";
    newline_emit_expr("LABEL fn_%s", builtin_name);
    newline_emit_expr("PUSHFRAME");
    newline_emit_expr("DEFVAR LF@str");
    newline_emit_expr("POPS LF@str");
    /*  Temprorary variable to save the lenght */
    newline_emit_expr("DEFVAR LF@$len");
    newline_emit_expr("STRLEN LF@$len LF@str");
    newline_emit_expr("PUSHS LF@$len");
    newline_emit_expr("POPFRAME");
    newline_emit_expr("RETURN");
}
/**
 * @brief   Built in function that returns char that is under the value converted by ASCII 
 */
void chr()
{
    char *builtin_name = "$builtin_chr";
    newline_emit_expr("LABEL fn_%s", builtin_name);
    newline_emit_expr("PUSHFRAME");
    newline_emit_expr("DEFVAR LF@value");
    newline_emit_expr("POPS LF@value");
    newline_emit_expr("DEFVAR LF@$char");
    /* Convert the integer to a char with the ASCII value of the integer and save it to a temporary variable $char, return the char*/
    newline_emit_expr("INT2CHAR LF@$char LF@value");
    newline_emit_expr("PUSHS LF@$char");
    newline_emit_expr("POPFRAME");
    newline_emit_expr("RETURN");
}
/**
 * @brief   Built in function that returns value that is created by converting the char to ASCII value, if the string is empty or the index is invalid, return -1
 */
void ord()
{
    char *builtin_name = "$builtin_ord";
    newline_emit_expr("LABEL fn_%s", builtin_name);
    newline_emit_expr("PUSHFRAME");
    newline_emit_expr("DEFVAR LF@idx");
    newline_emit_expr("POPS LF@idx");
    newline_emit_expr("DEFVAR LF@str");
    newline_emit_expr("POPS LF@str");
    newline_emit_expr("DEFVAR LF@$char");
    newline_emit_expr("DEFVAR LF@$value");
    newline_emit_expr("DEFVAR LF@$len");

    newline_emit_expr("LT LF@$value LF@idx int@0");
    newline_emit_expr("JUMPIFEQ ord_err LF@$value bool@true");

    newline_emit_expr("STRLEN LF@$len LF@str");
    newline_emit_expr("LT LF@$value LF@idx LF@$len");
    newline_emit_expr("JUMPIFEQ ord_err LF@$value bool@false");

    /*  Convert the char on the index of the string to a ASCII value and save it to temporary value $value*/
    newline_emit_expr("STRI2INT LF@$char LF@str LF@idx");
    newline_emit_expr("PUSHS LF@$char");
    newline_emit_expr("JUMP end_fn_%s", builtin_name);

    newline_emit_expr("LABEL ord_err");
    newline_emit_expr("PUSHS int@-1");

    newline_emit_expr("LABEL end_fn_%s", builtin_name);
    newline_emit_expr("POPFRAME");
    newline_emit_expr("RETURN");
}
/**
 * @brief   Built in function that converts a double to an integer
 */
void to_int()
{
    char *builtin_name = "$builtin_toInt";
    newline_emit_expr("LABEL fn_%s", builtin_name);
    newline_emit_expr("PUSHFRAME");
    newline_emit_expr("DEFVAR LF@value");
    newline_emit_expr("POPS LF@value");
    newline_emit_expr("DEFVAR LF@$float");
    /*  Convert float to integer and save it to temporary variable $float */
    newline_emit_expr("FLOAT2INT LF@$float LF@value");
    newline_emit_expr("PUSHS LF@$float");
    newline_emit_expr("POPFRAME");
    newline_emit_expr("RETURN");
}
/**
 * @brief   Built in function that returns converted any type of a value to a string based on the data type
 */
void to_str()
{
    char *builtin_name = "$builtin_toStr";
    newline_emit_expr("LABEL fn_%s", builtin_name);
    newline_emit_expr("PUSHFRAME");
    newline_emit_expr("DEFVAR LF@value$");
    newline_emit_expr("POPS LF@value$");

    /*  Compare data type of the value with all the other data types and jump to the right label */
    newline_emit_expr("DEFVAR LF@$type");
    newline_emit_expr("TYPE LF@$type LF@value$");
    newline_emit_expr("JUMPIFEQ int_type LF@$type string@int");
    newline_emit_expr("JUMPIFEQ float_type LF@$type string@float");
    newline_emit_expr("JUMPIFEQ string_type LF@$type string@string");
    newline_emit_expr("JUMPIFEQ type_nil LF@$type string@nil");
    newline_emit_expr("JUMPIFEQ bool_type LF@$type string@bool");

    /*  If the data type is string return the value */
    newline_emit_expr("LABEL string_type");
    newline_emit_expr("PUSHS LF@value$");
    newline_emit_expr("JUMP end_fn_%s", builtin_name);

    /*  If the data type is integer, convert the integer to a char */
    newline_emit_expr("LABEL int_type");
    newline_emit_expr("DEFVAR LF@$str");
    newline_emit_expr("INT2STR LF@$str LF@value$");
    newline_emit_expr("PUSHS LF@$str");
    newline_emit_expr("JUMP end_fn_%s", builtin_name);

    /* If the data type is float, convert the float to a char first */
    newline_emit_expr("LABEL float_type");
    newline_emit_expr("DEFVAR LF@$str");
    newline_emit_expr("FLOAT2STR LF@$str LF@value$");
    newline_emit_expr("PUSHS LF@$str");
    newline_emit_expr("JUMP end_fn_%s", builtin_name);

    /* If the data tupe is bool, push the boolean value */
    newline_emit_expr("LABEL bool_type");
    newline_emit_expr("JUMPIFEQ bool_false LF@value$ bool@false");
    newline_emit_expr("PUSHS string@true");
    newline_emit_expr("JUMP end_fn_%s", builtin_name);
    newline_emit_expr("LABEL bool_false");
    newline_emit_expr("PUSHS string@false");
    newline_emit_expr("JUMP end_fn_%s", builtin_name);

    /* If the data type is nil, print () */
    newline_emit_expr("LABEL type_nil");
    newline_emit_expr("PUSHS string@()");    

    newline_emit_expr("LABEL end_fn_%s", builtin_name);
    newline_emit_expr("POPFRAME");
    newline_emit_expr("RETURN");
}
/**
 * @brief   Built in function that reads one line of string ending with LF or until it reaches EOF
 */
void read_line()
{
    char *builtin_name = "$builtin_readLine";
    newline_emit_expr("LABEL fn_%s", builtin_name);
    newline_emit_expr("PUSHFRAME");
    newline_emit_expr("DEFVAR LF@$value");
    newline_emit_expr("READ LF@$value string");
    newline_emit_expr("DEFVAR LF@$type");
    newline_emit_expr("TYPE LF@$type LF@$value");

    newline_emit_expr("JUMPIFEQ eof_line LF@$type string@nil");
    newline_emit_expr("PUSHS LF@$value");
    newline_emit_expr("JUMP end_fn_%s", builtin_name);
    newline_emit_expr("LABEL eof_line");
    newline_emit_expr("PUSHS string@");

    newline_emit_expr("LABEL end_fn_%s", builtin_name);
    newline_emit_expr("POPFRAME");
    newline_emit_expr("RETURN");
}
void read_int()
{
    char *builtin_name = "$builtin_readInt";
    newline_emit_expr("LABEL fn_%s", builtin_name);
    newline_emit_expr("PUSHFRAME");
    newline_emit_expr("DEFVAR LF@$val");
    newline_emit_expr("READ LF@$val int");
    newline_emit_expr("DEFVAR LF@$type");
    newline_emit_expr("TYPE LF@$type LF@$val");

    newline_emit_expr("JUMPIFEQ int_err LF@$type string@nil");
    newline_emit_expr("PUSHS LF@$val");
    newline_emit_expr("JUMP end_fn_%s", builtin_name);

    newline_emit_expr("LABEL int_err");
    newline_emit_expr("PUSHS int@0");

    newline_emit_expr("LABEL end_fn_%s", builtin_name);
    newline_emit_expr("POPFRAME");
    newline_emit_expr("RETURN");
}
/*  Same as read int but instead of int, read type bool */
void read_bool()
{
    char *builtin_name = "$builtin_readBoolean";
    newline_emit_expr("LABEL fn_%s", builtin_name);
    newline_emit_expr("PUSHFRAME");
    newline_emit_expr("DEFVAR LF@$val");
    newline_emit_expr("READ LF@$val bool");
    newline_emit_expr("DEFVAR LF@$type");
    newline_emit_expr("TYPE LF@$type LF@$val");

    newline_emit_expr("JUMPIFEQ bool_err LF@$type string@nil");
    newline_emit_expr("PUSHS LF@$val");
    newline_emit_expr("JUMP end_fn_%s", builtin_name);

    newline_emit_expr("LABEL bool_err");
    newline_emit_expr("PUSHS bool@false");

    newline_emit_expr("LABEL end_fn_%s", builtin_name);
    newline_emit_expr("POPFRAME");
    newline_emit_expr("RETURN");
}
void read_double()
{
    char *builtin_name = "$builtin_readDouble";
    newline_emit_expr("LABEL fn_%s", builtin_name);
    newline_emit_expr("PUSHFRAME");
    newline_emit_expr("DEFVAR LF@$val");
    newline_emit_expr("READ LF@$val float");
    newline_emit_expr("DEFVAR LF@$type");
    newline_emit_expr("TYPE LF@$type LF@$val");

    newline_emit_expr("JUMPIFEQ float_err LF@$type string@nil");
    newline_emit_expr("PUSHS LF@$val");
    newline_emit_expr("JUMP end_fn_%s", builtin_name);

    newline_emit_expr("LABEL float_err");
    newline_emit_expr("PUSHS float@0x0p+0");

    newline_emit_expr("LABEL end_fn_%s", builtin_name);
    newline_emit_expr("POPFRAME");
    newline_emit_expr("RETURN");
}

void sub_str()
{
    char *builtin_name = "$builtin_subStr";
    newline_emit_expr("LABEL fn_%s", builtin_name);
    newline_emit_expr("PUSHFRAME");
    newline_emit_expr("DEFVAR LF@end");
    newline_emit_expr("POPS LF@end");
    newline_emit_expr("DEFVAR LF@start");
    newline_emit_expr("POPS LF@start");
    newline_emit_expr("DEFVAR LF@str");
    newline_emit_expr("POPS LF@str");

    newline_emit_expr("DEFVAR LF@$substr");
    newline_emit_expr("DEFVAR LF@$len");
    newline_emit_expr("DEFVAR LF@$value");
    newline_emit_expr("DEFVAR LF@$char");
    newline_emit_expr("STRLEN LF@$len LF@str");

    newline_emit_expr("LT LF@$value LF@start int@0");
    newline_emit_expr("JUMPIFEQ substr_err LF@$value bool@true");
    newline_emit_expr("LT LF@$value LF@end int@0");
    newline_emit_expr("JUMPIFEQ substr_err LF@$value bool@true");
    newline_emit_expr("LT LF@$value LF@end LF@start");
    newline_emit_expr("JUMPIFEQ substr_err LF@$value bool@true");
    newline_emit_expr("GT LF@$value LF@$len LF@start");
    newline_emit_expr("JUMPIFEQ substr_err LF@$value bool@false");
    newline_emit_expr("LT LF@$value LF@$len LF@end");
    newline_emit_expr("JUMPIFEQ substr_err LF@$value bool@true");

    newline_emit_expr("MOVE LF@$substr string@");
    newline_emit_expr("LABEL substr_add");
    newline_emit_expr("JUMPIFEQ substr_done LF@start LF@end");
    newline_emit_expr("GETCHAR LF@$char LF@str LF@start");
    newline_emit_expr("CONCAT LF@$substr LF@$substr LF@$char");
    newline_emit_expr("ADD LF@start LF@start int@1");
    newline_emit_expr("JUMP substr_add");

    newline_emit_expr("LABEL substr_done");
    newline_emit_expr("PUSHS LF@$substr");
    newline_emit_expr("JUMP end_fn_%s", builtin_name);
    newline_emit_expr("LABEL substr_err");
    newline_emit_expr("EXIT int@40");
    
    newline_emit_expr("LABEL end_fn_%s", builtin_name);
    newline_emit_expr("POPFRAME");
    newline_emit_expr("RETURN");
}
