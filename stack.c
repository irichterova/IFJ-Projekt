/**
 * Project name: Implementation of an IFJ26 Imperative Language Compiler
 * @author Izabela Richterová (xrichti00)
 * @file stack.c
 * @brief Implements the stack functions
 */

#include "stack.h"
#include <stdio.h>
#include <stdlib.h>

void stack_init(stack_t *stack) {
    stack->top = NULL;
}

void stack_push(stack_t *stack, int data) {
    element_t *new_elem = malloc(sizeof(element_t));
    if (!new_elem) {
        fprintf(stderr, "Error: Allocation fault\n");
        exit(99);
    }
    new_elem->data = data;
    new_elem->next = stack->top;
    stack->top = new_elem;
}

void stack_pop(stack_t *stack) {
    if (stack->top) {
        element_t *elem2delete = stack->top;
        stack->top = elem2delete->next;
        free(elem2delete);
    }
}

int stack_top(stack_t *stack) {
    if (!(stack->top)) {
        fprintf(stderr, "Error: Access to null pointer\n");
        exit(99);
    }
    return (stack->top->data);
}

bool stack_is_empty(stack_t *stack) {
    return (stack->top == NULL);
}

void stack_destruct(stack_t *stack) {
    while (!(stack_is_empty(stack))) {
        stack_pop(stack);
    }
}