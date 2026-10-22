/* 
IFJ/IAL project - Compiler for language IFJ26 to language IFJ26code
*
@file symtable.c
@brief 
*
@date 17-10-2026
@author xdongop00 Phuong Uyen Dongova
*/

#include "symtable.h"

/*
    @brief Initialization of a hashtable 

*/
bool hashtable_init(hashtable* hstable)
{
    hstable->capacity = INITIAL_CAPACITY;

    hstable->item_array = (item*) calloc(hstable->capacity, sizeof(item));
    if(hstable->item_array == NULL)
    {
        hstable->capacity = 0;
        fprintf(stderr, "Hashtable allocation failed.\n");
        return false;
    }

    hstable->parent = NULL;
    hstable->available_slots = INITIAL_CAPACITY;
    hstable->used_slots = 0;
    hstable->tombstoned_slots = 0;

    return true;
}

void delete_hashtable(hashtable* hstable)
{
    free(hstable->item_array);
    free(hstable);
}

size_t hash_function(hashtable* hstable, char* key)
{
    size_t ascii_addition = 0;

    int char_index = 0;
    while(key[char_index] != '\0')
    {
        ascii_addition += key[char_index];
        char_index++;
    }

    size_t hash_value = (ascii_addition % hstable->capacity);
    return hash_value;
}

bool rehash_hashtable(hashtable* hstable)
{
    size_t rehashed_capacity = hstable->capacity*2;
    item* rehashed_array = calloc(rehashed_capacity, sizeof(item));

    if(rehashed_array == NULL)
    {
        fprintf(stderr, "Rehash of hashtable failed.\n");
        return false;
    }

    for(int i = 0; i < hstable->capacity; i++)
    {
        if(hstable->item_array[i].state == 2)
        {
            rehashed_array[hash_function(rehashed_array, hstable->item_array[i].id)] = hstable->item_array[i];
        }
        
        else
        continue;
    }

    free(hstable->item_array);
    hstable->capacity = rehashed_capacity;
    hstable->item_array = rehashed_array;

    return true;
}