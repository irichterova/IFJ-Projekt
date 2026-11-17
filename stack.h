/**
 * Project name: Implementation of an IFJ26 Imperative Language Compiler
 * @author Izabela Richterová (xrichti00)
 * @file stack.h
 * @brief Declares stack structures and functions
 */

#ifndef STACK_H
#define STACK_H

#include <stdbool.h>

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
 * @brief Returns the value at the top of the stack 
 * @param stack Pointer to the stack whose top value is to be returned
 * @return Integer value at the top of the stack
 */
int stack_top (stack_t* stack);

/**
 * @brief Checks whether the stack is empty
 * @param stack Pointer to the stack 
 * @return true if the stack is empty; false otherwise
 */
bool stack_is_empty (stack_t* stack);

/**
 * @brief Destroys the data structure "stack"
 * @param stack Pointer to the stack to be destroyed
 */
void stack_destruct (stack_t* stack);

#endif