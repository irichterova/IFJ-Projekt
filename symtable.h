/*
*
IFJ/IAL project - Compiler for language IFJ26 to language IFJ26 code
*
@file symtable.h 
@brief header file for implementation of a hashtable 
*
@date 24-10-2026
@author xdongop00 Phuong Uyen Dongova
*
*/
#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#define INITIAL_CAPACITY 16

typedef enum 
{
    int_dt,
    double_dt,
    string_dt,
    bool_dt,
    unit_dt,

}param_type;

typedef enum  
{
    slot_available,
    slot_tombstoned,
    slot_taken,

}status;

typedef enum 
{
    insert_ok,
    insert_duplicate,
    insert_fail,

}insert_status;

typedef enum  
{
    nonmodif_val,
    modif_var,

}val_var;

typedef enum 
{
    variable,
    function,

}type_of_symbol;

typedef struct
{
    param_type data_type;
    val_var value_modif;
    bool main_flag;
    type_of_symbol symbol;
    param_type return_type;
    int num_of_param;

}attributes;

typedef struct 
{
    char* id;
    attributes* attribute;
    size_t hash_value;
    status state;

}item;

typedef struct hashtable
{
    size_t capacity;
    item* item_array; 
    int available_slots;
    int used_slots;
    int tombstoned_slots;
    struct hashtable* parent;

}hashtable;

size_t hash_function(size_t hstable_capacity, char* key);
hashtable* create_hashtable(void);
bool hashtable_init(hashtable* hstable);
bool rehash_hashtable(hashtable* hstable);
void delete_hashtable(hashtable* hstable);

insert_status insert_hash(hashtable* hstable, char* key, attributes* attr);
attributes* search_symbol(char* item_to_search, hashtable* hstable);
bool tombstone_hash(item* item_to_tombs, hashtable* hstable);

#endif