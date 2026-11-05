/**
 * Project name: Implementace překladače imperativního jazyka IFJ26
 * @author Izabela Richterová (xrichti00)
 * @file scanner.c
 */

#ifndef STACK_H
#define STACK_H

typedef struct element {
    int data;
    element_t* next;
} element_t;

typedef struct stack {
    element_t* top;
} stack_t;

void stack_init (stack_t* stack);
void stack_push (stack_t* stack, int data);
void stack_pop (stack_t* stack);
int stack_top (stack_t* stack);

#endif