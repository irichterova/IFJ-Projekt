/* 
IFJ/IAL project - Compiler for language IFJ26 to language IFJ26code
*
@file generator.c
*
@date 9-11-2026
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
static value_type var1_type = TYPE_UNKNOWN;
static value_type var2_type = TYPE_UNKNOWN;

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

void gen_builtin();

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
 * @param   var1_type   First variable that could be data type INT (integer)
 * @param   var2_type   Second variable that could be data type FLOAT (double/float)
 */
void int2float(value_type var1_type, value_type var2_type)
{
    if(var1_type == TYPE_INT && var2_type == TYPE_DOUBLE)
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
 * @brief  Generates the final code in IFJ26code 
 * @param  program_code    A valid pointer to a structure program code
*/
int gen_code(program_struct *program_code)
{
    newline_emit_expr(".IFJcode26");
    /* Define global temporary variables used for operations with strings - concatenation */
    newline_emit_expr("DEFVAR GF@$tmp-str1");
    newline_emit_expr("DEFVAR GF@$tmp-str2");
    /*  Create local frame */
    newline_emit_expr("CREATEFRAME");
    function_struct *main_func = program_code->first_func;

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
        /* Variables must be defined before and generated in emit_defvar - pushing defined local frame variable on a stack */
        case EXPR_VARIABLE:
            newline_emit_expr("%s LF@%s$%zu", msg, expression->data.variable.id_name, expression->data.variable.codegen_id);
            break;
        case EXPR_UNA_OP:
            check_ptr(expression->data.unary_op.operand);
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
        /* For generating expressions in a function, we have to print out every single expression in a body of function*/
            for(int i = 0; i < expression->data.call_fn.argc; i++)
            {
            gen_expression(expression->data.call_fn.args[i]);
            }
            /*  Create a local frame for the function and call the function */
            newline_emit_expr("CREATEFRAME");
            newline_emit_expr("CALL fn_%s", expression->data.call_fn.name);
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
void print_func()
{
    char *builtin_name = "$builtin_print";
    int builtin_id = 0;
    newline_emit_expr("LABEL fn_%s", builtin_name);
    newline_emit_expr("PUSHFRAME");
    newline_emit_expr("DEFVAR LF@msg$%i", builtin_id);
    newline_emit_expr("POPS LF@msg$%i", builtin_id);
    newline_emit_expr("DEFVAR LF@$type");
    newline_emit_expr("TYPE LF@$type LF@msg%i", builtin_id);
    newline_emit_expr("JUMPIFEQ print_nil LF@$type string@nil");
    newline_emit_expr("WRITE LF@msg$%i", builtin_id);
    newline_emit_expr("JUMP fn_end_%s", builtin_name);
    newline_emit_expr("LABEL print_nil");
    newline_emit_expr("WRITE string@()");
    newline_emit_expr("LABEL fn_end_%s", builtin_name);
    newline_emit_expr("PUSHS nil@nil");
    newline_emit_expr("POPFRAME");
    newline_emit_expr("RETURN");
}
void str_len()
{
    char *builtin_name = "$builtin_strlen";
    int builtin_id = 0;
    newline_emit_expr("LABEL fn_%s", builtin_name);
    newline_emit_expr("PUSHFRAME");
    newline_emit_expr("DEFVAR LF@str$%i", builtin_id);
    newline_emit_expr("POPS LF@str$%i", builtin_id);
    newline_emit_expr("DEFVAR LF@$len");
    newline_emit_expr("STRLEN LF@$len LF@str$%i", builtin_id);
    newline_emit_expr("PUSHS LF@$len");
    newline_emit_expr("POPFRAME");
    newline_emit_expr("RETURN");
}
void chr()
{
    char *builtin_name = "$builtin_chr";
    int builtin_id = 0;
    newline_emit_expr("LABEL fn_%s", builtin_name);
    newline_emit_expr("PUSHFRAME");
    newline_emit_expr("DEFVAR LF@value$%i", builtin_id);
    newline_emit_expr("POPS LF@value$%i", builtin_id);
    newline_emit_expr("DEFVAR LF@$char");
    newline_emit_expr("INT2STR LF@$char LF@value$%i", builtin_id);
    newline_emit_expr("PUSHS LF@$char");
    newline_emit_expr("POPFRAME");
    newline_emit_expr("RETURN");
}
void ord()
{
    char *builtin_name = "$builtin_ord";
    int builtin_id = 0;
    newline_emit_expr("LABEL fn_%s", builtin_name);
    newline_emit_expr("PUSHFRAME");
    newline_emit_expr("DEFVAR LF@str$%i", builtin_id);
    newline_emit_expr("DEFVAR LF@idx$%i", builtin_id);
    newline_emit_expr("POPS LF@str$%i", builtin_id);
    newline_emit_expr("POPS LF@idx$%i", builtin_id);
    newline_emit_expr("DEFVAR LF@$value");
    newline_emit_expr("STRI2INT LF@$value LF@str$%i LF@idx$%i", builtin_id, builtin_id);
    newline_emit_expr("PUSHS LF@$value");
    newline_emit_expr("POPFRAME");
    newline_emit_expr("RETURN");
}
void to_int()
{
    char *builtin_name = "$builtin_toint";
    int builtin_id = 0;
    newline_emit_expr("LABEL fn_%s", builtin_name);
    newline_emit_expr("PUSHFRAME");
    newline_emit_expr("DEFVAR LF@value$%i", builtin_id);
    newline_emit_expr("POPS LF@value$%i", builtin_id);
    newline_emit_expr("DEFVAR LF@$float");
    newline_emit_expr("FLOAT2INT LF@$float LF@value$%i", builtin_id);
    newline_emit_expr("PUSHS LF@$float");
    newline_emit_expr("POPFRAME");
    newline_emit_expr("RETURN");
}
