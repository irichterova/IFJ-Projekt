#include "generator.h"
#include <stdio.h>
#include <stdarg.h>


void emit_expr(char *msg_format,...)
{
    va_list argument;
    va_start(argument, msg_format);
    vprintf(msg_format, argument);
    va_end(argument);
}

void newline_emit_expr(char *msg_format,...)
{
    va_list argument;
    va_start(argument, msg_format);
    vprintf(msg_format, argument);
    va_end(argument);
    putchar('\n');
}

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
                    newline_emit_expr("MUL");
                    break;
                case OP_DIVISION:
                    newline_emit_expr("DIV");
                    break;
            }
        default:
        break;
    }
}

void gen_statement(statement_struct* statement)
{
    switch(statement->stmt_type)
    {
        case STMT_VAL:
        case STMT_VAR:
            newline_emit_expr("DEFVAR LF@%s$%zu", statement->data.var_stmt.id_name, statement->data.var_stmt.codegen_id);
            gen_expression(statement->data.var_stmt.init_expr);
            newline_emit_expr("POPS LF@%s$%zu", statement->data.var_stmt.id_name, statement->data.var_stmt.codegen_id);
            break;   
        case STMT_ASSIGN:
            gen_expression(statement->data.id_stmt.assign_expr);
            newline_emit_expr("POPS LF@%s$%zu", statement->data.id_stmt.id_name, statement->data.id_stmt.codegen_id);
            break;
        default:
        break;   
    }
}
