/* 
IFJ/IAL project - Compiler for language IFJ26 to language IFJ26code
*
@file symtable.c
*
@date 17-10-2026
@author xdongop00 Phuong Uyen Dongova
*/

#include "symtable.h"

/*
    @brief  Create and initialize the structure of a hashtable, including the array
    @return A valid pointer to an initialized hashtable structure or NULL when allocation/initialization fails
*/
hashtable* create_hashtable(void)
{
    hashtable* hstable = (hashtable*) malloc(sizeof(hashtable));
    if(hstable == NULL)
    {
        fprintf(stderr, "Hashtable allocation failed.\n");
        return NULL;
    }

    if(!hashtable_init(hstable))
    {
        fprintf(stderr, "Hashtable initialization failed.\n");
        free(hstable);
        return NULL;
    }

    return hstable;
}

/* 
    @brief Initialization of a hashtable 
    @param hstable  Valid pointer to an unitialized hashtable structure
    @return true    Succesful initialization of a hashtable
            false   Failed initialization of a hashtable 
*/
bool hashtable_init(hashtable* hstable)
{
    hstable->item_array = calloc(INITIAL_CAPACITY, sizeof(item));
    if(hstable->item_array == NULL)
    {
        hstable->capacity = 0;
        hstable->available_slots = 0;
        hstable->used_slots = 0;
        hstable->tombstoned_slots = 0;
        hstable->parent = NULL;
        return false;
    }

    hstable->capacity = INITIAL_CAPACITY;
    hstable->parent = NULL;
    hstable->available_slots = INITIAL_CAPACITY;
    hstable->used_slots = 0;
    hstable->tombstoned_slots = 0;

    return true;
}

/*
    @brief  Free attributes and id's of items in item array, free the item array and hashtable
    @param  hstable Valid pointer to a hashtable structure, initialized by function hashtable_init
*/
void delete_hashtable(hashtable* hstable)
{
    if(hstable == NULL)
    {
        free(hstable);
        return;
    }

    for(size_t item_index = 0; item_index < hstable->capacity; item_index++)
    {
        item *current = &hstable->item_array[item_index];
        if(current->state == 2)
        {
            free(current->attribute);
            free(current->id);
        }
    }
    free(hstable->item_array);
    free(hstable);
}

/*
    @brief Hash function using a djb2a algorithm
    @param hash_value   Magic constant used in dj2ba

*/
size_t hash_function(hashtable* hstable, char* key)
{
    size_t hash_value = 5381;

    int char_index = 0; 
    while(key[char_index] != '\0')
    {
        hash_value = ((hash_value * 33) ^ key[char_index]);
        char_index++;
    }

    size_t hstable_index = (hash_value % (hstable->capacity));
    return hstable_index;
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