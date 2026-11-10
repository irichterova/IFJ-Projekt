/**
 * Project name: Implementace překladače imperativního jazyka IFJ26
 * @author Izabela Richterová (xrichti00)
 * @file stack.h
 */

#ifndef STACK_H
#define STACK_H

typedef struct element {
    int data;
    struct element* next;
} element_t;

typedef struct stack {
    element_t* top;
} stack_t;

/**
 * @brief Initializes the data structure "stack"
 * @param stack Pointer to the stack to be initialized
 */
void stack_init (stack_t* stack);

/**
 * @brief Pushes data onto the stack
 * @param stack Pointer to the stack onto which the data are to be pushed
 * @param data Data to be pushed onto the stack
 */
void stack_push (stack_t* stack, int data);

/**
 * @brief Removes data from the top of the stack
 * @param stack Pointer to the stack from which data are to be removed
 */
void stack_pop (stack_t* stack);

/**
 * @brief Returns the value of the data at the top of the stack 
 * @param stack Pointer to the stack whose top data value is to be returned
 * @return Integer value of the data at the top of the stack
 */
int stack_top (stack_t* stack);

#endif