/**
 * Project name: Implementace překladače imperativního jazyka IFJ26
 * @author Izabela Richterová (xrichti00)
 * @file scanner.c
 */

#ifndef STACK_H
#define STACK_H

typedef struct stack t_stack;

void stack_init (t_stack* stack);
void stack_push (t_stack* stack, int data);
void stack_pop (t_stack* stack);
int stack_top (t_stack* stack);
bool stack_is_empty (t_stack* stack);

#endif