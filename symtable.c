/* 
IFJ/IAL project - Compiler for language IFJ26 to language IFJ26code
*
@file symtable.c
*
@date 17-10-2026
@author xdongop00 Phuong Uyen Dongova
*/

#include "symtable.h"
#include <string.h>

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
        if(current->state == slot_taken)
        {
            free(current->attribute);
            free(current->id);
        }
    }
    free(hstable->item_array);
    free(hstable);
}

/*
    @brief  Convert the key of an item into a hash value with djb2a algorithm
    @param  hstable Pointer to a hashtable stucture
    @param  key     Id of an item
    @return hash index calculated from the hash value of an item
*/
size_t hash_function(size_t hstable_capacity, char* key)
{
    size_t hash_value = 5381;   //Magic constant being used in djb2a algorithm 

    int char_index = 0; 
    while(key[char_index] != '\0')
    {
        hash_value = ((hash_value * 33) ^ key[char_index]);
        char_index++;
    }

    size_t hstable_index = (hash_value % (hstable_capacity));
    return hstable_index;
}

/*
    @brief Expansion of the hashtable when used capacity is over 50%, rehashing all items to new slot in array based on new index
    @param hstable  Pointer to a hashtable structure
    @return true when the expansion is succesful, false when the expansion fails
*/
bool rehash_hashtable(hashtable* hstable)
{
    size_t rehashed_capacity = hstable->capacity*2;
    item* rehashed_array = calloc(rehashed_capacity, sizeof(item));

    if(rehashed_array == NULL)
    {
        fprintf(stderr, "Rehash of hashtable failed.\n");
        return false;
    }

    for(size_t item_index = 0; item_index < hstable->capacity; item_index++)
    {
        if(hstable->item_array[item_index].state == slot_taken)
        {
            size_t new_index = hash_function(rehashed_capacity, hstable->item_array[item_index].id);
            rehashed_array[new_index] = hstable->item_array[item_index];
        }
        
        else 
        continue;
    }

    free(hstable->item_array);
    hstable->capacity = rehashed_capacity;
    hstable->item_array = rehashed_array;

    return true;
}

insert_status insert_hash(hashtable* hstable, char* key, attributes* attr)
{
    if(hstable->used_slots >= hstable->capacity/2 || hstable->available_slots == 0)
    {
        rehash_hashtable(hstable);
    }
    
    size_t hash_index = hash_function(hstable->capacity, key);
    size_t tombstone_index = 0;
    bool tombstone_found = false;
    size_t item_index = 0;
    bool slot_found = false;

    size_t item_count = 0;

    for(size_t index = 0; index < hstable->capacity; index++)
    {
        item_count = (hash_index + index) % hstable->capacity;

        if(hstable->item_array[item_count].state == slot_taken)
        {
            if(strcmp(hstable->item_array[item_count].id, key) == 0)
            {
                return insert_duplicate;
            }
        }
        if(hstable->item_array[item_count].state == slot_tombstoned && !(tombstone_found))
        {
            tombstone_index = item_count;
            tombstone_found = true;
        }
        if(hstable->item_array[item_count].state == slot_available)
        {
            item_index = item_count;
            slot_found = true;
            break;
        }
    }
    

    char *item_key = malloc((strlen(key) + 1) * sizeof(char));
    if(item_key == NULL)
    {
        return insert_fail;
    } 
    memcpy(item_key, key, (strlen(key) + 1) * sizeof(char));

    attributes *item_attribute = malloc (sizeof(attributes));
    if(item_attribute == NULL)
    {
        free(item_key);
        return insert_fail;
    }
    *item_attribute = *attr;

    /*
    if(tombstone_index != NULL)
    {
        hstable->item_array[tombstone_index].id = item_key;
        hstable->item_array[tombstone_index].attribute = item_attribute;
        hstable->item_array[tombstone_index].state = slot_taken;
        
        return insert_ok;
    }
    */
}
