/*
*
IFJ/IAL project - Compiler for language IFJ26 to language IFJ26 code
*
@file symtable.h 
@brief header file for implementation of a hashtable 
*
@date 08-10-2026
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
    nonmodif_val,
    modif_var,

}val_var;

typedef struct
{
    param_type data_type;
    val_var value;
    bool main_flag;
    bool func_flag;
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

size_t hash_function(hashtable* hstable, char* key);
bool hashtable_init(hashtable* hstable);
bool rehash_hashtable(hashtable* hstable);
void delete_hashtable(hashtable* hstable);

void insert_hash(item* item_to_insert, hashtable* hstable);
int redirect_hash(item* item_to_redirect, hashtable* hstable);
attributes* search_symbol(char* item_to_search, hashtable* hstable);
bool tombstone_hash(item* item_to_tombs, hashtable* hstable);

#endif