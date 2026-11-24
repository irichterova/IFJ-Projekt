/**
 * Project name: Implementation of an IFJ26 Imperative Language Compiler
 * @author Izabela Richterová (xrichti00)
 * @file buffer.h
 * @brief Declares the dynamic buffer structure and its functions
 */

#ifndef BUFFER_H
#define BUFFER_H

typedef struct buffer {
    char *data;
    int size;
    int capacity;
} buffer_t;

/**
 * @brief Initializes the data structure "buffer"
 * @param buffer Pointer to the buffer to be initialized
 */
void buffer_init(buffer_t *buffer);

/**
 * @brief Doubles the capacity of the buffer
 * @param buffer Pointer to the buffer whose capacity is to be increased
 */
void buffer_capacity_increase(buffer_t *buffer);

/**
 * @brief Adds the symbol to the buffer
 * @param buffer Pointer to the buffer the symbol is to be added to
 * @param symbol The character to be added to the buffer
 */
void add_symbol2buffer(buffer_t *buffer, char symbol);

/**
 * @brief Destroys the data structure "buffer"
 * @param buffer Pointer to the buffer to be destroyed
 */
void buffer_destruct(buffer_t *buffer);

#endif