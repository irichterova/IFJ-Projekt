/**
 * Project name: Implementation of an IFJ26 Imperative Language Compiler
 * @author Izabela Richterová (xrichti00)
 * @file buffer.c
 * @brief Implements the dynamic buffer functions
 */

#include "buffer.h"
#include <stdio.h>
#include <stdlib.h>

void buffer_init(buffer_t *buffer) {
    buffer->capacity = 10; // baseline capacity
    buffer->size = 0;
    buffer->data = malloc(buffer->capacity * sizeof(char));
    if (!(buffer->data))
        exit(99);
}

void buffer_capacity_increase(buffer_t *buffer) {
    char *new_data = realloc(buffer->data, buffer->capacity * 2 * sizeof(char));
    if (!new_data)
        exit(99);
    buffer->data = new_data;
    buffer->capacity = buffer->capacity * 2;
}

void add_symbol2buffer(buffer_t *buffer, char symbol) {
    if (buffer->size == buffer->capacity)
        buffer_capacity_increase(buffer);
    buffer->data[buffer->size] = symbol;
    buffer->size++;
}

void buffer_destruct(buffer_t *buffer) {
    buffer->capacity = 0;
    buffer->size = 0;
    free(buffer->data);
    buffer->data = NULL;
}