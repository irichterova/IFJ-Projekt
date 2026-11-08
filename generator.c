#include "generator.h"
#include <stdio.h>
#include <stdarg.h>

int main(void)
{

    expr_struct expr2 = {0};
    expr2.node_type = EXPR_LIT_DOUBLE;
    expr2.data.lit_double = 2.534;

    expr_struct expr = {0};
    expr.node_type = EXPR_LIT_INT;
    expr.data.lit_int = 3;

    statement_struct st = {0};
    st.stmt_type = STMT_VAL;
    st.data.var_stmt.id_name = "x";
    st.data.var_stmt.codegen_id = 1;
    st.data.var_stmt.init_expr = &expr2;

    statement_struct st2 = {0};
    st2.stmt_type = STMT_ASSIGN;
    st2.data.id_stmt.assign_expr = &expr;
    st2.data.id_stmt.id_name = st.data.var_stmt.id_name;

    gen_expression(&expr2);
    gen_statement(&st);
    gen_statement(&st2);

    return 0;
}


void emit_expr(char *msg_format,...)
{
    va_list argument;
    va_start(argument, msg_format);
    vprintf(msg_format, argument);
    va_end(argument);
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
            emit_expr("%s int@%i\n", msg, expression->data.lit_int);
            break;
        case EXPR_LIT_DOUBLE:
            emit_expr("%s float@%f\n", msg, expression->data.lit_double);
            break;
        case EXPR_LIT_UNIT:
            emit_expr("%s nil@nil\n", msg);
            break;
        case EXPR_LIT_STRING:
            emit_expr("%s string@", msg);
            emit_expr_str(expression->data.lit_string);
            emit_expr("\n");
            break;
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
            emit_expr("DEFVAR LF@%s$%zu\n", statement->data.var_stmt.id_name, statement->data.var_stmt.codegen_id);
            gen_expression(statement->data.var_stmt.init_expr);
            emit_expr("POPS LF@%s$%zu\n", statement->data.var_stmt.id_name, statement->data.var_stmt.codegen_id);
            break;   
        case STMT_ASSIGN:
            gen_expression(statement->data.id_stmt.assign_expr);
            emit_expr("POPS LF@%s$%zu\n", statement->data.id_stmt.id_name);
            break;
        default:
        break;   
    }
}
