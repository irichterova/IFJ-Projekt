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
    PSA_LEFT_P,      // (
    PSA_RIGHT_P,     // )
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

/**
 * @brief Transforms a token into a corresponding symbol
 * @param token Token from scanner
 * @param prev What symbol came before this one
 * @param depth Depth of the token in the expression
 * @return Symbol representing the token
 */
static psa_symbol token2symbol (token_t token, psa_symbol prev, int depth) {
    // Eol out of parentheses means end of expression
    if (token.after_eol && depth == 0) {
        return PSA_START_END;
    }

    switch (token.type) {
        // All of these result in i
        case TOKEN_IDENTIFIER:
        case TOKEN_INT_LITERAL:
        case TOKEN_DBL_LITERAL:
        case TOKEN_STR_LITERAL:
        case TOKEN_TRUE_LITERAL:
        case TOKEN_FALSE_LITERAL:
            return PSA_ID;

        case TOKEN_OP_ADD:
            return PSA_ADD;
        // Not OP_SUB, could be unary
        case TOKEN_OP_MUL:
            return PSA_MUL;
        case TOKEN_OP_DIV:
            return PSA_DIV;
        case TOKEN_OP_LT:
            return PSA_LESS;
        case TOKEN_OP_LE:
            return PSA_LESS_EQ;
        case TOKEN_OP_GT:
            return PSA_GREATER;
        case TOKEN_OP_GE:
            return PSA_GREATER_EQ;
        case TOKEN_OP_EQ:
            return PSA_EQUAL;
        case TOKEN_OP_NEQ:
            return PSA_NOT_EQUAL;

        case TOKEN_OP_NOT:
            return PSA_NOT;
        case TOKEN_OP_AND:
            return PSA_AND;
        case TOKEN_OP_OR:
            return PSA_OR;

        case TOKEN_L_PARENTH:
            return PSA_LEFT_P;
        case TOKEN_R_PARENTH:
            if (depth == 0) {
                return PSA_START_END; // $
            }    
            return PSA_RIGHT_P;
        

        case TOKEN_OP_SUB:
            if (prev == PSA_ID || prev == PSA_RIGHT_P) {
            // Binary -
                return PSA_SUB;
            }
            // Unary - (marked ~)
            return PSA_UNARY_MINUS;
            

        default:
            return PSA_START_END;
    }
}

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
        error_exit(ERR_INTERNAL, 0, "Failed to malloc a new PSA Stack in init_stack!");
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
    if (stack->top >= stack->capacity) {
        stack->capacity *= 2;

        Stack_Item *temp_stack_items = realloc(stack->stack_items, sizeof(Stack_Item) * stack->capacity);
        // Failed realloc
        if (temp_stack_items == NULL) {
            free(stack->stack_items);
            error_exit(ERR_INTERNAL, 0, "Failed to realloc a new PSA Stack in push_item!");
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
 * @brief Read the top item on stack
 * @param stack Pointer to stack var
 * @return Item at the top
 */
static Stack_Item top_item (PSA_Stack *stack) {
    if (stack->top == 0) {
        error_exit(ERR_INTERNAL, 0, "Tried to reach a negative item in stack!");
    }
    return stack->stack_items[stack->top - 1];
}


/**
 * @brief Find the top-most terminal
 * @param stack Pointer to stack var
 * @return Position of the top-most terminal
 */
static int top_terminal (PSA_Stack *stack) {
    // top - 1 is the first stack item
    for (int i = stack->top - 1; i >= 0; i--) {
        if (stack->stack_items[i].item_symbol != PSA_E && stack->stack_items[i].item_symbol != PSA_STOP) {
            return i;
        }
    }
    // None was found
    error_exit(ERR_INTERNAL, 0, "Couldn't find a terminal symbol in stack!");
    return -1;
}

/**
 * @brief Insert < symbol to stack to note Handle scope
 * @param stack Pointer to stack var
 */
static void insert_stop (PSA_Stack *stack) {
    if (stack->top == 0) {
        error_exit(ERR_INTERNAL, 0, "Tried to insert stop on an empty stack in insert_stop!");
    }

    // Check for space and increase capacity if we need to
    if (stack->top >= stack->capacity) {
        stack->capacity *= 2;

        Stack_Item *temp_stack_items = realloc(stack->stack_items, sizeof(Stack_Item) * stack->capacity);
        // Failed realloc
        if (temp_stack_items == NULL) {
            free(stack->stack_items);
            error_exit(ERR_INTERNAL, 0, "Failed to realloc a new PSA Stack in insert_stop!");
        }
        stack->stack_items = temp_stack_items;
    }
    // Go over items from the top, if E or < skip, if not insert PSA_STOP
    for (int i = stack->top - 1; i >= 0; i--) {
        if (stack->stack_items[i].item_symbol == PSA_E || stack->stack_items[i].item_symbol == PSA_STOP) {
            stack->stack_items[i + 1] = stack->stack_items[i];
        } else {
            stack->stack_items[i + 1] = (Stack_Item){ .item_symbol = PSA_STOP, .subtree = NULL };
            break;
        }
        if (i == 0) {
            error_exit(ERR_INTERNAL, 0 , "Failed to insert stop in insert_stop!");
        }
    }
    stack->top++;
}

/**
 * @brief Frees the allocated memory
 * @param stack Pointer to stack var
 */
static void free_stack (PSA_Stack *stack) {
    free(stack->stack_items);
    stack->stack_items = NULL;
    stack->capacity = 0;
    stack->top = 0;
}

// Stack code above

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
    (void)top_item;
    (void)top_terminal;
    (void)insert_stop;
    (void)free_stack;
    (void)token2symbol;

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
    (void)top_item;
    (void)top_terminal;
    (void)insert_stop;
    (void)free_stack;
    (void)token2symbol;

    return NULL;
}
