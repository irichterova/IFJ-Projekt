/**
 * Project name: Compiler for IFJ26
 * @author Vojtěch Ziaťko, xziatkv00
 * @file expression.c
 */

#include <stdlib.h>
#include "expression.h"
#include "error.h"

#define PSA_TABLE_SIZE 19
#define INITIAL_CAPACITY 16

// Symbols defining the table
typedef enum {
    PSA_UNARY_MINUS, // ~
    PSA_MUL,         // *
    PSA_DIV,         // /
    PSA_ADD,         // +
    PSA_SUB,         // -
    PSA_LESS,        // <
    PSA_LESS_EQ,     // <=
    PSA_GREATER,     // >
    PSA_GREATER_EQ,  // >=
    PSA_EQUAL,       // ==
    PSA_NOT_EQUAL,   // !=
    PSA_ID,          // i
    PSA_LEFT_B,      // (
    PSA_RIGHT_B,     // )
    PSA_START_END,   // $
    PSA_CAT,         // ++ [LISTS]
    PSA_NOT,         // !  [BOOLTHEN]
    PSA_AND,         // && [BOOLTHEN]
    PSA_OR,          // || [BOOLTHEN]
    // Below are stack only symbols, not in the table
    PSA_E,           // Expression E
    PSA_STOP,        // Marker <
} psa_symbol;

// What can be inside the table
typedef enum {
    PSA_REL_SHIFT,  // <
    PSA_REL_PUSH,   // =
    PSA_REL_REDUCE, // >
    PSA_REL_ERROR,  // Empty
} psa_relation;

static const psa_relation precedence_table[PSA_TABLE_SIZE][PSA_TABLE_SIZE] = {
    //          ~                *               /               +               -               <               <=             >               >=                ==              !=            i               (               )                $               ++              !           &&              ||
    /* ~ */ {PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* * */ {PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* / */ {PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* + */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* - */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* < */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* <= */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* > */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* >= */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* == */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* != */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* i */ {PSA_REL_ERROR, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_ERROR, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* ( */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_PUSH, PSA_REL_ERROR, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT},
    /* ) */ {PSA_REL_ERROR, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_ERROR, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* $ */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT},
    /* ++ */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* ! */ {PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* && */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* || */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE},
};

// Stack code below

typedef struct psa_stack_item_t {
    psa_symbol item_symbol; // Terminal, PSA_E or PSA_STOP
    token_t item_token;     // For terminals only
    expr_struct* subtree;   // Pointer to subtree created by reduction
} Stack_Item;

typedef struct psa_stack_t {
    Stack_Item* stack_items; // Field of malloc'd items
    int top;                 // Current number of items in the stack, plus points at the last one top-1
    int capacity;            // Maximum capacity of the stack
} PSA_Stack;

/**
 * @brief Create a new stack
 * @param stack Pointer to stack variable
*/
static void init_stack (PSA_Stack *stack) {
    // Init a new stack
    stack->stack_items = malloc(sizeof(Stack_Item) * INITIAL_CAPACITY);
    // Failed malloc
    if (stack->stack_items == NULL) {
        error_exit(ERR_INTERNAL, 0, "Failed to malloc a new PSA Stack");
    }
    stack->capacity = INITIAL_CAPACITY;
    stack->top = 0;
}

/**
 * @brief Push an item to the top of the stack
 * @param stack Stack to push into
 * @param item Item to put at the top
*/
static void push_item (PSA_Stack *stack, Stack_Item item) {
    // Add an item, expand if full
    if (stack->top == stack->capacity) {
        stack->capacity *= 2;

        Stack_Item *temp_stack_items = realloc(stack->stack_items, sizeof(Stack_Item) * stack->capacity);
        // Failed realloc
        if (temp_stack_items == NULL) {
            free(stack->stack_items);
            error_exit(ERR_INTERNAL, 0, "Failed to realloc a new PSA Stack");
        }
        stack->stack_items = temp_stack_items;
    }
    stack->stack_items[stack->top] = item;
    stack->top += 1;
}

/**
 * @brief Pop an item from the stack
 * @param stack Pointer to stack variable
 * @return The removed (formerly the top) item
 */
static Stack_Item pop_item (PSA_Stack *stack) {
    if (stack->top == 0) {
        error_exit(ERR_INTERNAL, 0, "Tried to pop an empty stack!");
    }
    stack->top--;
    return stack->stack_items[stack->top];
}

/**
 * @brief Parses an expression (expr OR expr_noid), creates its tree, stores following token
 * @param first First token
 * @param next Where to store the following token
 * @return Pointer to the root of the expression tree
 */
expr_struct *parse_expr(token_t first, token_t *next) {
    // Field of voided things
    // TODO: Remove when used
    (void)first;
    (void)next;
    (void)precedence_table;
    (void)init_stack;
    (void)push_item;
    (void)pop_item;

    return NULL;
}

/**
 * @brief Parses an expression (expr_rest), creates its tree, stores following token
 * @param first First token
 * @param second Second token
 * @param next Where to store the following token
 * @return Pointer to the root of the expression tree
 */
expr_struct *parse_expr_rest(token_t first, token_t second, token_t *next) {
    // Field of voided things
    // TODO: Remove when used
    (void)first;
    (void)second;
    (void)next;
    (void)precedence_table;
    (void)init_stack;
    (void)push_item;
    (void)pop_item;

    return NULL;
}
