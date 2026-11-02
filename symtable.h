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
    any_dt,

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

typedef enum
{
    ret_inferred,
    ret_unknown,

}return_state;

typedef struct
{
    param_type data_type;
    val_var value_modif;
    bool main_flag;
    bool builtin_flag;
    type_of_symbol symbol;
    param_type return_type;
    size_t num_of_param;
    param_type*  param_type_array;
    return_state ret_state;
    size_t  codegen_id;

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
    size_t available_slots;
    size_t used_slots;
    size_t tombstoned_slots;
    struct hashtable* parent;

}hashtable;

size_t hash_function(size_t hstable_capacity, char* key);
hashtable* create_hashtable(void);
void delete_hashtable(hashtable* hstable);
insert_status insert_hash(hashtable* hstable, char* key, attributes* attr);
attributes* search_symbol(hashtable* hstable, char* key);

#endif