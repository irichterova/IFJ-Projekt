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

/*
    @brief  Prints formatted line of IFJ26code without a newline
    @param  *msg_format Format of a printed line
    @param  ...         Substitution for unlimited values to print 
*/
void emit_expr(char *msg_format,...)
{
    va_list argument;
    va_start(argument, msg_format);
    vprintf(msg_format, argument);
    va_end(argument);
}

/*
    @brief Prints formatted line of IFJ26code with a newline
    @param  *msg_format Format of a printed line
    @param  ...         Substitution for unlimited values to print 
*/
void newline_emit_expr(char *msg_format,...)
{
    va_list argument;
    va_start(argument, msg_format);
    vprintf(msg_format, argument);
    va_end(argument);
    putchar('\n');
}

/*
    @brief  Prints edited chars of a string based on IFJ26code specifics
    @param  *str  String which is to be printed 
*/
void emit_expr_str(char *str)
{   
    for(size_t i = 0; str[i] != '\0'; i++)
    {
        char c = str[i];
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

/*
    @brief  Generate expressions based on their node type 
    @param  expression  Pointer to a valid expression 
*/
void gen_expression(expr_struct* expression)
{
    char* msg = "PUSHS";
    switch(expression->node_type)
    {
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
        case EXPR_VARIABLE:
            newline_emit_expr("%s LF@%s$%zu", msg, expression->data.variable.id_name, expression->data.variable.codegen_id);
            break;
        case EXPR_UNA_OP:
            if(expression->result_type == TYPE_INT)
            newline_emit_expr("%s int@0", msg);
            else
            newline_emit_expr("%s float@0x0p+0", msg);
            gen_expression(expression->data.unary_op.operand);
            newline_emit_expr("SUBS");
            break;
        case EXPR_BIN_OP:
        /* For binar operators, there are two expressions - operands, we have to print out both of them */
            gen_expression(expression->data.binary_op.left_operand);
            gen_expression(expression->data.binary_op.right_operand);
            switch(expression->data.binary_op.op_type)
            {
                case OP_PLUS:
                    newline_emit_expr("ADDS");
                    break;
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
        case EXPR_CALL_FUNC:
        /* For generating expressions in a function, we have to print out every single expression in a function*/
            for(int i = 0; i < expression->data.call_fn.argc; i++)
            {
            gen_expression(expression->data.call_fn.args[i]);
            }
            newline_emit_expr("CREATEFRAME");
            newline_emit_expr("CALL fn_%s", expression->data.call_fn.name);
            break;
        default:
        break;
    }
}

void gen_statement(statement_struct* statement)
{
    char* msg = "POPS";
    switch(statement->stmt_type)
    {
        case STMT_VAL:
        case STMT_VAR:
            newline_emit_expr("DEFVAR LF@%s$%zu", statement->data.var_stmt.id_name, statement->data.var_stmt.codegen_id);
            gen_expression(statement->data.var_stmt.init_expr);
            newline_emit_expr("%s LF@%s$%zu", msg, statement->data.var_stmt.id_name, statement->data.var_stmt.codegen_id);
            break;   
        case STMT_ASSIGN:
            gen_expression(statement->data.id_stmt.assign_expr);
            newline_emit_expr("%s LF@%s$%zu", msg, statement->data.id_stmt.id_name, statement->data.id_stmt.codegen_id);
            break;
        case STMT_EXPR:
            gen_expression(statement->data.expr_stmt);
            newline_emit_expr("%s LF@$trash", msg);
            break;
        case STMT_IF:
            int cycle_count = 1;
            gen_expression(statement->data.if_stmt.if_expr);
            newline_emit_expr("PUSHS bool@false");
            newline_emit_expr("JUMPIFEQS if_else_%i", cycle_count);
            statement_struct *then_stmts = statement->data.if_stmt.then_stmt;
            while(then_stmts != NULL)
            {
                gen_statement(then_stmts);
                then_stmts = then_stmts->next;
            }
            newline_emit_expr("JUMP if_end_%i", cycle_count);
            newline_emit_expr("LABEL if_else_%i", cycle_count);
            statement_struct *else_stmts = statement->data.if_stmt.else_stmt;
            while(else_stmts != NULL)
            {
                gen_statement(else_stmts);
                else_stmts = else_stmts->next;
            }
            newline_emit_expr("LABEL if_end_%i", cycle_count++);
            break;
            
        default:
        break;   
    }
}

void gen_function(function_struct *function)
{
    newline_emit_expr("LABEL fn_%s", function->func_name);
    newline_emit_expr("PUSHFRAME");
    newline_emit_expr("DEFVAR LF@$trash");
    for(int i = 0; i< function->param_argc; i++)
    {
     newline_emit_expr("DEFVAR LF@%s$%zu", function->param_args[i].param_name, function->param_args[i].codegen_id);
    }
    for(int j = function->param_argc - 1; j >= 0; j--)
    {
        newline_emit_expr("POPS LF@%s$%zu", function->param_args[j].param_name, function->param_args[j].codegen_id);
    }
    /* A temporary statement struct to help with readable code*/
    statement_struct *stmt = function->stmt_body;
    while(stmt != NULL)
    {
        if(stmt->next != NULL)
        {
            gen_statement(stmt);
        }
        switch(stmt->stmt_type)
        {
            case STMT_VAL:
            case STMT_VAR:
                gen_statement(stmt);
                break;
            case STMT_EXPR:                    
                if(stmt->data.expr_stmt->result_type == TYPE_UNIT)
                {
                    newline_emit_expr("POPS LF@$trash");
                    newline_emit_expr("PUSHS nil@nil");
                }
                break;
            case STMT_ASSIGN:
                if(stmt->next != NULL)
                {
                   gen_statement(stmt);
                   newline_emit_expr("POPS LF@%s$%zu", stmt->data.id_stmt.id_name, stmt->data.id_stmt.codegen_id);
                }
                else 
                {
                    newline_emit_expr("PUSHS nil@nil");
                }
                break;
            default:
            break;
        }
        stmt = stmt->next;
    }
    newline_emit_expr("POPFRAME");
    newline_emit_expr("RETURN");
}