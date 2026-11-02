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
#include <stdio.h>
#include <stdlib.h>

bool rehash_hashtable(hashtable* hstable);
bool hashtable_init(hashtable* hstable);

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
            free(current->attribute->param_type_array);
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

    for(size_t index = 0; index < hstable->capacity; index++)
    {
        if(hstable->item_array[index].state == slot_taken)
        {
            size_t hash_index = hash_function(rehashed_capacity, hstable->item_array[index].id);
            for(size_t new_index = 0; new_index < rehashed_capacity; new_index++)
            {
                size_t item_count = (hash_index + new_index) % rehashed_capacity;

                if(rehashed_array[item_count].state == slot_available)
                {
                    rehashed_array[item_count] = hstable->item_array[index];
                    break;
                }
            }
        }
    }

    free(hstable->item_array);
    hstable->capacity = rehashed_capacity;
    hstable->item_array = rehashed_array;
    hstable->tombstoned_slots = 0;
    hstable->available_slots = hstable->capacity - hstable->used_slots;

    return true;
}

/*
    @brief  Insert item into the hashtable
    @param  hstable Pointer to a hashtable structure
    @param  key     Key of an item, in this case the Id
    @param  attr    Attributes of an item 
    @return insert_fail when insert fails for any of these reasons : expansion of hashtable is unsuccesful, 
                        allocation of a key or attributes fails, there is no available slot for the item / the hashtable is full 
    @return insert_duplicate    when trying to insert an item that is already in the hashtable
    @return insert_ok   when insert item succeeds 

*/
insert_status insert_hash(hashtable* hstable, char* key, attributes* attr)
{
    if(hstable->used_slots >= hstable->capacity/2 || hstable->available_slots == 0)
    {
        if(!rehash_hashtable(hstable))
        return insert_fail;
    }
    if(key == NULL || attr == NULL)
    {
        return insert_fail;
    }
    
    size_t hash_index = hash_function(hstable->capacity, key);
    size_t tombstone_index = 0;
    bool tombstone_found = false;
    size_t item_index = 0;
    bool slot_found = false;

    size_t item_count = 0;

    /* Search for an available slot based on the computed hash index, ensuring circularity of the hashtable (after end -> beginning) 
        If the computed slot is taken, then it is either taken by the same item, that was already inserted -> return duplicate
        or it is taken by another item -> a cycle finds another spot using the linear algorithm of moving to next slot, 
        until it finds an available one */
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
        /* If the current computed slot is tombstoned and is the first tombstoned slot we found, save the index and use it later to insert the item 
        to this slot */
        if(hstable->item_array[item_count].state == slot_tombstoned && !(tombstone_found))
        {
            tombstone_index = item_count;
            tombstone_found = true;
        }
        /* If the current computed slot is available save the index and break out of the loop */
        if(hstable->item_array[item_count].state == slot_available)
        {
            item_index = item_count;
            slot_found = true;
            break;
        }
    }

    if(!tombstone_found && !slot_found)
    {
        return insert_fail;
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
    memcpy(item_attribute, attr, sizeof(attributes));

    if(attr->symbol == variable)
    {
        item_attribute->num_of_param = 0;
        item_attribute->param_type_array = NULL;
    }

    if(attr->symbol == function)
    {
        if(attr->num_of_param > 0)
        {
            param_type *param_array = malloc(attr->num_of_param * sizeof(param_type));
            if(param_array == NULL)
            {
                free(item_key);
                free(item_attribute);
                return insert_fail;
            }
            memcpy(param_array, attr->param_type_array, attr->num_of_param * sizeof(param_type));
            item_attribute->param_type_array = param_array;
        }
        else
        {
        item_attribute->param_type_array = NULL;
        }
    }


    /* If we found a tombstone slot during the loop, we insert the item to this slot with the index of the first tombstone */
    if(tombstone_found)
    {
        hstable->item_array[tombstone_index].id = item_key;
        hstable->item_array[tombstone_index].attribute = item_attribute;
        hstable->item_array[tombstone_index].state = slot_taken;
        hstable->tombstoned_slots--;
        hstable->used_slots++;
        hstable->available_slots = hstable->capacity - hstable->used_slots;
        return insert_ok;
    }
    /* If an available slot was found for the item and there wasnt tombstone before, we insert the item to this slot with the index we saved*/
    else if(slot_found)
    {
        hstable->item_array[item_index].id = item_key;
        hstable->item_array[item_index].attribute = item_attribute;
        hstable->item_array[item_index].state = slot_taken;
        hstable->used_slots++;
        hstable->available_slots = hstable->capacity - hstable->used_slots;
        return insert_ok;
    }
    return insert_fail;
}

/*
    @brief  Search item(symbol) to check if it is in the hashtable and get the item attributes
    @param  hstable Pointer to a hashtable structure
    @param  key     A key of an item / the identification of the item 
    @return A pointer to the item attributes if the item is found, NULL if the item is not found
*/
attributes* search_symbol(hashtable* hstable, char* key)
{
    if(key == NULL || hstable == NULL)
    {
        return NULL;
    }

    size_t hash_index = hash_function(hstable->capacity, key);
   
    for(size_t index = 0; index < hstable->capacity; index++)
    {
        size_t item_count = (hash_index + index) % hstable->capacity;

        if(hstable->item_array[item_count].state == slot_available)
        {
            return NULL;
        }

        if(hstable->item_array[item_count].state == slot_taken)
        {
            if(strcmp(hstable->item_array[item_count].id, key) == 0 )
            {
                return hstable->item_array[item_count].attribute;
            }
        }
    }

    return NULL;
}
