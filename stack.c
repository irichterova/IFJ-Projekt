/**
 * Project name: Implementace překladače imperativního jazyka IFJ26
 * @author Izabela Richterová (xrichti00)
 * @file scanner.c
 */

#include <stdio.h>
#include <stdbool.h>

typedef struct element {
    int data;
    t_element* next;
} t_element;

typedef struct stack {
    t_element* top;
} t_stack;

void stack_init (t_stack* stack) {
    stack->top = NULL;
}

void stack_push (t_stack* stack, int data) {
    t_element* new_elem = malloc(sizeof(t_element));
    if (!new_elem) {
        fprintf(stderr, "Error: Allocation fault\n");
        exit(99);
    }
    new_elem->data = data;
    new_elem->next = stack->top;
    stack->top = new_elem;
}

void stack_pop (t_stack* stack) {
    if (stack->top) {
        t_element* elem2delete = stack->top;
        stack->top = elem2delete->next;
        free(elem2delete);
    }
}

int stack_top (t_stack* stack) {
    if (!(stack->top)) {
        fprintf(stderr, "Error: Access to null pointer\n");
        exit(99);
    }
    return (stack->top->data);
}

bool stack_is_empty (t_stack* stack) {
    return (stack->top == NULL);
}